DB_PROJECT:=demo
DB_BUILD_TOP:=$(shell pwd)
DB_OUT_PATH:=$(DB_BUILD_TOP)/../out/$(DB_PROJECT)
DB_ALKAID_PROJ:=$(DB_BUILD_TOP)/../../../../project/configs/current.configs
include $(DB_ALKAID_PROJ)
include $(PROJ_ROOT)/configs/current.configs
include $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/toolchain.mk

MODULES :=dla_classify dla_detect dla vpe ldc dualsensor preview venc audio_all_test_case uvc_cus3a rtos_preload dla_bench dla_detect_yolo_v3_7
ifeq ($(DUAL_OS), on)
    MODULES += mipitx
endif
INTERNAL_LIBS := $(patsubst ./internal/%,%,$(wildcard ./internal/*))
EXTRA_LIBS := $(patsubst ../common/%,%,$(wildcard ../common/*))
MODULES_CLEAN := $(foreach m,$(MODULES),$(m)_clean)
MODULES_APP_ALL := $(foreach m,$(MODULES),$(m)_app_all)
MODULES_APP_CLEAN := $(foreach m,$(MODULES),$(m)_app_clean)
MODULES_OBJS_ALL := $(foreach m,$(INTERNAL_LIBS),$(m)_obj_all)
MODULES_OBJS_CLEAN := $(foreach m,$(INTERNAL_LIBS),$(m)_obj_clean)
MODULES_OBJS_EX_ALL := $(foreach m,$(EXTRA_LIBS),$(m)_obj_all)
MODULES_OBJS_EX_CLEAN := $(foreach m,$(EXTRA_LIBS),$(m)_obj_clean)
MODULES_LIBS_ALL := $(foreach m,$(INTERNAL_LIBS) $(EXTRA_LIBS),$(m)_lib_all)
MODULES_LIBS_CLEAN := $(foreach m,$(INTERNAL_LIBS) $(EXTRA_LIBS),$(m)_lib_clean)

export PROJ_ROOT CHIP PRODUCT BOARD TOOLCHAIN TOOLCHAIN_VERSION KERNEL_VERSION CUSTOMER_OPTIONS MOD_PREFIX INTERFACE_ENABLED INTERFACE_DISABLED MHAL_ENABLED MHAL_DISABLED DUAL_OS
export DB_BUILD_TOP
export DB_OUT_PATH
export DB_PROJECT
export DB_ALKAID_PROJ
.PHONY: all lib obj clean $(MODULES) $(MODULES_CLEAN) $(MODULES_APP_ALL) $(MODULES_APP_CLEAN) $(MODULES_OBJS_ALL) $(MODULES_OBJS_CLEAN) $(MODULES_LIBS_ALL) $(MODULES_LIBS_CLEAN)

all:
	$(foreach mod,$(MODULES),$(MAKE) $(mod);)
clean:
	$(foreach mod,$(MODULES_OBJS_EX_CLEAN),$(MAKE) $(mod);)
	$(foreach mod,$(MODULES_OBJS_CLEAN),$(MAKE) $(mod);)
	$(foreach mod,$(MODULES_LIBS_CLEAN),$(MAKE) $(mod);)
	$(foreach mod,$(MODULES_APP_CLEAN),$(MAKE) $(mod);)
	@rm -rvf MakefileTmp_*
	@rm -rvf $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/prog_*

install:
	@$(STRIP) --strip-unneeded ../out/$(DB_PROJECT)/app/*
	@mkdir -p $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/
	@cp -vrf ../out/$(DB_PROJECT)/app/* $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/
	@cp -vrf ./preview/config/ $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/
ifeq ($(DUAL_OS), on)
	@cp -vrf ../out/$(DB_PROJECT)/app/prog_rtos_preload $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/
	@cp -vrf ../out/$(DB_PROJECT)/app/prog_audio_all_test_case $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/
endif
	@cp -vrf ../common/res/ascii_8x16 $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/
	@cp -vrf ../common/res/ascii_16x32 $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/
	@cp -vrf ../common/res/hanzi_16x16 $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mi_demo/

$(MODULES):
	@$(MAKE) MODULE=$@ -f ./MakeModule.mk all
	@$(MAKE) $(patsubst %,%_app_all,$@)

$(MODULES_CLEAN):
	@$(MAKE) $(patsubst %_clean,%_app_clean,$@)
	@$(MAKE) MODULE=$(patsubst %_clean,%,$@) -f ./MakeModule clean

$(MODULES_APP_ALL):
	@$(MAKE) MODULE=$(patsubst %_app_all,%,$@) -f ./MakeApp.mk all

$(MODULES_APP_CLEAN):
	@$(MAKE) MODULE=$(patsubst %_app_clean,%,$@) -f ./MakeApp.mk clean

$(MODULES_OBJS_ALL):
	@$(MAKE) MODULE_OBJ=$(patsubst %_obj_all,%,$@) -f ./MakeObj.mk all

$(MODULES_OBJS_CLEAN):
	@$(MAKE) MODULE_OBJ=$(patsubst %_obj_clean,%,$@) -f ./MakeObj.mk clean

$(MODULES_OBJS_EX_ALL):
	@$(MAKE) MODULE_OBJ=$(patsubst %_obj_all,%,$@) -f ./MakeObjEx.mk all

$(MODULES_OBJS_EX_CLEAN):
	@$(MAKE) MODULE_OBJ=$(patsubst %_obj_clean,%,$@) -f ./MakeObjEx.mk clean

$(MODULES_LIBS_ALL):
	@$(MAKE) MODULE_LIB=$(patsubst %_lib_all,%,$@) -f ./MakeLib.mk all

$(MODULES_LIBS_CLEAN):
	@$(MAKE) MODULE_LIB=$(patsubst %_lib_clean,%,$@) -f ./MakeLib.mk clean
