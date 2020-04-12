include clear-config.mk
CFLAGS:=-O0
SRCS:=test_vif_vpe_venc.c
LIBS:= mi_sys mi_vpe mi_vif mi_venc mi_sensor
include add-config.mk