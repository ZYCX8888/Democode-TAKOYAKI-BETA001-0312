#ifndef __ES7210__
#define __ES7210__
// es7243 chip addr
#define ES7210_CHIP_ADDR                               (0x43)
#if 0
// es7243 chip reg
#define REG_MODE_CONFIGURE                              (0x00)
#define REG_SERIAL_DATA_PORT                            (0X01)
#define REG_LRCK_DIVIDER                                (0x02)
#define REG_BCLK_DIVIDER                                (0x03)
#define REG_CLOCK_DIVIDER_FOR_ADC                       (0x04)
#define REG_MUTE_CONTROL_FOR_ADC                        (0x05)
#define REG_STATE_CONTROL_FOR_CHIP                      (0x06)
#define REG_ANALOG_CONTROL_REGISTER_0                   (0x07)
#define REG_ANALOG_CONTROL_REGISTER_1                   (0x08)
#define REG_ANALOG_CONTROL_REGISTER_2                   (0x09)
#define REG_PERIOD_FOR_ANALOG_CHARGING_STATE            (0x0A)
#define REG_PERIOD_FOR_DIGITAL_INITIATING_STATE_1       (0x0B)
#define REG_PERIOD_FOR_DIGITAL_INITIATING_STATE_2       (0x0C)
#define REG_CHIP_STATE_AND_OSR_SETTING                  (0x0D)
#define REG_CHIP_ID                                     (0x0E)

#define MODE_CONFIGURE_VAL(MCLK_DIV, ADC_HPF_DIS,   \
        SPEED_MODE, MS_MODE, WORK_MODE)  \
        (((MCLK_DIV) << 5) \
        | ((ADC_HPF_DIS) << 4)    \
        | ((SPEED_MODE) << 2)    \
        | ((MS_MODE) << 1)    \
        | ((WORK_MODE) << 0))

#define SERIAL_DATA_PORT_VAL(TDM_ENA, SP_BCLKINV, SP_LRP, SP_WL, SP_PROTOCAL)   \
        (((TDM_ENA) << 7)   \
        |((SP_BCLKINV) << 6)    \
        |((SP_LRP) << 5)    \
        |((SP_WL) << 2) \
        |((SP_PROTOCAL) << 0))

#define LRCK_DIVIDER_VAL(LRCKDIV)   \
        ((LRCKDIV) << 0)

#define BCLK_DIVIDER_VAL(BCLKDIV)   \
        ((BCLKDIV) << 0)

#define CLOCK_DIVIDER_FOR_ADC_VAL(CLK_ADC_DIV)  \
        ((CLK_ADC_DIV) << 0)

#define MUTE_CONTROL_FOR_ADC_VAL(AUTOMUTE_DETED, ADC_MUTE_SIZE, \
        ADC_SDP_MUTE, ADC_NOISETHD, ADC_AUTOMUTE)   \
        (((AUTOMUTE_DETED) << 5)    \
        |((ADC_MUTE_SIZE) << 4) \
        |((ADC_SDP_MUTE) << 3)  \
        |((ADC_NOISETHD) << 1)  \
        |((ADC_AUTOMUTE) << 0))

#define STATE_CONTROL_FOR_CHIP_VAL(SP_TRI, MCLK_DIS, SEQ_DIS,    \
        RST_DIG, RST_ADCDIG, FORCE_CSM) \
        (((SP_TRI) << 7)    \
        |((MCLK_DIS) << 6)  \
        |((SEQ_DIS) << 5)   \
        |((RST_DIG) << 4)   \
        |((RST_ADCDIG) << 3)    \
        |((FORCE_CSM) << 0))

#define ANALOG_CONTROL_REGISTER_0_VAL(VMIDSEL, PDN_ADCVREFGEN, MODTOP_RST,   \
        PDN_MODL, PDN_MODR, PDN_PGAL, PDN_PGAR) \
        (((VMIDSEL) << 6)   \
        |((PDN_ADCVREFGEN) << 5)    \
        |((MODTOP_RST) << 4)    \
        |((PDN_MODL) << 3)  \
        |((PDN_MODR) << 2)  \
        |((PDN_PGAL) << 1)  \
        |((PDN_PGAR) << 0))

#define ANALOG_CONTROL_REGISTER_1_VAL(GAIN_SW, INPUT_SEL2, GAIN_SEL, INPUT_SEL) \
        (((GAIN_SW) << 4)   \
        |((INPUT_SEL2) <<3) \
        |((GAIN_SEL) << 1)  \
        |((INPUT_SEL) << 0))

#define ANALOG_CONTROL_REGISTER_2_VAL(ANA_PDN, VMIDLOW, ADC_LP_VRP, \
        ADC_LP_VCMMOD, ADC_LP_PGA, ADC_LP_INT1, ADC_LP_FLASH)   \
        (((ANA_PDN) << 7)   \
        |((VMIDLOW) << 5)   \
        |((ADC_LP_VRP) << 4)    \
        |((ADC_LP_VCMMOD) << 3) \
        |((ADC_LP_PGA) << 2)    \
        |((ADC_LP_INT1) << 1)   \
        |((ADC_LP_FLASH) << 0))

#define PERIOD_FOR_ANALOG_CHARGING_STATE_VAL(CHIPINI_CON)   \
        ((CHIPINI_CON) << 0)

#define PERIOD_FOR_DIGITAL_INITIATING_STATE_1_VAL(POWERUP_CON)  \
        ((POWERUP_CON) << 0)

#define PERIOD_FOR_DIGITAL_INITIATING_STATE_2(ADCBIAS_SWH, SRATIO_DIV)  \
        (((ADCBIAS_SWH) << 4) | ((SRATIO_DIV) << 0))

#define CHIP_STATE_AND_OSR_SETTING_VAL(ADC_CSM, ADC_OSR)    \
        (((ADC_CSM) << 6) | ((ADC_OSR) << 0))

#define CHIP_ID_VAL(CHIP_ID)    \
        ((CHIP_ID) << 0)
#endif
#endif

