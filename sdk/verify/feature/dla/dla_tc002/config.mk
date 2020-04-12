include clear-config.mk
#build cpp execute
COMPILE_TARGET:=cpp_bin
SRCS_CPP:=dla_tc002.cpp
#end
CFLAGS:=-std=c++11
LIBS:= cnrt
include add-config.mk
