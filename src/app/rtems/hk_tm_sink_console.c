/*
 * hk_tm_sink_console.c (target build only)
 *
 * Console telemetry sink (hk_tm_sink.h, HKS-SR-090): writes each packet
 * to the debug UART as one line, "TM <hex bytes>", which
 * tools/hk_tm_decode.py parses and checks as an independent ground
 * decoder. Stands in for a real downlink until one exists (DD-03).
 */

#include <rtems/bspIo.h>

#include "hk_tm_sink.h"

bool hk_tm_sink_send(const uint8_t* packet, size_t length)
{
    if ((packet == NULL) || (length == 0u))
    {
        return false;
    }

    printk("TM ");
    for (size_t i = 0u; i < length; i++)
    {
        printk("%02X", (unsigned int)packet[i]);
    }
    printk("\n");
    return true;
}
