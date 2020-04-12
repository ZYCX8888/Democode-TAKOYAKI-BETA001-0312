include clear-config.mk
CFLAGS:=-O0
SRCS:=mi_ipu_test.c
LIBS:=mi_sys mi_ipu mi_common dl pthread
include add-config.mk
