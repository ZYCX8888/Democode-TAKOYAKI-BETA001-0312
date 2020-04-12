include clear-config.mk
CFLAGS:=-O0
SRCS:=test_case_06.c
LIBS:= mi_sys mi_vpe mi_vif mi_venc mi_divp
include add-config.mk