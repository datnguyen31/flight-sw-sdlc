#ifndef HK_TIME_H
#define HK_TIME_H

/*
 * Onboard-Time Interface (HKS-SR-070)
 *
 * Supplies the collection time stamped into each report, as CUC 4+2:
 * coarse seconds in bits 47..16, binary fraction of a second in bits 15..0
 * (design/data-model.md). Only the target build implements it; host tests
 * pass the time to hk_collection_step() directly.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    uint64_t hk_time_now_cuc(void);

#ifdef __cplusplus
}
#endif

#endif /* HK_TIME_H */
