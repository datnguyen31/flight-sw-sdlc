/* test/unit/test_hk_manager.c */
#include "unity.h"
#include "hk_manager.h"

void setUp(void) { hk_manager_init(); } /* each test starts DISABLED */
void tearDown(void) { }

/* HKS-SR-120: reporting disabled after init */
void test_init_starts_disabled(void) {
  TEST_ASSERT_FALSE(hk_manager_reports_enabled());
}

/* DD-07: setter enables reporting, getter reflects it */
void test_setter_enables(void) {
  hk_manager_set_reports_enabled(true);
  TEST_ASSERT_TRUE(hk_manager_reports_enabled());
}

/* DD-07: setter disables reporting, getter reflects it */
void test_setter_disables(void) {
  hk_manager_set_reports_enabled(true);
  hk_manager_set_reports_enabled(false);
  TEST_ASSERT_FALSE(hk_manager_reports_enabled());
}

/* detailed-design.md S1.1: the per-tick sync hook is callable and inert */
void test_on_tick_is_callable(void) {
  hk_manager_set_reports_enabled(true);
  hk_manager_on_tick();
  TEST_ASSERT_TRUE(hk_manager_reports_enabled());
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_init_starts_disabled);
  RUN_TEST(test_setter_enables);
  RUN_TEST(test_setter_disables);
  RUN_TEST(test_on_tick_is_callable);
  return UNITY_END();
}
