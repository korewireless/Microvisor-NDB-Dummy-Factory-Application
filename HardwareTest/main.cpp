/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include "freertos_thread.h"
#include "mv_syscalls.h"

#include "messages.h"
#include "gpio_shorts_test.h"
#include "test_runner.h"
#include "system.h"
#include "logger.h"


int main(void) {
    /* Initialize the system */
    system_init();

    /* Initialize the logging */
    server_log_init();

    //server_log("main: initializing the threads");

    QueueHandle_t gpio_shorts_in_queue = xQueueCreate(5, sizeof(enum Message));
    QueueHandle_t gpio_shorts_out_queue = xQueueCreate(5, sizeof(struct TestResultMessage));

    /* Create the threads */

    static auto gpio_thread = GpioShortsTest(gpio_shorts_in_queue, gpio_shorts_out_queue);
    static auto runner_thread = TestRunner(gpio_shorts_out_queue, gpio_shorts_in_queue);

    vTaskStartScheduler();

    //server_log("main: threads initialised, waiting for test to complete");
    while (1) {}
}
