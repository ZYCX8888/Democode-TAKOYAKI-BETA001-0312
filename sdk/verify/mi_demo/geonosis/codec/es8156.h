#ifndef __ES8156__
#define __ES8156__
// es8156 chip addr
#define ES8156_CHIP_ADDR            (0x08)

// es8156 chip reg
// page 0
#define REG_RESET_CONTROL           (0x00)
#define REG_MAIN_CLOCK_CONTROL      (0x01)
#define REG_MODE_CONFIG_1           (0x02)
#define REG_MASTER_LRCK_DIVIDER_1   (0x03)
#define REG_MASTER_LRCK_DIVIDER_0   (0x04)
#define REG_MASTER_CLOCK_CONTROL    (0x05)
#define REG_NFS_CONFIG              (0x06)
#define REG_MISC_CONTROL_1          (0x07)
#define REG_CLOCK_OFF               (0x08)
#define REG_MISC_CONTROL_2          (0x09)
#define REG_TIME_CONTROL_1          (0x0A)
#define REG_TIME_CONTROL_2          (0x0B)
#define REG_CHIP_STATUS             (0x0C)
#define REG_P2S_CONTRL              (0x0D)
#define REG_DAC_COUNTER_PARAMETER   (0x10)
#define REG_SDP_INTERFACE_CONFIG_1  (0x11)
#define REG_AUTOMUTE_CONTROL        (0x12)
#define REG_MUTE_CONTROL            (0x13)
#define REG_VOLUME_CONTROL          (0x14)
#define REG_ALC_CONFIG_1            (0x15)
#define REG_ALC_CONFIG_2            (0x16)
#define REG_ALC_LEVEL               (0x17)
#define REG_MISC_CONTROL_3          (0x18)
#define REG_EQ_CONTROL_1            (0x19)
#define REG_EQ_CONFIG_2             (0x1A)
#define REG_ANALOG_SYSTEM_1         (0x20)
#define REG_ANALOG_SYSTEM_2         (0x21)
#define REG_ANALOG_SYSTEM_3         (0x22)
#define REG_ANALOG_SYSTEM_4         (0x23)
#define REG_ANALOG_SYSTEM_5         (0x24)
#define REG_ANALOG_SYSTEM_6         (0x25)
#define REG_EQ_DATA_RAM_CLEAR_START (0x40)
#define REG_EQ_DATA_RAM_CLEAR_END   (0x5E)
#define REG_PAGE_SELECT             (0xFC)
#define REG_CHIP_ID1                (0xFD)
#define REG_CHIP_ID0                (0xFE)
#define REG_CHIP_VERSION            (0xFF)

// page 1
#define REG_EQ_COEFFICENT_START     (0x00)
#define REG_EQ_COEFFICENT_END       (0xD1)

// reg value
#define RESET_CONTROL_VAL(RST_REGS, RST_MSTGEN, RST_DAC_DIG, RST_DIG, SEQ_DIS, CSM_ON) \
        (((RST_REGS) << 5) \
        | ((RST_MSTGEN) << 4)   \
        | ((RST_DAC_DIG) << 3)   \
        | ((RST_DIG) << 2)   \
        | ((SEQ_DIS) << 1)   \
        | ((CSM_ON) << 0))

#define MAIN_CLOCK_CONTROL_VAL(MULTP_FACTOR, OSR128_SEL, CLK_DAC_DIV)   \
        (((MULTP_FACTOR) << 6)  \
        | ((OSR128_SEL) << 5)  \
        | ((CLK_DAC_DIV) << 0))

#define MODE_CONFIG_1_VAL(SCLK_AS_MCLK, ISCLKLRCK_SEL, SCLKLRCK_TRI,   \
        SCLK_INV_MODE, EQ_HIGH_MODE, SOFT_MODE_SEL, SPEED_MODE, MS_MODE)    \
        (((SCLK_AS_MCLK) << 7) \
        | ((ISCLKLRCK_SEL) << 6)   \
        | ((SCLKLRCK_TRI) << 5)   \
        | ((SCLK_INV_MODE) << 4)   \
        | ((EQ_HIGH_MODE) << 3)   \
        | ((SOFT_MODE_SEL) << 2)   \
        | ((SPEED_MODE) << 1)   \
        | ((MS_MODE) << 0))

