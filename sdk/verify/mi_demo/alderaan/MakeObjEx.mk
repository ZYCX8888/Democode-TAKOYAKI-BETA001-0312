
export CUR_DIR=../common/$(MODULE_OBJ)
############################################################ source files.
SUBDIRS := ../common/$(MODULE_OBJ)
OUTPUT_DIR := ./obj/$(MODULE_OBJ)

############################################################ depnedent header files.
-include ../common/$(MODULE_OBJ)/$(MODULE_OBJ).mk
include ./dirs.mk
include $(DB_BUILD_TOP)/compile.mk
all: gen_obj
clean: clean_files