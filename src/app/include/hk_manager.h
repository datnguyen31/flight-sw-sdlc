#ifndef HK_MANAGER_H
#define HK_MANAGER_H

/*
 * HK Manager: service state and lifecycle (HKS-SR-100, HKS-SR-110,
 * HKS-SR-120)
 *
 * Holds the enabled/disabled reporting state, applies command results, and
 * gates whether the report step emits output.
 */

#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void hk_manager_init(void);    /* reporting starts DISABLED (SR-120) */
    void hk_manager_on_tick(void); /* called by the scheduler each tick */
    bool hk_manager_reports_enabled(void);

    /* Set by the Command Handler under a critical section (SR-100, SR-110). */
    void hk_manager_set_reports_enabled(bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* HK_MANAGER_H */
