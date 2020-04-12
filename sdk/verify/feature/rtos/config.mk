include clear-config.mk
CFLAGS:=-O0
SRCS:=rtos_preload.c
LIBS:= mi_sys mi_vpe mi_vif mi_venc
include add-config.mk