/**
 *
 * Microvisor Network Helper
 * Version 1.0.0
 * Copyright © 2022, Twilio
 * Licence: Apache 2.0
 *
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "FreeRTOS.h"
#include "queue.h"
#include "mv_syscalls.h"

/*
 * These functions take ownership from Microvisor for the network state.  By default on
 * Microvisor startup, the network will always be avaialble until you tell it otherwise.
 *
 * These calls manage the network handle from Microvisor which allows you to open
 * additional communication channels (for example, HTTP or MQTT) and provide a
 * notification center which will alert you to changes in network status.
 */

#ifdef __cplusplus
extern "C" {
#endif

struct NetworkTaskArgument
{
    QueueHandle_t in_queue;
    QueueHandle_t out_queue;
};

void start_network_task(void *argument);
enum MvStatus open_http_channel(MvChannelHandle *handle_out);

#ifdef __cplusplus
}
#endif

#endif /* NETWORK_H */
