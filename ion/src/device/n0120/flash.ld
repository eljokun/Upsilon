/* N0120 memory map: STM32H725 internal flash, AXI SRAM, external flash. */

MEMORY {
  INTERNAL_FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K
  SRAM (rw) : ORIGIN = 0x24000000, LENGTH = 256K
  EXTERNAL_FLASH (rx) : ORIGIN = 0x90000000, LENGTH = 8M
}

STACK_SIZE = 32K;
FIRST_EXTERNAL_FLASH_SECTOR_SIZE = 4K;

SECTIONS {
  .isr_vector_table ORIGIN(INTERNAL_FLASH) : {
    KEEP(*(.isr_vector_table))
  } >INTERNAL_FLASH

  .header : {
    KEEP(*(.header))
  } >INTERNAL_FLASH

  .text.internal_to_external : {
    *(.text._ZL22jump_to_external_flashv*)
  } >INTERNAL_FLASH

  .text.internal : {
    . = ALIGN(4);
    *(.text.start)
    *(.text.abort)
    *(.text.hard_fault_handler)
    *(.text.mem_fault_handler)
    *(.text.usage_fault_handler)
    *(.text.bus_fault_handler)
    *(.text.isr_systick)
    *(.text.__assert)
    *(.text.memcpy)
    *(.text.memset)
    *(.text.__udivmoddi4)
    *(.text._ZN3Ion6Device5Board4initEv)
    *(.text._ZN3Ion6Device5Board7initFPUEv)
    *(.text._ZN3Ion6Device5Board7initMPUEv)
    *(.text._ZN3Ion6Device5Board10initClocksEv)
    *(.text._ZNV3Ion6Device4Regs*)
    *(.text._ZN3Ion6Device4Regs*)
    *(.text.*3Ion6Device4Regs4GPIO5PUPDR*)
    *(.text._ZNK3Ion6Device4Regs9AFGPIOPin*)
    *(.text._ZN3Ion6Device13ExternalFlash*)
    *(.text._ZN3Ion6Device5Cache*)
    *(.text._ZN3Ion6Timing6usleepEj)
    *(.text._ZN3Ion6Device5Reset4coreEv)
    *(.text._ZN3Ion6Device5Power20internalFlashStandbyEv)
    *(.text._ZN3Ion6Device5Power17enterLowPowerModeEv)
    *(.text._ZN3Ion6Device5Board14shutdownClocksEb)
    *(.text._ZN3Ion6Device5Reset16coreWhilePluggedEv)
    *(.text._ZN3Ion6Device5Power20internalFlashSuspendEb)
    *(.text._ZN3Ion6Device5Reset17internalFlashJumpEj)
    */libgcc.a:(.text)
  } >INTERNAL_FLASH

  .rodata.internal : {
    . = ALIGN(4);
    *(.rodata._ZN3Ion6Device13ExternalFlash*)
    *(.rodata._ZN3Ion6Device4RegsL5GPIOAE)
    *(.rodata._ZN3Ion6Device4RegsL5GPIOBE)
    *(.rodata._ZN3Ion6Device5Board4initEv.str1.4)
  } >INTERNAL_FLASH

  .exam_mode_buffer ORIGIN(EXTERNAL_FLASH) : {
    _exam_mode_buffer_start = .;
    KEEP(*(.exam_mode_buffer))
    . = ORIGIN(EXTERNAL_FLASH) + FIRST_EXTERNAL_FLASH_SECTOR_SIZE;
    _exam_mode_buffer_end = .;
  } >EXTERNAL_FLASH

  .text.external : {
    . = ALIGN(4);
    *(.text)
    *(.text.*)
  } >EXTERNAL_FLASH

  .rodata.external : {
    *(.rodata)
    *(.rodata.*)
  } >EXTERNAL_FLASH

  .init_array : {
    . = ALIGN(4);
    _init_array_start = .;
    KEEP (*(.init_array*))
    _init_array_end = .;
  } >INTERNAL_FLASH

  .data : {
    . = ALIGN(4);
    _data_section_start_flash = LOADADDR(.data);
    _data_section_start_ram = .;
    *(.data)
    *(.data.*)
    _data_section_end_ram = .;
  } >SRAM AT> INTERNAL_FLASH

  .bss (NOLOAD) : {
    . = ALIGN(4);
    _bss_section_start_ram = .;
    _static_storage_start = .;
    KEEP (*(.static_storage))
    _static_storage_end = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    _bss_section_end_ram = .;
  } >SRAM

  .heap : {
    _heap_start = .;
    . = (ORIGIN(SRAM) + LENGTH(SRAM) - STACK_SIZE);
    _heap_end = .;
  } >SRAM

  .stack : {
    . = ALIGN(8);
    _stack_end = .;
    . += (STACK_SIZE - 8);
    . = ALIGN(8);
    _stack_start = .;
  } >SRAM

  /DISCARD/ : {
    *(.ARM.exidx*)
    *(.ARM.extab*)
  }
}

NOCROSSREFS_TO(.text.external .text.internal);
NOCROSSREFS_TO(.rodata.external .text.internal);
NOCROSSREFS_TO(.text.external .rodata.internal);
NOCROSSREFS_TO(.rodata.external .rodata.internal);
NOCROSSREFS_TO(.text.external .isr_vector_table);
NOCROSSREFS_TO(.rodata.external .isr_vector_table);
NOCROSSREFS_TO(.text.external .header);
NOCROSSREFS_TO(.rodata.external .header);
NOCROSSREFS_TO(.exam_mode_buffer .text.internal);
NOCROSSREFS_TO(.exam_mode_buffer .rodata.internal);
NOCROSSREFS_TO(.exam_mode_buffer .isr_vector_table);
NOCROSSREFS_TO(.exam_mode_buffer .header);
