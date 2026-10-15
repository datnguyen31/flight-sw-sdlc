/*
 * init.c (target build only)
 *
 * RTEMS system initialization for the HK-TM service, then a scripted
 * ground-station sequence for the end-to-end run under emulation.
 *
 * The service itself starts in hk_service_start() (hk_startup.c), shared
 * with the system test image, and the RTEMS configuration is
 * hk_rtems_config.h.
 *
 * The ground sequence stands in for an uplink: enable reporting (SR-100),
 * send an unknown telecommand that must be rejected with no state change
 * (SR-130), then disable reporting (SR-110) and confirm the stream stops.
 */

#include <rtems.h>
#include <rtems/bspIo.h>
#include <rtems/version.h>
#include <stdlib.h>

#include "hk_startup.h"
#include "hk_tasks.h"

#define GROUND_UNKNOWN_TC ((hk_command_id_t)0x7F)

static void ground_wait_ms(uint32_t ms)
{
    (void)rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(ms));
}

static void ground_uplink(hk_command_id_t cmd, const char* what)
{
    printk("[ground] uplink %s\n", what);
    hk_startup_check(hk_command_uplink(cmd), "uplink");
}

static void ground_script(void)
{
    ground_wait_ms(2500u);
    ground_uplink(HK_CMD_ENABLE_REPORTS, "ENABLE_REPORTS");
    ground_wait_ms(10000u);
    ground_uplink(GROUND_UNKNOWN_TC, "unknown telecommand 0x7F");
    ground_wait_ms(6000u);
    ground_uplink(HK_CMD_DISABLE_REPORTS, "DISABLE_REPORTS");
    ground_wait_ms(6000u);
}

rtems_task Init(rtems_task_argument arg)
{
    (void)arg;
    printk("\n[init] HK-TM service starting (RTEMS %s)\n", rtems_version());

    hk_service_start();

    ground_script();

    printk("[init] HK-TM end-to-end run complete\n");
    exit(0);
}

#include "hk_rtems_config.h"
