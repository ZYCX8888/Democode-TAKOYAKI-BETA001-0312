include clear-config.mk
DEPS:=exp_lib example/sd1 busybox
CFLAGS:=-O0
SRCS:=cat1.c cat2.c cat.c
LIBS:=mi_bar busybox
include add-config.mk
