include clear-config.mk
DEP_MODULE:common sys
CFLAGS:=-O0
SRCS:=gfx.c
LIBS:=mi_sys mi_gfx
include add-config.mk
