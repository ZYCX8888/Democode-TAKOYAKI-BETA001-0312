INTERNAL_LIBS := $(patsubst ./internal/%,%,$(wildcard ./internal/*))

MODULES_OBJS_ALL := $(foreach m,$(INTERNAL_LIBS),$(m)_obj_all)
MODULES_OBJS_CLEAN := $(foreach m,$(INTERNAL_LIBS),$(m)_obj_clean)
MODULES_LIBS_ALL := $(foreach m,$(INTERNAL_LIBS),$(m)_lib_all)
MODULES_LIBS_CLEAN := $(foreach m,$(INTERNAL_LIBS),$(m)_lib_clean)

.PHONY :$(MODULES) $(MODULES_ALL) $(MODULES_CLEAN) $(MODULES_OBJS_ALL) $(MODULES_OBJS_CLEAN) $(MODULES_LIBS_ALL) $(MODULES_LIBS_CLEAN)

all: $(MODULES_OBJS_ALL) $(MODULES_LIBS_ALL) $(MODULES_ALL)
clean: $(MODULES_CLEAN) $(MODULES_LIBS_CLEAN) $(MODULES_OBJS_CLEAN)

$(MODULES_OBJS_ALL):
	@cp ./MakeObj.mk ./MakefileTmp2.mk
	@sed -i "s/MODULE_OBJ/$(patsubst %_obj_all,%,$@)/g" ./MakefileTmp2.mk
	@$(DB_MAKE) -f ./MakefileTmp2.mk all
	@rm ./MakefileTmp2.mk -rf
$(MODULES_OBJS_CLEAN):
	@cp ./MakeObj.mk ./MakefileTmp2.mk
	@sed -i "s/MODULE_OBJ/$(patsubst %_obj_clean,%,$@)/g" ./MakefileTmp2.mk
	@$(DB_MAKE) -f ./MakefileTmp2.mk clean
	@rm ./MakefileTmp2.mk -rf

$(MODULES_LIBS_ALL):
	@cp ./MakeLib.mk ./MakefileTmp2.mk
	@sed -i "s/MODULE_LIB/$(patsubst %_lib_all,%,$@)/g" ./MakefileTmp2.mk
	@$(DB_MAKE) -f ./MakefileTmp2.mk all
	@rm ./MakefileTmp2.mk -rf

$(MODULES_LIBS_CLEAN):
	@cp ./MakeLib.mk ./MakefileTmp2.mk
	@sed -i "s/MODULE_LIB/$(patsubst %_lib_clean,%,$@)/g" ./MakefileTmp2.mk
	@$(DB_MAKE) -f ./MakefileTmp2.mk clean
	@rm ./MakefileTmp2.mk -rf

$(MODULES_ALL):
	@cp ./MakeApp.mk ./MakefileTmp2.mk
	@sed -i "s/MODULE/$(patsubst %_all,%,$@)/g" ./MakefileTmp2.mk
	@$(DB_MAKE) -f ./MakefileTmp2.mk all
	@rm ./MakefileTmp2.mk -rf

$(MODULES_CLEAN):
	@cp ./MakeApp.mk ./MakefileTmp2.mk
	@sed -i "s/MODULE/$(patsubst %_clean,%,$@)/g" ./MakefileTmp2.mk
	@$(DB_MAKE) -f ./MakefileTmp2.mk clean
	@rm ./MakefileTmp2.mk -rf
