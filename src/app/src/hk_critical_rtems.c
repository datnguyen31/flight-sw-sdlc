/*
 * hk_critical_rtems.c (target build only)
 *
 * RTEMS build of the critical-section interface (hk_critical.h): a binary
 * semaphore with priority inheritance guarding the reporting-state flag
 * (design/detailed-design.md S1.6). Inheritance prevents unbounded priority
 * inversion between the higher-priority command task and the collection
 * task. A failed directive here means the mutex is unusable and the flag
 * unprotected, so it is escalated to a fatal error: the defined safe
 * response (coding-standard.md rules 5 and 7), not silently ignored.
 */

#include <rtems.h>

#include "hk_critical.h"

#define HK_FATAL_CRITICAL 0x484B4D58u /* 'HKMX' */

static rtems_id s_mtx;

static void check(rtems_status_code sc)
{
    if (sc != RTEMS_SUCCESSFUL)
    {
        rtems_fatal(RTEMS_FATAL_SOURCE_APPLICATION, HK_FATAL_CRITICAL);
    }
}

void hk_critical_init(void)
{
    check(rtems_semaphore_create(rtems_build_name('H', 'K', 'M', 'X'),
                                 1u,
                                 RTEMS_BINARY_SEMAPHORE | RTEMS_PRIORITY | RTEMS_INHERIT_PRIORITY,
                                 0u,
                                 &s_mtx));
}

void hk_critical_enter(void)
{
    check(rtems_semaphore_obtain(s_mtx, RTEMS_WAIT, RTEMS_NO_TIMEOUT));
}

void hk_critical_leave(void)
{
    check(rtems_semaphore_release(s_mtx));
}
