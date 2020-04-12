include $(DB_ALKAID_PROJ)
include $(PROJ_ROOT)/configs/current.configs

ifeq ($(TOOLCHAIN), uclibc)
ifeq ($(PRODUCT), nvr)
    MODULES := ao fb vdec vifvo venc
else ifeq ($(PRODUCT), dvr)
    MODULES := ao fb vdec vifvo venc
else ifeq ($(PRODUCT), usb_cam)
    MODULES := ao fb vdec venc warp uvc_sd isp_tuning
endif
else
ifeq ($(PRODUCT), nvr)
    MODULES := ao fb vdec vifvo venc
else ifeq ($(PRODUCT), dvr)
    MODULES := ao fb vdec vifvo venc
else ifeq ($(PRODUCT), usb_cam)
    MODULES := ao fb vdec vifvo venc warp ceva uvc ailab ceva_vx rtsp uvc_sd isp_tuning
endif
endif
MODULES_ALL := $(foreach m,$(MODULES),$(m)_all)
MODULES_CLEAN := $(foreach m,$(MODULES),$(m)_clean)

export MODULES MODULES_ALL MODULES_CLEAN
.PHONY: all clean $(MODULES) $(MODULES_CLEAN)

all:
	@$(DB_MAKE) -f ./Make.mk all
clean:
	@$(DB_MAKE) -f ./Make.mk clean

$(MODULES):
	@cp ./MakeModule.mk ./MakefileTmp1.mk
	@sed -i "s/MODULE/$@/g" ./MakefileTmp1.mk
	@$(DB_MAKE) -f ./MakefileTmp1.mk all
	@rm ./MakefileTmp1.mk -rf
	@$(DB_MAKE) -f Make.mk $(patsubst %,%_all,$@)

$(MODULES_CLEAN):
	@cp ./MakeModule.mk ./MakefileTmp1.mk
	@sed -i "s/MODULE/$(patsubst %_clean,%,$@)/g" ./MakefileTmp1.mk
	@$(DB_MAKE) -f ./MakefileTmp1.mk clean
	@rm ./MakefileTmp1.mk -rf
	@$(DB_MAKE) -f Make.mk $@
