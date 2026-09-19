/*
 * RTEMS build-integration smoke target.
 *
 * This is not the HK-TM application (Session 6/7 add that); it exists to
 * prove the cross-compile and link against the beagleboneblack BSP produces
 * a runnable RTEMS image. The real Init task will call into the Scheduler
 * (design/detailed-design.md S1.1) once that code exists.
 */

#include <bsp.h>
#include <rtems.h>
#include <rtems/bspIo.h>
#include <stdlib.h>

rtems_task Init(rtems_task_argument arg)
{
    (void)arg;
    printk("HK-TM RTEMS build-integration smoke test OK\n");
    exit(0);
}

#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_MAXIMUM_TASKS 2
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#define CONFIGURE_INIT
#include <rtems/confdefs.h>
