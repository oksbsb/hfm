/**
  * @file private.h
  * @author phuong.do
  * @date 2017-05-24
  * @brief Contains the common struct, macros ... used in the program
  */
#ifndef __HFM_PRIVATE_H__
#define __HFM_PRIVATE_H__

#include <stdint.h>
#include <libvmi/libvmi.h>

#define PATH_MAX_LEN 1024           /**< Maximum len of file or directory path */

/**
  * @brief Return status code
  */
typedef enum {
    SUCCESS,
    FAIL
} hfm_status_t;

/**
  * @brief monitoring plugin id
  */
typedef enum {
    MON_CREATE,
    MON_DELETE,
    MON_CONTENT_MODIFY,
    MON_LOGFILE_MODIFY,
    MON_ATTR_READONLY_CHANGE,
    MON_ATTR_PERMISSION_CHANGE,
    MON_ATTR_OWNERSHIP_CHANGE,
    MON_ATTR_HIDDEN_CHANGE
} monitor_t;

/**
  * @brief Policy severity
  */
typedef enum {
    WARNING,
    CRITICAL
} severity_t;

/**
  * @brief Trap types
  */
typedef enum {
    TRAP_BREAKPOINT,
    TRAP_MEM
} trap_type_t;

/**
  * @brief Address type
  */
typedef enum {
    ADDR_VA,    /** < Virtual Address */
    ADDR_RVA    /** < Relative virtual address */
} addr_type_t;


/**
  * @brief This struct will be used to transfer neccessary information to callbacks in plugins
  *
  * @see drakvuf/src/libdrakvuf/libdrakvuf.h, line 201
  */
typedef struct _trap_info {

} trap_info_t;

/**
  * @brief Save all data relating to a trap
  */
typedef struct _trap {
    unsigned int vcpu;
    uint16_t altp2m_idx;
    trap_type_t type;
} trap_t;

typedef struct _policy {
    char path[PATH_MAX_LEN];
    monitor_t type;
    int8_t is_dir;
    int8_t is_recursive;
    int8_t is_extract;
    severity_t severity;
} policy_t;

typedef struct _vmhdlr {
    char rekall[PATH_MAX_LEN];
    vmi_instance_t vmi;
    page_mode_t pm;
    uint32_t vcpus;
    uint32_t memsize;
} vmhdlr_t;

#endif  /* __HFM_PRIVATE_H__ */
