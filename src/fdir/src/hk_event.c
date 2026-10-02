/*
 * hk_event.c
 *
 * Event reporter (HKS-FD-040, HKS-FD-050, HKS-FD-070, HKS-FD-080). The three
 * public calls are thin wrappers over one encoder, which uses the shared
 * TM header and PEC writers (hk_pus_encoder.h), so an event packet follows
 * the same header discipline as the housekeeping report. A warning and a
 * critical event differ only in subtype and condition. The packet buffer is
 * static (HKS-FD-090).
 */

#include "hk_event.h"

#include "hk_tm_sink.h"

#define CCSDS_SEQ_COUNT_MASK 0x3FFFu

typedef struct
{
    uint8_t       subtype;
    hk_event_id_t event_id;
    hk_param_id_t param_id;
    uint8_t       condition;
} event_t;

static uint8_t  s_packet[HK_EVT_PACKET_BYTES];
static uint16_t s_seq; /* CCSDS sequence count for HK_EVT_APID, 14 bits */

void hk_event_init(void)
{
    s_seq = 0u;
}

static size_t encode(const event_t* evt, uint64_t now_cuc)
{
    const hk_tm_header_t hdr = {.apid         = HK_EVT_APID,
                                .seq          = s_seq,
                                .service      = HK_EVT_SVC,
                                .subtype      = evt->subtype,
                                .packet_bytes = HK_EVT_PACKET_BYTES,
                                .time_cuc     = now_cuc};
    uint8_t*             p   = hk_pus_put_tm_header(s_packet, &hdr);

    /* Source data: event definition id, then auxiliary data */
    p    = hk_pus_put16(p, (uint16_t)evt->event_id);
    p    = hk_pus_put16(p, evt->param_id);
    *p++ = evt->condition;
    return hk_pus_put_pec(s_packet, p);
}

static bool emit(const event_t* evt, uint64_t now_cuc)
{
    const size_t n = encode(evt, now_cuc);

    /* Advances per generated packet, as for housekeeping, so ground sees a
     * gap for an event the sink dropped. */
    s_seq = (uint16_t)((s_seq + 1u) & CCSDS_SEQ_COUNT_MASK);
    return hk_tm_sink_send(s_packet, n);
}

bool hk_event_limit(uint64_t now_cuc, hk_param_id_t id, hk_limit_state_t st)
{
    event_t evt = {.subtype   = HK_EVT_SUB_LOW,
                   .event_id  = HK_EVT_ID_LIMIT,
                   .param_id  = id,
                   .condition = (uint8_t)st};

    if (st == HK_LIMIT_OK)
    {
        return false;
    }
    if ((st == HK_CRIT_LOW) || (st == HK_CRIT_HIGH)) /* FD-050 */
    {
        evt.subtype = HK_EVT_SUB_HIGH;
    }
    return emit(&evt, now_cuc);
}

bool hk_event_stale(uint64_t now_cuc, hk_param_id_t id, hk_param_status_t st)
{
    const event_t evt = {.subtype   = HK_EVT_SUB_MEDIUM,
                         .event_id  = HK_EVT_ID_STALE,
                         .param_id  = id,
                         .condition = (uint8_t)st};

    return emit(&evt, now_cuc);
}

bool hk_event_safe_mode(uint64_t now_cuc)
{
    const event_t evt = {.subtype   = HK_EVT_SUB_HIGH,
                         .event_id  = HK_EVT_ID_SAFE_MODE,
                         .param_id  = HK_EVT_PARAM_NONE,
                         .condition = 0u};

    return emit(&evt, now_cuc);
}
