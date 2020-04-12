include clear-config.mk
WITH_DISP_TEST?=yes
ifeq ($(WITH_DISP_TEST),yes)
COMPILE_TARGET:=bin
CFLAGS += -DENABLE_WITH_DISP_TEST
else
COMPILE_TARGET:=nop
endif

SRCS:= ../vpe_test_common.c vpe_disp_tc002.c
LIBS:= mi_sys mi_vpe mi_hdmi mi_disp
include add-config.mk
