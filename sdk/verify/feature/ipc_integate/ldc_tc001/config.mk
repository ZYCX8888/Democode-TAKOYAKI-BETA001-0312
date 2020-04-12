include clear-config.mk
SRCS:= ../common/st_common.c ../common/st_vif.c ../common/st_vpe.c  ldc_tc001.c
LIBS:= mi_sys mi_vpe  mi_vif mi_ldc
include add-config.mk
