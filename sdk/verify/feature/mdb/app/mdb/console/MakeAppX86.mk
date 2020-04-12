############################################################ source files.
SUBDIRS:=.

EXEFILE    := console
OUTPUT_DIR := $(DB_OUT_PATH)/app/cmdX86
INC	 +=./inc
LIBS += -L$(DB_OUT_PATH)/lib
LIBS += -lMdbConsoleX86
USE_X86 := 1

############################################################ depnedent header files.
include $(DB_BUILD_TOP)/compile.mk

all: gen_exe
clean: clean_files