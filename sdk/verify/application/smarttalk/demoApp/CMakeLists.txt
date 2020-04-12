cmake_minimum_required(VERSION 2.8.0)

## project 路径,配置自己的project路径
set(PROJECT_PTH /home/aaron.feng/Code/P2/Release_SDK/sdk/verify/application/smarttalk/demoApp/../../../../../project)

### SET CROSS-COMPILE ###
set(CMAKE_SYSTEM_NAME Linux)
## set(CMAKE_BUILD_TYPE Release)
set(CMAKE_BUILD_TYPE DEBUG)

set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)
set(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_ROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
set(CROSS_COMPILER_VERSION 8.2.1)
###

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -mfloat-abi=hard -mfpu=neon -Wno-write-strings")

#add_subdirectory(modules)
add_subdirectory(modules/src/voice)
add_subdirectory(modules/src/common)
add_subdirectory(modules/src/font)
add_subdirectory(modules/src/ao)
add_subdirectory(modules/src/disp)
add_subdirectory(modules/src/venc)
add_subdirectory(modules/src/vdec)
add_subdirectory(modules/src/socket)
add_subdirectory(modules/src/system)
add_subdirectory(modules/src/stream)
add_subdirectory(modules/src/xmlprase)
add_subdirectory(modules/src/gpio)
add_subdirectory(modules/src/usb)

include_directories(${PROJECT_PTH}/release/include)
include_directories(${PROJECT_PTH}/kbuild/4.9.84/i2m/include/uapi/mstar)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/modules/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/modules/include/common)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/CSpotter/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/awn/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/guilib/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/omron/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/xml/include/libxml2)

link_directories(${PROJECT_PTH}/release/nvr/i2m/common/glibc/8.2.1/mi_libs/dynamic)
link_directories(${PROJECT_PTH}/release/nvr/i2m/common/glibc/8.2.1/ex_libs/dynamic)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/CSpotter/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/awn/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/guilib/dynamic)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/omron/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/xml/lib)

set(demoApp_libs
	st_usb st_gpio st_common st_ao st_voice st_disp st_font st_venc st_stream st_socket st_vdec st_system st_xmlprase
	mi_sys mi_disp mi_panel mi_ao mi_gfx mi_divp mi_venc mi_vdec mi_common g711 g726 APC_LINUX SRC_LINUX mi_ai AEC_LINUX
)
#	UsageEnvironment BasicUsageEnvironment groupsock liveMedia UsageEnvironment
set(dynamic_libs
	dl m rt z CSpotter 
)
#set (gui_libs
#	chipmunk freetype jpeg mgeff mgncs mgncs4touch mgplus mgutils minigui_procs png ts z
#)
#-lmgncs -lmgutils -lmgeff -lmgplus -lminigui_procs -lfreetype -ljpeg -lpng  -lmgncs4touch  -lchipmunk
set (gui_static_libs
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/guilib/static/${CROSS_COMPILER_VERSION}/libmgncs.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/guilib/static/${CROSS_COMPILER_VERSION}/libmgutils.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/guilib/static/${CROSS_COMPILER_VERSION}/libmgeff.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/guilib/static/${CROSS_COMPILER_VERSION}/libmgplus.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/guilib/static/${CROSS_COMPILER_VERSION}/libminigui_ths.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/guilib/static/${CROSS_COMPILER_VERSION}/libfreetype.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/guilib/static/${CROSS_COMPILER_VERSION}/libjpeg.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/guilib/static/${CROSS_COMPILER_VERSION}/libpng.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/guilib/static/${CROSS_COMPILER_VERSION}/libmgncs4touch.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/guilib/static/${CROSS_COMPILER_VERSION}/libchipmunk.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/guilib/static/${CROSS_COMPILER_VERSION}/libts.a
)
set (omron_static_libs
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/omron/lib/eOmcvBd.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/omron/lib/eOkaoFr.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/omron/lib/eOkaoPt.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/omron/lib/eOkaoDt.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/omron/lib/eOkaoCo.a
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/omron/lib/eOkao.a
)
set (xml_static_libs
	${CMAKE_CURRENT_SOURCE_DIR}/../3partyLib/xml/lib/libxml2.a
)
set (v4l2_libs
	st_v4l2
)

add_definitions(-std=gnu++11)
add_executable(mginit demoApp.cpp main_ui.cpp cmdqueue.cpp omron.cpp)
target_link_libraries(mginit pthread ${v4l2_libs} ${demoApp_libs} ${dynamic_libs} ${gui_static_libs} ${omron_static_libs} ${xml_static_libs})
