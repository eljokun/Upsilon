#include "keyboard.h"

#include <stm32h725xx.h>

namespace Ion {
namespace Keyboard {

using namespace Ion::Device::Keyboard;

State scan() {
  uint64_t state = 0;

  for (uint8_t i = 0; i < Config::numberOfRows; i++) {
    activateRow(Config::numberOfRows - 1 - i);

    uint8_t columns = 0;
    for (uint8_t j = 0; j < Config::numberOfColumns; j++) {
      if (columnIsActive(j)) {
        columns |= 1u << j;
      }
    }
    state = (state << Config::numberOfColumns) | columns;
  }

  return State(Config::ValidKeys(state));
}

}
}

namespace Ion {
namespace Device {
namespace Keyboard {

static void configurePinAsAnalog(GPIORegisters * gpio, uint8_t pin) {
  const uint32_t shift = 2u * pin;
  gpio->MODER |= 0x3u << shift;
  gpio->PUPDR &= ~(0x3u << shift);
}

void init() {
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN | RCC_AHB4ENR_GPIOCEN;

  GPIORegisters * rows = rowGPIO();
  for (uint8_t i = 0; i < Config::numberOfRows; i++) {
    const uint8_t pin = Config::RowPins[i];
    const uint32_t shift = 2u * pin;
    rows->ODR |= 1u << pin;
    rows->MODER = (rows->MODER & ~(0x3u << shift)) | (0x1u << shift);
    rows->OTYPER |= 1u << pin;
    rows->PUPDR &= ~(0x3u << shift);
    rows->OSPEEDR |= 0x3u << shift;
  }

  GPIORegisters * columns = columnGPIO();
  for (uint8_t i = 0; i < Config::numberOfColumns; i++) {
    const uint8_t pin = Config::ColumnPins[i];
    const uint32_t shift = 2u * pin;
    columns->MODER &= ~(0x3u << shift);
    columns->PUPDR = (columns->PUPDR & ~(0x3u << shift)) | (0x1u << shift);
  }
}

void shutdown() {
  GPIORegisters * rows = rowGPIO();
  for (uint8_t i = 0; i < Config::numberOfRows; i++) {
    configurePinAsAnalog(rows, Config::RowPins[i]);
  }

  GPIORegisters * columns = columnGPIO();
  for (uint8_t i = 0; i < Config::numberOfColumns; i++) {
    configurePinAsAnalog(columns, Config::ColumnPins[i]);
  }
}

}
}
}
