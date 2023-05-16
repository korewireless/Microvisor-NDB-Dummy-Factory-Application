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

#include "FreeRTOS.h"
#include "mv_syscalls.h"

#include "messages.h"
#include "hardware_test.h"
#include "network.h"
#include "test_runner.h"
#include "system.h"
#include "logger.h"

static struct NetworkTaskArgument network_task_argument;
static struct HardwareTestTaskArgument hardware_test_task_argument;
static struct HardwareTestTaskArgument test_runner_task_argument;

int main(void) {
    /* Initialize the system */
    system_init();

    /* Initialize the logging */
    server_log_init();

    server_log("main: initializing the threads");

    network_task_argument.in_queue = xQueueCreate(5, sizeof(enum Message));
    network_task_argument.out_queue = xQueueCreate(5, sizeof(enum Message));

    hardware_test_task_argument.in_message_queue = xQueueCreate(5, sizeof(enum Message));
    hardware_test_task_argument.out_result_queue = xQueueCreate(5, sizeof(struct HardwareTestResultMessage));

    /* Create the thread(s) */
    if (!xTaskCreate(start_hardware_test_task,
                     "HardwareTestTask",
                     configMINIMAL_STACK_SIZE/sizeof(StackType_t),
                     &hardware_test_task_argument,
                     0,
                     NULL))
    {
        mvTestingComplete(InternalErrorTestResult);
    }

    if (!xTaskCreate(start_network_task,
                     "NetworkTask",
                     configMINIMAL_STACK_SIZE/sizeof(StackType_t),
                     &network_task_argument,
                     0,
                     NULL))
    {
        mvTestingComplete(InternalErrorTestResult);
    }

    if (!xTaskCreate(start_test_runner_task,
                     "TestRunnerTask",
                     configMINIMAL_STACK_SIZE/sizeof(StackType_t),
                     &test_runner_task_argument,
                     0,
                     NULL))
    {
        mvTestingComplete(InternalErrorTestResult);
    }

    vTaskStartScheduler();

    server_log("main: threads initialised, waiting for test to complete");
    while (1) {}
}
