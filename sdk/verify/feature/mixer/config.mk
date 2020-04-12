include clear-config.mk
CFLAGS:=-O0 -DCAM_OS_LINUX_USER -Wall -Werror
SRCS:=mixer_main.c mixer_test.c mixer_video_info.c mixer_memmap.c mixer_i2c.c mixer_ad_dh9931.c mixer_util.c
SRCS+=mixer_vif.c mixer_vpe.c mixer_disp.c
SRCS+=mixer_path.c mixer_path_vif.c mixer_path_vif_vpe_venc.c mixer_path_vif_vpe_disp.c mixer_path_vif_vpe.c mixer_path_vif_divp_disp.c
LIBS:=mi_sys mi_vif mi_vpe mi_venc mi_divp mi_disp mi_hdmi m
ifeq ($(CHIP),i2)
CFLAGS += -DMIXER_ENABLE_LIBADDA
LIBS+=adda
endif
include add-config.mk
