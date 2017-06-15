#include <stdio.h>
#include <stdlib.h>
#include <libvmi/libvmi.h>
#include <libvmi/events.h>
#include <libvmi/slat.h>
#include <glib.h>

#include "hfm.h"
#include "xen_helper.h"
#include "config.h"
#include "log.h"
#include "trapmngr.h"

/**
* hfm maintains two page tables (two views), first page table (ORIGINAL_IDX) maps the kernel
* with no modification. The second (altp2m_idx, shadow table) adds breakpoints to the kernel
*
* ORIGIN_IDX is activated :
* (1) For a single instruction after trapping a read (such a read is likely the result of Kernel
*  Patch Protection. This allows to hide the present of hfm from other checking software
* (2) For a single instruction after trapping a guest trace-emplaced breakpoint. This allows
* the kernel to execute as expected after servicing the breakpoint.
*
* ALTP2M_IDX is activate : following a single-step execution, this restore the breakpoints after condition
* (1) or (2)
*/

#define ORIGIN_IDX 0

static event_response_t _int3_cb(vmi_instance_t vmi, vmi_event_t *event);
static event_response_t _pre_mem_cb(vmi_instance_t vmi, vmi_event_t *event);
static event_response_t _post_mem_cb(vmi_instance_t vmi, vmi_event_t *event);
static event_response_t _singlestep_cb(vmi_instance_t vmi, vmi_event_t *event);

static hfm_status_t _inject_trap(vmhdlr_t *handler, addr_t pa, trap_t *trap);
static hfm_status_t _init_vmi(vmhdlr_t *handler);
static void _close_vmi(vmhdlr_t *handler);
static hfm_status_t _setup_altp2m(vmhdlr_t *handler);
static void _reset_altp2m(vmhdlr_t *handler);
static uint64_t _create_shadow_page(vmhdlr_t *handler, uint64_t original_gfn);

extern config_t *config;
uint8_t INT3_CHAR = 0xCC;

hfm_status_t hfm_init(vmhdlr_t *handler)
{
    /* Init LibVMI */
    if (SUCCESS != _init_vmi(handler)) {
        goto error1;
    }
    /* Init xen interface*/
    if ((handler->xen = xen_init_interface(handler->name)) == NULL) {
        writelog(LV_ERROR, "Failed to init XEN on domain %s", handler->name);
        xen_free_interface(handler->xen);
        goto error2;
    }
    /* Create altp2m view */
    if (SUCCESS != _setup_altp2m(handler)) {
        goto error3;
    }
    /* Init trap manager */
    handler->trap_manager = trapmngr_init(handler);
    if (handler->trap_manager == NULL) {
        writelog(LV_ERROR, "Failed to init trap manager on domain %s", handler->name);
        goto error4;
    }

    return SUCCESS;
error4:
    _reset_altp2m(handler);
error3:
    xen_free_interface(handler->xen);
error2:
    _close_vmi(handler);
error1:
    return FAIL;
}

void hfm_close(vmhdlr_t *handler)
{
    writelog(LV_INFO, "Close LibVMI on domain %s", handler->name);
    vmi_pause_vm(handler->vmi);

    /* Destroy all traps */
    trapmngr_destroy(handler->trap_manager);

    /* Reset altp2m view */
    _reset_altp2m(handler);

    /* Close xen interface */
    xen_free_interface(handler->xen);

    vmi_resume_vm(handler->vmi);

    /* Close LibVMI */
    _close_vmi(handler);
}

void hfm_listen(vmhdlr_t *handler)
{
    vmi_events_listen(handler->vmi, 500);
}

hfm_status_t hfm_monitor_syscall(vmhdlr_t *handler, const char *func_name, event_response_t (*cb)(vmhdlr_t *, trap_data_t *))
{
    hfm_status_t ret = SUCCESS;
    addr_t func_addr;
    vmi_pause_vm(handler->vmi);

    /* Translate syscall name into physical address */
    func_addr = vmi_translate_ksym2v(handler->vmi, func_name);
    if (0 == func_addr) {
        writelog(LV_WARN, "Counldn't locate the address of kernel symbol '%s'", func_name);
        goto done;
    }

    addr_t pa = vmi_translate_kv2p(handler->vmi, func_addr);
    if (0 == pa) {
        writelog(LV_DEBUG, "Virtual addr translation failed: %lx", func_addr);
        goto done;
    }
    //Create a trap
    trap_t *trap = (trap_t *)calloc(1, sizeof(trap_t));
    strncpy(trap->name, func_name, STR_BUFF);
    trap->cb = cb;

    //Inject trap at physical address
    _inject_trap(handler, pa, trap);
done:
    vmi_resume_vm(handler->vmi);
    return ret;
}

