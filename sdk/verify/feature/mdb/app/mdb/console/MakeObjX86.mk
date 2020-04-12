
############################################################ source files.
SUBDIRS :=./src
INC     +=./inc

OUTPUT_DIR := ./obj/X86/
USE_X86 := 1

############################################################ depnedent header files.
include $(DB_BUILD_TOP)/compile.mk
all: gen_obj
clean: clean_files
