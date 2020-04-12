//INC  += $(PROJ_ROOT)/kbuild/$(KERNEL_VERSION)/drivers/mstar/include
INC  += ./common/cdnn
INC  += ./common/cdnn/install_generic/include
INC  += ./common/cdnn/install_xm6/Include/amf_client
INC  += ./common/cdnn/install_xm6/Include/ceva_link_share
INC  += ./common/cdnn/install_xm6/Include/ceva_liblink
INC  += ./common/cdnn/install_xm6/Include/ceva_liblink/ceva
INC  += ./common/cdnn/Include
#LIBS += -L./common/cdnn/lib
#LIBS += -lCDNNComponent -lCDNNAppUtility -lcevaamf -lcevalink  -lstdc++

INC  += ./rtsp/include/UsageEnvironment
INC  += ./rtsp/include/groupsock
INC  += ./rtsp/include/liveMedia
INC  += ./rtsp/include/BasicUsageEnvironment
INC  += ./rtsp/src
LIBS += -L./rtsp/lib
#LIBS += libliveMedia.a libgroupsock.a libBasicUsageEnvironment.a libUsageEnvironment.a 
LIBS += -lmi_divp -lmi_vdisp -lmi_vif -lmi_vpe -lmi_venc -lmi_disp -lmi_hdmi -lliveMedia -lgroupsock -lBasicUsageEnvironment -lUsageEnvironment

SUBDIRS += ./rtsp/src

ST_DEP := common hdmi disp vpe vif vdisp fb mess tem