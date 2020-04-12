-include ../../project/configs/current.configs
-include ../configs/current.configs
PROJ_ROOT?=$(PDIR)
export PROJ_ROOT

all:
	$(MAKE) -C feature all
ifneq ($(findstring $(CHIP),  i6  i6e i6b0),)
	$(MAKE) -C mi_demo all
	#$(MAKE) -C feature/mdb all
	$(MAKE) -C live555/sample/mixer
endif

install:
	$(MAKE) -C feature install
ifneq ($(findstring $(CHIP),  i6  i6e i6b0),)
	$(MAKE) -C mi_demo install
	#$(MAKE) -C feature/mdb install
	$(MAKE) -C live555/sample/mixer install
endif

clean:
	$(MAKE) -C feature clean
ifneq ($(findstring $(CHIP),  i6  i6e i6b0),)
	$(MAKE) -C live555/sample/mixer clean
	#$(MAKE) -C feature/mdb clean
	$(MAKE) -C mi_demo clean
endif
