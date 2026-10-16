/*
 * init.c (target build only)
 *
 * RTEMS system initialization for the HK-TM service, then a scripted
 * ground-station sequence for the end-to-end run under emulation.
 *
 * Startup order matters: hk_critical_init() creates the mutex the HK
 * Manager and the safe-mode controller use, so it runs before
 * hk_manager_init() and hk_safemode_init(); all run before either task
 * starts (hk_critical.h). All tasks and resources are created here, once,
 * statically (HKS-SR-180).
 *
 * The ground sequence stands in for an uplink: enable reporting (SR-100),
 * send an unknown telecommand that must be rejected with no state change
 * (SR-130), then disable reporting (SR-110) and confirm the stream stops.
 */

#include <rtems.h>
#include <rtems/bspIo.h>
#include <rtems/version.h>
#include <stdlib.h>

#include "hk_collection.h"
#include "hk_critical.h"
#include "hk_event.h"
#include "hk_manager.h"
#include "hk_monitor.h"
#include "hk_safemode.h"
#include "hk_tasks.h"

#define HK_FATAL_STARTUP  0x484B494Eu /* 'HKIN' */
#define GROUND_UNKNOWN_TC ((hk_command_id_t)0x7F)

static void check_startup(rtems_status_code sc, const char* what)
{
    if (sc != RTEMS_SUCCESSFUL)
    {
        printk("[init] %s failed: %s\n", what, rtems_status_text(sc));
        rtems_fatal(RTEMS_FATAL_SOURCE_APPLICATION, HK_FATAL_STARTUP);
    }
}

static void ground_wait_ms(uint32_t ms)
{
    (void)rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(ms));
}

static void ground_uplink(hk_command_id_t cmd, const char* what)
{
    printk("[ground] uplink %s\n", what);
    check_startup(hk_command_uplink(cmd), "uplink");
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

    hk_critical_init();
    hk_manager_init(); /* reporting DISABLED (SR-120) */
    hk_collection_init();
    hk_safemode_init(); /* safe mode inactive (FD-080) */
    hk_monitor_init();  /* every parameter NOMINAL */
    hk_event_init();
    check_startup(hk_command_task_start(), "command task");
    check_startup(hk_collection_task_start(), "collection task");
    printk("[init] tasks started; reporting disabled until commanded\n");

    ground_script();

    printk("[init] HK-TM end-to-end run complete\n");
    exit(0);
}

/* RTEMS configuration: every object the service uses is counted here, so
 * the object pools are sized at build time (HKS-SR-160, SR-180). */
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER

#define CONFIGURE_MICROSECONDS_PER_TICK                    1000
#define CONFIGURE_MAXIMUM_TASKS                            3 /* Init + command + collection */
#define CONFIGURE_MINIMUM_TASKS_WITH_USER_PROVIDED_STORAGE 2
#define CONFIGURE_MAXIMUM_THREAD_LOCAL_STORAGE_SIZE        HK_TASK_MAX_TLS_SIZE
#define CONFIGURE_MAXIMUM_SEMAPHORES                       1 /* HK Manager + safe-mode mutex */
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES                   1 /* command queue */
#define CONFIGURE_MAXIMUM_PERIODS                          1 /* collection period */

#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_INIT_TASK_ATTRIBUTES RTEMS_DEFAULT_ATTRIBUTES

#define CONFIGURE_INIT
#include <rtems/confdefs.h>
