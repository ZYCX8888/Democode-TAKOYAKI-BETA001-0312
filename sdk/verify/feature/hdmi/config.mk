include clear-config.mk
DEP_MODULE:common sys
CFLAGS:=-O0
SRCS:=hdmi.c
LIBS:=mi_hdmi mi_disp mi_ao SRC_LINUX APC_LINUX g711 g726 dl mi_common
include add-config.mk
