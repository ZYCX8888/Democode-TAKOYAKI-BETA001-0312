include clear-config.mk
CFLAGS:=-O0 -lm
SRCS:=module_test_16bit_to_8bit.c module_test_add.c module_test_and.c module_test_canny.c module_test_ccl.c module_test_csc.c module_test_dilate.c module_test_equalize_hist.c module_test_erode.c module_test_filter.c module_test_filter_csc.c module_test_gmm.c module_test_hist.c module_test_integ.c module_test_lbp.c module_test_lk_optical_flow.c module_test_mag_and_ang.c module_test_main.c module_test_main.h module_test_map.c module_test_ncc.c module_test_normal_gradient.c module_test_or.c module_test_order_statistic_filter.c module_test_sad.c module_test_sobel.c module_test_sub.c module_test_thresh.c module_test_thresh_s16.c module_test_thresh_u16.c module_test_xor.c
include add-config.mk
