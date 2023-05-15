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

#include "cmsis_os.h"
#include "mv_syscalls.h"

#include "messages.h"
#include "hardware_test.h"
#include "network.h"
#include "test_runner.h"
#include "system.h"

osThreadId_t HardwareTestTask;
const osThreadAttr_t HardwareTestTask_attributes = {
    .name = "HardwareTestTask",
    .priority = (osPriority_t)osPriorityNormal,
    .stack_size = configMINIMAL_STACK_SIZE
};

osThreadId_t NetworkTask;
const osThreadAttr_t NetworkTask_attributes = {
    .name = "NetworkTask",
    .priority = (osPriority_t)osPriorityNormal,
    .stack_size = configMINIMAL_STACK_SIZE
};

osThreadId_t TestRunnerTask;
const osThreadAttr_t TestRunnerTask_attributes = {
    .name = "TestRunnerTask",
    .priority = (osPriority_t)osPriorityNormal,
    .stack_size = configMINIMAL_STACK_SIZE
};

void        SystemClock_Config(void);

static struct NetworkTaskArgument network_task_argument;
static struct HardwareTestTaskArgument hardware_test_task_argument;
static struct HardwareTestTaskArgument test_runner_task_argument;

int main(void) {
    /* Initialize the system */
    system_init();

    /* Init scheduler */
    osKernelInitialize();

    network_task_argument.in_queue = xQueueCreate(5, sizeof(enum Message));
    network_task_argument.out_queue = xQueueCreate(5, sizeof(enum Message));

    hardware_test_task_argument.in_message_queue = xQueueCreate(5, sizeof(enum Message));
    hardware_test_task_argument.out_result_queue = xQueueCreate(5, sizeof(struct HardwareTestResultMessage));

    /* Create the thread(s) */
    HardwareTestTask = osThreadNew(start_hardware_test_task, &hardware_test_task_argument, &HardwareTestTask_attributes);
    NetworkTask = osThreadNew(start_network_task, &network_task_argument, &NetworkTask_attributes);
    TestRunnerTask = osThreadNew(start_test_runner_task, &test_runner_task_argument, &TestRunnerTask_attributes);

    osKernelStart();

    while (1) {}
}
