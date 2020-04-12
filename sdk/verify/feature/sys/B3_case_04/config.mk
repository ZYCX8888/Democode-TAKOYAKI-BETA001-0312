include clear-config.mk
CFLAGS:=-O0
SRCS:=test_case_04.c
LIBS:= mi_sys mi_vpe mi_vif mi_venc mi_divp mi_sensor
include add-config.mk