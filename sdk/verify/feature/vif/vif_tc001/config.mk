ifeq ($(CHIP),i2)
include clear-config.mk
CFLAGS:=-O0 -DCAM_OS_LINUX_USER -Wall -Werror -fno-builtin-log
SRCS:=../i2c.c ../memmap.c vif_tc001.c
LIBS:=mi_sys mi_vif mi_vpe mi_venc mi_divp mi_disp mi_hdmi mi_common

CFLAGS += -DTEST_VIF_ENABLE_LIBADDA
LIBS+=m adda
include add-config.mk
endif