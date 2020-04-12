
############################################################ source files.
SUBDIRS :=./src
INC     +=./inc
INC     += $(DB_BUILD_TOP)/interface/inc
OUTPUT_DIR := ./obj/

############################################################ depnedent header files.
include $(DB_BUILD_TOP)/compile.mk
all: gen_obj
clean: clean_files
