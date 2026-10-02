/*
 * hk_time_rtems.c (target build only)
 *
 * Onboard time (hk_time.h, HKS-SR-070) from the RTEMS uptime clock, as
 * CUC 4+2. The epoch is boot: the baseline has no time correlation with
 * ground yet (design/data-model.md), so coarse time is seconds since boot.
 */

#include <rtems.h>

#include "hk_time.h"

#define NS_PER_S 1000000000u

uint64_t hk_time_now_cuc(void)
{
    struct timespec up = {0, 0};

    /* Only fails for a NULL argument. */
    (void)rtems_clock_get_uptime(&up);

    const uint64_t coarse = (uint64_t)(uint32_t)up.tv_sec;
    const uint64_t fine   = ((uint64_t)up.tv_nsec << 16) / NS_PER_S; /* 2^-16 s */
    return (coarse << 16) | (fine & 0xFFFFu);
}
