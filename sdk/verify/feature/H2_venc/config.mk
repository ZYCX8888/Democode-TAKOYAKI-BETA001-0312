include clear-config.mk
DEP_MODULE:common sys
CFLAGS:=-O0
SRCS:=st_main_venc.c
LIBS:=mi_sys mi_venc
include add-config.mk
