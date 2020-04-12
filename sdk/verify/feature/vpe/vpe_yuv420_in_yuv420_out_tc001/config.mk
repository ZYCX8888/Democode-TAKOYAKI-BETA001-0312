include clear-config.mk
COMPILE_TARGET:=bin
WITH_DISP_TEST?=yes
SRCS:= ../vpe_test_common.c vpe_yuv420_in_yuv420_out_tc001.c
ifeq ($(WITH_DISP_TEST),yes)
CFLAGS += -DENABLE_WITH_DISP_TEST
LIBS:= mi_sys mi_vpe mi_disp mi_hdmi
else
LIBS:= mi_sys mi_vpe
endif
include add-config.mk
