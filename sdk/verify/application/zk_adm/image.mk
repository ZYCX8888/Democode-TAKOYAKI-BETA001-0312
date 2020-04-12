app:
ifeq ($(IMAGE_INSTALL_DIR),)
	@echo "directory of image is not defined"
	@exit 1
endif
	@cp -rf $(APPLICATION_PATH)/zk_adm/bin/zkgui_1024_600 $(IMAGE_INSTALL_DIR)/customer/zkgui
	#@cp -rf $(APPLICATION_PATH)/zk_adm/bin/zkgui_bgz $(IMAGE_INSTALL_DIR)/customer/zkgui
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/customer/zkgui
	@cp -rf $(APPLICATION_PATH)/zk_adm/lib $(IMAGE_INSTALL_DIR)/customer
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/customer/lib/*
	@cp -rf $(APPLICATION_PATH)/zk_adm/res $(IMAGE_INSTALL_DIR)/customer
	@cp -rf $(APPLICATION_PATH)/zk_adm/etc/* $(IMAGE_INSTALL_DIR)/rootfs/etc/
	@cp -rf $(APPLICATION_PATH)/zk_adm/etc/fbdev.ini $(IMAGE_INSTALL_DIR)/tvconfig/config/fbdev.ini 
	@cp -rf $(APPLICATION_PATH)/zk_adm/cursor.raw $(IMAGE_INSTALL_DIR)/customer
	@cp -rf $(APPLICATION_PATH)/zk_adm/demo.mp4 $(IMAGE_INSTALL_DIR)/customer
	@cat $(APPLICATION_PATH)/zk_adm/demo.sh >> $(IMAGE_INSTALL_DIR)/customer/demo.sh

