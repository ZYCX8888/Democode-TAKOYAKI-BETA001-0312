app:
ifeq ($(IMAGE_INSTALL_DIR),)
	@echo "directory of image is not defined"
	@exit 1
endif
	@cp -rf $(APPLICATION_PATH)/ssplayer/RunFile/minigui $(IMAGE_INSTALL_DIR)/customer
	@cp -rf $(APPLICATION_PATH)/ssplayer/RunFile/lib $(IMAGE_INSTALL_DIR)/customer
	@cp -rf $(APPLICATION_PATH)/ssplayer/RunFile/etc/* $(IMAGE_INSTALL_DIR)/rootfs/etc/
	@cat $(APPLICATION_PATH)/ssplayer/RunFile/demo.sh >> $(IMAGE_INSTALL_DIR)/customer/demo.sh

