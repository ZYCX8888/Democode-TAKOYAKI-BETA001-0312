app:
ifeq ($(IMAGE_INSTALL_DIR),)
	@echo "directory of image is not defined"
	@exit 1
endif
	@cp $(APPLICATION_PATH)/smarttalk/demoApp/build/mginit $(IMAGE_INSTALL_DIR)/customer
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/customer/mginit
	@cp -rf $(APPLICATION_PATH)/smarttalk/RunFile/appres  $(IMAGE_INSTALL_DIR)/customer
	@cp -rf $(APPLICATION_PATH)/smarttalk/RunFile/CSpotter  $(IMAGE_INSTALL_DIR)/customer
	@cp -rf $(APPLICATION_PATH)/smarttalk/RunFile/minigui $(IMAGE_INSTALL_DIR)/customer
	@cp -rf $(APPLICATION_PATH)/smarttalk/RunFile/lib $(IMAGE_INSTALL_DIR)/customer
	@cp -rf $(APPLICATION_PATH)/smarttalk/RunFile/etc/* $(IMAGE_INSTALL_DIR)/rootfs/etc/
	@cp $(APPLICATION_PATH)/smarttalk/RunFile/fbdev.ini $(IMAGE_INSTALL_DIR)/tvconfig/config/fbdev.ini
	@cat $(APPLICATION_PATH)/smarttalk/RunFile/demo.sh >> $(IMAGE_INSTALL_DIR)/customer/demo.sh
	@cp $(APPLICATION_PATH)/smarttalk/RunFile/hscrollview.rc $(IMAGE_INSTALL_DIR)/customer
	@cp $(APPLICATION_PATH)/smarttalk/RunFile/*.xml $(IMAGE_INSTALL_DIR)/customer
	@cp $(APPLICATION_PATH)/smarttalk/RunFile/720P25.h264 $(IMAGE_INSTALL_DIR)/customer
	@cp $(APPLICATION_PATH)/smarttalk/RunFile/8K_16bit_MONO.wav $(IMAGE_INSTALL_DIR)/customer
	#@mv $(IMAGE_INSTALL_DIR)/customer/smartLayout_800x480.xml $(IMAGE_INSTALL_DIR)/customer/smartLayout.xml
	@mv $(IMAGE_INSTALL_DIR)/customer/smartLayout_1024x600.xml $(IMAGE_INSTALL_DIR)/customer/smartLayout.xml

