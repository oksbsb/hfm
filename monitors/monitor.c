#include "libmon.h"
#include "traps.h"


void mon_init(monitor_t type)
{
}

hfm_status_t mon_add_policy(vmhdlr_t *hdlr, policy_t *policy)
{
    traps_register(hdlr, "NtCreateFile");
    return SUCCESS;
}
