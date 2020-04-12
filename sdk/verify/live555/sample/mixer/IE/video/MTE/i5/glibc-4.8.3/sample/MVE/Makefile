CROSS_COMPILE ?= ~/tools/gcc-linaro-arm-linux-gnueabihf-4.8-2014.04_linux/bin/arm-linux-gnueabihf-
ARCH := -march=armv7-a -mfpu=neon
INCLUDES := -I../../include/ -I../../../../../../MI/inc
LIB_ID := MVE
TEST_FILE :=
TEST_FILE += sample_16bit_to_8bit.c
TEST_FILE += sample_mag_and_ang.c
TEST_FILE += sample_and.c
TEST_FILE += sample_canny.c
TEST_FILE += sample_ccl.c
TEST_FILE += sample_or.c
TEST_FILE += sample_xor.c
TEST_FILE += sample_add.c
TEST_FILE += sample_sub.c
TEST_FILE += sample_ncc.c
TEST_FILE += sample_gmm.c
TEST_FILE += sample_dilate.c
TEST_FILE += sample_erode.c
TEST_FILE += sample_thresh.c
TEST_FILE += sample_thresh_s16.c
TEST_FILE += sample_thresh_u16.c
TEST_FILE += sample_bernsen.c
TEST_FILE += sample_adp_thresh.c
TEST_FILE += sample_equalize_hist.c
TEST_FILE += sample_hist.c
TEST_FILE += sample_lbp.c
TEST_FILE += sample_line_filter_hor.c
TEST_FILE += sample_map.c
TEST_FILE += sample_noise_remove_hor.c
TEST_FILE += sample_sad.c
TEST_FILE += sample_sobel.c
TEST_FILE += sample_csc.c
TEST_FILE += sample_filter.c
TEST_FILE += sample_filter_csc.c
TEST_FILE += sample_integ.c
TEST_FILE += sample_lk_optical_flow.c
TEST_FILE += sample_order_statistic_filter.c
TEST_FILE += sample_normal_gradient.c
TEST_FILE += sample_main.c
TEST_FILE += sample_main.h

.run:
	$(CC) $(INCLUDES) $(TEST_FILE) -O2 -g -o run_linux_static -L../../lib/ -lMTE_LINUX -static -lm
	$(CC) $(INCLUDES) $(TEST_FILE) -O2 -g -o run_linux_dynamic -L../../lib/ -lMTE_LINUX -lm -Wl,-rpath=../../lib/

run_$(LIB_ID)_I3: CC := $(CROSS_COMPILE)gcc $(ARCH)
run_$(LIB_ID)_I3: CHIP_ID :=I3
run_$(LIB_ID)_I3: .run
