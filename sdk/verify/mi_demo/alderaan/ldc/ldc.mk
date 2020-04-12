INC  += $(DB_BUILD_TOP)/../common/live555/UsageEnvironment/include
INC  += $(DB_BUILD_TOP)/../common/live555/groupsock/include
INC  += $(DB_BUILD_TOP)/../common/live555/liveMedia/include
INC  += $(DB_BUILD_TOP)/../common/live555/BasicUsageEnvironment/include
INC  += $(DB_BUILD_TOP)/../common/live555/mediaServer/include
INC  += ./internal/ldc

ST_DEP := common vpe venc vif  live555

LIBS += -L./internal/ldc

LIBS += -lmi_vif -lmi_vpe -lmi_ldc -lmi_venc -lmi_isp -lmi_iqserver -leptz
