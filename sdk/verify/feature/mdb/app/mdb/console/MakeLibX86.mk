OUTPUT_DIR := $(DB_OUT_PATH)/lib

OBJ_FILES := ./obj/X86/*.o
LIB_NAME=MdbConsoleX86
LIB_TYPE=$(DB_LIB_TYPE)
USE_X86 := 1

include $(DB_BUILD_TOP)/compile.mk

all: gen_lib