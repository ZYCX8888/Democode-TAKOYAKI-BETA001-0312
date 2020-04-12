app:
ifeq ($(IMAGE_INSTALL_DIR),)
	@echo "directory of image is not defined"
	@exit 1
endif
	@cp -rf $(APPLICATION_PATH)/zk_full/bin/zkgui_1024_600 $(IMAGE_INSTALL_DIR)/customer/zkgui
	#@cp -rf $(APPLICATION_PATH)/zk_full/bin/zkgui_800_480 $(IMAGE_INSTALL_DIR)/customer/zkgui
	#@cp -rf $(APPLICATION_PATH)/zk_full/bin/zkgui_bgz $(IMAGE_INSTALL_DIR)/customer/zkgui
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/customer/zkgui
	@cp -rf $(APPLICATION_PATH)/zk_full/lib $(IMAGE_INSTALL_DIR)/customer
	@cp -rf $(APPLICATION_PATH)/zk_full/libdns $(IMAGE_INSTALL_DIR)/customer
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/customer/lib/*
	@cp -rf $(APPLICATION_PATH)/zk_full/res $(IMAGE_INSTALL_DIR)/customer
	@cp -rf $(APPLICATION_PATH)/zk_full/etc/* $(IMAGE_INSTALL_DIR)/rootfs/etc/
	@cat $(APPLICATION_PATH)/zk_full/demo.sh >> $(IMAGE_INSTALL_DIR)/customer/demo.sh
	@echo "nameserver 172.19.30.11" >> $(IMAGE_INSTALL_DIR)/rootfs/etc/resolv.conf 

