/*
 * hk_command.c
 *
 * Command Handler validation and dispatch (HKS-SR-100, HKS-SR-110,
 * HKS-SR-130). Validation is an explicit switch: known opcodes act through
 * the HK Manager setter and are accepted; anything else is rejected with
 * no state change. The handler never touches the reporting flag directly
 * (DD-07).
 */

#include "hk_command.h"
#include "hk_manager.h"

hk_command_result_t hk_command_dispatch(hk_command_id_t cmd)
{
    hk_command_result_t result;

    switch (cmd)
    {
        case HK_CMD_ENABLE_REPORTS:
            hk_manager_set_reports_enabled(true); /* SR-100 */
            result = HK_CMD_ACCEPTED;
            break;
        case HK_CMD_DISABLE_REPORTS:
            hk_manager_set_reports_enabled(false); /* SR-110 */
            result = HK_CMD_ACCEPTED;
            break;
        default:
            result = HK_CMD_REJECTED; /* SR-130: no state change */
            break;
    }

    return result;
}