#define MASTER_LRCK_DIVIDER_1_VAL(M_LRCK_DIV_HIGH)   \
        ((M_LRCK_DIV_HIGH) << 0)

#define MASTER_LRCK_DIVIDER_0_VAL(M_LRCK_DIV_LOW)   \
        ((M_LRCK_DIV_LOW) << 0)

#define MASTER_CLOCK_CONTROL_VAL(M_SCLK_MODE, M_SCLK_DIV)   \
        (((M_SCLK_MODE) << 7) | ((M_SCLK_DIV << 0)))

#define NFS_CONFIG_VAL(LRCK_RATE_MODE)  \
        ((LRCK_RATE_MODE) << 0)

#define MISC_CONTROL_1_VAL(MCLK_INV, CLK_DAC_DIV0, CLKDBL_PW_SEL, CLKDBL_PATH_SEL, LRCK_EXTEND)    \
        (((MCLK_INV) << 7)  \
        | ((CLK_DAC_DIV0) << 4)    \
        | ((CLKDBL_PW_SEL) << 2)   \
        | ((CLKDBL_PATH_SEL) << 1) \
        | ((LRCK_EXTEND) << 0))

#define CLOCK_OFF_VAL(P2S_CLK_ON, MASTER_CLK_ON, EXT_SCLKLRCK_ON,  \
        ANA_CLK_ON, DAC_MCLK_ON, MCLK_ON)   \
        (((P2S_CLK_ON) << 5) \
        | ((MASTER_CLK_ON) << 4)   \
        | ((EXT_SCLKLRCK_ON) << 3)   \
        | ((ANA_CLK_ON) << 2)   \
        | ((DAC_MCLK_ON) << 1)   \
        | ((MCLK_ON) << 1))

#define MISC_CONTROL_2_VAL(MSTCLK_SRCSEL, CSM_CNTSEL, DLL_ON, PUPDN_OFF)    \
        (((MSTCLK_SRCSEL) << 5) \
        | ((CSM_CNTSEL) << 4)   \
        | ((DLL_ON) << 1)   \
        | ((PUPDN_OFF) << 0))

#define TIME_CONTROL_1_VAL(V_T1)    \
        ((V_T1) << 0)

#define TIME_CONTROL_2_VAL(V_T2)    \
        ((V_T2) << 0)

#define CHIP_STATUS_VAL(CSM_STATE, FORCE_CSM)   \
        (((CSM_STATE) <<  4) | ((FORCE_CSM) << 0))

#define P2S_CONTRL_VAL(P2S_NFS_FLAGOFF, P2S_SDOUT_MUTEB, P2S_SDOUT_SEL, P2S_SDOUT_TRI)	\
        (((P2S_NFS_FLAGOFF) << 3) | ((P2S_SDOUT_MUTEB) << 2)   \
        | ((P2S_SDOUT_SEL) << 1) | ((P2S_SDOUT_TRI) << 0))

#define DAC_COUNTER_PARAMETER_VAL(DAC_NS)   \
        ((DAC_NS) << 0)

#define SDP_INTERFACE_CONFIG_1_VAL(SP_WL, SP_MUTE, SP_LRP, SP_PROTOCAL) \
        (((SP_WL) << 4) \
        | ((SP_MUTE) << 3)   \
        | ((SP_LRP) << 2)   \
        | ((SP_PROTOCAL) << 0))

#define AUTOMUTE_CONTROL_VAL(AUTOMUTE_NG, AUTOMUTE_SIZE)    \
        (((AUTOMUTE_NG) << 4) | ((AUTOMUTE_SIZE) << 0))

#define MUTE_CONTROL_VAL(INTOUT_CLIPEN, AM_ATTENU6_ENA, AM_ACLKOFF_ENA, \
        AM_DSMMUTE_ENA, LCH_DSM_SMUTE, RCH_DSM_SMUTE, AM_ENA)   \
        (((INTOUT_CLIPEN) << 7) \
        |((AM_ATTENU6_ENA) << 6)   \
        |((AM_ACLKOFF_ENA) << 5)   \
        |((AM_DSMMUTE_ENA) << 4)   \
        |((LCH_DSM_SMUTE) << 2)    \
        |((RCH_DSM_SMUTE) << 1)    \
        |((AM_ENA) << 0))