static event_response_t _int3_cb(vmi_instance_t vmi, vmi_event_t *event)
{
    event_response_t rsp = 0;
    vmhdlr_t *handler = event->data;
    handler->regs[event->vcpu_id] = event->x86_regs;

    /* Calculate pa of interrupt event */
    addr_t pa = (event->interrupt_event.gfn << 12)
                    + event->interrupt_event.offset + event->interrupt_event.insn_length - 1;
    writelog(LV_DEBUG, "INT3 event vCPU %u pa %lx", event->vcpu_id, pa);

    /* Looking for traps registered at this pa */
    int3_wrapper_t *w = trapmngr_find_breakpoint(handler->trap_manager, pa);
    if (w) {
        GSList *loop = w->traps;
        while (loop) {
            trap_t *trap = loop->data;
            if (trap->cb)
                rsp |= trap->cb(handler, NULL);
            loop = loop->next;
        }
    }

    event->slat_id = ORIGIN_IDX;
    event->interrupt_event.reinject = 0;
    handler->step_event[event->vcpu_id]->callback = _singlestep_cb;
    handler->step_event[event->vcpu_id]->data = handler;
    return rsp |
            VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP |
            VMI_EVENT_RESPONSE_VMM_PAGETABLE_ID;
}

static event_response_t _pre_mem_cb(vmi_instance_t vmi, vmi_event_t *event)
{
    printf("pre_mem_cb called\n");
    vmhdlr_t *handler = event->data;
    event->slat_id = ORIGIN_IDX;
    handler->step_event[event->vcpu_id]->callback = _post_mem_cb;
    handler->step_event[event->vcpu_id]->data = handler;//TODO
    return VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP
            | VMI_EVENT_RESPONSE_VMM_PAGETABLE_ID;
}

static event_response_t _post_mem_cb(vmi_instance_t vmi, vmi_event_t *event)
{
    printf("post_mem_cb called\n");
    vmhdlr_t *handler = event->data;
    event->slat_id = handler->altp2m_idx;
    handler->step_event[event->vcpu_id]->callback = _singlestep_cb;
    handler->step_event[event->vcpu_id]->data = handler;
    return VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP |
            VMI_EVENT_RESPONSE_VMM_PAGETABLE_ID;
}

static event_response_t _singlestep_cb(vmi_instance_t vmi, vmi_event_t *event)
{
    vmhdlr_t *handler = event->data;
    event->slat_id = handler->altp2m_idx;
    return VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP |
            VMI_EVENT_RESPONSE_VMM_PAGETABLE_ID;
}

hfm_status_t _inject_trap(vmhdlr_t *handler, addr_t pa, trap_t *trap)
{
    hfm_status_t status = FAIL;
    addr_t frame = pa >> PAGE_OFFSET_BITS;

    //Check if there is breakpoint inserted at this position or not
    int3_wrapper_t *int3w = trapmngr_find_breakpoint(handler->trap_manager, pa);
    if (int3w) {
        int3w->traps = g_slist_append(int3w->traps, trap);
        return SUCCESS;
    }

    //Create new wrapper for breakpoints at this address
    int3w = (int3_wrapper_t *)calloc(1, sizeof(int3_wrapper_t));
    int3w->traps = g_slist_append(int3w->traps, trap);

    //Check if there is a shadow page created at this gfn or not
    //If not, create the shadow page
    uint64_t remapped = trapmngr_find_remapped(handler->trap_manager, frame);
    if (!remapped) {
        remapped = _create_shadow_page(handler, frame);
        if (remapped)
            trapmngr_add_remapped(handler->trap_manager, frame, remapped);
    }

    //Callback invoked on a R/W of a monitored page (likely Windows kernel patch protection). Switch the VCPU's SLAT to its original, step once, switch SLAT back
    mem_wrapper_t *memw = trapmngr_find_memtrap(handler->trap_manager, frame);
    if (!memw) {
        //Create new wrapper for memaccess at this page
        memw = (mem_wrapper_t *)calloc(1, sizeof(mem_wrapper_t));
        trapmngr_add_memtrap(handler->trap_manager, g_memdup(&pa, sizeof(uint64_t)), memw);
    }
    vmi_set_mem_event(handler->vmi, frame, VMI_MEMACCESS_RW, handler->altp2m_idx);

    addr_t rpa = (remapped << PAGE_OFFSET_BITS) + pa % PAGE_SIZE;
    if (VMI_SUCCESS != vmi_write_8_pa(handler->vmi, rpa, &INT3_CHAR)) {
        writelog(LV_DEBUG, "Failed to write interrupt to shadow page");
        goto done;
    }
    trapmngr_add_breakpoint(handler->trap_manager, g_memdup(&pa, sizeof(uint64_t)), int3w);
    status = SUCCESS;
done:
    return status;
}

