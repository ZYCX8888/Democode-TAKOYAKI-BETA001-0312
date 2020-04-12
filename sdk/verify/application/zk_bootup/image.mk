app:
ifeq ($(IMAGE_INSTALL_DIR),)
	@echo "directory of image is not defined"
	@exit 1
endif

	@cp -rf $(APPLICATION_PATH)/zk_bootup/RunFiles $(IMAGE_INSTALL_DIR)/rootfs/customer_app
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/rootfs/customer_app/lib/*
	@rm -rf $(IMAGE_INSTALL_DIR)/rootfs/customer_app/etc
	@rm -rf $(IMAGE_INSTALL_DIR)/rootfs/customer_app/bin
	@cp -rf $(APPLICATION_PATH)/zk_bootup/RunFiles/etc/* $(IMAGE_INSTALL_DIR)/rootfs/etc/
	@cat $(APPLICATION_PATH)/zk_bootup/RunFiles/demo.sh >> $(IMAGE_INSTALL_DIR)/rootfs/etc/init.sh
	@cp -rf $(APPLICATION_PATH)/zk_bootup/RunFiles/bin/zkgui $(IMAGE_INSTALL_DIR)/rootfs/customer_app/zkgui
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/rootfs/customer_app/zkgui

