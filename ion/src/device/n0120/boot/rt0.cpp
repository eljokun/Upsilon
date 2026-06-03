#include <stdint.h>
#include <string.h>
#include <boot/isr.h>
#include <drivers/board.h>
#include <drivers/reset.h>
#include <drivers/timing.h>
#include <ion.h>
#include <stm32h725xx.h>

typedef void (*cxx_constructor)();

extern "C" {
  extern char _data_section_start_flash;
  extern char _data_section_start_ram;
  extern char _data_section_end_ram;
  extern char _bss_section_start_ram;
  extern char _bss_section_end_ram;
  extern cxx_constructor _init_array_start;
  extern cxx_constructor _init_array_end;
}

extern "C" volatile uint32_t n0120FaultICSR;
extern "C" volatile uint32_t n0120FaultCFSR;
extern "C" volatile uint32_t n0120FaultHFSR;
extern "C" volatile uint32_t n0120FaultDFSR;
extern "C" volatile uint32_t n0120FaultAFSR;
extern "C" volatile uint32_t n0120FaultBFAR;
extern "C" volatile uint32_t n0120FaultMMFAR;

volatile uint32_t n0120FaultICSR;
volatile uint32_t n0120FaultCFSR;
volatile uint32_t n0120FaultHFSR;
volatile uint32_t n0120FaultDFSR;
volatile uint32_t n0120FaultAFSR;
volatile uint32_t n0120FaultBFAR;
volatile uint32_t n0120FaultMMFAR;

static void __attribute__((section(".text.abort"), noinline)) haltAfterFault() {
  n0120FaultICSR = SCB->ICSR;
  n0120FaultCFSR = SCB->CFSR;
  n0120FaultHFSR = SCB->HFSR;
  n0120FaultDFSR = SCB->DFSR;
  n0120FaultAFSR = SCB->AFSR;
  n0120FaultBFAR = SCB->BFAR;
  n0120FaultMMFAR = SCB->MMFAR;
  __disable_irq();
  while (true) {
    __asm volatile("bkpt 0");
    __asm volatile("nop");
  }
}

extern "C" void __attribute__((noinline)) abort() {
  haltAfterFault();
}

extern "C" void __attribute__((noinline)) hard_fault_handler() {
  haltAfterFault();
}

extern "C" void __attribute__((noinline)) mem_fault_handler() {
  haltAfterFault();
}

extern "C" void __attribute__((noinline)) bus_fault_handler() {
  haltAfterFault();
}

extern "C" void __attribute__((noinline)) usage_fault_handler() {
  haltAfterFault();
}

static void __attribute__((noinline)) external_flash_start() {
  Ion::Device::Board::initPeripherals(false);
  return ion_main(0, nullptr);
}

static void __attribute__((noinline)) jump_to_external_flash() {
  external_flash_start();
}

extern "C" void __attribute__((noinline)) start() {
  size_t dataSectionLength = (&_data_section_end_ram - &_data_section_start_ram);
  memcpy(&_data_section_start_ram, &_data_section_start_flash, dataSectionLength);

  size_t bssSectionLength = (&_bss_section_end_ram - &_bss_section_start_ram);
  memset(&_bss_section_start_ram, 0, bssSectionLength);

  Ion::Device::Board::initFPU();

#define SUPPORT_CPP_GLOBAL_CONSTRUCTORS 0
#if SUPPORT_CPP_GLOBAL_CONSTRUCTORS
  for (cxx_constructor * c = &_init_array_start; c < &_init_array_end; c++) {
    (*c)();
  }
#else
  if (&_init_array_start != &_init_array_end) {
    abort();
  }
#endif

  Ion::Device::Board::init();
  jump_to_external_flash();
  abort();
}

extern "C" void __attribute__((interrupt, noinline)) isr_systick() {
  auto t = Ion::Device::Timing::MillisElapsed;
  t++;
  Ion::Device::Timing::MillisElapsed = t;
}
