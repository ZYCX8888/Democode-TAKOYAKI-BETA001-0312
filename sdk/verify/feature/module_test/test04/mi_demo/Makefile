# All rights reserved.
#
# Makefile used for building application.
#
################################################################################
.PHONY : all clean lib app

include $(DB_ALKAID_PROJ)
export PROJ_ROOT CHIP PRODUCT BOARD TOOLCHAIN TOOLCHAIN_VERSION KERNEL_VERSION CUSTOMER_OPTIONS MOD_PREFIX INTERFACE_ENABLED INTERFACE_DISABLED MHAL_ENABLED MHAL_DISABLED

INTERFACE_MAKEFILE=./interface/

all: app

app: lib
ifneq ($(DB_OUT_PATH), )
ifeq ($(CHIP), k6)
	$(DB_MAKE) -C ./tatooine all
endif
ifeq ($(CHIP), k6l)
	$(DB_MAKE) -C ./tatooine all
endif
ifeq ($(CHIP), i2)
	$(DB_MAKE) -C ./naboo all
endif
else
	@echo "Please source xxx.sh first!"
endif

clean:
ifneq ($(DB_OUT_PATH), )
ifeq ($(CHIP), i2)
	$(DB_MAKE) -C ./naboo clean
endif
ifeq ($(CHIP), k6l)
	$(DB_MAKE) -C ./tatooine clean
endif
ifeq ($(CHIP), k6)
	$(DB_MAKE) -C ./tatooine clean
endif
	@rm -rvf ./out -rvf
else
	@echo "Please source xxx.sh first!"
endif

