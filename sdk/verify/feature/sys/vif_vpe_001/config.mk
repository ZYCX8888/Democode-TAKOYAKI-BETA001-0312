include clear-config.mk
CFLAGS:=-O0
SRCS:=vif_vpe_001.c
LIBS:= mi_sys mi_vpe mi_vif mi_sensor
include add-config.mk