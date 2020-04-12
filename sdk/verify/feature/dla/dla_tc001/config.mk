include clear-config.mk
#build cpp execute
COMPILE_TARGET:=cpp_bin
#end
SRCS_CPP:=dla_tc001.cpp
LIBS:= cnrt
include add-config.mk
