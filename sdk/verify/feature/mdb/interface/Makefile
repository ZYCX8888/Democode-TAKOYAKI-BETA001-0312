############################################################ source files.
SUBDIRS :=./src
INC     +=./inc
INC     += $(DB_BUILD_TOP)/internal/inc

OUTPUT_DIR := $(DB_OUT_PATH)/obj/
CODEDEFINE += $(CC_OPTS)

############################################################ depnedent header files.
include $(DB_BUILD_TOP)/compile.mk
all: gen_obj
clean: clean_files
