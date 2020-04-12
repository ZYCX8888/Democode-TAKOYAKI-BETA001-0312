include clear-config.mk
CFLAGS:=-O0
SRCS:=test_venc.c
LIBS:= mi_sys mi_venc
include add-config.mk