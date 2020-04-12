ST_DEP := common vif vpe mess

INC+= -I $(PROJ_ROOT)/release/include/
INC+= -I $(PROJ_ROOT)/../sdk/misc/cus3a/cus3a/inc
INC+= -I $(PROJ_ROOT)/../sdk/misc/ispalgo/drv/inc/camalgo

LIBS += -lmi_vif -lmi_vpe -lAEC_LINUX -lmi_isp -lcus3a -lispalgo
CODEDEFINE += -D__USE_USERSPACE_3A__
