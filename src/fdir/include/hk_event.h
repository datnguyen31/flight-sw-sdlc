#ifndef HK_EVENT_H
#define HK_EVENT_H

/*
 * Event reporter: PUS Service 5 (HKS-FD-040, HKS-FD-050, HKS-FD-070,
 * HKS-FD-080; DD-12)
 *
 * Encodes event reports as TM[5,s] packets, with the subtype s carrying
 * the severity, and emits them through the telemetry-sink interface
 * (hk_tm_sink.h), the same one the housekeeping report uses. Events have
 * their own APID and sequence count, so the housekeeping stream is
 * unchanged. Layout: design/data-model.md, "Event report".
 *
 * Called from the collection task only (FDIR monitor and safe-mode entry),
 * so the packet buffer and sequence count need no locking.
 */

#include <stdbool.h>
#include <stdint.h>

#include "hk_limits.h"
#include "hk_param_source.h"
#include "hk_pus_encoder.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define HK_EVT_APID         0x0CBu /* FDIR events (DD-12) */
#define HK_EVT_SVC          5u
#define HK_EVT_SUB_INFO     1u /* informative */
#define HK_EVT_SUB_LOW      2u /* low severity: warning limit */
#define HK_EVT_SUB_MEDIUM   3u /* medium severity: stale parameter */
#define HK_EVT_SUB_HIGH     4u /* high severity: critical limit, safe mode */
#define HK_EVT_DATA_BYTES   5u /* event id 2, parameter id 2, condition 1 */
#define HK_EVT_PACKET_BYTES (HK_TM_HEADER_BYTES + HK_EVT_DATA_BYTES + HK_PEC_BYTES)
#define HK_EVT_PARAM_NONE   0x0000u /* parameter field of a non-parameter event */

    typedef enum
    {
        HK_EVT_ID_LIMIT     = 1, /* condition: hk_limit_state_t (FD-040) */
        HK_EVT_ID_STALE     = 2, /* condition: hk_param_status_t (FD-070) */
        HK_EVT_ID_SAFE_MODE = 3  /* condition: 0 (FD-080) */
    } hk_event_id_t;

    /* Reset the event sequence count. Call once at startup. */
    void hk_event_init(void);

    /* Confirmed limit condition on parameter id, stamped now_cuc. Subtype
     * HK_EVT_SUB_LOW for a warning state, HK_EVT_SUB_HIGH for a critical
     * one (FD-050). Returns false if st is HK_LIMIT_OK (nothing to report)
     * or the sink refused the packet. */
    bool hk_event_limit(uint64_t now_cuc, hk_param_id_t id, hk_limit_state_t st);

    /* Parameter id stale for HK_STALE_M cycles, last status st (FD-070). */
    bool hk_event_stale(uint64_t now_cuc, hk_param_id_t id, hk_param_status_t st);

    /* Safe mode entered (FD-080). */
    bool hk_event_safe_mode(uint64_t now_cuc);

#ifdef __cplusplus
}
#endif

#endif /* HK_EVENT_H */
