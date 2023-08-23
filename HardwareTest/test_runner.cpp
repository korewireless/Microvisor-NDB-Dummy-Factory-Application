#include "test_runner.h"

#include "logger.h"
#include "messages.h"
#include "gpio_shorts_test.h"

#include "mv_syscalls.h"

#include <stdio.h>
#include <string.h>

#define GPIO_SHORTS_TEST_TIMEOUT_MS 600000
#define NETWORK_CONNECTION_TIMEOUT_MS 30000

void TestRunner::run() {
    server_log("test runner: starting the test");

    enum Message message = StartTestMessage;
    if (!xQueueSend(out_queue, &message, 0)) {
        server_log("test runner: failed to send to FreeRTOS queue");
        mvTestingComplete(InternalErrorTestResult);
        return;
    }

    struct TestResultMessage result_message;
    if (!xQueueReceive(in_queue, &result_message, GPIO_SHORTS_TEST_TIMEOUT_MS)) {
        server_log("test runner: GPIO shorts test timed out");
        mvTestingComplete(GpioShortsTestTimeoutTestResult);
        return;
    }

    server_log("test runner: test complete with result %d", result_message.result_code);
    mvTestingComplete(result_message.result_code);
}

