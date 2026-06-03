ifeq ($(filter bootloader, ${MAKECMDGOALS}), bootloader)
ion_device_src += $(addprefix bootloader/boot/, \
  isr.c \
)
else
ifeq ($(MODEL),n0120)
ion_device_src += $(addprefix ion/src/device/n0120/boot/, \
  isr.c \
)
else
ion_device_src += $(addprefix ion/src/device/shared/boot/, \
  isr.c \
)
endif
endif