#define VOLUME_CONTROL_VAL(DAC_VOLUME_DB)   \
        ((DAC_VOLUME_DB) << 0)

#define ALC_CONFIG_1_VAL(ALC_MUTE_GAIN, DAC_ALC_EN) \
        (((ALC_MUTE_GAIN) << 1) | ((DAC_ALC_EN) << 0))

#define ALC_CONFIG_2_VAL(ALC_RAMP_RATE, ALC_WIN_SIZE)   \
        (((ALC_RAMP_RATE) << 4) | ((ALC_WIN_SIZE) << 0))

#define ALC_LEVEL_VAL(ALC_MAXLEVEL, ALC_MINLEVEL)   \
        (((ALC_MAXLEVEL) << 4) | ((ALC_MINLEVEL) << 0))

#define MISC_CONTROL_3_VAL(P2S_DATA_BITNUM, P2S_DPATH_SEL, CHN_CROSS,   \
        LCH_INV, RCH_INV, DSM_DITHERON, DAC_RAM_CLR)    \
        (((P2S_DATA_BITNUM) << 7) \
        | ((P2S_DPATH_SEL) << 6)   \
        | ((CHN_CROSS) << 4)   \
        | (((LCH_INV) << 3))   \
        | ((RCH_INV) << 2)   \
        | ((DSM_DITHERON) << 1)   \
        | ((DAC_RAM_CLR) << 0))

#define EQ_CONTROL_1_VAL(EQ_BAND_NUM, EQ_RST, EQ_CFG_RD, EQ_CFG_WR, EQ_ON)  \
        (((EQ_BAND_NUM) << 4)   \
        | ((EQ_RST) << 3)  \
        | ((EQ_CFG_RD) << 2)   \
        | ((EQ_CFG_WR) << 1)   \
        | ((EQ_ON) << 0))

#define EQ_CONFIG_2_VAL(EQ_1STCNT_VLD, EQ_RUN_1STCNT)   \
        (((EQ_1STCNT_VLD) << 7) | ((EQ_RUN_1STCNT) << 0))

#define ANALOG_SYSTEM_1_VAL(S3_SEL, S2_SEL, S6_SEL) \
        (((S3_SEL) << 4) | ((S2_SEL) << 2) | ((S6_SEL) << 0))

#define ANALOG_SYSTEM_2_VAL(VREF_RMPDN2, VREF_RMPDN1, VSEL) \
        (((VREF_RMPDN2) << 6) | ((VREF_RMPDN1) << 5) | ((VSEL) << 0))

#define ANALOG_SYSTEM_3_VAL(HPSW, SWHWSEL, OUT_MUTE) \
        (((HPSW) << 3) | ((SWHWSEL) << 1) | ((OUT_MUTE) << 0))

#define ANALOG_SYSTEM_4_VAL(IBIAS_SW, VMIDLVL, DAC_IBIAS_SW, VROI, HPCOM_REF2, HPCOM_REF1) \
        (((IBIAS_SW) << 6) | ((VMIDLVL) << 4) | ((DAC_IBIAS_SW) << 3) \
        | ((VROI) << 2) | ((HPCOM_REF2) << 1) | ((HPCOM_REF1) << 0))

#define ANALOG_SYSTEM_5_VAL(LPDAC, LPDACVRP, LPHPCOM, LPVREFBUF)    \
        (((LPDAC) << 3) | ((LPDACVRP) << 2) | ((LPHPCOM) << 1)    \
        | ((LPVREFBUF) << 0))

#define ANALOG_SYSTEM_6_VAL(PDN_ANA, ENREFR, VMIDSEL, ENHPCOM,  \
        PDN_DACVREFGEN, PDN_VREFBUF, PDN_DAC)    \
        (((PDN_ANA) << 7) | ((ENREFR) << 6) | ((VMIDSEL) << 4)    \
        | ((ENHPCOM) << 3) | ((PDN_DACVREFGEN) << 2) | ((PDN_VREFBUF) << 1)  \
        | ((PDN_DAC) << 0))

#define PAGE_SELECT_VAL(PAGE_SEL)   \
        ((PAGE_SEL) << 0)
#endif