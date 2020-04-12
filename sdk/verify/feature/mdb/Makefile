# All rights reserved.
#
# Makefile used for building application.
#
################################################################################
.PHONY : all clean lib app install

DB_PROJECT=demo
DB_BUILD_TOP:=$(shell pwd)
DB_OUT_PATH:=$(DB_BUILD_TOP)/out/$(DB_PROJECT)
DB_LIB_NAME:=mdb
DB_LIB_TYPE:=static
DB_TRANS_BUFFER_SIZE:=256 #IPC buffer size is 256K
DB_ALKAID_PROJ:=../../../../project/configs/current.configs

export DB_BUILD_TOP
export DB_OUT_PATH
export DB_PROJECT
export DB_LIB_TYPE
export DB_LIB_NAME
export DB_TRANS_BUFFER_SIZE
export DB_ALKAID_PROJ
include $(DB_ALKAID_PROJ)
include $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/toolchain.mk
export PROJ_ROOT PRODUCT CHIP BOARD TOOLCHAIN TOOLCHAIN_VERSION KERNEL_VERSION CUSTOMER_OPTIONS MOD_PREFIX INTERFACE_ENABLED INTERFACE_DISABLED MHAL_ENABLED MHAL_DISABLED FBDEV

all: app

install: app
	@$(STRIP) --strip-unneeded ./out/$(DB_PROJECT)/app/cmd/*
	@mkdir -p $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mdb/
	@cp -vrf ./out/$(DB_PROJECT)/app/cmd/console  $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mdb/
	@cp -vrf ./out/$(DB_PROJECT)/app/cmd/service  $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mdb/
	@cp -vrf ./out/$(DB_PROJECT)/app/cmd/mdbcmd  $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mdb/
	@cp -vrf ./out/$(DB_PROJECT)/app/cmd/mdbrec  $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mdb/
	@cp -vrf ./scripts/rtsp.sh  $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mdb/

lib:
	$(MAKE) -C ./internal/ all
	$(MAKE) -C ./interface/ all
	$(MAKE) -f ./MakeLib.mk all

app: lib
	$(MAKE) -C ./app/ all

clean:
	$(MAKE) -C ./app/ clean
	$(MAKE) -C ./interface/ clean
	$(MAKE) -C ./internal/ clean
	@rm -rvf ./out/ -rvf
