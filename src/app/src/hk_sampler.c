/*
 * hk_sampler.c
 *
 * Sampler (HKS-SR-010, HKS-SR-020, HKS-SR-030, HKS-SR-040). One pass over
 * the fixed parameter table per collection cycle, each value read through
 * the parameter-source interface only (SR-030). A read that is not
 * HK_PARAM_OK writes the stale marker, records the status, and the loop
 * continues: one failing source never stops the cycle (SR-040). The loop
 * bound is the compile-time table size (coding-standard.md rule 2).
 */

#include "hk_sampler.h"

#include <stddef.h>

void hk_sampler_collect(hk_report_t* report)
{
    if (report == NULL)
    {
        return;
    }

    for (uint8_t i = 0u; i < HK_PARAM_COUNT; i++)
    {
        uint32_t          value = 0u;
        hk_param_status_t st    = hk_param_source_read(g_param_ids[i], &value);

        if (st == HK_PARAM_OK)
        {
            report->values[i] = value;
        }
        else
        {
            report->values[i] = HK_STALE_MARKER; /* SR-040 */
        }
        report->status[i] = (uint8_t)st;
    }
}
