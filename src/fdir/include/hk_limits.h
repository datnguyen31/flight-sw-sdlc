#ifndef HK_LIMITS_H
#define HK_LIMITS_H

/*
 * Limit table and classification (HKS-FD-010)
 *
 * Each monitored parameter has four thresholds: hard (critical) and soft
 * (warning) limits on the low and high side, ordered
 * low_crit < low_warn <= high_warn < high_crit. Classification is a pure
 * function of one value and one row, so it is unit-tested exhaustively at
 * every threshold.
 */

#include <stdint.h>

#include "hk_report.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        uint32_t low_crit;  /* at or below: critical low */
        uint32_t low_warn;  /* below: warning low */
        uint32_t high_warn; /* above: warning high */
        uint32_t high_crit; /* at or above: critical high */
    } hk_limit_t;

    /* One row per parameter, in report order (g_param_ids, HKS-SR-050). */
    extern const hk_limit_t g_limits[HK_PARAM_COUNT];

    typedef enum
    {
        HK_LIMIT_OK  = 0,
        HK_WARN_LOW  = 1,
        HK_WARN_HIGH = 2,
        HK_CRIT_LOW  = 3,
        HK_CRIT_HIGH = 4
    } hk_limit_state_t;

    /* Classify value against lim. A NULL lim means no limits: HK_LIMIT_OK. */
    hk_limit_state_t hk_limit_classify(uint32_t value, const hk_limit_t* lim);

#ifdef __cplusplus
}
#endif

#endif /* HK_LIMITS_H */
