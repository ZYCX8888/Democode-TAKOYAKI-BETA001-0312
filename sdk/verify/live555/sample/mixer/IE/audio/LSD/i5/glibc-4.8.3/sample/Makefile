CROSS_COMPILE := /users/haoteng.fan/swork/tools/gcc-linaro-arm-linux-gnueabihf-4.8-2014.04_linux/bin/arm-linux-gnueabihf-
ARCH := -march=armv7-a -mfpu=neon
INCLUDES := -I../include/
LIB_ID := LSD
TEST_FILE := ./../sample/src/sample.c

.run:
	@mkdir -p ../bin/
	$(CC) $(INCLUDES) $(TEST_FILE) -O2 -g -o ../bin/run_$(LIB_ID)_$(CHIP_ID)_linux_static -L../lib/ -l$(LIB_ID)_LINUX -static -lm
	$(CC) $(INCLUDES) $(TEST_FILE) -O2 -g -o ../bin/run_$(LIB_ID)_$(CHIP_ID)_linux_dynamic -L../lib/ -l$(LIB_ID)_LINUX -Wl,-rpath=../lib/ -lm

run_$(LIB_ID)_I3: CC := $(CROSS_COMPILE)gcc $(ARCH)
run_$(LIB_ID)_I3: CHIP_ID :=I3
run_$(LIB_ID)_I3: .run
