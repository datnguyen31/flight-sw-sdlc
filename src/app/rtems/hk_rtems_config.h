#ifndef HK_RTEMS_CONFIG_H
#define HK_RTEMS_CONFIG_H

/*
 * RTEMS configuration for every HK-TM target image (target build only).
 *
 * Include from exactly one translation unit per image, the one that
 * defines Init: it instantiates the configuration tables (confdefs.h).
 * The flight image (init.c) and the system test image
 * (test/system/st_main.c) share it, so both run with the same object
 * pools. Every object the service uses is counted here, so the pools are
 * sized at build time (HKS-SR-160, SR-180). Both images run their ground
 * script in the Init task, so it is counted as the third task.
 */

#include "hk_tasks.h"

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

#endif /* HK_RTEMS_CONFIG_H */
