OUTPUT_DIR := $(DB_OUT_PATH)/lib

OBJ_FILES := ./obj/*.o
LIB_NAME=MdbService
LIB_TYPE=$(DB_LIB_TYPE)

include $(DB_BUILD_TOP)/compile.mk

all: gen_lib