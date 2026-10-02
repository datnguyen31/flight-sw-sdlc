/* test/unit/test_hk_safemode.c (verifies HKS-FD-060, FD-080) */
#include "unity.h"
#include "hk_command.h"
#include "hk_event.h"
#include "hk_safemode.h"
#include "hk_test_doubles.h"

void setUp(void) {
  hk_safemode_init();
  hk_event_init();
  hk_capture_sink_reset();
}
void tearDown(void) { }

#define T0 0x0000000A00000000u

/* Inactive at startup */
void test_inactive_after_init(void) {
  TEST_ASSERT_FALSE(hk_safemode_active());
}

/* FD-080: entry sets the indicator and emits one safe-mode event */
void test_entry_sets_indicator_and_emits_event(void) {
  hk_safemode_enter(T0);
  TEST_ASSERT_TRUE(hk_safemode_active());
  TEST_ASSERT_EQUAL_size_t(1u, hk_capture_sink_count());
  TEST_ASSERT_EQUAL_HEX8(HK_EVT_SVC, hk_capture_sink_last()[7]);
  TEST_ASSERT_EQUAL_HEX8(HK_EVT_ID_SAFE_MODE, hk_capture_sink_last()[18]);
}

/* FD-080: entry is idempotent, reported only on the first transition */
void test_repeated_entry_reports_once(void) {
  for (uint32_t k = 0u; k < 5u; k++) {
    hk_safemode_enter(T0);
  }
  TEST_ASSERT_TRUE(hk_safemode_active());
  TEST_ASSERT_EQUAL_size_t(1u, hk_capture_sink_count());
}

/* FD-080: the ground clear command returns safe mode to inactive, and a
 * later entry is a new transition, reported again */
void test_ground_clear_then_reentry(void) {
  hk_safemode_enter(T0);
  TEST_ASSERT_EQUAL(HK_CMD_ACCEPTED, hk_command_dispatch(HK_CMD_CLEAR_SAFE_MODE));
  TEST_ASSERT_FALSE(hk_safemode_active());

  hk_safemode_enter(T0);
  TEST_ASSERT_TRUE(hk_safemode_active());
  TEST_ASSERT_EQUAL_size_t(2u, hk_capture_sink_count());
}

/* FD-080: nothing but the clear command leaves safe mode; the other
 * telecommands, valid or not, leave it set */
void test_only_clear_command_leaves_safe_mode(void) {
  hk_safemode_enter(T0);
  (void)hk_command_dispatch(HK_CMD_ENABLE_REPORTS);
  (void)hk_command_dispatch(HK_CMD_DISABLE_REPORTS);
  TEST_ASSERT_EQUAL(HK_CMD_REJECTED, hk_command_dispatch((hk_command_id_t)99));
  TEST_ASSERT_TRUE(hk_safemode_active());
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_inactive_after_init);
  RUN_TEST(test_entry_sets_indicator_and_emits_event);
  RUN_TEST(test_repeated_entry_reports_once);
  RUN_TEST(test_ground_clear_then_reentry);
  RUN_TEST(test_only_clear_command_leaves_safe_mode);
  return UNITY_END();
}
