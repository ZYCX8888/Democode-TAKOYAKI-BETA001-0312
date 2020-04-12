include clear-config.mk
COMPILE_TARGET:=bin
SRCS:= ../vpe_test_common.c vpe_tc003.c
WITH_DISP_TEST?=yes
ifeq ($(WITH_DISP_TEST),yes)
CFLAGS += -DENABLE_WITH_DISP_TEST
LIBS:= mi_sys mi_vpe mi_disp mi_hdmi
else
LIBS:= mi_sys mi_vpe
endif
include add-config.mk
