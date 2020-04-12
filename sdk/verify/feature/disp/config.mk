include clear-config.mk

COMPILE_TARGET:=bin

USE_CASE:=disp

DEPS:=disp/common

ifeq ($(USE_CASE),fb)
	SRCS:= fb_ut.c
	DEPS+=disp/ugui
else
	ifeq ($(USE_CASE),disp)
		SRCS:= disp_ut.c
	else
		ifeq ($(USE_CASE),vdec_disp)
			SRCS:= vdec_ut.c
		endif
	endif
endif

LIBS:= mi_sys mi_common dl mi_disp
ifneq ($(interface_panel),disable)
	LIBS+=mi_panel
endif
ifneq ($(interface_vdec),disable)
	LIBS+=mi_vdec
endif
ifneq ($(interface_divp),disable)
	LIBS+=mi_divp
endif
ifneq ($(interface_hdmi),disable)
	LIBS+=mi_hdmi
endif
ifneq ($(interface_venc),disable)
	LIBS+=mi_venc
endif

DEP_INCS+=$(PROJ_ROOT)/../sdk/verify/feature/disp/include/ugui
DEP_INCS+=$(PROJ_ROOT)/../sdk/verify/feature/disp/include/common
DEP_INCS+=$(PROJ_ROOT)/../sdk/verify/feature/disp/include/panel/ttl
DEP_INCS+=$(PROJ_ROOT)/../sdk/verify/feature/disp/include/panel/mipi
DEP_INCS+=$(PROJ_ROOT)/../sdk/verify/feature/disp/include/panel/lvds

include add-config.mk
