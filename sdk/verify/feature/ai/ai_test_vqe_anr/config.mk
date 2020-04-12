include clear-config.mk
COMPILE_TARGET:=bin
CFLAGS:=-O0
SRCS:=ai_test_vqe_anr.c
LIBS:=mi_sys mi_ai SRC_LINUX APC_LINUX AEC_LINUX g711 g726 mi_ao
include add-config.mk
