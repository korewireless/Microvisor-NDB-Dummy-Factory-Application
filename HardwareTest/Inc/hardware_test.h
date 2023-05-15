/**
 *
 * Microvisor Hardware Test Sample
 * Version 1.0.0
 * Copyright © 2023, Twilio
 * Licence: Apache 2.0
 *
 */

#ifndef HARDWARE_TEST_H
#define HARDWARE_TEST_H

#include "FreeRTOS.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

struct HardwareTestTaskArgument
{
    QueueHandle_t in_message_queue;
    QueueHandle_t out_result_queue;
};

void start_hardware_test_task(void *argument);

#ifdef __cplusplus
}
#endif

#endif // HARDWARE_TEST_H
