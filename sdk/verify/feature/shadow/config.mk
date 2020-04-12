include clear-config.mk
CFLAGS:=-O0
SRCS:=shadow.c
LIBS:=mi_sys mi_shadow mi_vdec
include add-config.mk