/*
 * hk_critical_host.c
 *
 * Host build of the critical-section interface (hk_critical.h): a no-op,
 * since the host unit-test harness is single-threaded. The target build
 * links hk_critical_rtems.c instead.
 */

#include "hk_critical.h"

void hk_critical_init(void) {}

void hk_critical_enter(void) {}

void hk_critical_leave(void) {}
