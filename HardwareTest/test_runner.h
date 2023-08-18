/**
 *
 * Microvisor Test Runner
 * Version 1.0.0
 * Copyright © 2023, Twilio
 * Licence: Apache 2.0
 *
 */

#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include "FreeRTOS.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

struct TestRunnerTaskArgument
{
    QueueHandle_t gpio_shorts_test_in_queue;
    QueueHandle_t gpio_shorts_test_out_queue;
};

void start_test_runner_task(void *argument);

#ifdef __cplusplus
}
#endif



#endif /* NETWORK_H */
