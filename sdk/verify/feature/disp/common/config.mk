include clear-config.mk

COMPILE_TARGET:=lib

SRCS:=init_panel_driveric.c draw.c spi_operation.c gpio_operation.c

LIBS:= mi_sys mi_common dl mi_disp
ifneq ($(interface_panel),disable)
	LIBS+=mi_panel
endif

DEP_INCS+=$(PROJ_ROOT)/../sdk/verify/feature/disp/include/panel/ttl
DEP_INCS+=$(PROJ_ROOT)/../sdk/verify/feature/disp/include/panel/mipi
DEP_INCS+=$(PROJ_ROOT)/../sdk/verify/feature/disp/include/common
DEP_INCS+=$(PROJ_ROOT)/../sdk/verify/feature/disp/include/cmd

include add-config.mk
