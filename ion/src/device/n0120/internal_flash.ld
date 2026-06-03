/* Same as flash.ld but everything is linked in internal flash. */

MEMORY {
  INTERNAL_FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 512K
  SRAM (rw) : ORIGIN = 0x24000000, LENGTH = 256K
}

STACK_SIZE = 32K;
TRAMPOLINES_OFFSET = 0x7E000;
CUSTOM_TRAMPOLINES_OFFSET = 512K - 64;
PSEUDO_OTP_OFFSET = 512K - 0x600;
PSEUDO_OTP_SIZE = 512;

SECTIONS {
  .isr_vector_table ORIGIN(INTERNAL_FLASH) : {
    KEEP(*(.isr_vector_table))
  } >INTERNAL_FLASH

  .header : {
    KEEP(*(.header))
  } >INTERNAL_FLASH

  .rodata : {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata.*)
  } >INTERNAL_FLASH

  .exam_mode_buffer ORIGIN(INTERNAL_FLASH) + PSEUDO_OTP_OFFSET : {
    _exam_mode_buffer_start = .;
    KEEP(*(.exam_mode_buffer))
    . = ORIGIN(INTERNAL_FLASH) + PSEUDO_OTP_OFFSET + PSEUDO_OTP_SIZE;
    _exam_mode_buffer_end = .;
  } >INTERNAL_FLASH

  .text : {
    . = ALIGN(4);
    *(.text)
    *(.text.*)
  } >INTERNAL_FLASH

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

  .trampolines_table : {
    . = ORIGIN(INTERNAL_FLASH) + TRAMPOLINES_OFFSET;
    KEEP(*(.trampolines_table));
  } > INTERNAL_FLASH

  .custom_trampolines_table : {
    . = ORIGIN(INTERNAL_FLASH) + CUSTOM_TRAMPOLINES_OFFSET;
    KEEP(*(.custom_trampolines_table));
  } > INTERNAL_FLASH

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
}
