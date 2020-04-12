LINK_TYPE ?= static

INC  += $(PROJ_ROOT)/release/include
INC  += $(PROJ_ROOT)/kbuild/$(KERNEL_VERSION)/include/uapi/mstar
INC  += $(PROJ_ROOT)/kbuild/$(KERNEL_VERSION)/drivers/mstar/include

INC  += $(DB_BUILD_TOP)/naboo/internal/common
LIBS += -lrt -lpthread -lm
LIBS += -lmi_sys -lmi_common -lAPC_LINUX -lSRC_LINUX -lAEC_LINUX
#
LIBS += -L$(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/lib/$(LINK_TYPE)
LIBS += -L./lib
