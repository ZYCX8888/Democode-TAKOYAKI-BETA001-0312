
############################################################ source files.
SUBDIRS :=./src ./src/base/common/src ./src/base/rtsp/src
SUBDIRS += ./src/base/live555/BasicUsageEnvironment
SUBDIRS += ./src/base/live555/groupsock
SUBDIRS += ./src/base/live555/liveMedia
SUBDIRS += ./src/base/live555/UsageEnvironment
SUBDIRS += ./src/base/live555/mediaServer

INC     +=./inc ./src/base/common/inc
INC     += $(PROJ_ROOT)/release/include

INC += ./src/base/live555/BasicUsageEnvironment/include
INC += ./src/base/live555/groupsock/include
INC += ./src/base/live555/liveMedia/include
INC += ./src/base/live555/UsageEnvironment/include
INC += ./src/base/live555/mediaServer/include
INC += $(PROJ_ROOT)/../sdk/misc/fbdev/fbdev_video

OUTPUT_DIR := $(DB_OUT_PATH)/obj/

LOCAL_CXXFLAGS += -DSOCKLEN_T=socklen_t -DNO_SSTREAM=1 -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 -DBSD=1 -DLOCALE_NOT_USED -DSO_REUSEPORT -DALLOW_SERVER_PORT_REUSE -DALLOW_RTSP_SERVER_PORT_REUSE

############################################################ depnedent header files.
include $(DB_BUILD_TOP)/compile.mk
all: gen_obj
clean: clean_files
