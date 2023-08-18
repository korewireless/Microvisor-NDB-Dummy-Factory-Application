#include "gpio_shorts_test.h"
#include "stm32u5xx_hal.h"

#include "logger.h"
#include "messages.h"

#include <stdbool.h>
#include <string.h>

#define ARRAY_LEN(a) (sizeof(a)/sizeof((a)[0]))

static QueueHandle_t in_queue;
static QueueHandle_t out_queue;

enum GpioLocation {
    P__ = 0,
    PA0 = 16, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
    PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
    PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, PC14, PC15,
    PD0, PD1, PD2, PD3, PD4, PD5, PD6, PD7, PD8, PD9, PD10, PD11, PD12, PD13, PD14, PD15,
    PE0, PE1, PE2, PE3, PE4, PE5, PE6, PE7, PE8, PE9, PE10, PE11, PE12, PE13, PE14, PE15,
    PF0, PF1, PF2, PF3, PF4, PF5, PF6, PF7, PF8, PF9, PF10, PF11, PF12, PF13, PF14, PF15,
    PG0, PG1, PG2, PG3, PG4, PG5, PG6, PG7, PG8, PG9, PG10, PG11, PG12, PG13, PG14, PG15,
    PH0, PH1, PH2, PH3, PH4, PH5, PH6, PH7, PH8, PH9, PH10, PH11, PH12, PH13, PH14, PH15,
    PI0, PI1, PI2, PI3, PI4, PI5, PI6, PI7, PI8
};

#define ALL_GPIOS_NUM (8*16+8) // 16 pins on PA to PH and 8 pins on PI, all initialised to P__

static inline unsigned gpio_port_for_location(enum GpioLocation location) {
    return ((location >> 4) - 1);
}

static inline GPIO_TypeDef *gpio_periph_for_location(enum GpioLocation location) {
    return (GPIO_TypeDef*) (((unsigned) GPIOA_NS) + 0x400 * gpio_port_for_location(location));
}

static inline uint32_t gpio_pin_for_location(enum GpioLocation location) {
    return 1UL << (location & 0xF);
}

static enum GpioLocation gpios_to_test[ALL_GPIOS_NUM] = {0};

static uint32_t gpio_in_masks[] = {
    0x0000ffff, // GPIOA
    0x0000ffff, // GPIOB
    0x0000ffff, // GPIOC
    0x0000ffff, // GPIOD
    0x0000ffff, // GPIOE
    0x0000ffff, // GPIOF
    0x0000ffff, // GPIOG
    0x0000ffff, // GPIOH
    0x000000ff, // GPIOI
};

struct GpioWithDescription {
    const enum GpioLocation gpio;
    const char *const description;
};

const struct GpioWithDescription gpios_to_ignore[] = {
    {PA13, "PA13 = SWDIO"},
    {PA14, "PA14 = SWCLK"},
    {PB3, "PB3 = TRACESWO"},
    {PC14, "PC14 = OSC32_IN"},
    {PC15, "PC15 = OSC32_OUT"},
    {PE3, "PE3 = LED_RED"},
    {PE6, "PE6 = LED_BLUE"},
    {PE5, "PE5 = LED_GREEN"},
    {PB13, "PB13 = ESP_BOOT"},
    {PB13, "PE4 = PROV_UART RX"},
    {PB13, "PB0 = PROV_UART TX"},
};

struct GpioShort {
    enum GpioLocation left;
    enum GpioLocation right;
    bool seen;
};

