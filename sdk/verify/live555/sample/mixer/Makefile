TOP_DIR  ?=$(shell pwd)
SDK_ROOT ?=$(shell pwd)/../../../../../
SDK_DIR  ?=$(SDK_ROOT)/sdk
PROJECT_DIR  ?=$(SDK_ROOT)/project

MVXV_START := MVX3
MVXV_LIB_TYPE := I6e
MS_PLATFORM_ID := __
COMMITNUMBER_MIXER := g$(shell cd $(TOP_DIR); git log --format=%h -n 1 2> /dev/null)$(shell cd -> /dev/null;)

COMP_MIXER := [MIXER]
MVXV_EXT_MIXER := [$(shell cd $(TOP_DIR); git rev-parse --abbrev-ref HEAD 2> /dev/null | sed -e 's/\//_/g')$(shell cd -> /dev/null;) build.$(shell date +%Y%m%d%H%M)
MVXV_END := ]XVM

EXTRA_FLAGS = -D MIXER_VERSION="$(MVXV_START)$(MVXV_LIB_TYPE)$(MS_PLATFORM_ID)$(COMMITNUMBER_MIXER)$(COMP_MIXER)$(MVXV_EXT_MIXER)$(MVXV_END)"

BUILD_DATE := $(shell date +%F)
BUILD_TIME := $(shell date +%H:%M:%S)
USE_HC_HD ?= 2


include $(PROJECT_DIR)/configs/current.configs
include $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/toolchain.mk
ifeq ($(BENCH),yes)
ifeq ($(CHIP),i6b0)
PRODUCTID := 7
endif
endif
include $(TOP_DIR)/devCfg/product_$(CHIP).mk


ifeq ($(CHIP), i6)
USE_SED ?= 0
endif

ifeq ($(CHIP), i5)
SDK_INC ?=$(PROJECT_DIR)/release/$(PRODUCT)/include
else
SDK_INC ?=$(PROJECT_DIR)/release/include
endif

ifeq (static, $(BUILD_MODE))

