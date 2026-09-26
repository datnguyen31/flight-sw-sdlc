/*
 * hk_report_builder.c
 *
 * Report Builder (HKS-SR-050, HKS-SR-070). Stamps the collected report with
 * the fixed structure identifier and the onboard collection time. The
 * values are already in fixed table order from the Sampler; nothing is
 * allocated (HKS-SR-160).
 */

#include "hk_report_builder.h"

#include <stddef.h>

void hk_report_build(hk_report_t* report, uint64_t now_cuc)
{
    if (report == NULL)
    {
        return;
    }

    report->structure_id        = HK_BASELINE_SID; /* SR-050 */
    report->collection_time_cuc = now_cuc;         /* SR-070 */
}