struct GpioShort loopback_baseboard_connections[] = {
    { PF6,  PC13, false },
    { PF7,  PB7 , false },
    { PA0,  PG2 , false },
    { PA1,  PD3 , false },
    { PD1,  PD5 , false },
    { PD6,  PF0 , false },
    { PD7,  PF1 , false },
    { PE2,  PF2 , false },
    { PD0,  PD9 , false },
    { PG0,  PG12, false },
    { PE1,  PG9 , false },
    { PF8,  PG13, false },
    { PF9,  PG10, false },
    { PG1,  PG15, false },
    { PB9,  PA7 , false },
    { PB6,  PA9 , false },
    { PD8,  PB11, false },
    { PA8,  PD13, false },
    { PB4,  PA3 , false },
    { PB5,  PA10, false },
    { PB2,  PB15, false },
    { PB14, PF5 , false },
    { PF4,  PF10, false },
    { PD12, PE13, false },
    { PD11, PE15, false },
    { PE12, PE14, false },
    { PF14, PE9 , false },
    { PF13, PD10, false },
    { PF12, PG14, false },
    { PE11, PF11, false },
    { PF3,  PF15, false },
    { PB8,  PE0 , false },
};

static bool mark_baseboard_short(enum GpioLocation one, enum GpioLocation another) {
    for (int i = 0; i < ARRAY_LEN(loopback_baseboard_connections); i++) {
       if (loopback_baseboard_connections[i].left == one && loopback_baseboard_connections[i].right == another) {
           loopback_baseboard_connections[i].seen = true;
           return true;
       }

       if (loopback_baseboard_connections[i].right == one && loopback_baseboard_connections[i].left == another) {
           loopback_baseboard_connections[i].seen = true;
           return true;
       }
    }
    return false;
}

static void init_gpio_periphs()
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOA_FORCE_RESET();
    __HAL_RCC_GPIOA_RELEASE_RESET();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOB_FORCE_RESET();
    __HAL_RCC_GPIOB_RELEASE_RESET();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOC_FORCE_RESET();
    __HAL_RCC_GPIOC_RELEASE_RESET();

    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOD_FORCE_RESET();
    __HAL_RCC_GPIOD_RELEASE_RESET();

    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOE_FORCE_RESET();
    __HAL_RCC_GPIOE_RELEASE_RESET();

    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOF_FORCE_RESET();
    __HAL_RCC_GPIOF_RELEASE_RESET();

    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOG_FORCE_RESET();
    __HAL_RCC_GPIOG_RELEASE_RESET();

    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOH_FORCE_RESET();
    __HAL_RCC_GPIOH_RELEASE_RESET();

    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_GPIOI_FORCE_RESET();
    __HAL_RCC_GPIOI_RELEASE_RESET();
}

static void tristate_gpio(enum GpioLocation location)
{
    if (location == P__) {
        return;
    }

    GPIO_TypeDef *periph = gpio_periph_for_location(location);
    uint32_t pin_number = gpio_pin_for_location(location);

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin   = pin_number;
    GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(periph, &GPIO_InitStruct);
}

static void output_gpio(enum GpioLocation location, GPIO_PinState state)
{
    if (location == P__) {
        return;
    }

    GPIO_TypeDef *periph = gpio_periph_for_location(location);
    uint32_t pin_number = gpio_pin_for_location(location);

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin   = pin_number;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(periph, &GPIO_InitStruct);
    HAL_GPIO_WritePin(periph, pin_number, state);
}
static void collect_gpios_to_test()
{
    unsigned i = 0;
    for (enum GpioLocation location = PA0; location <= PI8; location++) {
        bool ignore = false;
        for (int i = 0; i < ARRAY_LEN(gpios_to_ignore); i++) {
            if (location == gpios_to_ignore[i].gpio) {
                server_log("Not testing %s\n", gpios_to_ignore[i].description);
                ignore = true;
            }
        }
        if (!ignore) {
            gpios_to_test[i] = location;
            ++i;
        } else{
            unsigned bank = gpio_port_for_location(location);
            uint32_t pin = gpio_pin_for_location(location);
            gpio_in_masks[bank] &= ~pin;
        }
    }
}

