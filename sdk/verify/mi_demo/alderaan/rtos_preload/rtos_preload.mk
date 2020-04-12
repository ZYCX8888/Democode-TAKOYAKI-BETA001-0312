INC  += $(DB_BUILD_TOP)/../common/live555/UsageEnvironment/include
INC  += $(DB_BUILD_TOP)/../common/live555/groupsock/include
INC  += $(DB_BUILD_TOP)/../common/live555/liveMedia/include
INC  += $(DB_BUILD_TOP)/../common/live555/BasicUsageEnvironment/include
INC  += $(DB_BUILD_TOP)/../common/live555/mediaServer/include

ST_DEP := common vpe venc vif rgn onvif ao live555 tem

LIBS += -lmi_vif -lmi_vpe -lmi_venc -lmi_rgn -lmi_divp -lmi_isp -lmi_iqserver -lmi_ai -lmi_ao -ldl
