#ifndef ION_DEVICE_N0120_KEYBOARD_H
#define ION_DEVICE_N0120_KEYBOARD_H

#include <drivers/config/keyboard.h>
#include <ion/keyboard.h>
#include <ion/timing.h>

namespace Ion {
namespace Device {
namespace Keyboard {

using namespace Ion::Keyboard;

void init();
void shutdown();

struct GPIORegisters {
  volatile uint32_t MODER;
  volatile uint32_t OTYPER;
  volatile uint32_t OSPEEDR;
  volatile uint32_t PUPDR;
  volatile uint32_t IDR;
  volatile uint32_t ODR;
};

inline GPIORegisters * rowGPIO() {
  return reinterpret_cast<GPIORegisters *>(Config::RowGPIOAddress);
}

inline GPIORegisters * columnGPIO() {
  return reinterpret_cast<GPIORegisters *>(Config::ColumnGPIOAddress);
}

inline uint8_t rowForKey(Key key) {
  return static_cast<int>(key) / Config::numberOfColumns;
}

inline uint8_t columnForKey(Key key) {
  return static_cast<int>(key) % Config::numberOfColumns;
}

inline void activateRow(uint8_t row) {
  GPIORegisters * gpio = rowGPIO();
  for (uint8_t i = 0; i < Config::numberOfRows; i++) {
    gpio->ODR |= 1u << Config::RowPins[i];
  }
  gpio->ODR &= ~(1u << Config::RowPins[row]);
  Timing::usleep(100);
}

inline bool columnIsActive(uint8_t column) {
  return !(columnGPIO()->IDR & (1u << Config::ColumnPins[column]));
}

}
}
}

#endif
