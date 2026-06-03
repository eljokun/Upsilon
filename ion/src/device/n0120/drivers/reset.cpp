#include <drivers/board.h>
#include <drivers/cache.h>
#include <drivers/external_flash.h>
#include <drivers/reset.h>
#include <stm32h725xx.h>

namespace Ion {
namespace Device {
namespace Reset {

void core() {
  Cache::dsb();
  NVIC_SystemReset();
  while (true) {
    __asm volatile("nop");
  }
}

void coreWhilePlugged() {
  core();
}

void __attribute__((noinline)) internalFlashJump(uint32_t jumpIsrVectorAddress) {
  ExternalFlash::shutdown();
  Board::shutdownClocks();

  uint32_t * stackPointerAddress = reinterpret_cast<uint32_t *>(jumpIsrVectorAddress);
  uint32_t * resetHandlerAddress = stackPointerAddress + 1;

  __asm volatile (
      "msr MSP, %[stackPointer] ; bx %[resetHandler]"
      : :
      [stackPointer] "r" (*stackPointerAddress),
      [resetHandler] "r" (*resetHandlerAddress)
  );
}

void jump(uint32_t jumpIsrVectorAddress) {
  Cache::disable();
  Board::shutdownPeripherals();
  internalFlashJump(jumpIsrVectorAddress);
}

}
}
}
