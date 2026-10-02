/*
 * hk_collection.c
 *
 * One collection cycle (design/detailed-design.md S1.1; DD-01). The report
 * is built on every release whether or not a report is due, so per-tick
 * timing is independent of reporting state. On every HK_REPORT_DIVIDER-th
 * release the divider resets and, if reporting is enabled, the report is
 * encoded and handed to the telemetry sink (SR-060, SR-090, SR-100..120).
 * The report and packet buffers are static (HKS-SR-160, DD-02).
 */

#include "hk_collection.h"

#include "hk_manager.h"
#include "hk_pus_encoder.h"
#include "hk_report_builder.h"
#include "hk_sampler.h"
#include "hk_tm_sink.h"

#define CCSDS_SEQ_COUNT_MASK 0x3FFFu

static hk_report_t s_report;
static uint8_t     s_packet[HK_MAX_PACKET];
static uint16_t    s_seq;     /* CCSDS packet sequence count, 14 bits */
static uint8_t     s_divider; /* releases since the last report tick */

void hk_collection_init(void)
{
    s_seq     = 0u;
    s_divider = 0u;
}

static hk_step_result_t emit_report(void)
{
    const size_t n = hk_pus_encode(&s_report, s_seq, s_packet, sizeof s_packet);

    /* The count advances per generated packet, so ground sees a gap for a
     * packet the sink dropped. */
    s_seq = (uint16_t)((s_seq + 1u) & CCSDS_SEQ_COUNT_MASK);

    /* n is never 0 here: arguments are non-NULL and the buffer is
     * HK_MAX_PACKET by construction. */
    if (hk_tm_sink_send(s_packet, n)) /* SR-090 */
    {
        return HK_STEP_REPORT_SENT;
    }
    return HK_STEP_SINK_REJECTED;
}

hk_step_result_t hk_collection_step(uint64_t now_cuc)
{
    hk_sampler_collect(&s_report);       /* SR-020 */
    hk_manager_on_tick();                /* sync hook, no state effect */
    hk_report_build(&s_report, now_cuc); /* SR-050, SR-070, unconditional */

    s_divider++;
    if (s_divider < HK_REPORT_DIVIDER) /* SR-060 */
    {
        return HK_STEP_COLLECTED;
    }
    s_divider = 0u;

    if (!hk_manager_reports_enabled()) /* gate (SR-100/110/120) */
    {
        return HK_STEP_REPORT_SUPPRESSED;
    }
    return emit_report();
}
