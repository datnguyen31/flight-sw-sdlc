/*
 * hk_param_source_sim.c (target build only)
 *
 * Simulated parameter source (hk_param_source.h) for the emulated target,
 * standing in for the real providers until they exist (DD-03). Each value
 * is (id << 16) | cycle, so a decoded packet shows which parameter and
 * which collection it came from. One parameter (0x0203) goes stale every
 * third cycle to exercise the stale path (HKS-SR-040) end to end.
 */

#include <stddef.h>

#include "hk_param_source.h"
#include "hk_report.h"

#define SIM_STALE_ID    0x0203u
#define SIM_STALE_EVERY 3u

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
    *out_value = ((uint32_t)id << 16) | (cycle & 0xFFFFu);
    return HK_PARAM_OK;
}
