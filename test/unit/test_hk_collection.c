/* test/unit/test_hk_collection.c (verifies HKS-SR-060, SR-090; gating
 * SR-100..120 through the full pipeline) */
#include "unity.h"
#include "hk_collection.h"
#include "hk_manager.h"
#include "hk_pus_encoder.h"
#include "hk_test_doubles.h"

void setUp(void) {
  hk_manager_init();
  hk_collection_init();
  hk_stub_source_reset();
  hk_capture_sink_reset();
}
void tearDown(void) { }

static uint64_t cuc_seconds(uint32_t s) { return (uint64_t)s << 16; }

/* HKS-SR-120: disabled at start, so report ticks emit nothing */
void test_disabled_emits_nothing(void) {
  for (uint32_t t = 1u; t <= 2u * HK_REPORT_DIVIDER; t++) {
    hk_step_result_t res = hk_collection_step(cuc_seconds(t));
    TEST_ASSERT_EQUAL((t % HK_REPORT_DIVIDER == 0u) ? HK_STEP_REPORT_SUPPRESSED
                                                    : HK_STEP_COLLECTED,
                      res);
  }
  TEST_ASSERT_EQUAL_size_t(0u, hk_capture_sink_count());
}

/* HKS-SR-060: one report every HK_REPORT_DIVIDER collection cycles, each
 * built from a just-completed collection */
void test_enabled_reports_every_divider_cycles(void) {
  hk_manager_set_reports_enabled(true);

  for (uint32_t t = 1u; t <= 3u * HK_REPORT_DIVIDER; t++) {
    hk_step_result_t res = hk_collection_step(cuc_seconds(t));
    TEST_ASSERT_EQUAL((t % HK_REPORT_DIVIDER == 0u) ? HK_STEP_REPORT_SENT
                                                    : HK_STEP_COLLECTED,
                      res);
    TEST_ASSERT_EQUAL_size_t(t / HK_REPORT_DIVIDER, hk_capture_sink_count());
  }
  TEST_ASSERT_EQUAL_size_t(HK_PARAM_COUNT * 3u * HK_REPORT_DIVIDER,
                           hk_stub_source_read_count());
}

/* HKS-SR-090: the sink receives the complete encoded packet, stamped with
 * the time of the collection it reports (SR-070) */
void test_sink_receives_encoded_packet(void) {
  hk_manager_set_reports_enabled(true);
  for (uint32_t t = 1u; t <= HK_REPORT_DIVIDER; t++) {
    (void)hk_collection_step(cuc_seconds(t) | 0x8000u);
  }

  const uint8_t *pkt = hk_capture_sink_last();
  TEST_ASSERT_EQUAL_size_t(HK_MAX_PACKET, hk_capture_sink_last_len());
  TEST_ASSERT_EQUAL_HEX8(0x08, pkt[0]); /* TM, sec-hdr, APID hi */
  TEST_ASSERT_EQUAL_HEX8(0xCA, pkt[1]);
  TEST_ASSERT_EQUAL_HEX8(0x03, pkt[7]); /* service 3 */
  TEST_ASSERT_EQUAL_HEX8(0x19, pkt[8]); /* subtype 25 */
  /* CUC of the 4th collection: coarse 4 s, fine 0x8000 (0.5 s) */
  const uint8_t cuc[6] = {0x00, 0x00, 0x00, 0x04, 0x80, 0x00};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(cuc, &pkt[11], sizeof cuc);
  TEST_ASSERT_EQUAL_HEX8(HK_BASELINE_SID, pkt[17]);
  /* first value is the stub's value for the first parameter */
  const uint32_t v0 = ((uint32_t)pkt[18] << 24) | ((uint32_t)pkt[19] << 16) |
                      ((uint32_t)pkt[20] << 8) | pkt[21];
  TEST_ASSERT_EQUAL_HEX32(hk_stub_value(0x0101u), v0);
  /* the PEC in the packet checks against the rest of the packet */
  const uint16_t pec = (uint16_t)((pkt[HK_MAX_PACKET - 2u] << 8) |
                                  pkt[HK_MAX_PACKET - 1u]);
  TEST_ASSERT_EQUAL_HEX16(hk_crc16_ccitt(pkt, HK_MAX_PACKET - 2u), pec);
}

/* Consecutive packets carry consecutive sequence counts */
void test_sequence_count_increments_per_packet(void) {
  hk_manager_set_reports_enabled(true);
  for (uint32_t t = 1u; t <= HK_REPORT_DIVIDER; t++) {
    (void)hk_collection_step(cuc_seconds(t));
  }
  TEST_ASSERT_EQUAL_HEX8(0x00, hk_capture_sink_last()[3]);
  for (uint32_t t = 1u; t <= HK_REPORT_DIVIDER; t++) {
    (void)hk_collection_step(cuc_seconds(t));
  }
  TEST_ASSERT_EQUAL_HEX8(0x01, hk_capture_sink_last()[3]);
}

/* DD-01: the divider keeps counting while disabled, so enabling mid-way
 * keeps reports aligned to the original cadence */
void test_enable_midway_keeps_report_alignment(void) {
  (void)hk_collection_step(cuc_seconds(1u));
  (void)hk_collection_step(cuc_seconds(2u));
  hk_manager_set_reports_enabled(true);

  TEST_ASSERT_EQUAL(HK_STEP_COLLECTED, hk_collection_step(cuc_seconds(3u)));
  TEST_ASSERT_EQUAL(HK_STEP_REPORT_SENT, hk_collection_step(cuc_seconds(4u)));
}

/* A sink rejection is reported; the sequence count still advances so
 * ground can detect the dropped packet */
void test_sink_rejection_reported_and_counted(void) {
  hk_manager_set_reports_enabled(true);
  hk_capture_sink_set_accept(false);
  for (uint32_t t = 1u; t < HK_REPORT_DIVIDER; t++) {
    (void)hk_collection_step(cuc_seconds(t));
  }
  TEST_ASSERT_EQUAL(HK_STEP_SINK_REJECTED,
                    hk_collection_step(cuc_seconds(HK_REPORT_DIVIDER)));

  hk_capture_sink_set_accept(true);
  for (uint32_t t = 1u; t <= HK_REPORT_DIVIDER; t++) {
    (void)hk_collection_step(cuc_seconds(t));
  }
  TEST_ASSERT_EQUAL_HEX8(0x01, hk_capture_sink_last()[3]);
}

/* HKS-SR-110 through the pipeline: disabling stops further packets */
void test_disable_stops_packets(void) {
  hk_manager_set_reports_enabled(true);
  for (uint32_t t = 1u; t <= HK_REPORT_DIVIDER; t++) {
    (void)hk_collection_step(cuc_seconds(t));
  }
  hk_manager_set_reports_enabled(false);
  for (uint32_t t = 1u; t <= 2u * HK_REPORT_DIVIDER; t++) {
    (void)hk_collection_step(cuc_seconds(t));
  }
  TEST_ASSERT_EQUAL_size_t(1u, hk_capture_sink_count());
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_disabled_emits_nothing);
  RUN_TEST(test_enabled_reports_every_divider_cycles);
  RUN_TEST(test_sink_receives_encoded_packet);
  RUN_TEST(test_sequence_count_increments_per_packet);
  RUN_TEST(test_enable_midway_keeps_report_alignment);
  RUN_TEST(test_sink_rejection_reported_and_counted);
  RUN_TEST(test_disable_stops_packets);
  return UNITY_END();
}
