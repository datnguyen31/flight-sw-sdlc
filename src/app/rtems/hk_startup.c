/*
 * hk_startup.c (target build only)
 *
 * HK-TM service startup (hk_startup.h). All tasks and resources are
 * created here, once, statically (HKS-SR-180).
 */

#include "hk_startup.h"

#include <rtems/bspIo.h>

#include "hk_collection.h"
#include "hk_critical.h"
#include "hk_event.h"
#include "hk_manager.h"
#include "hk_monitor.h"
#include "hk_safemode.h"
#include "hk_tasks.h"

void hk_startup_check(rtems_status_code sc, const char* what)
{
    if (sc != RTEMS_SUCCESSFUL)
    {
        printk("[init] %s failed: %s\n", what, rtems_status_text(sc));
        rtems_fatal(RTEMS_FATAL_SOURCE_APPLICATION, HK_FATAL_STARTUP);
    }
}

void hk_service_start(void)
{
    hk_critical_init();
    hk_manager_init(); /* reporting DISABLED (SR-120) */
    hk_collection_init();
    hk_safemode_init(); /* safe mode inactive (FD-080) */
    hk_monitor_init();  /* every parameter NOMINAL */
    hk_event_init();
    hk_startup_check(hk_command_task_start(), "command task");
    hk_startup_check(hk_collection_task_start(), "collection task");
    printk("[init] tasks started; reporting disabled until commanded\n");
}
