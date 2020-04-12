include clear-config.mk
SRCS:= ../common/st_common.c ../common/st_vif.c ../common/st_vpe.c  vif_vpe_venc_tc001.c ../common/env.c ../common/venc_common.c
LIBS:= mi_sys mi_vpe  mi_vif mi_venc
include add-config.mk
