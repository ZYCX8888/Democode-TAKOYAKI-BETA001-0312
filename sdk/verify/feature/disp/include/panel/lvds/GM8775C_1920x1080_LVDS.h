
#include "mi_panel_datatype.h"

#define FLAG_DELAY            0xFE
#define FLAG_END_OF_TABLE     0xFF   // END OF REGISTERS MARKER

MI_PANEL_ParamConfig_t stPanelParam =
{
    "GM8775C_1920x1080",    //const char *pPanelName; ///<  PanelName
    0,                    //MI_U8             MI_U8Dither;         ///<  Diether On?off
    E_MI_PNL_LINK_MIPI_DSI, //MIPnlLinkType_e eLinkType;     ///<  Panel LinkType

    0,        //MI_U8 MI_U8DualPort      :1;          ///<  DualPort on/off
    0,        //MI_U8 MI_U8SwapPort      :1;          ///<  Swap Port on/off
    0,        //MI_U8 MI_U8SwapOdd_ML    :1;          ///<  Swap Odd ML
    0,        //MI_U8 MI_U8SwapEven_ML   :1;          ///<  Swap Even ML
    0,        //MI_U8 MI_U8SwapOdd_RB    :1;          ///<  Swap Odd RB
    0,        //MI_U8 MI_U8SwapEven_RB   :1;          ///<  Swap Even RB

    0,        //MI_U8 MI_U8SwapLVDS_POL  :1;          ///<  Swap LVDS Channel Polairyt
    0,        //MI_U8 MI_U8SwapLVDS_CH   :1;          ///<  Swap LVDS channel
    0,        //MI_U8 MI_U8PDP10BIT      :1;          ///<  PDP 10bits on/off
    0,        //MI_U8 MI_U8LVDS_TI_MODE  :1;          ///<  Ti Mode On/Off


    0,        //MI_U8 MI_U8DCLKDelay;                 ///<  DCLK Delay
    0,        //MI_U8 MI_U8InvDCLK   :1;              ///<  CLK Invert
    0,        //MI_U8 MI_U8InvDE     :1;              ///<  DE Invert
    0,        //MI_U8 MI_U8InvHSync  :1;              ///<  HSync Invert
    0,        //MI_U8 MI_U8InvVSync  :1;              ///<  VSync Invert

    0,        //MI_U8 MI_U8DCKLCurrent;              ///< PANEL_DCLK_CURRENT
    0,        //MI_U8 MI_U8DECurrent;                ///< PANEL_DE_CURRENT
    0,        //MI_U8 MI_U8ODDDataCurrent;           ///< PANEL_ODD_DATA_CURRENT
    0,        //MI_U8 MI_U8EvenDataCurrent;          ///< PANEL_EVEN_DATA_CURRENT

    0,       //u16 u16OnTiming1;                ///<  time between panel & data while turn on power
    0,      //u16 u16OnTiming2;                ///<  time between data & back light while turn on power
    0,       //u16 u16OffTiming1;               ///<  time between back light & data while turn off power
    0,       //u16 u16OffTiming2;               ///<  time between data & panel while turn off power

    32,       //u16 u16HSyncWidth;               ///<  Hsync Width
    128,      //u16 u16HSyncBackPorch;           ///<  Hsync back porch

    5,        //u16 u16VSyncWidth;               ///<  Vsync width
    36,       //u16 u16VSyncBackPorch;           ///<  Vsync back porch

    160,       //u16 u16HStart;                   ///<  HDe start
    41,       //u16 u16VStart;                   ///<  VDe start
    1920,     //u16 u16Width;                    ///<  Panel Width
    1080,     //u16 u16Height;                   ///<  Panel Height

    0,        //u16 u16MaxHTotal;                ///<  Max H Total
    2200,      //u16 u16HTotal;                   ///<  H Total
    0,        //u16 u16MinHTotal;                ///<  Min H Total

    0,        //u16 u16MaxVTotal;                ///<  Max V Total
    1125,  //u16 u16VTotal;                   ///<  V Total
    0,        //u16 u16MinVTotal;                ///<  Min V Total

    0,        //u16 u16MaxDCLK;                  ///<  Max DCLK
    148,      //u16 u16DCLK;                     ///<  DCLK ( Htt * Vtt * Fps)
    0,        //u16 u16MinDCLK;                  ///<  Min DCLK

    0,       //u16 u16SpreadSpectrumStep;       ///<  Step of SSC
    0,      //u16 u16SpreadSpectrumSpan;       ///<  Span of SSC

    0,      //MI_U8 MI_U8DimmingCtl;                 ///<  Dimming Value
    0,      //MI_U8 MI_U8MaxPWMVal;                  ///<  Max Dimming Value
    0,       //MI_U8 MI_U8MinPWMVal;                  ///<  Min Dimming Value

    0,                          //MI_U8 MI_U8DeinterMode   :1;                  ///<  DeInter Mode
    E_MI_PNL_ASPECT_RATIO_WIDE, //MIPnlAspectRatio_e ePanelAspectRatio; ///<  Aspec Ratio

    0,                         //u16 u16LVDSTxSwapValue;         // LVDS Swap Value
    E_MI_PNL_TI_8BIT_MODE,     //MIPnlTiBitMode_e eTiBitMode;  // Ti Bit Mode
    E_MI_PNL_OUTPUT_8BIT_MODE, //MIPnlOutputFormatBitMode_e eOutputFormatBitMode;

    0,        //MI_U8 MI_U8SwapOdd_RG    :1;          ///<  Swap Odd RG
    0,        //MI_U8 MI_U8SwapEven_RG   :1;          ///<  Swap Even RG
    0,        //MI_U8 MI_U8SwapOdd_GB    :1;          ///<  Swap Odd GB
    0,        //MI_U8 MI_U8SwapEven_GB   :1;          ///<  Swap Even GB

    0,        //MI_U8 MI_U8DoubleClk     :1;                      ///<  Double CLK On/off
    0x0, //u32 u32MaxSET;                              ///<  Max Lpll Set
    0x0, //u32 u32MinSET;                              ///<  Min Lpll Set
    E_MI_PNL_CHG_VTOTAL,   //MIPnlOutputTimingMode_e eOutTimingMode;   ///<  Define which panel output timing change mode is used to change VFreq for same panel
    0,                     //MI_U8 MI_U8NoiseDith     :1;                      ///<  Noise Dither On/Off
    (MI_PANEL_ChannelSwapType_e)2,
    (MI_PANEL_ChannelSwapType_e)4,
    (MI_PANEL_ChannelSwapType_e)3,
    (MI_PANEL_ChannelSwapType_e)1,
    (MI_PANEL_ChannelSwapType_e)0,
};

