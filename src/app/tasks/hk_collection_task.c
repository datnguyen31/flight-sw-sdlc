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
 *
 * Timing instrumentation (Session 9, verification/timing-measured.md):
 * each release reads the uptime clock on wake-up and after
 * hk_collection_step(). It keeps the best and worst cycle time of a
 * release that sent no report, the worst of one that did, and the worst
 * deviation of the wake-up from its ideal release time. The record is
 * printed every HK_TIMING_PRINT_EVERY cycles. Its cost is two clock reads
 * per cycle; it stays in the flight build so the same figures can be read
 * on flight-representative hardware (DD-17).
 */

#include <rtems.h>
#include <rtems/bspIo.h>

#include "hk_collection.h"
#include "hk_safemode.h"
#include "hk_tasks.h"
#include "hk_time.h"

#define NS_PER_MS             1000000u
#define HK_TIMING_PRINT_EVERY 60u
#define HK_TIMING_ANCHOR      2u /* first timer-released cycle */

RTEMS_ALIGNED(RTEMS_TASK_STORAGE_ALIGNMENT)
static char s_storage[RTEMS_TASK_STORAGE_SIZE(HK_TASK_STACK_SIZE + HK_TASK_MAX_TLS_SIZE,
                                              RTEMS_DEFAULT_ATTRIBUTES)];

static hk_timing_t s_timing; /* collection task only */

static uint64_t hk_timer_now(void)
{
    return rtems_clock_get_uptime_nanoseconds();
}

/* Record one release: woke at t_wake, step done at t_done. The ideal
 * releases are anchored on t_anchor, release HK_TIMING_ANCHOR: release 1
 * is the period's first call, which returns at once, part-way through a
 * clock tick; from release 2 on, the period's tick-based timer releases
 * the task, so that is the schedule whose jitter is measured. */
static void timing_record(uint64_t t_anchor, uint64_t t_wake, uint64_t t_done, hk_step_result_t res)
{
    const uint64_t dt = t_done - t_wake;

    if (s_timing.releases >= HK_TIMING_ANCHOR)
    {
        const uint64_t ideal = t_anchor + ((uint64_t)(s_timing.releases - HK_TIMING_ANCHOR) *
                                           (uint64_t)HK_COLLECTION_PERIOD_MS * NS_PER_MS);
        const uint64_t dev   = (t_wake > ideal) ? (t_wake - ideal) : (ideal - t_wake);

        if (dev > s_timing.jitter_max_ns)
        {
            s_timing.jitter_max_ns    = dev;
            s_timing.jitter_max_cycle = s_timing.releases;
        }
    }
    if (res == HK_STEP_REPORT_SENT)
    {
        if (dt > s_timing.report_max_ns)
        {
            s_timing.report_max_ns    = dt;
            s_timing.report_max_cycle = s_timing.releases;
        }
    }
    else
    {
        if ((s_timing.collect_min_ns == 0u) || (dt < s_timing.collect_min_ns))
        {
            s_timing.collect_min_ns = dt;
        }
        if (dt > s_timing.collect_max_ns)
        {
            s_timing.collect_max_ns    = dt;
            s_timing.collect_max_cycle = s_timing.releases;
        }
    }
}

static void timing_print(void)
{
    printk("[timing] cycles %lu overruns %lu collect_min %lu ns collect_max %lu ns (cycle %lu) "
           "report_max %lu ns (cycle %lu) jitter_max %lu ns (cycle %lu)\n",
           (unsigned long)s_timing.releases,
           (unsigned long)s_timing.overruns,
           (unsigned long)s_timing.collect_min_ns,
           (unsigned long)s_timing.collect_max_ns,
           (unsigned long)s_timing.collect_max_cycle,
           (unsigned long)s_timing.report_max_ns,
           (unsigned long)s_timing.report_max_cycle,
           (unsigned long)s_timing.jitter_max_ns,
           (unsigned long)s_timing.jitter_max_cycle);
}

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
    uint64_t             t_anchor = 0u;

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
            s_timing.overruns++;
        }
        const uint64_t t_wake = hk_timer_now();
        s_timing.releases++;
        if (s_timing.releases == HK_TIMING_ANCHOR)
        {
            t_anchor = t_wake;
        }

        hk_step_result_t res = hk_collection_step(hk_time_now_cuc());

        timing_record(t_anchor, t_wake, hk_timer_now(), res);
        printk("[hk] cycle %3lu: %s (overruns %lu)%s\n",
               (unsigned long)s_timing.releases,
               step_name(res),
               (unsigned long)s_timing.overruns,
               hk_safemode_active() ? " [SAFE MODE]" : "");
        if ((s_timing.releases % HK_TIMING_PRINT_EVERY) == 0u)
        {
            timing_print();
        }
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
