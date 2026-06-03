#include <stm32h725xx.h>

#include <drivers/board.h>
#include <drivers/cache.h>
#include <drivers/config/clocks.h>
#include <drivers/config/internal_flash.h>

#include <assert.h>
#include <stddef.h>

namespace Ion {
const char * fccId();
namespace Device {
namespace Backlight { void init(); void shutdown(); }
namespace Battery { void init(); void shutdown(); }
namespace Console { void init(); void shutdown(); }
namespace Display { void init(); void shutdown(); }
namespace ExternalFlash { void init(); }
namespace InternalFlash { void WriteMemory(uint8_t * destination, uint8_t * source, size_t length); }
namespace Keyboard { void init(); void shutdown(); }
namespace LED { void init(); void shutdown(); }
namespace SWD { void init(); void shutdown(); }
namespace Timing { void init(); void shutdown(); void setSysTickFrequency(int frequency); }
namespace USB { void init(); void shutdown(); }
}
}

typedef void(*ISR)(void);
extern ISR InitialisationVector[];

namespace {

constexpr uint32_t kExternalFlashBase = 0x90000000;
constexpr uint32_t kFMCBase = 0x60000000;

constexpr uint32_t mpuSize(uint32_t log2Size) {
  return (log2Size - 1) << MPU_RASR_SIZE_Pos;
}

constexpr uint32_t mpuAccessFull = 3 << MPU_RASR_AP_Pos;
constexpr uint32_t mpuAccessNone = 0 << MPU_RASR_AP_Pos;

void configureMPURegion(uint32_t region, uint32_t base, uint32_t sizeField, uint32_t attributes) {
  MPU->RNR = region;
  MPU->RBAR = base;
  MPU->RASR = attributes | sizeField | MPU_RASR_ENABLE_Msk;
}

void configureAnalogGPIO(GPIO_TypeDef * gpio, uint32_t moder, uint32_t pupdr) {
  gpio->MODER = moder;
  gpio->PUPDR = pupdr;
}

void enablePeripheralClocks() {
  RCC->AHB4ENR |=
    RCC_AHB4ENR_GPIOAEN |
    RCC_AHB4ENR_GPIOBEN |
    RCC_AHB4ENR_GPIOCEN |
    RCC_AHB4ENR_GPIODEN |
    RCC_AHB4ENR_GPIOEEN |
    RCC_AHB4ENR_GPIOFEN |
    RCC_AHB4ENR_GPIOGEN |
    RCC_AHB4ENR_GPIOHEN;

  RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN | RCC_AHB1ENR_DMA2EN;
  RCC->AHB3ENR |= RCC_AHB3ENR_FMCEN | RCC_AHB3ENR_OSPI1EN | RCC_AHB3ENR_IOMNGREN;
  RCC->APB1LENR |= RCC_APB1LENR_TIM3EN;
  RCC->APB2ENR |= RCC_APB2ENR_USART6EN;
  RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN | RCC_APB4ENR_RTCAPBEN;
}

void disablePeripheralClocks(bool keepLEDAwake) {
  RCC->APB4ENR = 0;
  RCC->APB2ENR = 0;
  RCC->AHB3ENR = 0;
  RCC->AHB1ENR = 0;
  RCC->APB1LENR = keepLEDAwake ? RCC_APB1LENR_TIM3EN : 0;
  RCC->AHB4ENR = keepLEDAwake ? RCC_AHB4ENR_GPIOBEN : 0;
}

}

const char * Ion::fccId() {
  return "2ALWP-N0120";
}

