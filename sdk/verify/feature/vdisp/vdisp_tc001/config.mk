include clear-config.mk
CFLAGS:=-O0
SRCS:=vdisp_basic.c ../common/sstardisp.c ../common/vdisp_common.c
LIBS:=mi_common mi_sys mi_vdisp mi_disp  mi_hdmi mi_panel dl mi_gfx
include add-config.mk
