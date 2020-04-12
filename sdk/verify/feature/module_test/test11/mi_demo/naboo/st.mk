INC += $(foreach m,$(ST_DEP),$(DB_BUILD_TOP)/naboo/internal/$(m))
LIBS += $(foreach m,$(ST_DEP),-lst_$(m))