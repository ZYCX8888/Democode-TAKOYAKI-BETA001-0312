INC += $(wildcard $(DB_BUILD_TOP)/internal/*)
INC += $(wildcard $(DB_BUILD_TOP)/../common/*)
LIBS += $(foreach m,$(ST_DEP),-lst_$(m))