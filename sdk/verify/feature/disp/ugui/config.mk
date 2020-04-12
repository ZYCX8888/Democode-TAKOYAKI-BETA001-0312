include clear-config.mk

COMPILE_TARGET:=nop

ifeq ($(USE_CASE),fb)
COMPILE_TARGET:=lib
SRCS:=ugui.c
DEP_INCS+=$(PROJ_ROOT)/../sdk/verify/feature/disp/include/ugui
endif

include add-config.mk
