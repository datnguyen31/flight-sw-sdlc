#ifndef HK_CRITICAL_H
#define HK_CRITICAL_H

/*
 * Critical-Section Interface (design/detailed-design.md S1.6)
 *
 * Isolates the RTOS dependency of the reporting-state flag's mutual
 * exclusion behind a swappable pair of calls: a no-op on the host so the
 * HK Manager logic is unit-testable without an RTOS, and a
 * priority-inheritance mutex under RTEMS (hk_critical_rtems.c).
 *
 * hk_critical_init() must run once at startup, before hk_manager_init()
 * and before any task that uses the HK Manager is started.
 */

#ifdef __cplusplus
extern "C"
{
#endif

    void hk_critical_init(void);  /* host: no-op; RTEMS: create the mutex */
    void hk_critical_enter(void); /* host: no-op; RTEMS: obtain the mutex */
    void hk_critical_leave(void); /* host: no-op; RTEMS: release the mutex */

#ifdef __cplusplus
}
#endif

#endif /* HK_CRITICAL_H */
