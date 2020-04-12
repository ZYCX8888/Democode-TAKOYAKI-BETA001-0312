LIBS     += -Wl,-rpath=./
LIBS     += -L$(DB_OUT_PATH)/lib
LIBS 	 += -L$(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/mi_libs/static
LIBS 	 += -L$(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/common/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/ex_libs/static
LIBS     += -l$(DB_LIB_NAME)
LIBS     += -lmi_rgn -lmi_vpe -lmi_divp -lmi_venc -lmi_vif -lmi_iqserver -lmi_isp -lcus3a -lispalgo -lmi_common -lmi_sys -lmi_sensor -ldl
