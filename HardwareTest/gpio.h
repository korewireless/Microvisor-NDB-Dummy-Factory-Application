/**
 *
 * GPIO utilities
 * Version 1.0.0
 * Copyright © 2023, Kore Wireless
 * Licence: Apache 2.0
 *
 */

#ifndef GPIO_H
#define GPIO_H

#include "stm32u5xx_hal.h"

#include <array>

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

  unsigned port() const { return ((location >> 4) - 1); }

  GPIO_TypeDef* periph() const { return (GPIO_TypeDef*) (((unsigned) GPIOA_NS) + 0x400 * port()); }

  uint32_t pin() const { return 1UL << (location & 0xF); }

  bool operator<(const Gpio& other) const { return location < other.location; }
  bool operator==(Gpio other) const { return location == other.location; }
  bool operator==(GpioLocation other) const { return location == other; }
  operator unsigned() const { return location; }

  void tristate() const;
  void output(GPIO_PinState state) const;

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

#endif // GPIO_H

