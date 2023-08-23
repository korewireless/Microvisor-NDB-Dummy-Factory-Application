#include "gpio.h"

void Gpio::tristate() const
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

void Gpio::output(GPIO_PinState state) const
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
