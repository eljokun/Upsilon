#ifndef ION_DEVICE_N0120_CONFIG_KEYBOARD_H
#define ION_DEVICE_N0120_CONFIG_KEYBOARD_H

#include <stdint.h>

namespace Ion {
namespace Device {
namespace Keyboard {
namespace Config {

constexpr uintptr_t RowGPIOAddress = 0x58020000;    // GPIOA
constexpr uintptr_t ColumnGPIOAddress = 0x58020800; // GPIOC

constexpr uint8_t numberOfRows = 9;
constexpr uint8_t RowPins[numberOfRows] = {1, 0, 2, 3, 6, 7, 8, 10, 15};

constexpr uint8_t numberOfColumns = 6;
constexpr uint8_t ColumnPins[numberOfColumns] = {1, 9, 11, 4, 5, 6};

/* Undefined keys numbers are: 7, 9, 10, 11, 35, 41, 47 and 53.
 * Force them to zero in the returned keyboard state. */
inline uint64_t ValidKeys(uint64_t state) {
  return state & 0x1F7DF7FFFFF17F;
}

}
}
}
}

#endif
