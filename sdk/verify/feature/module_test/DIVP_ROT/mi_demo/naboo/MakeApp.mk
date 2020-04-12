############################################################ source files.
SUBDIRS:=./MODULE ./common ./common/cdnn

EXEFILE    := prog_MODULE
OUTPUT_DIR := $(DB_OUT_PATH)/app

############################################################ depnedent header files.
include ./dirs.mk
-include ./MODULE/MODULE.mk
include $(DB_BUILD_TOP)/compile.mk

all: gen_exe
clean: clean_files