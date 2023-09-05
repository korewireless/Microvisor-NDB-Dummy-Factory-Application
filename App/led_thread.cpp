#include "led_thread.h"
#include "stm32u5xx_hal.h"

#include "logger.h"
#include "messages.h"
#include "gpio.h"

#include <cstring>
#include <array>
#include <map>
#include <vector>

namespace {
class LedController {
private:
  TIM_HandleTypeDef timer;

  // GPIO, timer and channel are hard-coded, can become configurable if needed
  const Gpio LED_PIN = PA5;
  TIM_TypeDef* regs = TIM2;
  constexpr static auto channel = TIM_CHANNEL_1;

  constexpr static auto gpio_af = GPIO_AF1_TIM2;

  void init_periphs()
  {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOA_FORCE_RESET();
    __HAL_RCC_GPIOA_RELEASE_RESET();

    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_TIM2_FORCE_RESET();
    __HAL_RCC_TIM2_RELEASE_RESET();
  }

  void init_pwm();
  void set_duty_cycle(int);

public:

  void run();
};

void LedController::init_pwm() {
  LED_PIN.af_pp(gpio_af);

  memset(&timer, 0, sizeof(timer));
  timer.Instance = regs;
  timer.Init.CounterMode = TIM_COUNTERMODE_UP;
  timer.Init.Prescaler = 80;
  timer.Init.Period = 2501;
  timer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  timer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  HAL_TIM_PWM_Init(&timer);

  TIM_OC_InitTypeDef channel_config = {0};
  channel_config.OCMode = TIM_OCMODE_PWM1;
  channel_config.OCIdleState = TIM_OCIDLESTATE_SET;
  channel_config.Pulse = 2500;
  channel_config.OCPolarity = TIM_OCPOLARITY_HIGH;
  channel_config.OCFastMode = TIM_OCFAST_ENABLE;

  HAL_TIM_PWM_ConfigChannel(&timer, &channel_config, channel);

  HAL_TIM_GenerateEvent(&timer, channel);

  HAL_TIM_PWM_Start(&timer, channel);
}

void LedController::set_duty_cycle(int duty) {
  if (duty < 0) {
    duty = 0;
  }

  if (duty > 100) {
    duty = 100;
  }

  regs->CCR1 = 25*duty;
}

void LedController::run() {
  init_periphs();
  init_pwm();

  int duty = 1;
  bool up = true;
  for (;;) {
    set_duty_cycle(duty);

    if (up) {
      if (duty == 0) {
        duty = 1;
      } else {
        duty *= 2;
      }
      if (duty >= 100) {
	up = false;
      }
    } else {
      duty /= 2;
      if (duty == 0) {
	up = true;
      }
    }

    vTaskDelay(100);
  }
}

} // anonymous namespace

void LedThread::run() {
  auto controller = LedController();
  controller.run();
}
