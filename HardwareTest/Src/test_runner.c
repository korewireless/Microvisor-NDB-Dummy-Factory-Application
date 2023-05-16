#include "test_runner.h"

#include "logger.h"
#include "messages.h"
#include "network.h"
#include "hardware_test.h"

#include <stdio.h>
#include <string.h>

static char http_request_buffer[512];

static QueueHandle_t hardware_test_in_queue;
static QueueHandle_t hardware_test_out_queue;

static QueueHandle_t network_in_queue;
static QueueHandle_t network_out_queue;

#define HARDWARE_TEST_TIMEOUT_MS 10000
#define NETWORK_CONNECTION_TIMEOUT_MS 30000

int do_run_test(const char** result_string) {
    enum Message message = StartTestMessage;
    if (!xQueueSend(hardware_test_in_queue, &message, 0)) {
        *result_string = "Failed to send to FreeRTOS queue";
        return InternalErrorTestResult;
    }

    struct HardwareTestResultMessage result_message;
    if (!xQueueReceive(hardware_test_out_queue, &result_message, HARDWARE_TEST_TIMEOUT_MS)) {
        *result_string = "Hardware test timed out";
        return HardwareTestTimeoutTestResult;
    }

    *result_string = result_message.failure_description;
    return result_message.result_code;
}

int do_send_test_result(int result_code, const char* result_string) {
    enum Message message = JoinNetworkMessage;

    if (!xQueueSend(network_in_queue, &message, 0)) {
        return InternalErrorTestResult;
    }

    if (!xQueueReceive(network_out_queue, &message, NETWORK_CONNECTION_TIMEOUT_MS)) {
        return NetworkTimeoutTestResult;
    }

    MvChannelHandle http_handle;
    if (open_http_channel(&http_handle) != MV_STATUS_OKAY) {
        return HttpErrorTestResult;
    }

    if (result_code != OkTestResult) {
        snprintf(http_request_buffer, sizeof(http_request_buffer), "{\"result_code\":%d,\"error_description\":\"%s\"}", result_code, result_string);
    } else {
        snprintf(http_request_buffer, sizeof(http_request_buffer), "{\"result_code\":%d}", result_code);
    }
    struct MvHttpRequest http_request = {
        .method = {
            .data = (uint8_t*) "GET",
            .length = 3
        },
        .url = {
            .data = (uint8_t*) "https://mockbin.org/request",
            .length = sizeof("https://mockbin.org/request") - 1
        },
        .body = {
            .data = (uint8_t*)http_request_buffer,
            .length = strlen(http_request_buffer)
        },
        .headers = NULL,
        .num_headers = 0,
        .timeout_ms = 5000,
    };

    if (mvSendHttpRequest(http_handle, &http_request) != MV_STATUS_OKAY) {
        return HttpErrorTestResult;
    }

    struct MvHttpResponseData response_data;
    int timeout_counter = 0;
    for (int timeout_counter = 0; timeout_counter < 10; timeout_counter++) {
        enum MvStatus status = mvReadHttpResponseData(http_handle, &response_data);

        if (status == MV_STATUS_OKAY) {
            break;
        } else if (status == MV_STATUS_RESPONSENOTPRESENT) {
            vTaskDelay(1000);
        } else {
            return HttpErrorTestResult;
        }
    }

    if (timeout_counter == 10) {
        return HttpErrorTestResult;
    }

    if (response_data.status_code == 200) {
        return result_code;
    } else {
        return HttpErrorTestResult;
    }
}

void start_test_runner_task(void *argument) {
    struct TestRunnerTaskArgument *typed_argument = (struct TestRunnerTaskArgument *) argument;
    const char *result_string;

    hardware_test_in_queue = typed_argument->hardware_test_in_queue;
    hardware_test_out_queue = typed_argument->hardware_test_out_queue;
    network_in_queue = typed_argument->network_in_queue;
    network_out_queue = typed_argument->network_out_queue;

    server_log("test runner: starting the test");
    int result = do_run_test(&result_string);
    server_log("test runner: test complete with result %d, error string %s", result, result_string);
#ifdef CONFIG_CONNECTED_FACTORY
    result = do_send_test_result(result, result_string);
#endif

    mvTestingComplete(result);
}