namespace Ion {
namespace Device {
namespace Board {

void initFPU() {
  SCB->CPACR |= (0b11UL << 20) | (0b11UL << 22);
  Cache::dsb();
  Cache::isb();
}

void bootloaderMPU() {
  Cache::dmb();
  MPU->CTRL = 0;
  configureMPURegion(7, kExternalFlashBase, mpuSize(28), mpuAccessFull);
  MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
  Cache::disable();
  Cache::dsb();
  Cache::isb();
}

void initMPU() {
  Cache::dmb();
  SCB->SHCSR &= ~SCB_SHCSR_MEMFAULTENA_Msk;
  MPU->CTRL = 0;

  uint32_t region = 0;
  configureMPURegion(region++, kFMCBase, mpuSize(28), MPU_RASR_XN_Msk | mpuAccessNone | (2 << MPU_RASR_TEX_Pos));
  configureMPURegion(region++, kFMCBase, mpuSize(5), MPU_RASR_XN_Msk | mpuAccessFull | (2 << MPU_RASR_TEX_Pos));
  configureMPURegion(region++, kFMCBase + 0x20000, mpuSize(5), MPU_RASR_XN_Msk | mpuAccessFull | (2 << MPU_RASR_TEX_Pos));
  configureMPURegion(region++, kExternalFlashBase, mpuSize(28), MPU_RASR_XN_Msk | mpuAccessNone);
  configureMPURegion(region++, kExternalFlashBase, mpuSize(23), mpuAccessFull | MPU_RASR_C_Msk);

  MPU->CTRL = MPU_CTRL_PRIVDEFENA_Msk | MPU_CTRL_ENABLE_Msk;
  Cache::dsb();
  Cache::isb();
}

void initClocks() {
  RCC->CR |= RCC_CR_HSION;
  while ((RCC->CR & RCC_CR_HSIRDY) == 0) {}

  RCC->CR |= RCC_CR_HSEON;
  while ((RCC->CR & RCC_CR_HSERDY) == 0) {}

  RCC->APB4ENR |= RCC_APB4ENR_SYSCFGEN;

  PWR->CR3 |= PWR_CR3_LDOEN;
  PWR->D3CR = (PWR->D3CR & ~PWR_D3CR_VOS) | PWR_D3CR_VOS_1;
  while ((PWR->D3CR & PWR_D3CR_VOSRDY) == 0) {}

  FLASH->ACR = (FLASH->ACR & ~FLASH_ACR_LATENCY) | FLASH_ACR_LATENCY_4WS;

  /* HSE=8 MHz, DIVM1=4, DIVN1=192, DIVP1=2 gives a 192 MHz system clock. */
  RCC->CR &= ~RCC_CR_PLL1ON;
  while ((RCC->CR & RCC_CR_PLL1RDY) != 0) {}
  RCC->PLLCKSELR = (RCC->PLLCKSELR & ~(RCC_PLLCKSELR_PLLSRC | RCC_PLLCKSELR_DIVM1))
    | RCC_PLLCKSELR_PLLSRC_HSE
    | (4 << RCC_PLLCKSELR_DIVM1_Pos);
  RCC->PLLCFGR = (RCC->PLLCFGR & ~(RCC_PLLCFGR_PLL1RGE | RCC_PLLCFGR_PLL1VCOSEL))
    | RCC_PLLCFGR_PLL1RGE_1
    | RCC_PLLCFGR_DIVP1EN
    | RCC_PLLCFGR_DIVQ1EN
    | RCC_PLLCFGR_DIVR1EN;
  RCC->PLL1DIVR = ((192 - 1) << RCC_PLL1DIVR_N1_Pos)
    | ((2 - 1) << RCC_PLL1DIVR_P1_Pos)
    | ((8 - 1) << RCC_PLL1DIVR_Q1_Pos)
    | ((2 - 1) << RCC_PLL1DIVR_R1_Pos);

  RCC->D1CFGR = RCC_D1CFGR_D1CPRE_DIV1 | RCC_D1CFGR_D1PPRE_DIV2 | RCC_D1CFGR_HPRE_DIV1;
  RCC->D2CFGR = RCC_D2CFGR_D2PPRE1_DIV4 | RCC_D2CFGR_D2PPRE2_DIV2;
  RCC->D3CFGR = RCC_D3CFGR_D3PPRE_DIV2;

  RCC->CR |= RCC_CR_PLL1ON;
  while ((RCC->CR & RCC_CR_PLL1RDY) == 0) {}

  RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL1;
  while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL1) {}

