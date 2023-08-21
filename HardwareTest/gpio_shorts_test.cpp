#include "gpio_shorts_test.h"
#include "stm32u5xx_hal.h"

#include "logger.h"
#include "messages.h"

#include <cstring>
#include <array>
#include <set>
#include <vector>


#define ARRAY_LEN(a) (sizeof(a)/sizeof((a)[0]))

static QueueHandle_t in_queue;
static QueueHandle_t out_queue;

enum GpioLocation : unsigned int {
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


struct Gpio {
  GpioLocation location;

  Gpio(GpioLocation location_in) : location(location_in) {}
  Gpio(unsigned location_in) : location((GpioLocation) location_in) {}

  unsigned port() { return ((location >> 4) - 1); }

  GPIO_TypeDef* periph() { return (GPIO_TypeDef*) (((unsigned) GPIOA_NS) + 0x400 * port()); }

  uint32_t pin() { return 1UL << (location & 0xF); }

  bool operator<(const Gpio& other) const { return location < other.location; }
  bool operator==(Gpio other) const { return location == other.location; }
  bool operator==(GpioLocation other) const { return location == other; }
  //operator GpioLocation() const { return location; }
  operator unsigned() const { return location; }

  void tristate();
  void output(GPIO_PinState state);

  static Gpio from_port_and_pin(unsigned port, unsigned pin) {
    return Gpio(PA0 + port*16 + pin);
  }

  static std::array<Gpio, 8*16+8> all() {
    std::array<Gpio, 8*16+8> res = {
      PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
      PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
      PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, PC14, PC15,
      PD0, PD1, PD2, PD3, PD4, PD5, PD6, PD7, PD8, PD9, PD10, PD11, PD12, PD13, PD14, PD15,
      PE0, PE1, PE2, PE3, PE4, PE5, PE6, PE7, PE8, PE9, PE10, PE11, PE12, PE13, PE14, PE15,
      PF0, PF1, PF2, PF3, PF4, PF5, PF6, PF7, PF8, PF9, PF10, PF11, PF12, PF13, PF14, PF15,
      PG0, PG1, PG2, PG3, PG4, PG5, PG6, PG7, PG8, PG9, PG10, PG11, PG12, PG13, PG14, PG15,
      PH0, PH1, PH2, PH3, PH4, PH5, PH6, PH7, PH8, PH9, PH10, PH11, PH12, PH13, PH14, PH15,
      PI0, PI1, PI2, PI3, PI4, PI5, PI6, PI7
    };

    return res;
  };
};

void Gpio::tristate()
{
  if (location == P__) {
      return;
  }

  GPIO_InitTypeDef GPIO_InitStruct = { 0 };
  GPIO_InitStruct.Pin   = pin();
  GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull  = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(periph(), &GPIO_InitStruct);
}

void Gpio::output(GPIO_PinState state)
{
  if (location == P__) {
    return;
  }

  GPIO_InitTypeDef GPIO_InitStruct = { 0 };
  GPIO_InitStruct.Pin   = pin();
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull  = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  HAL_GPIO_Init(periph(), &GPIO_InitStruct);
  HAL_GPIO_WritePin(periph(), pin(), state);
}

class GpioShortsTestRun {
private:
  bool has_illegal_shorts = false;
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

  const std::array<std::pair<Gpio, const char*>, 11> gpios_to_ignore = {{
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
  }};

  std::set<std::pair<Gpio, Gpio> > loopback_baseboard_connections = {
    { PF6,  PC13 },
    { PF7,  PB7  },
    { PA0,  PG2  },
    { PA1,  PD3  },
    { PD1,  PD5  },
    { PD6,  PF0  },
    { PD7,  PF1  },
    { PE2,  PF2  },
    { PD0,  PD9  },
    { PG0,  PG12 },
    { PE1,  PG9  },
    { PF8,  PG13 },
    { PF9,  PG10 },
    { PG1,  PG15 },
    { PB9,  PA7  },
    { PB6,  PA9  },
    { PD8,  PB11 },
    { PA8,  PD13 },
    { PB4,  PA3  },
    { PB5,  PA10 },
    { PB2,  PB15 },
    { PB14, PF5  },
    { PF4,  PF10 },
    { PD12, PE13 },
    { PD11, PE15 },
    { PE12, PE14 },
    { PF14, PE9  },
    { PF13, PD10 },
    { PF12, PG14 },
    { PE11, PF11 },
    { PF3,  PF15 },
    { PB8,  PE0  },
  };

  bool mark_baseboard_short(Gpio one, Gpio another) {
    bool found = false;

    auto lr = loopback_baseboard_connections.find({one, another});
    if (lr != loopback_baseboard_connections.end()) {
      found = true;
      loopback_baseboard_connections.erase(lr);
    }

    auto rl = loopback_baseboard_connections.find({another, one});
    if (rl != loopback_baseboard_connections.end()) {
      found = true;
      loopback_baseboard_connections.erase(rl);
    }

    return found;
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

  bool test_has_shorts();
  bool test_got_all_shorts_from_loopback();

public:

  uint32_t test();
};


bool GpioShortsTestRun::test_has_shorts() {
  collect_gpios_to_test();
  init_gpio_periphs();

  for (auto g : gpios_to_test) {
    g.tristate();
  }

  bool has_shorts = false;
  has_illegal_shorts = false;
  for (auto g : gpios_to_test) {
    if (g == P__) {
      continue;
    }

    g.output(GPIO_PIN_SET);
    vTaskDelay(10);

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
            has_shorts = true;
            if (!mark_baseboard_short(g, another_location)) {
              server_log("Illegal loopback for P%c%u <=> P%c%u\n",
                (g >> 4) + 'A' - 1, g & 0xf,
                (another_location >> 4) + 'A' - 1, another_location & 0xf);

              has_illegal_shorts = true;
            }
          }
        }
      }
    }
    g.tristate();
    vTaskDelay(1);
  }

  return !has_shorts;
}

bool GpioShortsTestRun::test_got_all_shorts_from_loopback() {
  bool success = true;
  for (auto conn: loopback_baseboard_connections) {
    server_log("Missing loopback for P%c%u <=> P%c%u\n",
         (conn.first >> 4) + '@', conn.first & 0xf,
         (conn.second >> 4) + '@', conn.second & 0xf);
    success = false;
  }

  return success;
}

uint32_t GpioShortsTestRun::test() {
  if (test_has_shorts()) {
    return 0;
  }

  if (has_illegal_shorts) {
    return 1;
  }

  if (test_got_all_shorts_from_loopback()) {
    return 0;
  }

  return 2;
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

void GpioShortsTest::run() {
  ::in_queue = in_queue;
  ::out_queue = out_queue;
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

