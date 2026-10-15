/*
 * st_param_provider.c (system test image only)
 *
 * Representative parameter provider with a fault injector
 * (hk_param_source.h; verification/system-test-spec.md). It replaces the
 * simulator of the flight image, src/app/rtems/hk_param_source_sim.c.
 *
 * Nominal: parameter index i reads (low_warn + high_warn) / 2 + i, the
 * middle of its soft band plus its own index. Every value is nominal (the
 * narrowest soft band is 200 counts wide), constant from cycle to cycle,
 * and tells the capture which report position it came from, so packet
 * parameter order is checkable (ST-05). Nothing is stale.
 *
 * Fault injection: st_fault_*() overrides one parameter with a scripted
 * value or with HK_PARAM_STALE until it is cleared. The sequencer runs in
 * the Init task and changes an override only half-way between collection
 * releases, so a read never sees an override half-written. The fields are
 * volatile because the two tasks share them.
 */

#include <stdbool.h>
#include <stddef.h>

#include "hk_limits.h"
#include "hk_report.h"
#include "hk_sampler.h"
#include "st_rig.h"

typedef enum
{
    OVR_NONE  = 0,
    OVR_VALUE = 1,
    OVR_STALE = 2
} override_t;

static volatile override_t s_mode[HK_PARAM_COUNT];
static volatile uint32_t   s_value[HK_PARAM_COUNT];

static bool index_of(hk_param_id_t id, uint8_t* out)
{
    for (uint8_t i = 0u; i < HK_PARAM_COUNT; i++)
    {
        if (g_param_ids[i] == id)
        {
            *out = i;
            return true;
        }
    }
    return false;
}

uint32_t st_nominal_value(uint8_t i)
{
    return ((g_limits[i].low_warn + g_limits[i].high_warn) / 2u) + (uint32_t)i;
}

hk_param_status_t hk_param_source_read(hk_param_id_t id, uint32_t* out_value)
{
    uint8_t i = 0u;

    if ((out_value == NULL) || !index_of(id, &i))
    {
        return HK_PARAM_UNKNOWN;
    }
    switch (s_mode[i])
    {
        case OVR_STALE:
            return HK_PARAM_STALE;
        case OVR_VALUE:
            *out_value = s_value[i];
            return HK_PARAM_OK;
        default:
            *out_value = st_nominal_value(i);
            return HK_PARAM_OK;
    }
}

void st_fault_value(hk_param_id_t id, uint32_t value)
{
    uint8_t i = 0u;

    if (index_of(id, &i))
    {
        s_value[i] = value;
        s_mode[i]  = OVR_VALUE;
    }
}

void st_fault_stale(hk_param_id_t id)
{
    uint8_t i = 0u;

    if (index_of(id, &i))
    {
        s_mode[i] = OVR_STALE;
    }
}

void st_fault_clear(hk_param_id_t id)
{
    uint8_t i = 0u;

    if (index_of(id, &i))
    {
        s_mode[i] = OVR_NONE;
    }
}
