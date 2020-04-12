OUTPUT_DIR := $(DB_OUT_PATH)/lib

OBJ_FILES := ./obj/arm/*.o
LIB_NAME=MdbConsole
LIB_TYPE=$(DB_LIB_TYPE)

include $(DB_BUILD_TOP)/compile.mk

all: gen_lib