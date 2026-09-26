#ifndef HK_REPORT_H
#define HK_REPORT_H

/*
 * Housekeeping report data model (HKS-SR-010, HKS-SR-050, HKS-SR-070,
 * HKS-SR-160)
 *
 * The concrete types the data path shares: the Sampler fills a report, the
 * Report Builder stamps it, the PUS Encoder serializes it. The report is
 * statically sized at build time (DD-02); the parameter count is the value
 * the Session 5 timing budget assumed (verification/timing-analysis.md), so
 * keep the two in step.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define HK_PARAM_COUNT  32u         /* baseline set; matches timing analysis */
#define HK_BASELINE_SID 1u          /* fixed structure id (SR-050, DD-05) */
#define HK_STALE_MARKER 0xFFFFFFFFu /* value written when a source is stale */

    typedef struct
    {
        uint8_t  structure_id;           /* SID (SR-050) */
        uint64_t collection_time_cuc;    /* onboard time, CUC 4+2 (SR-070) */
        uint32_t values[HK_PARAM_COUNT]; /* fixed order (SR-050) */
        uint8_t  status[HK_PARAM_COUNT]; /* per-parameter hk_param_status_t */
    } hk_report_t;

#ifdef __cplusplus
}
#endif

#endif /* HK_REPORT_H */
