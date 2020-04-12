-include ../../../project/configs/current.configs
-include ../../configs/current.configs
PROJ_ROOT?=$(PDIR)
include $(PROJ_ROOT)/configs/current.configs
include $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/toolchain.mk
include $(PROJ_ROOT)/release/customer_tailor/$(CUSTOMER_TAILOR)

CONFIG_LIST:=

include */config.mk

CONFIG_LIST:=$(patsubst %_enable_,%,$(patsubst %__,%,$(filter-out %_disable_, $(foreach n,$(CONFIG_LIST),$(n)_$(interface_$(n))_))))

.PHONY: $(CONFIG_LIST)

CONFIG_CLEAN_LIST:=$(foreach n,$(CONFIG_LIST),$(n)_clean)

CONFIG_INSTALL_LIST:=$(foreach n,$(CONFIG_LIST),$(n)_install)

all: $(CONFIG_LIST)

install: $(CONFIG_INSTALL_LIST)

clean: $(CONFIG_CLEAN_LIST)

$(CONFIG_LIST): %:
	$(CC) $(foreach n,$($@_SRCS),$@/$(n)) -o $@/$@ -I$@ -I$(PROJ_ROOT)/release/include -g $($@_CFLAGS) -L$(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/lib/dynamic -Wl,--start-group -lpthread -lrt -lmi_sys -lmi_$@ -Wl,--end-group

$(CONFIG_CLEAN_LIST): %_clean:
	rm -f $(patsubst %_clean,%,$@)/$(patsubst %_clean,%,$@)

$(CONFIG_INSTALL_LIST): %_install: $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/verify
	cp $(patsubst %_install,%,$@)/$(patsubst %_install,%,$@) $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/verify/module_$(patsubst %_install,%,$@)

$(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/verify:
	mkdir -p $(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/bin/verify
