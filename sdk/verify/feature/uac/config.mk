include clear-config.mk
CFLAGS:=-O0
SRCS:=mi_uac_test.c
LIBS:=mi_sys mi_ai mi_ao mi_uac SRC_LINUX APC_LINUX AEC_LINUX g711 g726
include add-config.mk
