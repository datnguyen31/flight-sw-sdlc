#ifndef HK_MONITOR_H
#define HK_MONITOR_H

/*
 * FDIR monitor (HKS-FD-020, HKS-FD-030, HKS-FD-050, HKS-FD-060,
 * HKS-FD-070; DD-13)
 *
 * Checks a built housekeeping report against the limit table once per
 * collection cycle. Each parameter carries its own confirmed level
 * (NOMINAL, WARNING, CRITICAL), which moves only after HK_DEBOUNCE_N
 * consecutive fresh samples support the move, up or down. Each rise in
 * level emits one event; a rise to CRITICAL also enters safe mode. A
 * parameter stale for HK_STALE_M consecutive cycles emits one stale event.
 * A sustained condition is acted on once, not every cycle.
 *
 * Collection task only: the per-parameter state is touched by no other
 * task and needs no locking.
 */

#include "hk_report.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define HK_DEBOUNCE_N 3u /* FD-030: build-time configurable, default 3 */
#define HK_STALE_M    3u /* FD-070: build-time configurable, default 3 */

    /* Every parameter NOMINAL, all counters zero. Call once at startup. */
    void hk_monitor_init(void);

    /* Check one built report (FD-020). Events are stamped with the
     * report's collection time. Does nothing if report is NULL. */
    void hk_monitor_check(const hk_report_t* report);

#ifdef __cplusplus
}
#endif

#endif /* HK_MONITOR_H */