ifeq ($(CHIP), i6)
SDK_EXLIBS ?=$(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/static/
SDK_LIBS ?=$(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/mi_libs/static/
else ifeq ($(CHIP), i6e)
SDK_EXLIBS ?=$(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/static/
SDK_LIBS ?=$(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/mi_libs/static/
else ifeq ($(CHIP), i6b0)
SDK_EXLIBS ?=$(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/static/
SDK_LIBS ?=$(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/mi_libs/static/
else
SDK_LIBS ?=$(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/lib/static
endif
BUILD_MESSAGE="===== make $(CHIP) mixer bin file with STATIC library ====="

else

ifeq ($(CHIP), i6e)
USE_SED ?= 1
SDK_EXLIBS ?=$(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/dynamic/
SDK_LIBS ?=$(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/mi_libs/dynamic/
else ifeq ($(CHIP), i5)
SDK_LIBS ?=$(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/lib/dynamic
else ifeq ($(CHIP), i6b0)
SDK_EXLIBS ?=$(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/dynamic/
SDK_LIBS ?=$(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/mi_libs/dynamic
else
SDK_LIBS ?=$(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/mi_libs/dynamic
endif
BUILD_MESSAGE ="===== make $(CHIP) mixer bin file with DYNAMIC library ====="

endif

LIVE_INC_PATH := $(shell pwd)/ext_src/live555/live/
LIVE_LIB_PATH := $(shell pwd)/ext_src/live555/live555/$(TOOLCHAIN)_$(TOOLCHAIN_VERSION)-lib


ifeq ($(CHIP), i5)
D_FLAGS += -D MAKE_TIME=\"$(BUILD_DATE)\ $(BUILD_TIME)\" -D TARGET_CHIP_I5="1" -D LOAD_MIXER_CFG="0" -D TARGET_BOARD=\"$(BOARD)\"
endif

ifeq ($(CHIP), i6)
ifeq ($(PRODUCT), ipc)
D_FLAGS += -D MAKE_TIME=\"$(BUILD_DATE)\ $(BUILD_TIME)\" -D TARGET_CHIP_I6="1" -D LOAD_MIXER_CFG="1" -D TARGET_BOARD=\"$(BOARD)\"
else
D_FLAGS += -D MAKE_TIME=\"$(BUILD_DATE)\ $(BUILD_TIME)\" -D TARGET_CHIP_I6="1" -D TARGET_OS_DUAL="1" -D LOAD_MIXER_CFG="1" -D TARGET_BOARD=\"$(BOARD)\"
endif
endif

ifeq ($(CHIP), i6e)
D_FLAGS += -D MAKE_TIME=\"$(BUILD_DATE)\ $(BUILD_TIME)\" -D TARGET_CHIP_I6E="1" -D LOAD_MIXER_CFG="1" -D TARGET_BOARD=\"$(BOARD)\"
endif

ifeq ($(CHIP), i6b0)
D_FLAGS += -D MAKE_TIME=\"$(BUILD_DATE)\ $(BUILD_TIME)\" -D TARGET_CHIP_I6B0="1" -D LOAD_MIXER_CFG="1" -D TARGET_BOARD=\"$(BOARD)\"
endif

ifneq ($(TOOLCHAIN), uclibc)

else
	export CXX_FLAGS := -DBUILD_UCLIBC_PROG
	export ONVIF_C_FLAGS := -DBUILD_UCLIBC_PROG
endif

C_FLAGS += -Wall -g
C_FLAGS += -fPIC -DPIC -DCONFIG_DEBUG_LEVEL=255
C_FLAGS += -DTEST_THREAD_ID_DETECTION
C_FLAGS += -g -O2 -funwind-tables
C_FLAGS += -ffunction-sections -fdata-sections
C_FLAGS += -fsigned-char
C_FLAGS += -Wwrite-strings
C_FLAGS += -Waddress
C_FLAGS += -DCONFIG_NO_VDF -DINCLUDE_SDK_SELF

CXX_FLAGS += -Wall -g -std=c++11
CXX_FLAGS += -fPIC -DPIC -DCONFIG_DEBUG_LEVEL=255
CXX_FLAGS += -DTEST_THREAD_ID_DETECTION
CXX_FLAGS += -g -O2 -funwind-tables
CXX_FLAGS += -ffunction-sections -fdata-sections
CXX_FLAGS += -fpermissive -fsigned-char
CXX_FLAGS += -Wwrite-strings
CXX_FLAGS += -Waddress
CXX_FLAGS += -DCONFIG_NO_VDF -DINCLUDE_SDK_SELF


ONVIF_C_FLAGS += -Wall -g
ONVIF_C_FLAGS += -fPIC -DPIC -DCONFIG_DEBUG_LEVEL=255
ONVIF_C_FLAGS += -DTEST_THREAD_ID_DETECTION
ONVIF_C_FLAGS += -g -O2 -funwind-tables
ONVIF_C_FLAGS += -ffunction-sections -fdata-sections
ONVIF_C_FLAGS += -fsigned-char
ONVIF_C_FLAGS += -DCONFIG_NO_VDF -DINCLUDE_SDK_SELF


LDFLAGS := -ldl -lpthread
##LDFLAGS += -Wl,--gc-sections,--print-gc-sections
##LDFLAGS += -lgcc_s -rdynamic
LDFLAGS += -Wall -g -mthumb -pipe -fPIC -rdynamic

ifeq ($(CHIP), i5)

C_INCLUDES += -I../../../../interface/include/isp_pretzel
C_INCLUDES += -I./inc/dla/I5

else ifeq ($(CHIP), i6e)

MOD_ROOT = $(TOP_DIR)/inc/dla/I6E/
C_INCLUDES += -I./inc/dla/I6E/
C_INCLUDES += -I./inc/dla/I6E/opecv/include/opencv4/
#SDK_EXLIBS +=  -L$(MOD_ROOT)/opecv/lib
#LDFLAGS +=  -lopencv_core -lopencv_imgproc -lopencv_imgcodecs
OPENCV_LIBS := $(MOD_ROOT)/opecv/lib_static/
OPENCV_3RD_LIBS := $(MOD_ROOT)/opecv/lib_static/opencv4/3rdparty/
LOCAL_LIBS :=  $(TOP_DIR)/lib/
#LDFLAGS += -L$(OPENCV_LIBS) -L$(OPENCV_3RD_LIBS)
LDFLAGS += -L$(SDK_DIR)/verify/mi_demo/alderaan/internal/ldc
endif

ifeq ($(CHIP),i6)
LDFLAGS += -L$(SDK_LIBS) -L$(SDK_EXLIBS) -L$(SDK_ROOT)/sdk/interface/src/isp -L$(SDK_ROOT)/sdk/interface/src/iqserver -L$(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/dynamic 
else ifeq ($(CHIP),i6b0)
LDFLAGS += -L$(SDK_LIBS) -L$(SDK_EXLIBS) -L$(SDK_ROOT)/sdk/interface/src/isp -L$(SDK_ROOT)/sdk/interface/src/iqserver
else
LDFLAGS += -L$(SDK_LIBS) -L$(SDK_EXLIBS) -L$(SDK_ROOT)/sdk/interface/src/isp -L$(SDK_ROOT)/sdk/interface/src/iqserver -L$(OPENCV_LIBS) -L$(OPENCV_3RD_LIBS) -L$(LOCAL_LIBS) 
endif

ifeq ($(CHIP), i5)
LDFLAGS += -lmi_sensor
LDFLAGS += -lmi_vdisp
endif

LDFLAGS += -lmi_vif
LDFLAGS += -lmi_vpe
LDFLAGS += -lmi_venc
LDFLAGS += -lmi_divp

#ifneq ($(CHIP), i6e)
LDFLAGS += -lmi_ai
LDFLAGS += -lmi_ao
#else
#LDFLAGS += $(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/mi_libs/static/libmi_ao.a
#LDFLAGS += $(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/mi_libs/static/libmi_ai.a
#endif

LDFLAGS += -lAPC_LINUX
LDFLAGS += -lAEC_LINUX
LDFLAGS += -lSRC_LINUX
LDFLAGS += -lg711
LDFLAGS += -lg726
LDFLAGS += -lmi_rgn
LDFLAGS += -lmi_sys
LDFLAGS += -lmi_vdf
LDFLAGS += -lmi_shadow

ifeq ($(CHIP), i5)

LDFLAGS += -lmi_isp_pretzel
LDFLAGS += -lmi_iqserver_pretzel
LDFLAGS += -lmi_ive
LDFLAGS += -lOD_LINUX
LDFLAGS += -lcnrt
LDFLAGS += -lcus3a

else ifeq ($(CHIP), i6)

LDFLAGS += -lmi_iqserver
LDFLAGS += -lmi_isp
LDFLAGS += -lmi_sensor
LDFLAGS += -lcus3a -lispalgo


else ifeq ($(CHIP), i6e)

ifeq ($(USE_SED),1)
LDFLAGS += -lmi_sed
D_FLAGS += -D MIXER_SED_ENABLE="1"
endif

LDFLAGS += -lmi_iqserver
LDFLAGS += -lmi_isp
LDFLAGS += -lmi_sensor
LDFLAGS += -lmi_ipu
LDFLAGS += -lcus3a -lispalgo
LDFLAGS += -lmi_ldc -leptz
LDFLAGS += -lmi_vdisp

LDFLAGS += -lopencv_imgcodecs
LDFLAGS += -lopencv_imgproc
LDFLAGS += -lopencv_core
LDFLAGS += -littnotify 
LDFLAGS += -llibjasper
LDFLAGS += -llibjpeg-turbo
LDFLAGS += -llibpng
LDFLAGS += -llibtiff
LDFLAGS += -llibwebp
LDFLAGS += -ltegra_hal
LDFLAGS += -lzlib

else ifeq ($(CHIP), i6b0)

LDFLAGS += -lmi_iqserver
LDFLAGS += -lmi_isp
LDFLAGS += -lmi_sensor
LDFLAGS += -lcus3a -lispalgo

endif

LDFLAGS += -lOD_LINUX
LDFLAGS += -lMD_LINUX
LDFLAGS += -lVG_LINUX
LDFLAGS += -lmi_ive
LDFLAGS += -lmi_common

C_INCLUDES += \
	       -I./inc \
	       -I./inc/mid \
	       -I./inc/rtsp \
	       -I./inc/common \
	       -I$(SDK_INC) \
	       -I./inc/json \
	       -I./inc/record \
	       -I./inc/record/fs \
	       -I./inc/record/mmc \
		   -I./inc/ldc \
		   -I./inc/uvc



ifeq ($(USE_HC_HD), 0)

ifeq ($(CHIP), i6)
LDFLAGS += $(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/lib/static/libHC_XS_ARM_I6.a
else ifeq ($(CHIP), i6e)
LDFLAGS += $(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/static/libHC_XS_ARM.a
else ifeq ($(CHIP), i6b0)
LDFLAGS += $(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/static/libHC_XS_ARM.a
else
LDFLAGS += ./IE/video/HCHD/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/lib/static/libHC_XS_ARM_I6.a
endif
ifneq ($(CHIP), i6e)
C_INCLUDES += -I./IE/video/MTE/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include  \
              -I./IE/video/FDFR/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include \
              -I./IE/video/VG/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include   \
              -I./IE/video/HCHD/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include
endif

else ifeq ($(USE_HC_HD), 1)
ifeq ($(CHIP), i6)
LDFLAGS += $(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/lib/static/libfaster_HD_ARM_I6.a
else ifeq ($(CHIP), i6e)
LDFLAGS += $(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/static/libHC_XS_ARM.a
else ifeq ($(CHIP), i6b0)
LDFLAGS += $(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/static/libfaster_HD_ARM.a
else
LDFLAGS += ./IE/video/HCHD/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/lib/static/libfaster_HD_ARM_I6.a
endif
ifneq ($(CHIP), i6e)
C_INCLUDES += -I./IE/video/MTE/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include  \
              -I./IE/video/FDFR/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include \
              -I./IE/video/VG/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include   \
              -I./IE/video/HCHD/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include
endif

else ifeq ($(USE_HC_HD), 2)

ifeq ($(CHIP), i5)
LDFLAGS += ./IE/video/HCHD/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/lib/static/libFD_HC_XS_ARM_I6.a
MACROFLAGS = -DFD_HC_LIB
else ifeq ($(CHIP), i6b0)
LDFLAGS += $(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/static/libFD_HC_XS_ARM.a
MACROFLAGS = -DFD_HC_LIB
else ifeq ($(CHIP), i6e)
LDFLAGS += $(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/static/libFD_HC_XS_ARM.a
MACROFLAGS = -DFD_HC_LIB
else
LDFLAGS += $(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/static/libHC_XS_ARM.a
MACROFLAGS = -DFD_HC_LIB
endif
ifneq ($(CHIP), i6e)
C_INCLUDES += -I./IE/video/MTE/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include  \
              -I./IE/video/FDFR/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include \
              -I./IE/video/VG/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include   \
              -I./IE/video/HCHD/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include
endif

else ifeq ($(USE_HC_HD), 3)

ifeq ($(CHIP), i6)
LDFLAGS += $(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/lib/static/libFD_ARM_I6.a
else ifeq ($(CHIP), i6e)
LDFLAGS += $(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/static/libHC_XS_ARM.a
else ifeq ($(CHIP), i6b0)
LDFLAGS += $(PROJECT_DIR)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/static/libFD_ARM.a
else
LDFLAGS += ./IE/video/HCHD/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/lib/static/libFD_ARM_I6.a
endif
ifneq ($(CHIP), i6e)
C_INCLUDES += -I./IE/video/MTE/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include  \
              -I./IE/video/FDFR/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include \
              -I./IE/video/VG/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include   \
              -I./IE/video/HCHD/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include
endif
endif


ifeq ($(CHIP), i6)
C_INCLUDES += \
           -I$(SDK_DIR)/interface/include/ai \
	       -I$(SDK_DIR)/interface/include/aio \
	       -I$(SDK_DIR)/interface/include/ao \
	       -I$(SDK_DIR)/interface/include/common \
	       -I$(SDK_DIR)/interface/include/md \
	       -I$(SDK_DIR)/interface/include/od \
	       -I$(SDK_DIR)/interface/include/vdf \
	       -I$(SDK_DIR)/interface/include/isp/macaron \
	       -I$(SDK_DIR)/interface/include/isp_cus3a/macaron
else ifeq ($(CHIP), i6e)
C_INCLUDES += \
           -I$(SDK_DIR)/interface/include/ai \
	       -I$(SDK_DIR)/interface/include/aio \
	       -I$(SDK_DIR)/interface/include/ao \
	       -I$(SDK_DIR)/interface/include/common \
	       -I$(SDK_DIR)/interface/include/md \
	       -I$(SDK_DIR)/interface/include/od \
	       -I$(SDK_DIR)/interface/include/vdf \
	       -I$(SDK_DIR)/interface/include/isp/pudding \
	       -I$(SDK_DIR)/interface/include/isp_cus3a/pudding	\
		   -I$(SDK_DIR)/verify/mi_demo/alderaan/internal/ldc
else ifeq ($(CHIP), i6b0)
C_INCLUDES += \
           -I$(SDK_DIR)/interface/include/ai \
	       -I$(SDK_DIR)/interface/include/aio \
	       -I$(SDK_DIR)/interface/include/ao \
	       -I$(SDK_DIR)/interface/include/common \
	       -I$(SDK_DIR)/interface/include/md \
	       -I$(SDK_DIR)/interface/include/od \
	       -I$(SDK_DIR)/interface/include/vdf \
	       -I$(SDK_DIR)/interface/include/isp/ispahan \
	       -I$(SDK_DIR)/interface/include/isp_cus3a/ispahan
endif

C_INCLUDES += -I$(LIVE_INC_PATH)/UsageEnvironment/include \
              -I$(LIVE_INC_PATH)/groupsock/include \
              -I$(LIVE_INC_PATH)/liveMedia/include \
              -I$(LIVE_INC_PATH)/BasicUsageEnvironment/include \
              -I$(LIVE_INC_PATH)/UsageEnvironment \
              -I$(LIVE_INC_PATH)/groupsock \
              -I$(LIVE_INC_PATH)/liveMedia  \
              -I$(LIVE_INC_PATH)/BasicUsageEnvironment

#C_INCLUDES += -I../../../../../project/release/ipc/include \
              -I./IE/video/MTE/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include  \
              -I./IE/video/FDFR/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include \
              -I./IE/video/VG/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include   \
              -I./IE/video/HCHD/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/include

LIVE555_LIBS := $(LIVE_LIB_PATH)/libliveMedia.a \
                $(LIVE_LIB_PATH)/libgroupsock.a \
                $(LIVE_LIB_PATH)/libBasicUsageEnvironment.a \
                $(LIVE_LIB_PATH)/libUsageEnvironment.a

ifeq ($(CHIP), i5)
IE_LIBS := ./IE/video/FDFR/$(CHIP)/$(TOOLCHAIN)-$(TOOLCHAIN_VERSION)/lib/libFDFR_LINUX.a
LIVE555_LIBS += $(IE_LIBS)
endif

PROGS = mixer

MIXER_MID_JSON_C_SRCS=$(wildcard ./src/mid/json/*.c)
MIXER_MID_JSON_C_SRCS_NO_DIR=$(notdir $(MIXER_MID_JSON_C_SRCS))
CPP_OBJECTS+=$(patsubst %.c, %.o,  $(MIXER_MID_JSON_C_SRCS_NO_DIR))

MIXER_MID_COMMON_STREAMER_CPP_SRCS=$(wildcard ./src/mid/common/streamer/*.cpp)
MIXER_MID_COMMON_STREAMER_CPP_SRCS_NO_DIR=$(notdir $(MIXER_MID_COMMON_STREAMER_CPP_SRCS))
CPP_OBJECTS+=$(patsubst %.cpp, %.o,  $(MIXER_MID_COMMON_STREAMER_CPP_SRCS_NO_DIR))


MIXER_MID_COMMON_CPP_SRCS=$(wildcard ./src/mid/common/*.cpp)
MIXER_MID_COMMON_CPP_SRCS_NO_DIR=$(notdir $(MIXER_MID_COMMON_CPP_SRCS))
CPP_OBJECTS+=$(patsubst %.cpp, %.o,  $(MIXER_MID_COMMON_CPP_SRCS_NO_DIR))

MIXER_MID_RTSP_CPP_SRCS=$(wildcard ./src/mid/rtsp/*.cpp)
MIXER_MID_RTSP_CPP_SRCS_NO_DIR=$(notdir $(MIXER_MID_RTSP_CPP_SRCS))
CPP_OBJECTS+=$(patsubst %.cpp, %.o,  $(MIXER_MID_RTSP_CPP_SRCS_NO_DIR))

ifeq ($(CHIP), i5)
MIXER_MID_DLA_CPP_SRCS=$(wildcard ./src/mid/dla/I5/*.cpp)
MIXER_MID_DLA_CPP_SRCS_NO_DIR=$(notdir $(MIXER_MID_DLA_CPP_SRCS))
CPP_OBJECTS+=$(patsubst %.cpp, %.o,  $(MIXER_MID_DLA_CPP_SRCS_NO_DIR))

MIXER_MID_DLA_C_SRCS=$(wildcard ./src/mid/dla/I5/*.c)
MIXER_MID_DLA_C_SRCS_NO_DIR=$(notdir $(MIXER_MID_DLA_C_SRCS))
CPP_OBJECTS+=$(patsubst %.c, %.o,  $(MIXER_MID_DLA_C_SRCS_NO_DIR))

else ifeq ($(CHIP), i6e)

MIXER_MID_DLA_CPP_SRCS=$(wildcard ./src/mid/dla/I6e/*.cpp)
MIXER_MID_DLA_CPP_SRCS_NO_DIR=$(notdir $(MIXER_MID_DLA_CPP_SRCS))
CPP_OBJECTS+=$(patsubst %.cpp, %.o,  $(MIXER_MID_DLA_CPP_SRCS_NO_DIR))

endif

MIXER_MID_CPP_SRCS=$(wildcard ./src/mid/*.cpp)
MIXER_MID_CPP_SRCS_NO_DIR=$(notdir $(MIXER_MID_CPP_SRCS))
CPP_OBJECTS+=$(patsubst %.cpp, %.o,  $(MIXER_MID_CPP_SRCS_NO_DIR))

MIXER_MODULE_CPP_SRCS=$(wildcard ./src/*.cpp)
MIXER_MODULE_CPP_SRCS_NO_DIR=$(notdir $(MIXER_MODULE_CPP_SRCS))
CPP_OBJECTS+=$(patsubst %.cpp, %.o,  $(MIXER_MODULE_CPP_SRCS_NO_DIR))

MIXER_RECORD_CPP_SRCS=$(wildcard ./src/record/*.cpp)
#MIXER_RECORD_CPP_SRCS_NO_DIR=$(notdir $(MIXER_RECORD_CPP_SRCS))
#CPP_OBJECTS+=$(patsubst %.cpp, %.o,  $(MIXER_RECORD_CPP_SRCS_NO_DIR))

MIXER_RECORD_CPP_SRCS+=$(wildcard ./src/record/fs/*.cpp)
#MIXER_RECORD_CPP_SRCS_NO_DIR+=$(notdir $(MIXER_RECORD_CPP_SRCS))

MIXER_RECORD_CPP_SRCS+=$(wildcard ./src/record/mmc/*.cpp)
MIXER_RECORD_CPP_SRCS_NO_DIR+=$(notdir $(MIXER_RECORD_CPP_SRCS))

CPP_OBJECTS+=$(patsubst %.cpp, %.o,  $(MIXER_RECORD_CPP_SRCS_NO_DIR))

MIXER_UVC_CPP_SRCS=$(wildcard ./src/uvc/*.cpp)
MIXER_UVC_CPP_SRCS_NO_DIR=$(notdir $(MIXER_UVC_CPP_SRCS))
CPP_OBJECTS+=$(patsubst %.cpp, %.o,  $(MIXER_UVC_CPP_SRCS_NO_DIR))


OBJDIR ?= $(shell pwd)/obj
BINDIR ?= $(shell pwd)/bin
INSTALLDIR ?= $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/mixer
INCDIR ?= $(shell pwd)/inc

OBJPROG = $(addprefix $(OBJDIR)/, $(PROGS))

.PHONY: clean prepare PROGS help

-all: prepare $(OBJPROG)
	@echo -e "\033[47;34m============================================ All  attr ==========================================\033[0m"
	@echo $(BUILD_MESSAGE) 
	@echo -e "\033[40;33m===hardware version is====$(CHIP)==$(HARDWARE_VERSION)======\033[0m"
	@echo -e "\033[40;33m=====your chice is : USE_HC_HD=$(USE_HC_HD)=================\033[0m"
	@echo -e "\033[40;33m=====mixer default cfg  is: ===PRODUCTID=$(PRODUCTID)=======\033[0m"
	@echo -e "\033[40;33m=====mixer open LDC is:===OPEN_LDC_FROM_PORT=$(OPEN_LDC_FROM_PORT)=======\033[0m"
	@echo -e "\033[47;34m++++++++++++++++++++++++++++++++++++++++++++ main stream+++++++++++++++++++++++++++++++++++++++++\033[0m"
	@echo -e "\033[40;33m===========M_VENC_BIND_TYPE:$(M_VENC_BIND_TYPE)       M_STREAM_FROM_PORT:$(M_STREAM_FROM_PORT)===\033[0m" 
	@echo -e "\033[40;33m=======M_STREAM_MAX_BITRATE:$(M_STREAM_MAX_BITRATE) M_STREAM_MIN_BITRATE:$(M_STREAM_MIN_BITRATE)=\033[0m" 
	@echo -e "\033[40;33m=================== M_STREAM_CUR_BITRATE:$(M_STREAM_CUR_BITRATE) ================================\033[0m" 
	@echo -e "\033[40;33m=======MAIN_STREAM_W:$(MAIN_STREAM_W)	      MAIN_STREAM_H:$(MAIN_STREAM_H)=\033[0m" 	 
	@echo -e "\033[47;34m******************************************** second stream attr *********************************\033[0m"
	@echo -e "\033[40;33m===========S_VENC_BIND_TYPE:$(S_VENC_BIND_TYPE)       S_STREAM_FROM_PORT:$(S_STREAM_FROM_PORT)===\033[0m" 
	@echo -e "\033[40;33m=======S_STREAM_MAX_BITRATE:$(S_STREAM_MAX_BITRATE) S_STREAM_MIN_BITRATE:$(S_STREAM_MIN_BITRATE)=\033[0m" 
	@echo -e "\033[40;33m=================== S_STREAM_CUR_BITRATE:$(S_STREAM_CUR_BITRATE) ================================\033[0m" 
	@echo -e "\033[40;33m============SECOND_STREAM_W:$(SECOND_STREAM_W)           SECOND_STREAM_H:$(SECOND_STREAM_H)======\033[0m"    
	@echo -e "\033[47;34m++++++++++++++++++++++++++++++++++++++++++++++ jpeg stream attr++++++++++++++++++++++++++++++++++\033[0m"	
	@echo -e "\033[40;33m===========J_VENC_BIND_TYPE:$(J_VENC_BIND_TYPE)       J_STREAM_FROM_PORT:$(J_STREAM_FROM_PORT)===\033[0m" 
	@echo -e "\033[40;33m=======J_STREAM_MAX_BITRATE:$(J_STREAM_MAX_BITRATE) J_STREAM_MIN_BITRATE:$(J_STREAM_MIN_BITRATE)=\033[0m" 
	@echo -e "\033[40;33m===================J_STREAM_CUR_BITRATE:$(J_STREAM_CUR_BITRATE) ================================\033[0m" 
	@echo -e "\033[40;33m==========================JPEG_STREAM_W:$(JPEG_STREAM_W)         JPEG_STREAM_H:$(JPEG_STREAM_H)=\033[0m" 
	@echo -e "\033[47;34m--------------------------------------------- third stream attr ---------------------------------\033[0m" 
	@echo -e "\033[40;33m===========T_VENC_BIND_TYPE:$(T_VENC_BIND_TYPE)       T_STREAM_FROM_PORT:$(T_STREAM_FROM_PORT)===\033[0m" 
	@echo -e "\033[40;33m=======T_STREAM_MAX_BITRATE:$(T_STREAM_MAX_BITRATE) T_STREAM_MIN_BITRATE:$(T_STREAM_MIN_BITRATE)=\033[0m" 
	@echo -e "\033[40;33m=================== T_STREAM_CUR_BITRATE:$(T_STREAM_CUR_BITRATE) ================================\033[0m" 
	@echo -e "\033[40;33m==============THIRD_STREAM_W:$(THIRD_STREAM_W)            THIRD_STREAM_H:$(THIRD_STREAM_H)=\033[0m" 
prepare:
	@echo '  CC' $(CC)
	@echo '  TOP_DIR' $(TOP_DIR)

help:
	@echo -e "\033[40;33m make USE_HC_HD=0 => 0: HC 1:HD 2: FD&HC 3:FD \033[0m"
	@echo -e "\033[40;33m make PRODUCTID=value =>main stream resultion: value=0: [1440*1440] 1:[1920*1080] 2:[2048*1536] 3:[2304*1296] 4:[2560*1440]\033[0m"
	@echo -e "\033[40;33m 5:[2560*1536] 6:[2688x1536] 7:[2952*1944] 8:[3072*2160] 9:[3840,2160]==>NOTE:Get Info by cat ./devCfg/product_$(CHIP).mk\033[0m"
	@echo -e "\033[44;33m If you have change value of USE_HC_HD\PRODUCTID,please make clean;make PRODUCTID=value\033[0m"
	@echo -e "\033[44;33m If you want to set value (is not PRODUCTID param ),please make clean;make param=value\033[0m"
	@echo -e "\033[44;33m Eg: make clean; make SECOND_STREAM_W=1280 SECOND_STREAM_H=720 \033[0m"
	@echo -e "\033[44;33m chice hardware version: make clean; make HARDWARE_VERSION=U02 or U01 \033[0m"
	$(warning "make command build mixer bin file with DYNAMIC library")
	$(warning "make BUILD_MODE=static command build mixer bin file with STATIC library")
clean:
	@rm -Rf $(OBJDIR)
	@rm -Rf $(BINDIR)

install:
	@mkdir -p ${INSTALLDIR}
	@$(STRIP) --strip-unneeded ${BINDIR}/mixer
	rm -rf ${INSTALLDIR}/mixer
	cp -vf ${BINDIR}/mixer ${INSTALLDIR}

$(OBJPROG):	$(addprefix $(OBJDIR)/, $(OBJECTS)) $(addprefix $(OBJDIR)/, $(CPP_OBJECTS))
	@echo "  BIN   $@"
	$(CXX) -o $@ $(addprefix $(OBJDIR)/, $(OBJECTS)) $(addprefix $(OBJDIR)/, $(CPP_OBJECTS)) $(LIVE555_LIBS)  $(LDFLAGS)
	@echo ""
	rm -rf ${BINDIR}
	mkdir ${BINDIR}
	cp -f ${OBJDIR}/$(PROGS) ${BINDIR}
	cp -f xiaopingguo.wav ${BINDIR}
	#@$(STRIP) --strip-unneeded ${BINDIR}/mixer

$(OBJDIR)/%.c.o : src/%.c 
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CC) $(D_FLAGS) $(C_FLAGS) $(C_INCLUDES) -c $< -o $@

$(OBJDIR)/%.o : src/%.cpp
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CXX) $(D_FLAGS) $(EXTRA_FLAGS) $(CXX_FLAGS) $(C_INCLUDES) -c $< -o $@

	
$(OBJDIR)/%.o : src/mid/%.cpp
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CXX) $(D_FLAGS) $(CXX_FLAGS) $(C_INCLUDES) -c $< -o $@

	
$(OBJDIR)/%.o : src/mid/%.cpp
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CXX) $(D_FLAGS) $(CXX_FLAGS) $(C_INCLUDES) -c $< -o $@ $(MACROFLAGS)


$(OBJDIR)/%.o : src/mid/common/%.cpp
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CXX) $(D_FLAGS) $(CXX_FLAGS) $(C_INCLUDES) -c $< -o $@


$(OBJDIR)/%.o : src/mid/common/streamer/%.cpp
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CXX) $(CXX_FLAGS) $(C_INCLUDES) -c $< -o $@


$(OBJDIR)/%.o : src/mid/rtsp/%.cpp
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CXX) $(D_FLAGS) $(CXX_FLAGS) $(C_INCLUDES) -c $< -o $@

$(OBJDIR)/%.o : src/record/%.cpp
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CXX) $(D_FLAGS) $(CXX_FLAGS) $(C_INCLUDES) -c $< -o $@

$(OBJDIR)/%.o : src/record/fs/%.cpp
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CXX) $(D_FLAGS) $(CXX_FLAGS) $(C_INCLUDES) -c $< -o $@

$(OBJDIR)/%.o : src/record/mmc/%.cpp
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CXX) $(D_FLAGS) $(CXX_FLAGS) $(C_INCLUDES) -c $< -o $@

$(OBJDIR)/%.o : src/uvc/%.cpp
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CXX) $(D_FLAGS) $(CXX_FLAGS) $(C_INCLUDES) -c $< -o $@

$(OBJDIR)/%.o : src/mid/json/%.c 
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CC) $(D_FLAGS) $(C_FLAGS) $(C_INCLUDES) -c $< -o $@

ifeq ($(CHIP), i5)
$(OBJDIR)/%.o : src/mid/dla/I5/%.cpp
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CXX) $(D_FLAGS) $(CXX_FLAGS) $(C_INCLUDES) -c $< -o $@

$(OBJDIR)/%.o : src/mid/dla/I5/%.c
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CC) $(D_FLAGS) $(C_FLAGS) $(C_INCLUDES) -c $< -o $@

else ifeq ($(CHIP), i6e)

$(OBJDIR)/%.o : src/mid/dla/I6e/%.cpp
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CXX) $(D_FLAGS) $(CXX_FLAGS) $(C_INCLUDES) -c $< -o $@

$(OBJDIR)/%.o : src/mid/dla/I6e/%.c
	@mkdir -p obj
	@echo "  CC    $<"
	@$(CC) $(D_FLAGS) $(C_FLAGS) $(C_INCLUDES) -c $< -o $@
endif
