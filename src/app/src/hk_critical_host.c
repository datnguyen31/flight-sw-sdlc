/*
 * hk_critical_host.c
 *
 * Host build of the critical-section interface (hk_critical.h): a no-op,
 * since the host unit-test harness is single-threaded. RTEMS provides the
 * real interrupt-disable/mutex implementation in Session 7.
 */

#include "hk_critical.h"

void hk_critical_enter(void) {}

void hk_critical_leave(void) {}