
############################################################ source files.
SUBDIRS := ./internal/MODULE_OBJ
OUTPUT_DIR := ./obj/MODULE_OBJ

############################################################ depnedent header files.
-include ./internal/MODULE_OBJ/MODULE_OBJ.mk
include ./dirs.mk
include $(DB_BUILD_TOP)/compile.mk
all: gen_obj
clean: clean_files