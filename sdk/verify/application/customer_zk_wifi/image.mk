app:
ifeq ($(IMAGE_INSTALL_DIR),)
	@echo "directory of image is not defined"
	@exit 1
endif
	@cp -rf $(APPLICATION_PATH)/customer_zk_wifi/bin/zkgui_1024_600 $(IMAGE_INSTALL_DIR)/customer/zkgui
	#@cp -rf $(APPLICATION_PATH)/customer_zk_wifi/bin/zkgui_800_480 $(IMAGE_INSTALL_DIR)/customer/zkgui
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/customer/zkgui
	@cp -rf $(APPLICATION_PATH)/customer_zk_wifi/lib $(IMAGE_INSTALL_DIR)/customer
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/customer/lib/*
	@cp -rf $(APPLICATION_PATH)/customer_zk_wifi/res $(IMAGE_INSTALL_DIR)/customer
	@cp -rf $(APPLICATION_PATH)/customer_zk_wifi/etc/* $(IMAGE_INSTALL_DIR)/rootfs/etc/
	@cat $(APPLICATION_PATH)/customer_zk_wifi/demo.sh >> $(IMAGE_INSTALL_DIR)/customer/demo.sh

