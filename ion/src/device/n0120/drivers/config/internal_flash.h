#ifndef ION_DEVICE_N0120_CONFIG_INTERNAL_FLASH_H
#define ION_DEVICE_N0120_CONFIG_INTERNAL_FLASH_H

#include <stdint.h>

namespace Ion {
namespace Device {
namespace InternalFlash {
namespace Config {

constexpr static uint32_t StartAddress = 0x08000000;
constexpr static uint32_t EndAddress = 0x08080000;
constexpr static int NumberOfSectors = 4;
constexpr static uint32_t SectorAddresses[NumberOfSectors + 1] = {
  0x08000000, 0x08020000, 0x08040000, 0x08060000,
  0x08080000
};

/* STM32H725 has no F7-style OTP array. Reserve pseudo-OTP bytes near the end
 * of internal flash, matching n0120/internal_flash.ld. */
constexpr static uint32_t OTPStartAddress = 0x0807FA00;
constexpr static uint32_t OTPLocksAddress = 0x0807FC00;
constexpr static int NumberOfOTPBlocks = 16;
constexpr static uint32_t OTPBlockSize = 0x20;
constexpr uint32_t OTPAddress(int block) { return OTPStartAddress + block * OTPBlockSize; }
constexpr uint32_t OTPLockAddress(int block) { return OTPLocksAddress + block; }

}
}
}
}

#endif
