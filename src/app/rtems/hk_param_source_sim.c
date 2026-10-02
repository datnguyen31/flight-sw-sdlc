/*
 * hk_param_source_sim.c (target build only)
 *
 * Simulated parameter source (hk_param_source.h) for the emulated target,
 * standing in for the real providers until they exist (DD-03). Each value
 * is the middle of the parameter's soft band in the FDIR limit table plus
 * (cycle % 64), so every value is nominal (test_hk_limits.c checks the
 * bands are wide enough). A decoded packet then shows which collection it
 * came from, and the ground decoder, which holds its own copy of the limits,
 * can check every value. One parameter (0x0203) goes stale every third
 * cycle to exercise the stale path (HKS-SR-040) end to end. That is never
 * HK_STALE_M = 3 consecutive cycles, so it raises no FDIR event (HKS-FD-070).
 */

#include <stddef.h>

#include "hk_limits.h"
#include "hk_param_source.h"
#include "hk_report.h"
#include "hk_sampler.h"

#define SIM_STALE_ID    0x0203u
#define SIM_STALE_EVERY 3u
#define SIM_CYCLE_SPAN  64u

static uint32_t s_reads;

hk_param_status_t hk_param_source_read(hk_param_id_t id, uint32_t* out_value)
{
    const uint32_t cycle = s_reads / HK_PARAM_COUNT;

    s_reads++;
    if (out_value == NULL)
    {
        return HK_PARAM_UNKNOWN;
    }
    if ((id == SIM_STALE_ID) && ((cycle % SIM_STALE_EVERY) == 0u))
    {
        return HK_PARAM_STALE;
    }
    for (uint8_t i = 0u; i < HK_PARAM_COUNT; i++)
    {
        if (g_param_ids[i] == id)
        {
            const uint32_t mid = (g_limits[i].low_warn + g_limits[i].high_warn) / 2u;
            *out_value         = mid + (cycle % SIM_CYCLE_SPAN);
            return HK_PARAM_OK;
        }
    }
    return HK_PARAM_UNKNOWN;
}
