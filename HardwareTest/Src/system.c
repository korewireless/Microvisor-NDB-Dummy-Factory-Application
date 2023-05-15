#include "system.h"
#include "stm32u5xx_hal.h"

/**
 * @brief Get the MV clock value.
 *
 * @retval The clock value.
 */
uint32_t SECURE_SystemCoreClockUpdate() {
    uint32_t clock = 0;
    mvGetHClk(&clock);
    return clock;
}

/**
 * @brief System clock configuration.
 */
static void SystemClock_Config(void) {
    SystemCoreClockUpdate();
    HAL_InitTick(TICK_INT_PRIORITY);
}

void system_init() {
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();
}
