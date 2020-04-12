include clear-config.mk
CFLAGS:=-O0
SRCS:=test_divp_venc.c
LIBS:= mi_sys mi_venc mi_divp
include add-config.mk