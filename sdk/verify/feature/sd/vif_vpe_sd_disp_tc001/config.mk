include clear-config.mk
SRCS:= ../common/st_common.c ../common/st_vif.c ../common/st_vpe.c ../common/st_sd.c ../common/st_disp.c ../common/st_hdmi.c vif_vpe_sd_disp_tc001.c 
LIBS:= mi_sys mi_vpe mi_hdmi mi_disp mi_vif mi_sd
include add-config.mk
