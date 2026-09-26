#ifndef HK_TASKS_H
#define HK_TASKS_H

/*
 * HK-TM RTEMS tasks (HKS-SR-180; design/architecture.md task model)
 *
 * Two statically defined tasks. Priorities and stack sizes are fixed here
 * and inspected against the architecture (SR-180); task storage and the
 * command queue buffers are statically allocated and handed to RTEMS with
 * the *_construct directives, so no task resource comes from the heap
 * (SR-160). RTEMS priorities: lower number is more urgent.
 */

#include <rtems.h>

#include "hk_command.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Command task above collection task (architecture.md). */
#define HK_COMMAND_TASK_PRIORITY    10u
#define HK_COLLECTION_TASK_PRIORITY 20u
#define HK_TASK_STACK_SIZE          (2u * RTEMS_MINIMUM_STACK_SIZE)
#define HK_TASK_MAX_TLS_SIZE        256u /* = CONFIGURE_MAXIMUM_THREAD_LOCAL_STORAGE_SIZE */

#define HK_COLLECTION_PERIOD_MS 1000u /* SR-020 default */
#define HK_COMMAND_QUEUE_DEPTH  4u

    /* Construct the command queue and start the command task. */
    rtems_status_code hk_command_task_start(void);

    /* Construct and start the collection task. */
    rtems_status_code hk_collection_task_start(void);

    /* Uplink ingress: queue one telecommand for the command task. */
    rtems_status_code hk_command_uplink(hk_command_id_t cmd);

#ifdef __cplusplus
}
#endif

#endif /* HK_TASKS_H */
