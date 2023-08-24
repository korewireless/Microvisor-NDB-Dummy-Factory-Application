#include "gpio_shorts_test.h"
#include "stm32u5xx_hal.h"

#include "logger.h"
#include "messages.h"
#include "gpio.h"

#include <cstring>
#include <array>
#include <map>
#include <vector>

namespace {
class GpioShortsTestRun {
private:
  const Gpio LED_PIN = PA5;

  std::vector<Gpio> gpios_to_test;

  std::array<uint32_t, 9> gpio_in_masks = {
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

  const std::array<std::pair<Gpio, const char*>, 12> gpios_to_ignore = {{
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
    {PA5, "PA5 = USER_LED"},
  }};

  std::map<std::pair<Gpio, Gpio>, bool> loopback_baseboard_connections = {
    {{ PF6,  PC13 }, false},
    {{ PF7,  PB7  }, false},
    {{ PA0,  PG2  }, false},
    {{ PA1,  PD3  }, false},
    {{ PD1,  PD5  }, false},
    {{ PD6,  PF0  }, false},
    {{ PD7,  PF1  }, false},
    {{ PE2,  PF2  }, false},
    {{ PD0,  PD9  }, false},
    {{ PG0,  PG12 }, false},
    {{ PE1,  PG9  }, false},
    {{ PF8,  PG13 }, false},
    {{ PF9,  PG10 }, false},
    {{ PG1,  PG15 }, false},
    {{ PB9,  PA7  }, false},
    {{ PB6,  PA9  }, false},
    {{ PD8,  PB11 }, false},
    {{ PA8,  PD13 }, false},
    {{ PB4,  PA3  }, false},
    {{ PB5,  PA10 }, false},
    {{ PB2,  PB15 }, false},
    {{ PB14, PF5  }, false},
    {{ PF4,  PF10 }, false},
    {{ PD12, PE13 }, false},
    {{ PD11, PE15 }, false},
    {{ PE12, PE14 }, false},
    {{ PF14, PE9  }, false},
    {{ PF13, PD10 }, false},
    {{ PF12, PG14 }, false},
    {{ PE11, PF11 }, false},
    {{ PF3,  PF15 }, false},
    {{ PB8,  PE0  }, false},
  };

  bool mark_baseboard_short(Gpio one, Gpio another) {
    bool found = false;

    auto lr = loopback_baseboard_connections.find({one, another});
    if (lr != loopback_baseboard_connections.end()) {
      found = true;
      lr->second = true;
    }

    auto rl = loopback_baseboard_connections.find({another, one});
    if (rl != loopback_baseboard_connections.end()) {
      found = true;
      lr->second = true;
    }

    return found;
  }

  void clear_baseboard_shorts() {
    for (auto it = loopback_baseboard_connections.begin(); it != loopback_baseboard_connections.end(); it++) {
      it->second = false;
    }
  }

  bool has_all_shorts() {
    for (auto it = loopback_baseboard_connections.begin(); it != loopback_baseboard_connections.end(); it++) {
      if(!it->second) {
        return false;
      }
    }
    return true;
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


  void collect_gpios_to_test()
  {
    for (auto g : Gpio::all()) {
      bool ignore = false;
      for (auto gi : gpios_to_ignore) {
        if (g == gi.first) {
          server_log("Not testing %s\n", gi.second);
          ignore = true;
        }
      }

      if (!ignore) {
        gpios_to_test.push_back(g);
      } else{
        unsigned bank = g.port();
        uint32_t pin = g.pin();
        gpio_in_masks[bank] &= ~pin;
      }
    }
  }

  bool test_has_shorts(bool with_loopback = false);
  bool wait_loopback_board();
  bool test_with_loopback();

public:

  uint32_t test();
};


bool GpioShortsTestRun::test_has_shorts(bool with_loopback) {
  for (auto g : gpios_to_test) {
    g.tristate();
  }

  bool has_shorts = false;
  for (auto g : gpios_to_test) {
    if (g == P__) {
      continue;
    }

    g.output(GPIO_PIN_SET);
    busy_wait(10);

    for (unsigned int i = 0; i < gpio_in_masks.size(); i++) {
      GPIO_TypeDef *another_bank = Gpio::from_port_and_pin(i, 0).periph();
      unsigned idr = another_bank->IDR & gpio_in_masks[i];

      if (i == g.port()) {
        // is actually the same bank, make sure current pin is not accounted for
        idr &= ~g.pin();
      }

      for (unsigned p=0; p<16; p++) {
        if (idr & (1<<p)) {
          // Short detected between output GPIO and this pin
          const auto another_location = Gpio(((i+1)<<4) | p);

          // Now drive the GPIO low and check whether the shorted pin deasserts
          g.output(GPIO_PIN_RESET);
          const unsigned idr2 = another_bank->IDR & gpio_in_masks[i];
          if (!(idr2 & (1<<p))) {
            if (with_loopback) {
              if (!mark_baseboard_short(g, another_location)) {
                has_shorts = true;
                server_log("Illegal loopback for P%c%u <=> P%c%u\n",
                  (g >> 4) + 'A' - 1, g & 0xf,
                  (another_location >> 4) + 'A' - 1, another_location & 0xf);
              }
            } else {
              server_log("Illegal loopback for P%c%u <=> P%c%u\n",
                (g >> 4) + 'A' - 1, g & 0xf,
                (another_location >> 4) + 'A' - 1, another_location & 0xf);


              has_shorts = true;
            }
          }
        }
      }
    }
    g.tristate();
  }

  return !has_shorts;
}

bool GpioShortsTestRun::wait_loopback_board() {
  auto led_pin_state = GPIO_PIN_SET;
  constexpr int timeout_cycles = 300; // 300*100 ticks = 30 seconds
  bool success = false;
  for (int i = 0; i < timeout_cycles; i++) {
    TickType_t cycle_start = xTaskGetTickCount();
    clear_baseboard_shorts();
    if (!test_has_shorts(true)) {
      break;
    } else if(has_all_shorts()) {
      success = true;
      break;
    }

    led_pin_state = (led_pin_state == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET;
    LED_PIN.output(led_pin_state);
    vTaskDelayUntil(&cycle_start, 100);
  }

  LED_PIN.output(GPIO_PIN_RESET);
  return success;
}

bool GpioShortsTestRun::test_with_loopback() {
  clear_baseboard_shorts();
  if (!test_has_shorts(true)) {
    return false;
  }

  return has_all_shorts();
}

uint32_t GpioShortsTestRun::test() {
  server_log("Start test");

  collect_gpios_to_test();
  init_gpio_periphs();

  server_log("Start without loopback baseboard");
  if (!test_has_shorts()) {
    return GpioHasShortsResult;
  }

  server_log("Waiting for loopback baseboard");
  if (!wait_loopback_board()) {
    return GpioLoopbackTimeoutResult;
  }

  server_log("Completing loopback test");
  if (!test_with_loopback()) {
    return GpioLoopbackFailureResult;
  }

  server_log("Loopback test done");

  return OkTestResult;
}

} // anonymous namespace

bool GpioShortsTest::got_start_test_message() {
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

void GpioShortsTest::run() {
  while (1) {
    if (got_start_test_message()) {
      auto run = GpioShortsTestRun();

      TestResultMessage response = {
        .result_code = run.test(),
      };
      xQueueSend(out_queue, &response, 0);
    }

    vTaskDelay(100);
  }
}

