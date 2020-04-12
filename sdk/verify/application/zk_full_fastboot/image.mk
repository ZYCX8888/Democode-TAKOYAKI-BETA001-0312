app:
ifeq ($(IMAGE_INSTALL_DIR),)
	@echo "directory of image is not defined"
	@exit 1
endif
	@rm -rf $(IMAGE_INSTALL_DIR)/rootfs/customer_app/lib
	
	@mkdir -p $(IMAGE_INSTALL_DIR)/rootfs/customer_app/lib
	
	@cp -rf $(APPLICATION_PATH)/zk_full_fastboot/RunFiles/res/RunFirst/* $(IMAGE_INSTALL_DIR)/rootfs/customer_app/
	@cp -rf $(APPLICATION_PATH)/zk_full_fastboot/RunFiles/lib/RunFirst/* $(IMAGE_INSTALL_DIR)/rootfs/customer_app/lib/
	@cp -rf $(APPLICATION_PATH)/zk_full_fastboot/RunFiles/lib/RunLater $(IMAGE_INSTALL_DIR)/customer/lib
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/rootfs/customer_app/lib/*
	@cp -rf $(APPLICATION_PATH)/zk_full_fastboot/RunFiles/res/RunLater $(IMAGE_INSTALL_DIR)/customer/res
	@cp -rf $(APPLICATION_PATH)/zk_full_fastboot/RunFiles/etc/* $(IMAGE_INSTALL_DIR)/rootfs/etc/
	@cat $(APPLICATION_PATH)/zk_full_fastboot/RunFiles/demo.sh >> $(IMAGE_INSTALL_DIR)/rootfs/etc/init.sh
	#@cp -rf $(APPLICATION_PATH)/zk_full_fastboot/RunFiles/bin/zkgui $(IMAGE_INSTALL_DIR)/rootfs/customer_app/zkgui
	@cp -rf $(APPLICATION_PATH)/zk_full_fastboot/RunFiles/bin/zkgui_cus $(IMAGE_INSTALL_DIR)/rootfs/customer_app/zkgui
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/rootfs/customer_app/zkgui
