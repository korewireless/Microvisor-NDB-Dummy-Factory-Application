#include "test_runner.h"

#include "logger.h"
#include "messages.h"
#include "gpio_shorts_test.h"

#include "mv_syscalls.h"

#include <stdio.h>
#include <string.h>

static QueueHandle_t gpio_shorts_test_in_queue;
static QueueHandle_t gpio_shorts_test_out_queue;

#define GPIO_SHORTS_TEST_TIMEOUT_MS 10000
#define NETWORK_CONNECTION_TIMEOUT_MS 30000

static int do_run_test() {
    enum Message message = StartTestMessage;
    if (!xQueueSend(gpio_shorts_test_in_queue, &message, 0)) {
        server_log("test runner: failed to send to FreeRTOS queue");
        return InternalErrorTestResult;
    }

    struct TestResultMessage result_message;
    if (!xQueueReceive(gpio_shorts_test_out_queue, &result_message, GPIO_SHORTS_TEST_TIMEOUT_MS)) {
	server_log("test runner: GPIO shorts test timed out");
        return GpioShortsTestTimeoutTestResult;
    }

    return result_message.result_code;
}

void start_test_runner_task(void *argument) {
    struct TestRunnerTaskArgument *typed_argument = (struct TestRunnerTaskArgument *) argument;

    gpio_shorts_test_in_queue = typed_argument->gpio_shorts_test_in_queue;
    gpio_shorts_test_out_queue = typed_argument->gpio_shorts_test_out_queue;

    server_log("test runner: starting the test");
    const int result = do_run_test();
    server_log("test runner: test complete with result %d", result);
    mvTestingComplete(result);
}

