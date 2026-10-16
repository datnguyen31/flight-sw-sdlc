/* test/unit/test_hk_event.c (verifies HKS-FD-040, FD-050; event layout
 * per design/data-model.md) */
#include "unity.h"
#include "hk_event.h"
#include "hk_test_doubles.h"

void setUp(void) {
  hk_event_init();
  hk_capture_sink_reset();
}
void tearDown(void) { }

#define T0 0x0000000102030405u /* coarse 0x00010203, fine 0x0405 */

/* The complete critical-limit event, derived by hand from
 * design/data-model.md. PEC computed independently (Python
 * binascii.crc_hqx, init 0xFFFF). */
static const uint8_t k_expected[HK_EVT_PACKET_BYTES] = {
    0x08, 0xCB,             /* ver 0, TM, sec-hdr 1, APID 0x0CB */
    0xC0, 0x00,             /* unsegmented, seq count 0 */
    0x00, 0x11,             /* data length: 18 bytes follow, minus 1 */
    0x20,                   /* PUS version 2, time-ref status 0 */
    0x05, 0x04,             /* service 5, subtype 4 (high severity) */
    0x00, 0x00,             /* message type counter */
    0x00, 0x01, 0x02, 0x03, /* CUC coarse */
    0x04, 0x05,             /* CUC fine */
    0x00, 0x01,             /* event id: limit */
    0x01, 0x01,             /* parameter 0x0101 */
    0x03,                   /* condition: HK_CRIT_LOW */
    0xBB, 0x0C,             /* PEC */
};

/* FD-040: a limit event identifies the parameter and the condition; the
 * whole packet matches the known vector byte for byte */
void test_limit_event_matches_known_vector(void) {
  TEST_ASSERT_TRUE(hk_event_limit(T0, 0x0101u, HK_CRIT_LOW));
  TEST_ASSERT_EQUAL_size_t(1u, hk_capture_sink_count());
  TEST_ASSERT_EQUAL_size_t(sizeof k_expected, hk_capture_sink_last_len());
  TEST_ASSERT_EQUAL_HEX8_ARRAY(k_expected, hk_capture_sink_last(),
                               sizeof k_expected);
}

/* FD-050: severity is the subtype, warning low, critical high */
void test_severity_subtypes(void) {
  (void)hk_event_limit(T0, 0x0101u, HK_WARN_LOW);
  TEST_ASSERT_EQUAL_HEX8(HK_EVT_SUB_LOW, hk_capture_sink_last()[8]);
  (void)hk_event_limit(T0, 0x0101u, HK_WARN_HIGH);
  TEST_ASSERT_EQUAL_HEX8(HK_EVT_SUB_LOW, hk_capture_sink_last()[8]);
  (void)hk_event_limit(T0, 0x0101u, HK_CRIT_HIGH);
  TEST_ASSERT_EQUAL_HEX8(HK_EVT_SUB_HIGH, hk_capture_sink_last()[8]);
  (void)hk_event_stale(T0, 0x0203u, HK_PARAM_STALE);
  TEST_ASSERT_EQUAL_HEX8(HK_EVT_SUB_MEDIUM, hk_capture_sink_last()[8]);
  (void)hk_event_safe_mode(T0);
  TEST_ASSERT_EQUAL_HEX8(HK_EVT_SUB_HIGH, hk_capture_sink_last()[8]);
}

/* Stale and safe-mode events carry their id, parameter and condition */
void test_stale_and_safe_mode_payloads(void) {
  (void)hk_event_stale(T0, 0x0203u, HK_PARAM_UNKNOWN);
  const uint8_t stale[] = {0x00, 0x02, 0x02, 0x03, 0x02};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(stale, &hk_capture_sink_last()[17], sizeof stale);

  (void)hk_event_safe_mode(T0);
  const uint8_t safe[] = {0x00, 0x03, 0x00, 0x00, 0x00};
  TEST_ASSERT_EQUAL_HEX8_ARRAY(safe, &hk_capture_sink_last()[17], sizeof safe);
}

/* Events have their own sequence count, one per generated packet,
 * advancing even when the sink drops one */
void test_sequence_count_advances_per_event(void) {
  (void)hk_event_safe_mode(T0);
  hk_capture_sink_set_accept(false);
  TEST_ASSERT_FALSE(hk_event_safe_mode(T0));
  hk_capture_sink_set_accept(true);
  (void)hk_event_safe_mode(T0);
  TEST_ASSERT_EQUAL_HEX8(0x00, hk_capture_sink_packet(0u)[3]);
  TEST_ASSERT_EQUAL_HEX8(0x02, hk_capture_sink_packet(2u)[3]);
}

/* No condition, no event */
void test_ok_state_emits_nothing(void) {
  TEST_ASSERT_FALSE(hk_event_limit(T0, 0x0101u, HK_LIMIT_OK));
  TEST_ASSERT_EQUAL_size_t(0u, hk_capture_sink_count());
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_limit_event_matches_known_vector);
  RUN_TEST(test_severity_subtypes);
  RUN_TEST(test_stale_and_safe_mode_payloads);
  RUN_TEST(test_sequence_count_advances_per_event);
  RUN_TEST(test_ok_state_emits_nothing);
  return UNITY_END();
}
