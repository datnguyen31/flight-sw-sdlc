/*
 * hk_manager.c
 *
 * HK Manager: service state and lifecycle (HKS-SR-100, HKS-SR-110,
 * HKS-SR-120). The reporting-state flag is this module's private data;
 * every access goes through the critical section here, not in the caller
 * (DD-07). The RTOS dependency is isolated behind hk_critical.h.
 */

#include "hk_manager.h"
#include "hk_critical.h"

static bool s_reports_enabled; /* protected by the critical section below */

void hk_manager_init(void)
{
    hk_critical_enter();
    s_reports_enabled = false; /* HKS-SR-120 */
    hk_critical_leave();
}

void hk_manager_on_tick(void)
{
    /* Synchronization point reserved for future per-tick concerns
     * (design/detailed-design.md S1.1); no state effect in the baseline. */
}

bool hk_manager_reports_enabled(void)
{
    bool enabled;

    hk_critical_enter();
    enabled = s_reports_enabled;
    hk_critical_leave();
    return enabled;
}

void hk_manager_set_reports_enabled(bool enabled) /* DD-07 */
{
    hk_critical_enter();
    s_reports_enabled = enabled;
    hk_critical_leave();
}
