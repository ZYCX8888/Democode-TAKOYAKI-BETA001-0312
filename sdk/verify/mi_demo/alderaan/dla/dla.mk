INC  += $(DB_BUILD_TOP)/../common/live555/UsageEnvironment/include
INC  += $(DB_BUILD_TOP)/../common/live555/groupsock/include
INC  += $(DB_BUILD_TOP)/../common/live555/liveMedia/include
INC  += $(DB_BUILD_TOP)/../common/live555/BasicUsageEnvironment/include
INC  += $(DB_BUILD_TOP)/../common/live555/mediaServer/include
INC  += $(DB_BUILD_TOP)/../common/opencv/include/opencv4

ST_DEP := common vpe venc vif rgn onvif live555 tem iniparser

LIBS += -lmi_vif -lmi_vpe -lmi_venc -lmi_rgn -lmi_divp -lmi_isp -lmi_iqserver -lmi_ipu -lcam_fs_wrapper

#static library path
LIBS += -L$(DB_BUILD_TOP)/../common/opencv/static_lib_${TOOLCHAIN_VERSION} -L$(DB_BUILD_TOP)/../common/opencv/static_lib_${TOOLCHAIN_VERSION}/opencv4/3rdparty
#shared library path
#LIBS += -L$(DB_BUILD_TOP)/../common/opencv/shared_lib_${TOOLCHAIN_VERSION}
#shared opencv library
#LIBS += -lopencv_core -lopencv_imgproc -lopencv_imgcodecs
#static library
LIBS += -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -littnotify -llibjasper  -llibjpeg-turbo  -llibpng -llibtiff  -llibwebp -ltegra_hal -lzlib -lcus3a -lispalgo

INC += $(DB_BUILD_TOP)/stream/
SUBDIRS += $(DB_BUILD_TOP)/stream/
