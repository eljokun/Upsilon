HANDY_TARGETS += bootloader

.PHONY: bootloader
bootloader: $(BUILD_DIR)/bootloader.bin
$(BUILD_DIR)/bootloader.$(EXE): $(call flavored_object_for,$(bootloader_src),usbxip)
$(BUILD_DIR)/bootloader.$(EXE): LDSCRIPT = ion/src/device/n0120/internal_flash.ld

.PHONY: %_flash
%_flash: $(BUILD_DIR)/%.dfu
	@echo "DFU     $@"
	@echo "INFO    About to flash your device. Please plug your device to your computer"
	@echo "        using an USB cable and enter the unlocked bootloader DFU mode."
	$(Q) $(PYTHON) build/device/dfu.py -u $(word 1,$^)
