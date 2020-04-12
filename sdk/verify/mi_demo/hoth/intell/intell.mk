INC  += $(DB_BUILD_TOP)/../common/live555/UsageEnvironment/include
INC  += $(DB_BUILD_TOP)/../common/live555/groupsock/include
INC  += $(DB_BUILD_TOP)/../common/live555/liveMedia/include
INC  += $(DB_BUILD_TOP)/../common/live555/BasicUsageEnvironment/include
INC  += $(DB_BUILD_TOP)/../common/live555/mediaServer/include

ST_DEP := common vpe venc vif font  rgn onvif ao  live555
LIBS += -lmi_vif -lmi_vpe -lmi_venc -lmi_rgn -lmi_divp -lmi_isp -lmi_iqserver -lmi_vdf \
		-lmi_shadow -lOD_LINUX -lMD_LINUX -lVG_LINUX -lmi_ive
