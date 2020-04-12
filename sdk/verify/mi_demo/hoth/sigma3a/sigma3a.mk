ST_DEP := common vif vpe mess
LIBS += -lmi_vif -lmi_vpe -lAEC_LINUX -lmi_isp -lcus3a -lispalgo
INC += $(PROJ_ROOT)/../sdk/misc/ispalgo/drv/inc/camalgo/
INC += $(PROJ_ROOT)/../sdk/misc/cus3a/cus3a/inc/
INC += $(PROJ_ROOT)/release/include/
CODEDEFINE += -D__USE_USERSPACE_3A__
