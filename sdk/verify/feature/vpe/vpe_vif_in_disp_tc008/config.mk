include clear-config.mk
WITH_DISP_TEST?=yes
ifeq ($(WITH_DISP_TEST)_$(CHIP),yes_i2)
COMPILE_TARGET:=bin
CFLAGS += -DENABLE_WITH_DISP_TEST
else
COMPILE_TARGET:=nop
endif

SRCS:= ../vpe_test_common.c vpe_vif_in_disp_tc008.c ../vpe_vif_util.c
LIBS:= mi_sys mi_vpe mi_hdmi mi_disp mi_vif
ifeq ($(CHIP),i2)
CFLAGS += -DTEST_VIF_ENABLE_LIBADDA
LDFLAGS+= -L../
LIBS+=adda
SRCS+=../i2c.c ../memmap.c 
endif
include add-config.mk
