OUTPUT_DIR := $(DB_OUT_PATH)/lib

OBJ_FILES := $(DB_OUT_PATH)/obj/*.o
LIB_NAME=$(DB_LIB_NAME)
LIB_TYPE=$(DB_LIB_TYPE)

include $(DB_BUILD_TOP)/compile.mk

all: gen_lib