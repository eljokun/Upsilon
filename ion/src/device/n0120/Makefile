# Temporary scaffold: keep the same source shape as other device models while
# the H725-specific drivers are ported one subsystem at a time.
ion_device_src += $(addprefix ion/src/device/n0120/drivers/, \
  cache.cpp \
  external_flash.cpp \
  keyboard.cpp \
  led.cpp \
  power.cpp \
  reset.cpp \
  usb.cpp \
  wakeup.cpp \
)

ifeq ($(filter bootloader, ${MAKECMDGOALS}), bootloader)
ion_device_src += $(addprefix bootloader/drivers/, \
  board.cpp \
)
ion_device_src += $(addprefix bootloader/boot/, \
  rt0.cpp \
)
else
ion_device_src += $(addprefix ion/src/device/n0120/drivers/, \
  board.cpp \
)
ion_device_src += $(addprefix ion/src/device/n0120/boot/, \
  rt0.cpp \
)
endif

ion_device_src += $(addprefix ion/src/device/n0120/, \
  platform_info.cpp \
)

LDSCRIPT ?= ion/src/device/n0120/flash.ld
