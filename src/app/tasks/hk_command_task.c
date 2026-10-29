/*
 * hk_command_task.c (target build only)
 *
 * Command task (HKS-SR-100, HKS-SR-110, HKS-SR-130, HKS-SR-180). Blocks on
 * a statically constructed message queue and hands each telecommand to
 * hk_command_dispatch(), which validates it and updates the HK Manager
 * under the critical section. No blocking work besides the receive.
 *
 * The uplink delivers frames of bytes. A frame of the wrong length is
 * malformed: it is rejected here, before dispatch, and logged like an
 * unknown opcode, so ground sees every rejection (HKS-SR-130). A frame
 * longer than a telecommand never reaches the task: the queue refuses it
 * at hk_command_uplink_frame().
 */

#include <rtems.h>
#include <rtems/bspIo.h>

#include "hk_command.h"
#include "hk_tasks.h"

RTEMS_ALIGNED(RTEMS_TASK_STORAGE_ALIGNMENT)
static char s_storage[RTEMS_TASK_STORAGE_SIZE(HK_TASK_STACK_SIZE + HK_TASK_MAX_TLS_SIZE,
                                              RTEMS_DEFAULT_ATTRIBUTES)];

static RTEMS_MESSAGE_QUEUE_BUFFER(sizeof(hk_command_id_t)) s_queue_buffers[HK_COMMAND_QUEUE_DEPTH];

static rtems_id s_cmd_queue;

static rtems_task hk_command_task(rtems_task_argument arg)
{
    (void)arg;
    for (;;)
    {
        hk_command_id_t cmd;
        size_t          size = 0u;

        if (rtems_message_queue_receive(s_cmd_queue, &cmd, &size, RTEMS_WAIT, RTEMS_NO_TIMEOUT) !=
            RTEMS_SUCCESSFUL)
        {
            continue;
        }
        if (size != sizeof(cmd))
        {
            printk("[cmd] malformed telecommand (%lu bytes) REJECTED\n", (unsigned long)size);
            continue; /* SR-130: no state change */
        }
        hk_command_result_t res = hk_command_dispatch(cmd);
        printk("[cmd] telecommand %d %s\n",
               (int)cmd,
               (res == HK_CMD_ACCEPTED) ? "accepted" : "REJECTED");
    }
}

rtems_status_code hk_command_task_start(void)
{
    static const rtems_message_queue_config queue_config =
        {.name                     = rtems_build_name('H', 'K', 'C', 'Q'),
         .maximum_pending_messages = HK_COMMAND_QUEUE_DEPTH,
         .maximum_message_size     = sizeof(hk_command_id_t),
         .storage_area             = s_queue_buffers,
         .storage_size             = sizeof(s_queue_buffers),
         .storage_free             = NULL,
         .attributes               = RTEMS_FIFO | RTEMS_LOCAL};
    static const rtems_task_config task_config = {.name = rtems_build_name('H', 'K', 'C', 'M'),
                                                  .initial_priority = HK_COMMAND_TASK_PRIORITY,
                                                  .storage_area     = s_storage,
                                                  .storage_size     = sizeof(s_storage),
                                                  .maximum_thread_local_storage_size =
                                                      HK_TASK_MAX_TLS_SIZE,
                                                  .storage_free  = NULL,
                                                  .initial_modes = RTEMS_DEFAULT_MODES,
                                                  .attributes    = RTEMS_DEFAULT_ATTRIBUTES};
    rtems_id                       id;
    rtems_status_code              sc = rtems_message_queue_construct(&queue_config, &s_cmd_queue);

    if (sc != RTEMS_SUCCESSFUL)
    {
        return sc;
    }
    sc = rtems_task_construct(&task_config, &id);
    if (sc != RTEMS_SUCCESSFUL)
    {
        return sc;
    }
    return rtems_task_start(id, hk_command_task, 0u);
}

rtems_status_code hk_command_uplink_frame(const void* frame, size_t size)
{
    return rtems_message_queue_send(s_cmd_queue, frame, size);
}

rtems_status_code hk_command_uplink(hk_command_id_t cmd)
{
    return hk_command_uplink_frame(&cmd, sizeof(cmd));
}
