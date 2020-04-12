include clear-config.mk
CFLAGS:=-O0
SRCS:=mi_ao_test.c
LIBS:=mi_sys mi_ao SRC_LINUX APC_LINUX g711 g726
include add-config.mk
