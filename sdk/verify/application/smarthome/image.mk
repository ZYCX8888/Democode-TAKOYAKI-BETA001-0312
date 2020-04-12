app:
ifeq ($(IMAGE_INSTALL_DIR),)
	@echo "directory of image is not defined"
	@exit 1
endif
	@cp -rf $(APPLICATION_PATH)/smarthome/bin/zkgui $(IMAGE_INSTALL_DIR)/customer/zkgui
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/customer/zkgui
	@cp -rf $(APPLICATION_PATH)/smarthome/lib $(IMAGE_INSTALL_DIR)/customer
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/customer/lib/*
	@cp -rf $(APPLICATION_PATH)/smarthome/res $(IMAGE_INSTALL_DIR)/customer
	@cp -rf $(APPLICATION_PATH)/smarthome/etc/* $(IMAGE_INSTALL_DIR)/rootfs/etc/
	@cat $(APPLICATION_PATH)/smarthome/demo.sh >> $(IMAGE_INSTALL_DIR)/customer/demo.sh
	@cp $(APPLICATION_PATH)/smarthome/fbdev.ini $(IMAGE_INSTALL_DIR)/tvconfig/config/fbdev.ini

