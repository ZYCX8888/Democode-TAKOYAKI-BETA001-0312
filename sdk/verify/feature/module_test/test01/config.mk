include clear-config.mk
DEP_MODULE:common sys hdmi disp vpe vidsp divp vdec vif rgn
CFLAGS:=-O0
SRCS:=module_main.c
LIBS:=mi_sys mi_hdmi mi_disp mi_vpe mi_disp mi_vdisp mi_divp mi_vdec mi_vif mi_rgn
include add-config.mk
