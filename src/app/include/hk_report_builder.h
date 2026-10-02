#ifndef HK_REPORT_BUILDER_H
#define HK_REPORT_BUILDER_H

/*
 * Report Builder (HKS-SR-050, HKS-SR-070)
 *
 * Stamps a collected report with the fixed structure identifier and the
 * onboard collection time. Allocates nothing; the report lives in the
 * caller's static storage (HKS-SR-160).
 */

#include <stdint.h>

#include "hk_report.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* now_cuc: CUC 4+2, coarse seconds << 16 | fine (design/data-model.md).
     * Does nothing if report is NULL. */
    void hk_report_build(hk_report_t* report, uint64_t now_cuc);

#ifdef __cplusplus
}
#endif

#endif /* HK_REPORT_BUILDER_H */
