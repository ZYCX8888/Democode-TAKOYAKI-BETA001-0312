############################################################ source files.
SRC := do_cmd.c mdbcmd.c

EXEFILE    := mdbrec
OUTPUT_DIR := $(DB_OUT_PATH)/app/cmdX86
USE_X86 := 1

############################################################ depnedent header files.
include $(DB_BUILD_TOP)/compile.mk
all: gen_exe
clean: clean_files