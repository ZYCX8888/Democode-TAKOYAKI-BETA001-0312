CROSS_COMPILE ?= /tool/gcc-linaro-arm-linux-gnueabihf-4.8-2014.04_linux/bin/arm-linux-gnueabihf-
ARCH := -march=armv7-a -mfpu=neon
INCLUDES := -I../include/
LIB_ID := VG
TEST_FILE := VG_Sample_Code.c

.run:
	@mkdir -p ../bin/
	$(CC) $(INCLUDES) $(TEST_FILE) -O2 -g -o run_linux_static -L../lib/ -lVG_LINUX -L../../MTE/lib -lMTE_LINUX -static -lm
	$(CC) $(INCLUDES) $(TEST_FILE) -O2 -g -o run_linux_dynamic -L../lib/ -lVG_LINUX -L../../MTE/lib -lMTE_LINUX -lm -Wl,-rpath=../lib/ -Wl,-rpath=../../MTE/lib/

run_$(LIB_ID)_I3: CC := $(CROSS_COMPILE)gcc $(ARCH)
run_$(LIB_ID)_I3: CHIP_ID :=I3
run_$(LIB_ID)_I3: .run
