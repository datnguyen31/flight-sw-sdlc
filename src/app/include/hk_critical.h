#ifndef HK_CRITICAL_H
#define HK_CRITICAL_H

/*
 * Critical-Section Interface (design/detailed-design.md S1.6)
 *
 * Isolates the RTOS dependency of the reporting-state flag's mutual
 * exclusion behind a swappable pair of calls: a no-op on the host so the
 * HK Manager logic is unit-testable without an RTOS, and a real
 * interrupt-disable or mutex under RTEMS (Session 7).
 */

#ifdef __cplusplus
extern "C"
{
#endif

    void hk_critical_enter(void); /* host: no-op; RTEMS: disable preemption */
    void hk_critical_leave(void);

#ifdef __cplusplus
}
#endif

#endif /* HK_CRITICAL_H */
