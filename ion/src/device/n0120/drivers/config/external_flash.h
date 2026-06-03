#ifndef ION_DEVICE_N0120_CONFIG_EXTERNAL_FLASH_H
#define ION_DEVICE_N0120_CONFIG_EXTERNAL_FLASH_H

#include <stdint.h>

namespace Ion {
namespace Device {
namespace ExternalFlash {
namespace Config {

constexpr static uint32_t StartAddress = 0x90000000;
constexpr static uint32_t EndAddress = 0x90800000;

constexpr static int NumberOf4KSectors = 8;
constexpr static int NumberOf32KSectors = 1;
constexpr static int NumberOf64KSectors = 128 - 1;
constexpr static int NumberOfSectors =
    NumberOf4KSectors + NumberOf32KSectors + NumberOf64KSectors;

struct Pin {
  uintptr_t portAddress;
  uint8_t index;
  uint8_t alternateFunction;
};

/* N0120 OCTOSPI1 pins traced from STM32H725VETx board. */
constexpr static Pin Pins[] = {
  {0x58020400, 10, 9}, // PB10 OCTOSPI1_P1_NCS
  {0x58020400, 12, 9}, // PB12 OCTOSPI1_P1_IO0
  {0x58020C00, 12, 9}, // PD12 OCTOSPI1_P1_IO1
  {0x58020400, 13, 9}, // PB13 OCTOSPI1_P1_IO2
  {0x58020C00, 13, 9}, // PD13 OCTOSPI1_P1_IO3
  {0x58020400,  2, 9}, // PB2  OCTOSPI1_P1_CLK
};

}
}
}
}

#endif