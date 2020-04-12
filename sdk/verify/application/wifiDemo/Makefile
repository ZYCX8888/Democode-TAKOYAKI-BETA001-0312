TARGET = testWifi

#赋值对应的路径,此路径需要和本地sdk路径一致
#PROJECT_PATH=/home/beal.wu/i2m_i6_master/project
PROJECT_PATH=/home/beal.wu/Democode-TAKOYAKI-BETA001-0312/project

COMPILE_ROOT := $(shell pwd)
CC = arm-linux-gnueabihf-g++
STRIP = arm-linux-gnueabihf-strip

CFLAGS = -I$(PROJECT_PATH)/release/include
LDFLAGS = -L$(PROJECT_PATH)/release/nvr/i2m/common/glibc/8.2.1/mi_libs/dynamic -L$(PROJECT_PATH)/release/nvr/i2m/common/glibc/8.2.1/ex_libs/dynamic
CONFIG_PREFIX =.

LOCAL_CFLAGS =
LOCAL_LDFLAGS = -lpthread -ldl -lmi_common -lmi_sys -lmi_wlan -lcjson -Wl,--hash-style=gnu

CPPSRCS += ${wildcard *.cpp}
CPPOBJS = $(patsubst %.cpp, %.cpp.o, $(CPPSRCS))

CSRCS += ${wildcard *.c}
COBJS = $(patsubst %.c,%.c.o,$(CSRCS))

all:$(TARGET) install

$(TARGET):$(CPPOBJS) $(COBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(LOCAL_CFLAGS) $(LOCAL_LDFLAGS) -o $@ $^

$(CPPOBJS):%.cpp.o:%.cpp
	$(CC) $(CFLAGS) $(LDFLAGS) $(LOCAL_CFLAGS) $(LOCAL_LDFLAGS) -c $^ -o $@

$(COBJS):%.c.o:%.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(LOCAL_CFLAGS) $(LOCAL_LDFLAGS) -c $^ -o $@

install: 
	-mkdir -p $(CONFIG_PREFIX)/bin
	-mv $(TARGET) $(CONFIG_PREFIX)/bin/
	
clean:
	-rm -rf $(CPPOBJS) $(CONFIG_PREFIX)/bin/
	-rm -rf *.o
.PHONY:all clean $(TARGET) install
