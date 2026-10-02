#ifndef HK_PARAM_SOURCE_H
#define HK_PARAM_SOURCE_H

/*
 * Parameter-Source Interface (HKS-SR-030, HKS-SR-040)
 *
 * Abstracts where housekeeping parameter values come from, so the
 * collection path can be tested against a stub source and swapped for a
 * real one in flight.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef uint16_t hk_param_id_t;

    typedef enum
    {
        HK_PARAM_OK      = 0, /* fresh, valid value this cycle */
        HK_PARAM_STALE   = 1, /* no fresh value this cycle (SR-040) */
        HK_PARAM_UNKNOWN = 2  /* id not served by this source */
    } hk_param_status_t;

    /* Non-blocking, time-bounded read of one parameter (SR-150). */
    hk_param_status_t hk_param_source_read(hk_param_id_t id, uint32_t* out_value);

#ifdef __cplusplus
}
#endif

#endif /* HK_PARAM_SOURCE_H */
