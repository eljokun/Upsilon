#ifndef ION_DEVICE_N0120_CONFIG_CLOCKS_H
#define ION_DEVICE_N0120_CONFIG_CLOCKS_H

#include <stdint.h>

namespace Ion {
namespace Device {
namespace Clocks {
namespace Config {

constexpr static int HSE = 8;
constexpr static int PLL_M = 4;
constexpr static int PLL_N = 192;
constexpr static int PLL_P = 2;
constexpr static int PLL_Q = 8;
constexpr static int SYSCLKFrequency = ((HSE / PLL_M) * PLL_N) / PLL_P;
constexpr static int AHBPrescaler = 1;
constexpr static int AHBLowFrequencyPrescaler = 4;
constexpr static int HCLKFrequency = SYSCLKFrequency / AHBPrescaler;
static_assert(HCLKFrequency == 192, "HCLK frequency changed!");
constexpr static int HCLKLowFrequency = SYSCLKFrequency / AHBLowFrequencyPrescaler;
constexpr static int AHBFrequency = HCLKFrequency;
constexpr static int APB1Prescaler = 4;
constexpr static int APB1LowFrequency = HCLKLowFrequency / APB1Prescaler;
constexpr static int APB1TimerLowFrequency = 2 * APB1LowFrequency;

}
}
}
}

#endif
