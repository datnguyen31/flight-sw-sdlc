/* test/unit/test_hk_command.c */
#include "unity.h"
#include "hk_command.h"
#include "hk_manager.h"
#include "hk_safemode.h"

void setUp(void) { /* each test starts DISABLED, not in safe mode */
  hk_manager_init();
  hk_safemode_init();
}
void tearDown(void) { }

/* HKS-SR-120: reporting disabled after init */
void test_default_state_is_disabled(void) {
  TEST_ASSERT_FALSE(hk_manager_reports_enabled());
}

/* HKS-SR-100: enable is accepted and turns reporting on */
void test_enable_accepted(void) {
  TEST_ASSERT_EQUAL(HK_CMD_ACCEPTED, hk_command_dispatch(HK_CMD_ENABLE_REPORTS));
  TEST_ASSERT_TRUE(hk_manager_reports_enabled());
}

/* HKS-SR-110: disable is accepted and turns reporting off */
void test_disable_accepted(void) {
  (void)hk_command_dispatch(HK_CMD_ENABLE_REPORTS);
  TEST_ASSERT_EQUAL(HK_CMD_ACCEPTED, hk_command_dispatch(HK_CMD_DISABLE_REPORTS));
  TEST_ASSERT_FALSE(hk_manager_reports_enabled());
}

/* HKS-SR-130: unknown command rejected, state unchanged */
void test_unknown_rejected_no_change(void) {
  (void)hk_command_dispatch(HK_CMD_ENABLE_REPORTS);
  TEST_ASSERT_EQUAL(HK_CMD_REJECTED, hk_command_dispatch((hk_command_id_t)99));
  TEST_ASSERT_TRUE(hk_manager_reports_enabled());
}

/* HKS-FD-080: clear safe mode is accepted, leaves safe mode, and leaves
 * the reporting state alone */
void test_clear_safe_mode_accepted(void) {
  (void)hk_command_dispatch(HK_CMD_ENABLE_REPORTS);
  hk_safemode_enter(0u);
  TEST_ASSERT_EQUAL(HK_CMD_ACCEPTED, hk_command_dispatch(HK_CMD_CLEAR_SAFE_MODE));
  TEST_ASSERT_FALSE(hk_safemode_active());
  TEST_ASSERT_TRUE(hk_manager_reports_enabled());
}

/* HKS-SR-130 still holds with the new opcode: unknown rejected, safe mode
 * unchanged */
void test_unknown_rejected_safe_mode_unchanged(void) {
  hk_safemode_enter(0u);
  TEST_ASSERT_EQUAL(HK_CMD_REJECTED, hk_command_dispatch((hk_command_id_t)4));
  TEST_ASSERT_TRUE(hk_safemode_active());
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_default_state_is_disabled);
  RUN_TEST(test_enable_accepted);
  RUN_TEST(test_disable_accepted);
  RUN_TEST(test_unknown_rejected_no_change);
  RUN_TEST(test_clear_safe_mode_accepted);
  RUN_TEST(test_unknown_rejected_safe_mode_unchanged);
  return UNITY_END();
}
