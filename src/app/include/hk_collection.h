#ifndef HK_COLLECTION_H
#define HK_COLLECTION_H

/*
 * Collection cycle (HKS-SR-020, HKS-SR-060, HKS-SR-090, HKS-SR-100..120,
 * HKS-SR-160; DD-01)
 *
 * One call is one release of the collection task (design/detailed-design.md
 * S1.1): sample, build, check limits (HKS-FD-020, design/fdir-design.md),
 * count the report divider, and on a report tick with
 * reporting enabled encode and hand the packet to the telemetry sink. Pure
 * C, so the whole pipeline is unit-tested on the host; the RTEMS task in
 * src/app/tasks only supplies the period and the time.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define HK_REPORT_DIVIDER 4u /* 4000 ms report / 1000 ms collection (SR-060) */

    typedef enum
    {
        HK_STEP_COLLECTED         = 0, /* collection tick, no report due */
        HK_STEP_REPORT_SUPPRESSED = 1, /* report tick, reporting disabled */
        HK_STEP_REPORT_SENT       = 2, /* report tick, packet accepted by sink */
        HK_STEP_SINK_REJECTED     = 3  /* report tick, sink refused the packet */
    } hk_step_result_t;

    /* Reset the divider and sequence count. Call once at startup. */
    void hk_collection_init(void);

    /* Run one collection cycle stamped with now_cuc (CUC 4+2). */
    hk_step_result_t hk_collection_step(uint64_t now_cuc);

#ifdef __cplusplus
}
#endif

#endif /* HK_COLLECTION_H */