  enablePeripheralClocks();
}

void init() {
  initFPU();
  initMPU();
  initClocks();

  SCB->VTOR = reinterpret_cast<uint32_t>(&InitialisationVector);

  configureAnalogGPIO(GPIOA, 0xEBFFFFFF, 0x24000000);
  configureAnalogGPIO(GPIOB, 0xFFFFFFBF, 0x00000000);
  configureAnalogGPIO(GPIOC, 0xFFFFFFFF, 0x00000000);
  configureAnalogGPIO(GPIOD, 0xFFFFFFFF, 0x00000000);
  configureAnalogGPIO(GPIOE, 0xFFFFFFFF, 0x00000000);

  ExternalFlash::init();
  Cache::enable();
}

void initCompensationCell() {
  SYSCFG->CCCSR |= SYSCFG_CCCSR_EN;
  while ((SYSCFG->CCCSR & SYSCFG_CCCSR_READY) == 0) {}
}

void shutdownCompensationCell() {
  SYSCFG->CCCSR &= ~SYSCFG_CCCSR_EN;
}

void initPeripherals(bool initBacklight) {
  initCompensationCell();
  Display::init();
  if (initBacklight) {
    Backlight::init();
  }
  Keyboard::init();
  LED::init();
  Battery::init();
  USB::init();
  Console::init();
  SWD::init();
  Timing::init();
}

void shutdownPeripherals(bool keepLEDAwake) {
  Timing::shutdown();
  SWD::shutdown();
  Console::shutdown();
  USB::shutdown();
  Battery::shutdown();
  if (!keepLEDAwake) {
    LED::shutdown();
  }
  Keyboard::shutdown();
  Backlight::shutdown();
  Display::shutdown();
  shutdownCompensationCell();
}

void shutdownClocks(bool keepLEDAwake) {
  disablePeripheralClocks(keepLEDAwake);
}

static Frequency sStandardFrequency = Frequency::High;

Frequency standardFrequency() {
  return sStandardFrequency;
}

void setStandardFrequency(Frequency f) {
  sStandardFrequency = f;
}

void setClockFrequency(Frequency f) {
  if (f == Frequency::High) {
    RCC->D1CFGR = (RCC->D1CFGR & ~RCC_D1CFGR_HPRE) | RCC_D1CFGR_HPRE_DIV1;
    Device::Timing::setSysTickFrequency(Clocks::Config::HCLKFrequency);
  } else {
    assert(f == Frequency::Low);
    Device::Timing::setSysTickFrequency(Clocks::Config::HCLKLowFrequency);
    RCC->D1CFGR = (RCC->D1CFGR & ~RCC_D1CFGR_HPRE) | RCC_D1CFGR_HPRE_DIV4;
  }
}

constexpr int k_pcbVersionOTPIndex = 0;

PCBVersion pcbVersion() {
#if IN_FACTORY
  return PCB_LATEST;
#else
  PCBVersion version = readPCBVersionInMemory();
  return (version == k_alternateBlankVersion ? 0 : version);
#endif
}

PCBVersion readPCBVersionInMemory() {
  return ~(*reinterpret_cast<const PCBVersion *>(InternalFlash::Config::OTPAddress(k_pcbVersionOTPIndex)));
}

void writePCBVersion(PCBVersion version) {
  uint8_t * destination = reinterpret_cast<uint8_t *>(InternalFlash::Config::OTPAddress(k_pcbVersionOTPIndex));
  PCBVersion formattedVersion = ~version;
  InternalFlash::WriteMemory(destination, reinterpret_cast<uint8_t *>(&formattedVersion), sizeof(formattedVersion));
}

void lockPCBVersion() {
  uint8_t * destination = reinterpret_cast<uint8_t *>(InternalFlash::Config::OTPLockAddress(k_pcbVersionOTPIndex));
  uint8_t zero = 0;
  InternalFlash::WriteMemory(destination, &zero, sizeof(zero));
}

bool pcbVersionIsLocked() {
  return *reinterpret_cast<const uint8_t *>(InternalFlash::Config::OTPLockAddress(k_pcbVersionOTPIndex)) == 0;
}

void jumpToInternalBootloader() {}

}
}
}

namespace Ion {
namespace Board {

using namespace Device::Board;

void lockUnlockedPCBVersion() {
  if (pcbVersionIsLocked()) {
    return;
  }
  PCBVersion version = Ion::Device::Board::pcbVersion();
  if (version != 0) {
    writePCBVersion(k_alternateBlankVersion);
  }
  lockPCBVersion();
}

}
}