// Pins set to HIGH are ignored during the test
static void check_high_pins()
{
    for (int i = 0; i < ARRAY_LEN(gpio_in_masks); i++) {
        GPIO_TypeDef *gpio = gpio_periph_for_location(PA0 + i * 16);
	uint32_t high = gpio->IDR & gpio_in_masks[i];
	gpio_in_masks[i] &= ~high;

        if (high != 0) {
            for (unsigned p = 0; p < 16; p++) {
                if (high & (1 << p)) {
                    server_log("P%c%u was unexpectedly high at startup, ignoring", 'A'+i, p);
                }
	    }
        }
    }
}

static void test_gpio_shorts(bool *gpio_test_result, bool *loopback_test_result)
{
    collect_gpios_to_test();
    init_gpio_periphs();

    for (int i = 0; i < ALL_GPIOS_NUM; i++) {
        tristate_gpio(gpios_to_test[i]);
    }

    check_high_pins();

    bool has_shorts = false;
    bool has_illegal_shorts = false;
    for (int i = 0; i < ALL_GPIOS_NUM; i++) {
        enum GpioLocation location = gpios_to_test[i];
        if (location == P__) {
            continue;
        }

        output_gpio(location, GPIO_PIN_SET);
        vTaskDelay(10);

        for (int i = 0; i < ARRAY_LEN(gpio_in_masks); i++) {
            GPIO_TypeDef *another_bank = gpio_periph_for_location(PA0 + i * 16);
            unsigned idr = another_bank->IDR & gpio_in_masks[i];
            if (i == gpio_port_for_location(location)) {
                // is actually the same bank, make sure current pin is not accounted for
                idr &= ~gpio_pin_for_location(location);
            }

            for (unsigned p=0; p<16; p++) {
                if (idr & (1<<p)) {
                    // Short detected between output GPIO and this pin
                    const enum GpioLocation another_location = (enum GpioLocation)(((i+1)<<4) | p);

                    // Now drive the GPIO low and check whether the shorted pin deasserts
                    output_gpio(location, GPIO_PIN_RESET);
                    const unsigned idr2 = another_bank->IDR & gpio_in_masks[i];
                    if (!(idr2 & (1<<p))) {
                        has_shorts = true;
			if (!mark_baseboard_short(location, another_location)) {
                            server_log("Illegal loopback for P%c%u <=> P%c%u\n",
                                (location >> 4) + 'A' - 1, location & 0xf,
                                (another_location >> 4) + 'A' - 1, another_location & 0xf);

			    has_illegal_shorts = true;
                        }
                    }
                }
            }
        }
        tristate_gpio(location);
        vTaskDelay(1);
    }

    *gpio_test_result = !has_shorts;

    if (has_illegal_shorts || !has_shorts) {
        *loopback_test_result = false;
    } else {
        bool success = true;
        for (int i = 0; i < ARRAY_LEN(loopback_baseboard_connections); i++) {
            if (!loopback_baseboard_connections[i].seen) {
                const enum GpioLocation left = loopback_baseboard_connections[i].left;
                const enum GpioLocation right = loopback_baseboard_connections[i].right;
                server_log("Missing loopback for P%c%u <=> P%c%u\n",
                       (left >> 4) + '@', left & 0xf,
                       (right >> 4) + '@', right & 0xf);
                success = false;
            }
	}
	*loopback_test_result = success;
    }
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

void start_gpio_shorts_test_task(void *argument) {
    struct GpioShortsTestTaskArgument *typed_argument = (struct GpioShortsTestTaskArgument*) argument;
    out_queue = typed_argument->out_result_queue;
    in_queue = typed_argument->in_message_queue;

    while (1) {
        if (got_start_test_message()) {
            bool gpio_pass = false;
            bool loopback_pass = false;
            test_gpio_shorts(&gpio_pass, &loopback_pass);

            struct TestResultMessage response = {
                // TODO: split gpio test and loopback test into two stages
                .result_code = !(gpio_pass || loopback_pass),
            };

            xQueueSend(out_queue, &response, 0);
        }

        vTaskDelay(100);
    }
}

