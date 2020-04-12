TARGET_ROOT=$(shell pwd)

SDK_ROOT=$(TARGET_ROOT)/../../..

CROSS_COMPILE=arm-linux-gnueabihf-
CC  = $(CROSS_COMPILE)gcc
AR  = $(CROSS_COMPILE)ar
LD  = $(CROSS_COMPILE)ld
STRIP  = $(CROSS_COMPILE)strip


TARGET:= vdf_test

#Add source file
SRCS := vdf_test.c  

COBJS := $(SRCS:.c=.o)

VDF_LIB_DIR := $(SDK_ROOT)/interface/src/vdf
SHADOW_LIB_DIR := $(SDK_ROOT)/interface/src/shadow
VDEC_LIB_DIR := $(SDK_ROOT)/interface/src/vdec
MD_LIB_DIR := $(SDK_ROOT)/interface/src/md
OD_LIB_DIR := $(SDK_ROOT)/interface/src/od
IE_LIB_DIR := $(SDK_ROOT)/verify/module/IE/lib
SYS_LIB_DIR := $(SDK_ROOT)/interface/src/sys

VDF_LIB_SRC := 	-lmi_vdf  
MD_LIB_SRC := -lMD_LINUX
OD_LIB_SRC := -lOD_LINUX
SYS_LIB_SRC := -lmi_sys
VDEC_LIB_SRC := -lmi_vdec
SHADOW_LIB_SRC := -lmi_shadow

LIB += $(foreach lib,$(MD_LIB_SRC),$(addprefix -L$(MD_LIB_DIR) ,$(lib)))
LIB += $(foreach lib,$(OD_LIB_SRC),$(addprefix -L$(OD_LIB_DIR) ,$(lib)))
LIB += $(foreach lib,$(SYS_LIB_SRC),$(addprefix -L$(SYS_LIB_DIR) ,$(lib)))
LIB += $(foreach lib,$(VDEC_LIB_SRC),$(addprefix -L$(VDEC_LIB_DIR) ,$(lib)))
LIB += $(foreach lib,$(SHADOW_LIB_SRC),$(addprefix -L$(SHADOW_LIB_DIR) ,$(lib)))
LIB += $(foreach lib,$(VDF_LIB_SRC),$(addprefix -L$(VDF_LIB_DIR) ,$(lib)))

LDFLAGS += \
     -lpthread \
     -lrt \
     -ldl \
     -lc  \
     -lm 

#Add include files
SYS_INCDIR = $(SDK_ROOT)/interface/include/sys/
COMMON_INCDIR = $(SDK_ROOT)/interface/include/common/
VDEC_INCDIR = $(SDK_ROOT)/interface/include/vdec/
DIVP_INCDIR = $(SDK_ROOT)/interface/include/divp/
MD_INCDIR = $(SDK_ROOT)/interface/include/md/
OD_INCDIR = $(SDK_ROOT)/interface/include/od/
VDF_INCDIR = $(SDK_ROOT)/interface/include/vdf/
SHADOW_INCDIR = $(SDK_ROOT)/interface/include/shadow/

INCDIR += $(foreach incdir,$(SYS_INCDIR),$(addprefix -I,$(incdir)))
INCDIR += $(foreach incdir,$(COMMON_INCDIR),$(addprefix -I,$(incdir)))
INCDIR += $(foreach incdir,$(VDF_INCDIR),$(addprefix -I,$(incdir)))
INCDIR += $(foreach incdir,$(MD_INCDIR),$(addprefix -I,$(incdir)))
INCDIR += $(foreach incdir,$(OD_INCDIR),$(addprefix -I,$(incdir)))
INCDIR += $(foreach incdir,$(VDEC_INCDIR),$(addprefix -I,$(incdir)))
INCDIR += $(foreach incdir,$(DIVP_INCDIR),$(addprefix -I,$(incdir)))
INCDIR += $(foreach incdir,$(SHADOW_INCDIR),$(addprefix -I,$(incdir)))


#Add gcc compile flag
CFLAGS  = -Wall -Os
CFLAGS += -fPIE

all:
	@echo "make $(TARGET_NAME)"
	@$(MAKE) $(COBJS)
	@$(CC) $(COBJS) $(CFLAGS) $(INCDIR) $(LIB) $(LDFLAGS) -o $(TARGET)
	$(STRIP) -s $(TARGET)

clean:
	@$(shell if [ -f $(TARGET) ]; then rm $(TARGET); fi;)
	rm *.o

$(COBJS):%.o:%.c
	$(CC) $(CFLAGS) $(INCDIR) -c $< -o $@
	
