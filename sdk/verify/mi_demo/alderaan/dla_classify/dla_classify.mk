INC  += $(DB_BUILD_TOP)/../common/opencv/include/opencv4
ST_DEP := common  iniparser
LIBS += -lmi_ipu -lcam_fs_wrapper

#static library path
LIBS += -L$(DB_BUILD_TOP)/../common/opencv/static_lib_${TOOLCHAIN_VERSION} -L$(DB_BUILD_TOP)/../common/opencv/static_lib_${TOOLCHAIN_VERSION}/opencv4/3rdparty
#shared library path
#LIBS += -L$(DB_BUILD_TOP)/../common/opencv/shared_lib_${TOOLCHAIN_VERSION}
#shared opencv library
#LIBS += -lopencv_core -lopencv_imgproc -lopencv_imgcodecs
#static library
LIBS += -lopencv_imgcodecs -lopencv_imgproc -lopencv_core -littnotify -llibjasper  -llibjpeg-turbo  -llibpng -llibtiff  -llibwebp -ltegra_hal -lzlib

