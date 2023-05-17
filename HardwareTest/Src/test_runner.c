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
        server_log("test runner: error sending to network queue");
        return InternalErrorTestResult;
    }

    if (!xQueueReceive(network_out_queue, &message, NETWORK_CONNECTION_TIMEOUT_MS)) {
        server_log("test runner: time out joining network");
        return NetworkTimeoutTestResult;
    }

    MvChannelHandle http_handle;
    enum MvStatus status = open_http_channel(&http_handle);
    if (status != MV_STATUS_OKAY) {
        server_log("test runner: failed to open HTTP channel: %d", status);
        return HttpErrorTestResult;
    }

    if (result_code != OkTestResult) {
        snprintf(http_request_buffer, sizeof(http_request_buffer), "{\"result_code\":%d,\"error_description\":\"%s\"}", result_code, result_string);
    } else {
        snprintf(http_request_buffer, sizeof(http_request_buffer), "{\"result_code\":%d}", result_code);
    }

    const char *content_type = "Content-Type: application/json";
    struct MvHttpHeader header = {
        .length = strlen(content_type),
	.data = (uint8_t*) content_type
    };

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
        .headers = &header,
        .num_headers = 1,
        .timeout_ms = 5000,
    };

    status = mvSendHttpRequest(http_handle, &http_request);
    if (status != MV_STATUS_OKAY) {
        server_log("test runner: failed to send HTTP request: %d", status);
        return HttpErrorTestResult;
    }

    struct MvHttpResponseData response_data;
    int timeout_counter = 0;
    for (int timeout_counter = 0; timeout_counter < 10; timeout_counter++) {
        status = mvReadHttpResponseData(http_handle, &response_data);

        if (status == MV_STATUS_OKAY) {
            break;
        } else if (status == MV_STATUS_RESPONSENOTPRESENT) {
            vTaskDelay(1000);
        } else {
            server_log("test runner: failed to get HTTP response: %d", status);
            return HttpErrorTestResult;
        }
    }

    if (timeout_counter == 10) {
        server_log("test runner: time out on HTTP response");
        return HttpErrorTestResult;
    }

    if (response_data.result != MV_HTTPRESULT_OK) {
        server_log("test runner: HTTP request failed %d", response_data.result);
        return HttpErrorTestResult;
    }

    if (response_data.status_code == 200) {
        return result_code;
    } else {
        server_log("test runner: HTTP endpoint returned code %d", response_data.status_code);
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
    server_log("test runner: test complete with result %d, error string \"%s\"", result, result_string);
#ifdef CONFIG_CONNECTED_FACTORY
    server_log("test runner: sending result to remote endpoint");
    result = do_send_test_result(result, result_string);
    server_log("test runner: sending complete, final result code is %d", result);
#endif

    mvTestingComplete(result);
}

