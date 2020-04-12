INC  += $(PROJ_ROOT)/release/include
INC  += $(PROJ_ROOT)/kbuild/$(KERNEL_VERSION)/include/uapi/mstar
INC  += $(PROJ_ROOT)/kbuild/$(KERNEL_VERSION)/drivers/mstar/include

INC  += ./common
INC  += ./common/cdnn
INC  += ./common/cdnn/install_generic/include
INC  += ./common/cdnn/install_xm6/Include/amf_client
INC  += ./common/cdnn/install_xm6/Include/ceva_link_share
INC  += ./common/cdnn/install_xm6/Include/ceva_liblink
INC  += ./common/cdnn/install_xm6/Include/ceva_liblink/ceva
INC  += ./common/cdnn/Include
INC  += ./common/VX/include

LIBS += -lrt -lpthread -lm
LIBS += -lmi_hdmi -lmi_disp -lmi_vpe -lmi_disp -lmi_vdisp -lmi_divp \
		-lmi_vdec -ladda -lmi_vif -lmi_rgn -lmi_ao -lmi_venc \
		-lAPC_LINUX -lSRC_LINUX -lmi_common -lmi_sys -lmi_warp -lmi_gfx -lmi_ive -lmi_vdf -lOD_LINUX -lMD_LINUX -lmi_shadow \
		-lg711 -lg726
LIBS += -L$(PROJ_ROOT)/release/$(PRODUCT)/$(CHIP)/$(BOARD)/$(TOOLCHAIN)/$(TOOLCHAIN_VERSION)/lib/static
LIBS += -L./ex_libs
