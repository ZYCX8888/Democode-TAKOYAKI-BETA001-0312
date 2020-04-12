include clear-config.mk
CFLAGS:=-O0
SRCS:=wlan.c
LIBS:=mi_wlan cjson
include add-config.mk
