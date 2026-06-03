#include <stm32h725xx.h>

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include <drivers/cache.h>
#include <drivers/config/clocks.h>
#include <drivers/config/external_flash.h>
#include <drivers/external_flash.h>
#include <ion/timing.h>

namespace Ion {
namespace Device {
namespace ExternalFlash {

enum class Command : uint8_t {
  WriteStatusRegister     = 0x01,
  PageProgram             = 0x02,
  ReadStatusRegister1     = 0x05,
  WriteEnable             = 0x06,
  Erase4KbyteBlock        = 0x20,
  WriteStatusRegister2    = 0x31,
  ReadStatusRegister2     = 0x35,
  Erase32KbyteBlock       = 0x52,
  EnableReset             = 0x66,
  Reset                   = 0x99,
  ReadJEDECID             = 0x9F,
  ReleaseDeepPowerDown    = 0xAB,
  DeepPowerDown           = 0xB9,
  ChipErase               = 0xC7,
  Erase64KbyteBlock       = 0xD8,
  FastReadQuadIO          = 0xEB
};

enum class LineMode : uint32_t {
  None = 0,
  Single = 1,
  Dual = 2,
  Quad = 3
};

enum class FunctionalMode : uint32_t {
  IndirectWrite = 0,
  IndirectRead = OCTOSPI_CR_FMODE_0,
  AutoPolling = OCTOSPI_CR_FMODE_1,
  MemoryMapped = OCTOSPI_CR_FMODE_0 | OCTOSPI_CR_FMODE_1
};

struct Transfer {
  LineMode instructionMode = LineMode::Single;
  LineMode addressMode = LineMode::None;
  LineMode alternateMode = LineMode::None;
  LineMode dataMode = LineMode::None;
  uint32_t address = 0;
  bool hasAddress = false;
  uint32_t alternate = 0;
  size_t alternateLength = 0;
  uint8_t dummyCycles = 0;
};

static constexpr uint8_t NumberOfAddressBitsIn64KbyteBlock = 16;
static constexpr uint8_t NumberOfAddressBitsIn32KbyteBlock = 15;
static constexpr uint8_t NumberOfAddressBitsIn4KbyteBlock = 12;
static constexpr size_t PageSize = 256;

static constexpr uint32_t ModeBits(LineMode mode, uint32_t position) {
  return static_cast<uint32_t>(mode) << position;
}

static constexpr uint32_t AddressSize24Bits = 2 << OCTOSPI_CCR_ADSIZE_Pos;
static constexpr uint32_t AlternateSize8Bits = 0 << OCTOSPI_CCR_ABSIZE_Pos;
static constexpr int ClockFrequencyDivisor = 3;
static constexpr int FastReadQuadIODummyCycles = 4;

static uint32_t offsetFromStart(const uint8_t * address) {
  return reinterpret_cast<uintptr_t>(address) - Config::StartAddress;
}

static void waitControllerIdle() {
  while (OCTOSPI2->SR & OCTOSPI_SR_BUSY) {
  }
}

static void clearControllerFlags() {
  OCTOSPI2->FCR = OCTOSPI_FCR_CTEF | OCTOSPI_FCR_CTCF | OCTOSPI_FCR_CSMF | OCTOSPI_FCR_CTOF;
}

static void abortMemoryMappedMode() {
  if ((OCTOSPI2->CR & OCTOSPI_CR_FMODE) == static_cast<uint32_t>(FunctionalMode::MemoryMapped)) {
    OCTOSPI2->CR |= OCTOSPI_CR_ABORT;
    while (OCTOSPI2->CR & OCTOSPI_CR_ABORT) {
    }
  }
  waitControllerIdle();
}

static uint32_t ccrForTransfer(const Transfer & transfer) {
  uint32_t ccr = 0;
  ccr |= ModeBits(transfer.instructionMode, OCTOSPI_CCR_IMODE_Pos);
  if (transfer.hasAddress) {
    ccr |= ModeBits(transfer.addressMode, OCTOSPI_CCR_ADMODE_Pos);
    ccr |= AddressSize24Bits;
  }
  if (transfer.alternateLength > 0) {
    ccr |= ModeBits(transfer.alternateMode, OCTOSPI_CCR_ABMODE_Pos);
    ccr |= AlternateSize8Bits;
  }
  ccr |= ModeBits(transfer.dataMode, OCTOSPI_CCR_DMODE_Pos);
  return ccr;
}

static void configureTransfer(FunctionalMode functionalMode, Command command, const Transfer & transfer, size_t dataLength) {
  waitControllerIdle();
  clearControllerFlags();

  uint32_t cr = OCTOSPI2->CR & ~OCTOSPI_CR_FMODE;
  OCTOSPI2->CR = cr | static_cast<uint32_t>(functionalMode);
  if (functionalMode != FunctionalMode::MemoryMapped) {
    OCTOSPI2->DLR = dataLength > 0 ? dataLength - 1 : 0;
  }
  OCTOSPI2->TCR = (transfer.dummyCycles << OCTOSPI_TCR_DCYC_Pos) & OCTOSPI_TCR_DCYC;
  OCTOSPI2->IR = static_cast<uint8_t>(command);
  if (transfer.alternateLength > 0) {
    OCTOSPI2->ABR = transfer.alternate;
  }
  OCTOSPI2->CCR = ccrForTransfer(transfer);
  if (transfer.hasAddress) {
    OCTOSPI2->AR = transfer.address;
  }
}

static void sendCommand(Command command) {
  Transfer transfer;
  configureTransfer(FunctionalMode::IndirectWrite, command, transfer, 0);
  while (!(OCTOSPI2->SR & OCTOSPI_SR_TCF)) {
  }
  clearControllerFlags();
}

static void readCommand(Command command, uint8_t * data, size_t length) {
  Transfer transfer;
  transfer.dataMode = LineMode::Single;
  configureTransfer(FunctionalMode::IndirectRead, command, transfer, length);
  for (size_t i = 0; i < length; i++) {
    data[i] = static_cast<uint8_t>(OCTOSPI2->DR);
  }
  while (!(OCTOSPI2->SR & OCTOSPI_SR_TCF)) {
  }
  clearControllerFlags();
}

static void writeCommand(Command command, uint32_t address, const uint8_t * data, size_t length) {
  Transfer transfer;
  transfer.addressMode = LineMode::Single;
  transfer.hasAddress = true;
  transfer.address = address;
  transfer.dataMode = data != nullptr && length > 0 ? LineMode::Single : LineMode::None;
  configureTransfer(FunctionalMode::IndirectWrite, command, transfer, length);
  for (size_t i = 0; i < length; i++) {
    OCTOSPI2->DR = data[i];
  }
  while (!(OCTOSPI2->SR & OCTOSPI_SR_TCF)) {
  }
  clearControllerFlags();
}

static uint8_t readStatusRegister1() {
  uint8_t status = 0;
  readCommand(Command::ReadStatusRegister1, &status, sizeof(status));
  return status;
}

static uint8_t readStatusRegister2() {
  uint8_t status = 0;
  readCommand(Command::ReadStatusRegister2, &status, sizeof(status));
  return status;
}

static void waitChip() {
  Cache::dsb();
  while (readStatusRegister1() & 0x1) {
  }
}

static void writeStatusRegisters(uint8_t status1, uint8_t status2) {
  uint8_t registers[] = {status1, status2};
  sendCommand(Command::WriteEnable);
  waitChip();

  Transfer transfer;
  transfer.dataMode = LineMode::Single;
  configureTransfer(FunctionalMode::IndirectWrite, Command::WriteStatusRegister, transfer, sizeof(registers));
  for (uint8_t value : registers) {
    OCTOSPI2->DR = value;
  }
  while (!(OCTOSPI2->SR & OCTOSPI_SR_TCF)) {
  }
  clearControllerFlags();
  waitChip();
}

static void enableQuadIO() {
  uint8_t status2 = readStatusRegister2();
  if (status2 & (1 << 1)) {
    return;
  }
  status2 |= 1 << 1;
  sendCommand(Command::WriteEnable);
  waitChip();

  Transfer transfer;
  transfer.dataMode = LineMode::Single;
  configureTransfer(FunctionalMode::IndirectWrite, Command::WriteStatusRegister2, transfer, sizeof(status2));
  OCTOSPI2->DR = status2;
  while (!(OCTOSPI2->SR & OCTOSPI_SR_TCF)) {
  }
  clearControllerFlags();
  waitChip();
}

static void setAsMemoryMapped() {
  Transfer transfer;
  transfer.addressMode = LineMode::Quad;
  transfer.alternateMode = LineMode::Quad;
  transfer.dataMode = LineMode::Quad;
  transfer.hasAddress = true;
  transfer.address = FlashAddressSpaceSize;
  transfer.alternate = 0xA0;
  transfer.alternateLength = 1;
  transfer.dummyCycles = FastReadQuadIODummyCycles;
  configureTransfer(FunctionalMode::MemoryMapped, Command::FastReadQuadIO, transfer, 0);
  OCTOSPI2->CCR |= OCTOSPI_CCR_SIOO;
}

static void unsetMemoryMappedMode() {
  abortMemoryMappedMode();
}

static void initGPIOPin(const Config::Pin & pin) {
  GPIO_TypeDef * port = reinterpret_cast<GPIO_TypeDef *>(pin.portAddress);
  const uint32_t bit = 1u << pin.index;
  const uint32_t shift = 2u * pin.index;
  port->MODER = (port->MODER & ~(0x3u << shift)) | (0x2u << shift);
  port->PUPDR &= ~(0x3u << shift);
  port->OSPEEDR |= 0x3u << shift;
  port->OTYPER &= ~bit;

  const uint32_t afrIndex = pin.index >> 3;
  const uint32_t afrShift = 4u * (pin.index & 0x7u);
  port->AFR[afrIndex] = (port->AFR[afrIndex] & ~(0xFu << afrShift)) | (pin.alternateFunction << afrShift);
}

static void shutdownGPIOPin(const Config::Pin & pin) {
  GPIO_TypeDef * port = reinterpret_cast<GPIO_TypeDef *>(pin.portAddress);
  const uint32_t shift = 2u * pin.index;
  port->OSPEEDR &= ~(0x3u << shift);
  port->PUPDR &= ~(0x3u << shift);
  port->MODER |= 0x3u << shift;
}

static void initGPIO() {
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIOBEN | RCC_AHB4ENR_GPIOCEN | RCC_AHB4ENR_GPIODEN | RCC_AHB4ENR_GPIOEEN;
  for (const Config::Pin & pin : Config::Pins) {
    initGPIOPin(pin);
  }
}

static void shutdownGPIO() {
  for (const Config::Pin & pin : Config::Pins) {
    shutdownGPIOPin(pin);
  }
}

static void initOSPI() {
  RCC->AHB3ENR |= RCC_AHB3ENR_OSPI1EN | RCC_AHB3ENR_IOMNGREN;
  RCC->AHB3RSTR |= RCC_AHB3RSTR_OSPI1RST | RCC_AHB3RSTR_IOMNGRRST;
  RCC->AHB3RSTR &= ~(RCC_AHB3RSTR_OSPI1RST | RCC_AHB3RSTR_IOMNGRRST);

  OCTOSPIM->PCR[0] = OCTOSPIM_PCR_CLKEN | OCTOSPIM_PCR_NCSEN | OCTOSPIM_PCR_IOLEN;

  constexpr int ChipSelectHighTimeCycles =
      ((50ULL * Clocks::Config::AHBFrequency) / (ClockFrequencyDivisor * 1000000000ULL)) + 1;
  OCTOSPI2->DCR1 =
      OCTOSPI_DCR1_CKMODE |
      OCTOSPI_DCR1_DLYBYP |
      ((ChipSelectHighTimeCycles - 1) << OCTOSPI_DCR1_CSHT_Pos) |
      ((NumberOfAddressBitsInChip - 1) << OCTOSPI_DCR1_DEVSIZE_Pos);
  OCTOSPI2->DCR2 = (ClockFrequencyDivisor - 1) << OCTOSPI_DCR2_PRESCALER_Pos;
  OCTOSPI2->CR = OCTOSPI_CR_EN | (3 << OCTOSPI_CR_FTHRES_Pos);
}

static void shutdownOSPI() {
  RCC->AHB3RSTR |= RCC_AHB3RSTR_OSPI1RST;
  RCC->AHB3RSTR &= ~RCC_AHB3RSTR_OSPI1RST;
  RCC->AHB3ENR &= ~RCC_AHB3ENR_OSPI1EN;
}

static void initChip() {
  sendCommand(Command::ReleaseDeepPowerDown);
  Timing::usleep(30);
  enableQuadIO();
  setAsMemoryMapped();
}

static void shutdownChip() {
  unsetMemoryMappedMode();
  sendCommand(Command::EnableReset);
  sendCommand(Command::Reset);
  Timing::usleep(30);
  sendCommand(Command::DeepPowerDown);
  Timing::usleep(3);
}

void init() {
  if (Config::NumberOfSectors == 0) {
    return;
  }
  initGPIO();
  initOSPI();
  initChip();
}

void shutdown() {
  if (Config::NumberOfSectors == 0) {
    return;
  }
  shutdownChip();
  shutdownOSPI();
  shutdownGPIO();
}

int SectorAtAddress(uint32_t address) {
  int i = address >> NumberOfAddressBitsIn64KbyteBlock;
  if (i > Config::NumberOf64KSectors) {
    return -1;
  }
  if (i >= 1) {
    return Config::NumberOf4KSectors + Config::NumberOf32KSectors + i - 1;
  }
  i = address >> NumberOfAddressBitsIn32KbyteBlock;
  if (i >= 1) {
    i = Config::NumberOf4KSectors + i - 1;
    assert(i >= Config::NumberOf4KSectors && i <= Config::NumberOf4KSectors + Config::NumberOf32KSectors);
    return i;
  }
  i = address >> NumberOfAddressBitsIn4KbyteBlock;
  assert(i <= Config::NumberOf4KSectors);
  return i;
}

static void unlockFlash() {
  unsetMemoryMappedMode();
  writeStatusRegisters(0, readStatusRegister2() & (1 << 1));
}

void LockSlotA() {
  unsetMemoryMappedMode();
  unlockFlash();
  uint8_t status1 = (1 << 5) | (1 << 4) | (1 << 3);
  uint8_t status2 = (readStatusRegister2() & (1 << 1)) | 1;
  writeStatusRegisters(status1, status2);
  setAsMemoryMapped();
}

void LockSlotB() {
  unsetMemoryMappedMode();
  unlockFlash();
  uint8_t status1 = (1 << 4) | (1 << 3);
  uint8_t status2 = (readStatusRegister2() & (1 << 1)) | 1;
  writeStatusRegisters(status1, status2);
  setAsMemoryMapped();
}

void MassErase() {
  if (Config::NumberOfSectors == 0) {
    return;
  }
  unsetMemoryMappedMode();
  unlockFlash();
  sendCommand(Command::WriteEnable);
  waitChip();
  sendCommand(Command::ChipErase);
  waitChip();
  setAsMemoryMapped();
}

void __attribute__((noinline)) EraseSector(int i) {
  assert(i >= 0 && i < Config::NumberOfSectors);
  unsetMemoryMappedMode();
  unlockFlash();
  sendCommand(Command::WriteEnable);
  waitChip();

  if (i < Config::NumberOf4KSectors) {
    writeCommand(Command::Erase4KbyteBlock, i << NumberOfAddressBitsIn4KbyteBlock, nullptr, 0);
  } else if (i < Config::NumberOf4KSectors + Config::NumberOf32KSectors) {
    writeCommand(Command::Erase32KbyteBlock, (i - Config::NumberOf4KSectors + 1) << NumberOfAddressBitsIn32KbyteBlock, nullptr, 0);
  } else {
    writeCommand(Command::Erase64KbyteBlock, (i - Config::NumberOf4KSectors - Config::NumberOf32KSectors + 1) << NumberOfAddressBitsIn64KbyteBlock, nullptr, 0);
  }
  waitChip();
  setAsMemoryMapped();
}

void __attribute__((noinline)) WriteMemory(uint8_t * destination, const uint8_t * source, size_t length) {
  if (Config::NumberOfSectors == 0) {
    return;
  }
  uint32_t destinationOffset = offsetFromStart(destination);
  unsetMemoryMappedMode();
  while (length > 0) {
    const size_t offsetInPage = destinationOffset & (PageSize - 1);
    size_t lengthThatFitsInPage = PageSize - offsetInPage;
    if (lengthThatFitsInPage > length) {
      lengthThatFitsInPage = length;
    }
    sendCommand(Command::WriteEnable);
    waitChip();
    writeCommand(Command::PageProgram, destinationOffset, source, lengthThatFitsInPage);
    waitChip();
    destinationOffset += lengthThatFitsInPage;
    source += lengthThatFitsInPage;
    length -= lengthThatFitsInPage;
  }
  setAsMemoryMapped();
}

void JDECid(uint8_t * manufacturerID, uint8_t * memoryType, uint8_t * capacityType) {
  unsetMemoryMappedMode();
  uint8_t id[3] = {};
  readCommand(Command::ReadJEDECID, id, sizeof(id));
  *manufacturerID = id[0];
  *memoryType = id[1];
  *capacityType = id[2];
  setAsMemoryMapped();
}

}
}
}
