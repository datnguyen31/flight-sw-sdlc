/*
 * hk_monitor.c
 *
 * FDIR monitor (HKS-FD-020, HKS-FD-030, HKS-FD-050, HKS-FD-060,
 * HKS-FD-070; design/fdir-design.md §2). Per parameter, four run counters
 * of consecutive fresh samples drive the confirmed level:
 *
 *   n_warn      at the soft limit or beyond   NOMINAL  -> WARNING
 *   n_crit      at the hard limit or beyond   any      -> CRITICAL
 *   n_ok        within limits                 any      -> NOMINAL
 *   n_noncrit   inside the hard limits        CRITICAL -> WARNING
 *
 * A level changes when its run reaches HK_DEBOUNCE_N. That is the same
 * debounce on the way up and on the way down, so a parameter near a limit
 * does not flap. An event goes out only on a rise, so a sustained condition
 * is reported once. Counters saturate at their threshold, so they cannot
 * wrap. A stale sample (status not HK_PARAM_OK) carries no value: it
 * neither counts toward nor breaks a limit run, and drives the separate
 * stale counter instead. All state is static (HKS-FD-090).
 */

#include "hk_monitor.h"

#include <stdbool.h>
#include <stddef.h>

#include "hk_event.h"
#include "hk_limits.h"
#include "hk_safemode.h"
#include "hk_sampler.h"

_Static_assert((HK_DEBOUNCE_N >= 1u) && (HK_DEBOUNCE_N <= 255u), "N fits a uint8_t run");
_Static_assert((HK_STALE_M >= 1u) && (HK_STALE_M <= 255u), "M fits a uint8_t run");

typedef enum
{
    LEVEL_NOMINAL  = 0,
    LEVEL_WARNING  = 1,
    LEVEL_CRITICAL = 2
} level_t;

typedef struct
{
    level_t level; /* confirmed */
    uint8_t n_warn;
    uint8_t n_crit;
    uint8_t n_ok;
    uint8_t n_noncrit;
    uint8_t n_stale;
} param_state_t;

static param_state_t s_state[HK_PARAM_COUNT];

void hk_monitor_init(void)
{
    for (uint8_t i = 0u; i < HK_PARAM_COUNT; i++)
    {
        s_state[i] = (param_state_t) {.level = LEVEL_NOMINAL};
    }
}

/* One more sample in a run if cond, else the run is broken. */
static uint8_t run(uint8_t n, bool cond, uint8_t cap)
{
    if (!cond)
    {
        return 0u;
    }
    return (n < cap) ? (uint8_t)(n + 1u) : n;
}

static void count_sample(param_state_t* ps, hk_limit_state_t st)
{
    const bool ok   = (st == HK_LIMIT_OK);
    const bool crit = (st == HK_CRIT_LOW) || (st == HK_CRIT_HIGH);

    ps->n_warn    = run(ps->n_warn, !ok, (uint8_t)HK_DEBOUNCE_N);
    ps->n_crit    = run(ps->n_crit, crit, (uint8_t)HK_DEBOUNCE_N);
    ps->n_ok      = run(ps->n_ok, ok, (uint8_t)HK_DEBOUNCE_N);
    ps->n_noncrit = run(ps->n_noncrit, !crit, (uint8_t)HK_DEBOUNCE_N);
}

/* The level the runs support (FD-030): unchanged until a run reaches N. */
static level_t next_level(const param_state_t* ps)
{
    level_t next = ps->level;

    if (ps->n_crit >= HK_DEBOUNCE_N)
    {
        next = LEVEL_CRITICAL;
    }
    else if (ps->n_ok >= HK_DEBOUNCE_N)
    {
        next = LEVEL_NOMINAL;
    }
    else if ((ps->level == LEVEL_NOMINAL) && (ps->n_warn >= HK_DEBOUNCE_N))
    {
        next = LEVEL_WARNING;
    }
    else if ((ps->level == LEVEL_CRITICAL) && (ps->n_noncrit >= HK_DEBOUNCE_N))
    {
        next = LEVEL_WARNING;
    }
    else
    {
        /* no run complete: level holds */
    }
    return next;
}

/* The condition to report for a newly confirmed level: its severity, on
 * the side (low or high) of this sample. A WARNING confirmed on a sample
 * already past the hard limit reports the soft limit on that side. */
static hk_limit_state_t confirmed_condition(level_t level, hk_limit_state_t st)
{
    const bool low = (st == HK_WARN_LOW) || (st == HK_CRIT_LOW);

    if (level == LEVEL_CRITICAL)
    {
        return low ? HK_CRIT_LOW : HK_CRIT_HIGH;
    }
    return low ? HK_WARN_LOW : HK_WARN_HIGH;
}

static void check_value(
    param_state_t* ps, hk_param_id_t id, uint32_t value, const hk_limit_t* lim, uint64_t now_cuc)
{
    const hk_limit_state_t st = hk_limit_classify(value, lim);

    count_sample(ps, st);
    const level_t next = next_level(ps);

    if (next > ps->level) /* a rise is a newly confirmed condition */
    {
        /* A dropped event is not retried: the level stays confirmed and the
         * housekeeping values still show the condition to ground. */
        (void)hk_event_limit(now_cuc, id, confirmed_condition(next, st)); /* FD-040, 050 */
        if (next == LEVEL_CRITICAL)
        {
            hk_safemode_enter(now_cuc); /* FD-060 */
        }
    }
    ps->level = next;
}

static void check_stale(param_state_t* ps, hk_param_id_t id, hk_param_status_t st, uint64_t now_cuc)
{
    if (ps->n_stale < HK_STALE_M)
    {
        ps->n_stale++;
        if (ps->n_stale == HK_STALE_M) /* once per stale run (FD-070) */
        {
            (void)hk_event_stale(now_cuc, id, st); /* dropped: not retried */
        }
    }
}

void hk_monitor_check(const hk_report_t* report)
{
    if (report == NULL)
    {
        return;
    }

    for (uint8_t i = 0u; i < HK_PARAM_COUNT; i++) /* FD-020 */
    {
        param_state_t* ps = &s_state[i];

        if (report->status[i] != (uint8_t)HK_PARAM_OK)
        {
            check_stale(ps,
                        g_param_ids[i],
                        (hk_param_status_t)report->status[i],
                        report->collection_time_cuc);
        }
        else
        {
            ps->n_stale = 0u;
            check_value(ps,
                        g_param_ids[i],
                        report->values[i],
                        &g_limits[i],
                        report->collection_time_cuc);
        }
    }
}
