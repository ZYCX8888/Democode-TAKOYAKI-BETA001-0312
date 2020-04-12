include clear-config.mk
SRCS:= ../common/st_common.c ../common/st_divp.c  divp_tc001.c 
LIBS:= mi_sys mi_divp 
include add-config.mk
