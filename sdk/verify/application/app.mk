APPLICATION_PATH=$(PROJ_ROOT)/../sdk/verify/application
include $(PROJ_ROOT)/release/customer_tailor/$(CUSTOMER_TAILOR)
ifeq ($(verify_smarttalk),enable)
	include $(APPLICATION_PATH)/smarttalk/image.mk
endif
ifeq ($(verify_ssplayer),enable)
	include $(APPLICATION_PATH)/ssplayer/image.mk
endif
ifeq ($(verify_zk),enable)
	include $(APPLICATION_PATH)/customer_zk/image.mk
endif
ifeq ($(verify_zk_wifi),enable)
	include $(APPLICATION_PATH)/customer_zk_wifi/image.mk
endif
ifeq ($(verify_zk_wifi_fastboot),enable)
	include $(APPLICATION_PATH)/customer_zk_wifi_fastboot/image.mk
endif
ifeq ($(verify_zk_player),enable)
	include $(APPLICATION_PATH)/customer_zk_player/image.mk
endif
ifeq ($(verify_smarthome),enable)
	include $(APPLICATION_PATH)/smarthome/image.mk
endif
ifeq ($(verify_zk_bootup),enable)
	include $(APPLICATION_PATH)/zk_bootup/image.mk
endif
ifeq ($(verify_zk_mini),enable)
	include $(APPLICATION_PATH)/zk_mini/image.mk
endif
ifeq ($(verify_zk_mini_fastboot),enable)
	include $(APPLICATION_PATH)/zk_mini_fastboot/image.mk
endif
ifeq ($(verify_zk_full_fastboot),enable)
	include $(APPLICATION_PATH)/zk_full_fastboot/image.mk
endif
ifeq ($(verify_zk_full),enable)
	include $(APPLICATION_PATH)/zk_full/image.mk
endif
ifeq ($(verify_jpeg2disp),enable)
	include $(APPLICATION_PATH)/jpeg2disp/image.mk
endif
ifeq ($(verify_zk_adm),enable)
	include $(APPLICATION_PATH)/zk_adm/image.mk
endif
