include $(APPLICATION_PATH)/jpeg2disp/src/makefile
app:app_src
ifeq ($(IMAGE_INSTALL_DIR),)
	@echo "directory of image is not defined"
	@exit 1
endif
	@cp -rf $(APPLICATION_PATH)/jpeg2disp/lib $(IMAGE_INSTALL_DIR)/customer
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/customer/lib/*
	@cp -rf $(APPLICATION_PATH)/jpeg2disp/res/logo1024x600.jpg $(IMAGE_INSTALL_DIR)/customer/logo.jpg
	@cp -rf $(APPLICATION_PATH)/jpeg2disp/src/logo $(IMAGE_INSTALL_DIR)/customer
	@$(TOOLCHAIN_REL)strip --strip-unneeded $(IMAGE_INSTALL_DIR)/customer/logo
	@cat $(APPLICATION_PATH)/jpeg2disp/run.sh >> $(IMAGE_INSTALL_DIR)/customer/demo.sh
	@make app_clean

