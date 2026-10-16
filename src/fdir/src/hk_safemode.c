/*
 * hk_safemode.c
 *
 * Safe-mode controller (HKS-FD-060, HKS-FD-080). The indicator is this
 * module's private data, and like the reporting flag (DD-07) every access
 * is under the critical section here, not in the caller. The event is sent
 * outside the critical section, so the lock is never held across the sink.
 */

#include "hk_safemode.h"

#include "hk_critical.h"
#include "hk_event.h"

static bool s_safe_mode; /* protected by the critical section */

void hk_safemode_init(void)
{
    hk_critical_enter();
    s_safe_mode = false;
    hk_critical_leave();
}

void hk_safemode_enter(uint64_t now_cuc)
{
    bool first = false;

    hk_critical_enter();
    if (!s_safe_mode)
    {
        s_safe_mode = true;
        first       = true;
    }
    hk_critical_leave();

    if (first)
    {
        /* A dropped event is not retried: the indicator is already set and
         * the critical limit event that caused it identifies the cause. */
        (void)hk_event_safe_mode(now_cuc); /* FD-080 */
        /* Hook: system-level safing actions (powering down non-essential
         * loads, reorienting) are invoked here in a full system. They are
         * outside this software unit (requirements/fdir-increment.md). */
    }
}

bool hk_safemode_active(void)
{
    bool active;

    hk_critical_enter();
    active = s_safe_mode;
    hk_critical_leave();
    return active;
}

void hk_safemode_clear(void) /* FD-080: ground-commanded only */
{
    hk_critical_enter();
    s_safe_mode = false;
    hk_critical_leave();
}
