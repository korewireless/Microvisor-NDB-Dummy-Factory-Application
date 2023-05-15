#include "hardware_test.h"
#include "stm32u5xx_hal.h"

#include "messages.h"

#include <stdbool.h>
#include <string.h>

static QueueHandle_t in_queue;
static QueueHandle_t out_queue;

static char failure_string_buffer[128];

static void gpio_init(void) {
    // Enable GPIO port clock
    __HAL_RCC_GPIOA_CLK_ENABLE();

    __HAL_RCC_GPIOA_FORCE_RESET();
    __HAL_RCC_GPIOA_RELEASE_RESET();

    // Configure GPIO pin : PA7 - Pin under test
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin   = GPIO_PIN_7;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA_NS, &GPIO_InitStruct);
}

static bool got_start_test_message() {
    enum Message message = NoneMessage;

    while (xQueueReceive(in_queue, &message, 0)) {
        switch (message) {
            case StartTestMessage:
                return true;
            default:
                break;
        }
    }
    return false;
}

static int do_test_hardware()
{
    // A simple example of a hardware self-test. Check if a pin
    //   is pulled low externally.
    gpio_init();

    GPIO_PinState state = HAL_GPIO_ReadPin(GPIOA_NS, GPIO_PIN_7);

    if (state == GPIO_PIN_RESET) {
        return OkTestResult;
    } else {
        strncpy(failure_string_buffer,
                 "Pin PA7 is HIGH when LOW is expected",
                 sizeof(failure_string_buffer));
        return GpioHighTestResult;
    }
}

void start_hardware_test_task(void *argument) {
    struct HardwareTestTaskArgument *typed_argument = (struct HardwareTestTaskArgument*) argument;
    out_queue = typed_argument->out_result_queue;
    in_queue = typed_argument->in_message_queue;

    // The task's main loop
    while (1) {
        if (got_start_test_message()) {
            int result = do_test_hardware();

            struct HardwareTestResultMessage response = {
                .result_code = result,
                .failure_description = failure_string_buffer
            };

            xQueueSend(out_queue, &response, 0);
        }

        vTaskDelay(100);
    }
}

