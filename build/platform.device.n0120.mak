TOOLCHAIN ?= arm-gcc-m7f
ION_KEYBOARD_LAYOUT = layout_B3
PCB_LATEST = 430

SFLAGS += -DSTM32H725xx -Iion/src/device/n0120/cmsis
