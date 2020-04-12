include clear-config.mk
CFLAGS:=-O0
SRCS:=vdec.c
LIBS:=mi_sys mi_vdec mi_divp
include add-config.mk