static hfm_status_t _init_vmi(vmhdlr_t *handler)
{
    writelog(LV_INFO, "Init VMI on domain %s", handler->name);
    if (VMI_FAILURE == vmi_init(&handler->vmi, VMI_XEN, handler->name, VMI_INIT_DOMAINNAME | VMI_INIT_EVENTS, NULL, NULL)) {
        writelog(LV_ERROR, "Failed to init LibVMI on domain %s", handler->name);
        goto error;
    }
    char *rekall_profile = config_get_str(config, "rekall_profile");
    GHashTable *vmicfg = g_hash_table_new(g_str_hash, g_str_equal);
    g_hash_table_insert(vmicfg, "rekall_profile", rekall_profile);
    g_hash_table_insert(vmicfg, "os_type", "Windows");
    uint64_t flags = VMI_PM_INITFLAG_TRANSITION_PAGES;
    if (VMI_PM_UNKNOWN == vmi_init_paging(handler->vmi, flags)) {
        writelog(LV_ERROR, "Failed to init LibVMI paging on domain %s", handler->name);
        g_hash_table_destroy(vmicfg);
        goto error;
    }
    os_t os = vmi_init_os(handler->vmi, VMI_CONFIG_GHASHTABLE, vmicfg, NULL);
    g_hash_table_destroy(vmicfg);
    if (os != VMI_OS_WINDOWS) {
        writelog(LV_ERROR, "Failed to init LibVMI library on domain %s", handler->name);
        goto error;
    }
    handler->pm = vmi_get_page_mode(handler->vmi, 0);
    handler->vcpus = vmi_get_num_vcpus(handler->vmi);
    handler->memsize = handler->init_memsize = vmi_get_memsize(handler->vmi);

    /* Register events */
    int i;
    for (i = 0; i < handler->vcpus && i < 16; i++) {
        handler->step_event[i] = g_malloc0(sizeof(vmi_event_t));
        SETUP_SINGLESTEP_EVENT(handler->step_event[i], 1u << i, _singlestep_cb, 0);
        handler->step_event[i]->data = handler;
        if (VMI_FAILURE == vmi_register_event(handler->vmi, handler->step_event[i])) {
            writelog(LV_ERROR, "Failed to register singlestep for vCPU on %s", handler->name);
            goto error;
        }
    }
    SETUP_INTERRUPT_EVENT(&handler->interrupt_event, 0, _int3_cb);
    handler->interrupt_event.data = handler;
    if (VMI_FAILURE == vmi_register_event(handler->vmi, &handler->interrupt_event)) {
        writelog(LV_ERROR, "Failed to register interrupt event on %s", handler->name);
    }
    SETUP_MEM_EVENT(&handler->mem_event, ~0ULL, VMI_MEMACCESS_RWX, _pre_mem_cb, 1);
    handler->mem_event.data = handler;
    if (VMI_FAILURE == vmi_register_event(handler->vmi, &handler->mem_event)) {
        writelog(LV_ERROR, "Failed to register generic mem event on %s", handler->name);
        goto error;
    }
    return SUCCESS;
error:
    return FAIL;
}

static void _close_vmi(vmhdlr_t *handler)
{
    vmi_destroy(handler->vmi);
}

static hfm_status_t _setup_altp2m(vmhdlr_t *handler) {
    status_t status = vmi_slat_set_domain_state(handler->vmi, 1);
    if (status != VMI_SUCCESS) {
        writelog(LV_ERROR, "Failed to enable altp2m on domain %s", handler->name);
        goto error;
    }

    status = vmi_slat_create(handler->vmi, &handler->altp2m_idx);
    if (status != VMI_SUCCESS) {
        writelog(LV_ERROR, "Failed to create altp2m view idx on domain %s", handler->name);
        goto error;
    }

    status = vmi_slat_switch(handler->vmi, handler->altp2m_idx);
    if (status != VMI_SUCCESS) {
        writelog(LV_ERROR, "Failed to switch to altp2m idx view on domain %s", handler->name);
        goto error;
    }
    return SUCCESS;
error:
    return FAIL;
}

static void _reset_altp2m(vmhdlr_t *handler) {
    vmi_slat_switch(handler->vmi, ORIGIN_IDX);
    vmi_slat_destroy(handler->vmi, handler->altp2m_idx);
}

static uint64_t _create_shadow_page(vmhdlr_t *handler, uint64_t frame)
{
    /* Setup and activate shadow view */
    uint64_t proposed_memsize = handler->memsize + PAGE_SIZE;
    uint64_t remapped = xen_alloc_shadow_frame(handler->xen, proposed_memsize);
    if (remapped == 0) {
        writelog(LV_DEBUG, "Extend memory failed for shadow page");
        goto error;
    }
    writelog(LV_DEBUG, "Shadow page created at remapped frame %lx", remapped);
    handler->memsize = proposed_memsize;    //Update current memsize after extend

    /* Change altp2m_idx view to map to new remapped address */
    if (VMI_SUCCESS != vmi_slat_change_gfn(handler->vmi, handler->altp2m_idx, frame, remapped)) {
        writelog(LV_DEBUG, "Failed to update altp2m_idx view to new remapped address");
        goto error;
    }

    /* Copy original page to remapped page */
    uint8_t buff[PAGE_SIZE] = {0};
    size_t ret = vmi_read_pa(handler->vmi, frame << PAGE_OFFSET_BITS, buff, PAGE_SIZE);
    if (PAGE_SIZE != ret) {
        writelog(LV_DEBUG, "Failed to read in syscall page");
        goto error;
    }

    ret = vmi_write_pa(handler->vmi, remapped << PAGE_OFFSET_BITS, buff, PAGE_SIZE);
    if (PAGE_SIZE != ret) {
        writelog(LV_DEBUG, "Failed to write to remapped page");
        goto error;
    }

    return remapped;
error:
    return 0;
}