MI_U8 GM8775C_CMD[] =
{
    0x27,1,0xAA, 
    0x48,1,0x02,
    0xB6,1,0x20,
    0x01,1,0x80,
    0x02,1,0x38,
    0x03,1,0x47,
    0x04,1,0x58,
    0x05,1,0x2C,
    0x06,1,0x94,
    0x07,1,0x00,
    0x08,1,0x04,
    0x09,1,0x05,
    0x0A,1,0x24,
    0x0B,1,0x82,
    0x0C,1,0x14,
    0x0D,1,0x01,
    0x0E,1,0x80,
    0x0F,1,0x20,
    0x10,1,0x20,
    0x11,1,0x03,
    0x12,1,0x1B,
    0x13,1,0x53,
    0x14,1,0x01,
    0x15,1,0x23,
    0x16,1,0x40,
    0x17,1,0x00,
    0x18,1,0x01,
    0x19,1,0x23,
    0x1A,1,0x40,
    0x1B,1,0x00,
    0x1E,1,0x46,
    0x51,1,0x30,
    0x1F,1,0x10,
    0x2A,1,0x01,
    FLAG_END_OF_TABLE, FLAG_END_OF_TABLE,
};

MI_PANEL_MipiDsiConfig_t stMipiDsiConfig =
{
    //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
    0x14,     0x0a,   0x14,   0x07,      0x0d,     0x08,  0x20,     0x18,      0x0a,  0x00,
    //Lpx   TaGet  TaSure  TaGo
    0x10,    0x50, 0x18,  0x40,

    //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
    1920,   32,   128,  120,  1080, 5,   36,  4,    0,    60,

    E_MI_PNL_MIPI_DSI_LANE_4,      // MIPnlMipiDsiLaneMode_e enLaneNum;
    E_MI_PNL_MIPI_DSI_RGB888,      // MIPnlMipiDsiFormat_e enFormat;
    E_MI_PNL_MIPI_DSI_SYNC_PULSE,  // MIPnlMipiDsiCtrlMode_e enCtrl;

    GM8775C_CMD,
    sizeof(GM8775C_CMD),
    1, 0x01AF, 0x01B9, 0x80D2, 8,
};

