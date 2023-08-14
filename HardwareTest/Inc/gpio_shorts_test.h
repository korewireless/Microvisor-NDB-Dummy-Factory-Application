/**
 *
 * Microvisor Hardware Test Sample
 * Version 1.0.0
 * Copyright © 2023, Kore Wireless
 * Licence: Apache 2.0
 *
 */

#ifndef GPIO_SHORTS_TEST_H
#define GPIO_SHORTS_TEST_H

#include "FreeRTOS.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

struct GpioShortsTestTaskArgument
{
    QueueHandle_t in_message_queue;
    QueueHandle_t out_result_queue;
};

void start_gpio_shorts_test_task(void *argument);

#ifdef __cplusplus
}
#endif

#endif // GPIO_SHORTS_TEST_H
