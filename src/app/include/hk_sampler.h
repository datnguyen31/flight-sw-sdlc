#ifndef HK_SAMPLER_H
#define HK_SAMPLER_H

/*
 * Sampler (HKS-SR-010, HKS-SR-020, HKS-SR-030, HKS-SR-040)
 *
 * Walks the fixed parameter table once per collection cycle, reading each
 * parameter through the parameter-source interface (hk_param_source.h).
 * A read that is not HK_PARAM_OK records the stale marker and the cycle
 * continues (design/detailed-design.md S1.2).
 */

#include "hk_param_source.h"
#include "hk_report.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* The defined parameter set, in report order (SR-010, SR-050). */
    extern const hk_param_id_t g_param_ids[HK_PARAM_COUNT];

    /* Fill report->values/status from the parameter sources (SR-020..040).
     * Does nothing if report is NULL. */
    void hk_sampler_collect(hk_report_t* report);

#ifdef __cplusplus
}
#endif

#endif /* HK_SAMPLER_H */
