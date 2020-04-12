############################################################ source files.
SUBDIRS:=./MODULE

EXEFILE    := prog_MODULE
OUTPUT_DIR := $(DB_OUT_PATH)/app

############################################################ depnedent header files.
include ./st.mk
-include ./MODULE/MODULE.mk
include ./dirs.mk
include $(DB_BUILD_TOP)/compile.mk

all: gen_exe
clean: clean_files