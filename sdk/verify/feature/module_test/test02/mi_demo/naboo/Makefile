MODULES := ao fb vdec vifvo venc
MODULES_ALL := $(foreach m,$(MODULES),$(m)_all)
MODULES_CLEAN := $(foreach m,$(MODULES),$(m)_clean)
.PHONY :all clean $(MODULES) $(MODULES_ALL) $(MODULES_CLEAN)

all: $(MODULES_ALL)
clean: $(MODULES_CLEAN)

$(MODULES_ALL):
	@cp ./MakeApp.mk ./MakefileTmp.mk
	@sed -i "s/MODULE/$(patsubst %_all,%,$@)/g" ./MakefileTmp.mk
	$(DB_MAKE) -f ./MakefileTmp.mk all
	@rm ./MakefileTmp.mk -rvf

$(MODULES_CLEAN):
	@cp ./MakeApp.mk ./MakefileTmp.mk
	@sed -i "s/MODULE/$(patsubst %_clean,%,$@)/g" ./MakefileTmp.mk
	$(DB_MAKE) -f ./MakefileTmp.mk clean
	@rm ./MakefileTmp.mk -rvf