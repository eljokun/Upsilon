ion_device_src += $(addprefix ion/src/device/shared/drivers/, \
  backlight.cpp \
  battery.cpp \
  base64.cpp \
  clipboard.cpp \
  console_uart.cpp:+consoleuart \
  console_display.cpp:+consoledisplay \
  console_dummy.cpp:-consoledisplay \
  console_dummy.cpp:-consoleuart \
  crc32.cpp \
  display.cpp \
  events_keyboard_platform.cpp \
  exam_mode.cpp \
  flash.cpp \
  internal_flash.cpp \
  led.cpp \
  pcb_version.cpp \
  power.cpp\
  random.cpp\
  rtc.cpp \
  serial_number.cpp \
  swd.cpp \
  timing.cpp \
  usb.cpp \
  bldata.cpp \
)

ifneq ($(MODEL),n0120)
ion_device_src += $(addprefix ion/src/device/shared/drivers/, \
  keyboard.cpp \
  wakeup.cpp \
)
endif

ifneq ($(MODEL),n0120)
ion_device_src += ion/src/device/shared/drivers/board.cpp
endif

ifneq ($(MODEL),n0120)
ion_device_src += ion/src/device/shared/drivers/reset.cpp
endif
