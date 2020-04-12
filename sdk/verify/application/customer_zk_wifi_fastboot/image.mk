app:
ifeq ($(IMAGE_INSTALL_DIR),)
	@echo "directory of image is not defined"
	@exit 1
endif
	@echo ---------------------------------------------------
	@cp -rf $(APPLICATION_PATH)/customer_zk_wifi_fastboot/bin/zkgui_1024_600 $(IMAGE_INSTALL_DIR)/customer/zkgui
	#@cp -rf $(APPLICATION_PATH)/customer_zk_wifi_fastboot/bin/zkgui_800_480 $(IMAGE_INSTALL_DIR)/customer/zkgui
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/customer/zkgui
	@cp -rf $(APPLICATION_PATH)/customer_zk_wifi_fastboot/lib $(IMAGE_INSTALL_DIR)/customer
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/customer/lib/*
	@cp -rf $(APPLICATION_PATH)/customer_zk_wifi_fastboot/res $(IMAGE_INSTALL_DIR)/customer
	@cp -rf $(APPLICATION_PATH)/customer_zk_wifi_fastboot/etc/* $(IMAGE_INSTALL_DIR)/rootfs/etc/
	@cat $(APPLICATION_PATH)/customer_zk_wifi_fastboot/demo.sh >> $(IMAGE_INSTALL_DIR)/customer/demo.sh
	@echo ==++++++++++++++++++++++++++++++++++++++++++++++
