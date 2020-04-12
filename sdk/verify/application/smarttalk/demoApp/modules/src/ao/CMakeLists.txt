cmake_minimum_required(VERSION 2.8.0)

# 引用的头文件的路径
include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/
	include_directories(${PROJECT_PTH}/release/include)
	${CMAKE_CURRENT_SOURCE_DIR}/../../../modules/include/common
)

# 设置动态库的路径
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../../../lib)

# 源文件
aux_source_directory(./ LIB_SRCS)

string(REGEX REPLACE ".*/(.*)" "\\1" CURRENT_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}) 
add_library(st_${CURRENT_FOLDER} STATIC ${LIB_SRCS})