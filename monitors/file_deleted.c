#include "file_deleted.h"
#include "file_filter.h"
#include "context.h"
#include "private.h"
#include "hfm.h"
#include "constants.h"


static filter_t *filter = NULL;

/**
  * Callback when the functions NtSetInformatonFile, ZwSetInformationFile is called
  */
static void *setinformation_cb(vmhdlr_t *handler, context_t *context);

/**
  * Callback when the functions NtSetInformatonFile, ZwSetInformationFile is returned
  */
static void *setinformation_ret_cb(vmhdlr_t *handler, context_t *context);

hfm_status_t file_deleted_add_policy(vmhdlr_t *hdlr, policy_t *policy)
{
    if (!filter) {
        //Init plugin
        filter = filter_init();
        hfm_monitor_syscall(hdlr, "NtSetInformationFile", setinformation_cb, setinformation_ret_cb);
        hfm_monitor_syscall(hdlr, "ZwSetInformationFile", setinformation_cb, setinformation_ret_cb);
    }
    filter_add(filter, policy->path, policy->id);
    return SUCCESS;
}
/**
  * - Read FileInformationClass, check if class is FILE_DISPOSITION_INFORMATION
  * - Read FILE_DISPOSITION_INFORMATION struct for DeleteFile field
  * - Read FileHandle, searching for FILE_OBJECT, read FileName from FILE_OBJECT
  */
static void *setinformation_cb(vmhdlr_t *handler, context_t *context)
{
    vmi_instance_t vmi = hfm_lock_and_get_vmi(handler);
    addr_t fileinfo_addr = 0;
    uint32_t fileinfo_class = 0;
    reg_t handle = 0;

    //Read FileInformationClass and address of FileInformation
    if (handler->pm == VMI_PM_IA32E) {
        handle = context->regs->rcx;
        fileinfo_addr = context->regs->r8;
        fileinfo_class = hfm_read_32(vmi, context, context->regs->rsp + 5 * sizeof(addr_t));
    }
    else {
        handle = hfm_read_32(vmi, context, context->regs->rsp + 1 * sizeof(uint32_t));
        fileinfo_addr = hfm_read_32(vmi, context, context->regs->rsp + 3 * sizeof(uint32_t));
        fileinfo_class = hfm_read_32(vmi, context, context->regs->rsp + 5 * sizeof(uint32_t));
    }
    if (FILE_DISPOSITION_INFORMATION == fileinfo_class) {
        char filename[STR_BUFF] = "";
        hfm_read_filename_from_handler(vmi, context, handle, filename);
        if (filter_match(filter, filename) >= 0) {
            uint8_t delete = hfm_read_8(vmi, context, fileinfo_addr + FILE_DISPOSITION_INFORMATION_DELETE_FILE);
            if (delete) {
                printf("[DELETE] file %s\n", filename);
            }
        }
    }
done:
    hfm_release_vmi(handler);
    return NULL;
}

//Not used yet
static void *setinformation_ret_cb(vmhdlr_t *handler, context_t *context)
{
    return NULL;
}
