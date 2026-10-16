/*
 * hk_collection_task.c (target build only)
 *
 * HK collection task (HKS-SR-020, HKS-SR-060, HKS-SR-140, HKS-SR-180).
 * Thin glue: the RTEMS rate-monotonic period releases one collection
 * cycle per HK_COLLECTION_PERIOD_MS (DD-06), and hk_collection_step() does
 * the work (design/detailed-design.md S1.1). A period overrun (RTEMS_TIMEOUT
 * from rtems_rate_monotonic_period) is counted and reported, as evidence
 * for the jitter requirement. The cycle line also shows the safe-mode
 * indicator (HKS-FD-080), so a console log shows when it was set.
 */

#include <rtems.h>
#include <rtems/bspIo.h>

#include "hk_collection.h"
#include "hk_safemode.h"
#include "hk_tasks.h"
#include "hk_time.h"

RTEMS_ALIGNED(RTEMS_TASK_STORAGE_ALIGNMENT)
static char s_storage[RTEMS_TASK_STORAGE_SIZE(HK_TASK_STACK_SIZE + HK_TASK_MAX_TLS_SIZE,
                                              RTEMS_DEFAULT_ATTRIBUTES)];

static const char* step_name(hk_step_result_t res)
{
    switch (res)
    {
        case HK_STEP_COLLECTED:
            return "collected";
        case HK_STEP_REPORT_SUPPRESSED:
            return "report tick, reporting disabled";
        case HK_STEP_REPORT_SENT:
            return "report sent";
        default:
            return "report REJECTED by sink";
    }
}

static rtems_task hk_collection_task(rtems_task_argument arg)
{
    const rtems_interval ticks = RTEMS_MILLISECONDS_TO_TICKS(HK_COLLECTION_PERIOD_MS);
    rtems_id             period;
    uint32_t             cycle    = 0u;
    uint32_t             overruns = 0u;

    (void)arg;
    if (rtems_rate_monotonic_create(rtems_build_name('H', 'K', 'P', 'R'), &period) !=
        RTEMS_SUCCESSFUL)
    {
        rtems_fatal(RTEMS_FATAL_SOURCE_APPLICATION, 0x484B5052u); /* 'HKPR' */
    }

    for (;;)
    {
        /* Blocks until the next release; bounded jitter (SR-140). */
        if (rtems_rate_monotonic_period(period, ticks) == RTEMS_TIMEOUT)
        {
            overruns++;
        }
        cycle++;
        hk_step_result_t res = hk_collection_step(hk_time_now_cuc());
        printk("[hk] cycle %3lu: %s (overruns %lu)%s\n",
               (unsigned long)cycle,
               step_name(res),
               (unsigned long)overruns,
               hk_safemode_active() ? " [SAFE MODE]" : "");
    }
}

rtems_status_code hk_collection_task_start(void)
{
    static const rtems_task_config config = {.name = rtems_build_name('H', 'K', 'C', 'L'),
                                             .initial_priority = HK_COLLECTION_TASK_PRIORITY,
                                             .storage_area     = s_storage,
                                             .storage_size     = sizeof(s_storage),
                                             .maximum_thread_local_storage_size =
                                                 HK_TASK_MAX_TLS_SIZE,
                                             .storage_free  = NULL,
                                             .initial_modes = RTEMS_DEFAULT_MODES,
                                             .attributes    = RTEMS_DEFAULT_ATTRIBUTES};
    rtems_id                       id;
    rtems_status_code              sc = rtems_task_construct(&config, &id);

    if (sc != RTEMS_SUCCESSFUL)
    {
        return sc;
    }
    return rtems_task_start(id, hk_collection_task, 0u);
}
