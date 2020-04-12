#include "mi_panel_datatype.h"

#define FLAG_DELAY            0xFE
#define FLAG_END_OF_TABLE     0xFF   // END OF REGISTERS MARKER

MI_PANEL_ParamConfig_t stPanelParam =
{
    "Q10140IM7167AA0_800x1280", // const char *m_pPanelName;                ///<  PanelName
    0, //MS_U8 m_bPanelDither :1;                 ///<  PANEL_DITHER, keep the setting
    E_MI_PNL_LINK_MIPI_DSI, //MHAL_DISP_ApiPnlLinkType_e m_ePanelLinkType   :4;  ///<  PANEL_LINK

    0,        //MI_U8 MI_U8DualPort      :1;          ///<  DualPort on/off
    0,        //MI_U8 MI_U8SwapPort      :1;          ///<  Swap Port on/off
    0,        //MI_U8 MI_U8SwapOdd_ML    :1;          ///<  Swap Odd ML
    0,        //MI_U8 MI_U8SwapEven_ML   :1;          ///<  Swap Even ML
    0,        //MI_U8 MI_U8SwapOdd_RB    :1;          ///<  Swap Odd RB
    0,        //MI_U8 MI_U8SwapEven_RB   :1;          ///<  Swap Even RB

    1,        //MI_U8 MI_U8SwapLVDS_POL  :1;          ///<  Swap LVDS Channel Polairyt
    1,        //MI_U8 MI_U8SwapLVDS_CH   :1;          ///<  Swap LVDS channel
    0,        //MI_U8 MI_U8PDP10BIT      :1;          ///<  PDP 10bits on/off
    1,        //MI_U8 MI_U8LVDS_TI_MODE  :1;          ///<  Ti Mode On/Off


    0,        //MI_U8 MI_U8DCLKDelay;                 ///<  DCLK Delay
    0,        //MI_U8 MI_U8InvDCLK   :1;              ///<  CLK Invert
    0,        //MI_U8 MI_U8InvDE     :1;              ///<  DE Invert
    0,        //MI_U8 MI_U8InvHSync  :1;              ///<  HSync Invert
    0,        //MI_U8 MI_U8InvVSync  :1;              ///<  VSync Invert

    ///////////////////////////////////////////////
    // Output driving current setting
    ///////////////////////////////////////////////
    // driving current setting (0x00=4mA, 0x01=6mA, 0x02=8mA, 0x03=12mA)
    1,  //MS_U8 m_ucPanelDCKLCurrent;              ///<  define PANEL_DCLK_CURRENT
    1,  //MS_U8 m_ucPanelDECurrent;                ///<  define PANEL_DE_CURRENT
    1,  //MS_U8 m_ucPanelODDDataCurrent;           ///<  define PANEL_ODD_DATA_CURRENT
    1,  //MS_U8 m_ucPanelEvenDataCurrent;          ///<  define PANEL_EVEN_DATA_CURRENT

    ///////////////////////////////////////////////
    // panel on/off timing
    ///////////////////////////////////////////////
    30,  //MS_U16 m_wPanelOnTiming1;                ///<  time between panel & data while turn on power
    400,  //MS_U16 m_wPanelOnTiming2;                ///<  time between data & back light while turn on power
    80,  //MS_U16 m_wPanelOffTiming1;               ///<  time between back light & data while turn off power
    30,  //MS_U16 m_wPanelOffTiming2;               ///<  time between data & panel while turn off power

    20,        //u16 u16HSyncWidth;               ///<  Hsync Width
    20,       //u16 u16HSyncBackPorch;           ///<  Hsync back porch

    4,       //u16 u16VSyncWidth;               ///<  Vsync width
    10,      //u16 u16VSyncBackPorch;           ///<  Vsync back porch

    0,       //u16 u16HStart;                   ///<  HDe start
    0,        //u16 u16VStart;                   ///<  VDe start
    800,      //u16 u16Width;                    ///<  Panel Width
    1280,     //u16 u16Height;                   ///<  Panel Height

    0,      //u16 u16MaxHTotal;                ///<  Max H Total
    860,      //u16 u16HTotal;                   ///<  H Total
    0,      //u16 u16MinHTotal;                ///<  Min H Total

    0,     //u16 u16MaxVTotal;                ///<  Max V Total
    1316,     //u16 u16VTotal;                   ///<  V Total
    0,     //u16 u16MinVTotal;                ///<  Min V Total

    0,       //u16 u16MaxDCLK;                  ///<  Max DCLK
    68,       //u16 u16DCLK;                     ///<  DCLK ( Htt * Vtt * Fps)
    0,       //u16 u16MinDCLK;                  ///<  Min DCLK

    0,    //u16 u16SpreadSpectrumStep;       ///<  Step of SSC
    0,    //u16 u16SpreadSpectrumSpan;       ///<  Span of SSC

    0xA0,      //MI_U8 MI_U8DimmingCtl;                 ///<  Dimming Value
    0xFF,      //MI_U8 MI_U8MaxPWMVal;                  ///<  Max Dimming Value
    0x50,      //MI_U8 MI_U8MinPWMVal;                  ///<  Min Dimming Value

    0,                            //MI_U8 MI_U8DeinterMode   :1;                  ///<  DeInter Mode
    E_MI_PNL_ASPECT_RATIO_WIDE, //MIPnlAspectRatio_e ePanelAspectRatio; ///<  Aspec Ratio

    0,                            //u16 u16LVDSTxSwapValue;         // LVDS Swap Value
    E_MI_PNL_TI_8BIT_MODE,      //MIPnlTiBitMode_e eTiBitMode;  // Ti Bit Mode
    E_MI_PNL_OUTPUT_8BIT_MODE,  //MHAL_DISP_ApiPnlOutPutFormatBitMode_e m_ucOutputFormatBitMode;

    0,        //MI_U8 MI_U8SwapOdd_RG    :1;          ///<  Swap Odd RG
    0,        //MI_U8 MI_U8SwapEven_RG   :1;          ///<  Swap Even RG
    0,        //MI_U8 MI_U8SwapOdd_GB    :1;          ///<  Swap Odd GB
    0,        //MI_U8 MI_U8SwapEven_GB   :1;          ///<  Swap Even GB

    0,        //MI_U8 MI_U8DoubleClk     :1;                      ///<  Double CLK On/off
    0x1C848E, //u32 u32MaxSET;                              ///<  Max Lpll Set
    0x18EB59, //u32 u32MinSET;                              ///<  Min Lpll Set
    E_MI_PNL_CHG_HTOTAL, //MIPnlOutputTimingMode_e eOutTimingMode;   ///<  Define which panel output timing change mode is used to change VFreq for same panel
    0,                     //MI_U8 MI_U8NoiseDith     :1;                      ///<  Noise Dither On/Off
    E_MI_PNL_CH_SWAP_2,
    E_MI_PNL_CH_SWAP_0,
    E_MI_PNL_CH_SWAP_1,
    E_MI_PNL_CH_SWAP_3,
    E_MI_PNL_CH_SWAP_4,
};

MI_U8 JD9365_CMD[] =
{
    0xE0, 1, 0x00,
    0xE1, 1, 0x93,
    0xE2, 1, 0x65,
    0xE3, 1, 0xF8,
    0x80, 1, 0x03,
    0xE0, 1, 0x01,
    //0x4a, 1, 0x35,
    0x00, 1, 0x00,
    0x01, 1, 0x6F,
    0x03, 1, 0x00,
    0x04, 1, 0x6A,
    0x17, 1, 0x00,
    0x18, 1, 0xAF,
    0x19, 1, 0x01,
    0x1A, 1, 0x00,
    0x1B, 1, 0xAF,
    0x1C, 1, 0x01,
    0x1F, 1, 0x3E,
    0x20, 1, 0x28,
    0x21, 1, 0x28,
    0x22, 1, 0x7E,
    0x35, 1, 0x26,
    0x37, 1, 0x09,
    0x38, 1, 0x04,
    0x39, 1, 0x00,
    0x3A, 1, 0x01,
    0x3C, 1, 0x7C,
    0x3D, 1, 0xFF,
    0x3E, 1, 0xFF,
    0x3F, 1, 0x7F,
    0x40, 1, 0x06,
    0x41, 1, 0xA0,
    0x42, 1, 0x81,
    0x43, 1, 0x08,
    0x44, 1, 0x0B,
    0x45, 1, 0x28,
    0x55, 1, 0x01,
    0x57, 1, 0x69,
    0x59, 1, 0x0A,
    0x5A, 1, 0x28,
    0x5B, 1, 0x14,
    0x5D, 1, 0x7C,
    0x5E, 1, 0x65,
    0x5F, 1, 0x55,
    0x60, 1, 0x47,
    0x61, 1, 0x43,
    0x62, 1, 0x32,
    0x63, 1, 0x34,
    0x64, 1, 0x1C,
    0x65, 1, 0x33,
    0x66, 1, 0x31,
    0x67, 1, 0x30,
    0x68, 1, 0x4E,
    0x69, 1, 0x3C,
    0x6A, 1, 0x44,
    0x6B, 1, 0x35,
    0x6C, 1, 0x31,
    0x6D, 1, 0x23,
    0x6E, 1, 0x11,
    0x6F, 1, 0x00,
    0x70, 1, 0x7C,
    0x71, 1, 0x65,
    0x72, 1, 0x55,
    0x73, 1, 0x47,
    0x74, 1, 0x43,
    0x75, 1, 0x32,
    0x76, 1, 0x34,
    0x77, 1, 0x1C,
    0x78, 1, 0x33,
    0x79, 1, 0x31,
    0x7A, 1, 0x30,
    0x7B, 1, 0x4E,
    0x7C, 1, 0x3C,
    0x7D, 1, 0x44,
    0x7E, 1, 0x35,
    0x7F, 1, 0x31,
    0x80, 1, 0x23,
    0x81, 1, 0x11,
    0x82, 1, 0x00,
    0xE0, 1, 0x02,
    0x00, 1, 0x1E,
    0x01, 1, 0x1E,
    0x02, 1, 0x41,
    0x03, 1, 0x41,
    0x04, 1, 0x1F,
    0x05, 1, 0x1F,
    0x06, 1, 0x1F,
    0x07, 1, 0x1F,
    0x08, 1, 0x1F,
    0x09, 1, 0x1F,
    0x0A, 1, 0x1E,
    0x0B, 1, 0x1E,
    0x0C, 1, 0x1F,
    0x0D, 1, 0x47,
    0x0E, 1, 0x47,
    0x0F, 1, 0x45,
    0x10, 1, 0x45,
    0x11, 1, 0x4B,
    0x12, 1, 0x4B,
    0x13, 1, 0x49,
    0x14, 1, 0x49,
    0x15, 1, 0x1F,
    0x16, 1, 0x1E,
    0x17, 1, 0x1E,
    0x18, 1, 0x40,
    0x19, 1, 0x40,
    0x1A, 1, 0x1F,
    0x1B, 1, 0x1F,
    0x1C, 1, 0x1F,
    0x1D, 1, 0x1F,
    0x1E, 1, 0x1F,
    0x1F, 1, 0x1F,
    0x20, 1, 0x1E,
    0x21, 1, 0x1E,
    0x22, 1, 0x1F,
    0x23, 1, 0x46,
    0x24, 1, 0x46,
    0x25, 1, 0x44,
    0x26, 1, 0x44,
    0x27, 1, 0x4A,
    0x28, 1, 0x4A,
    0x29, 1, 0x48,
    0x2A, 1, 0x48,
    0x2B, 1, 0x1F,
    0x2C, 1, 0x1F,
    0x2D, 1, 0x1F,
    0x2E, 1, 0x40,
    0x2F, 1, 0x40,
    0x30, 1, 0x1F,
    0x31, 1, 0x1F,
    0x32, 1, 0x1E,
    0x33, 1, 0x1E,
    0x34, 1, 0x1F,
    0x35, 1, 0x1F,
    0x36, 1, 0x1E,
    0x37, 1, 0x1E,
    0x38, 1, 0x1F,
    0x39, 1, 0x48,
    0x3A, 1, 0x48,
    0x3B, 1, 0x4A,
    0x3C, 1, 0x4A,
    0x3D, 1, 0x44,
    0x3E, 1, 0x44,
    0x3F, 1, 0x46,
    0x40, 1, 0x46,
    0x41, 1, 0x1F,
    0x42, 1, 0x1F,
    0x43, 1, 0x1F,
    0x44, 1, 0x41,
    0x45, 1, 0x41,
    0x46, 1, 0x1F,
    0x47, 1, 0x1F,
    0x48, 1, 0x1E,
    0x49, 1, 0x1E,
    0x4A, 1, 0x1E,
    0x4B, 1, 0x1F,
    0x4C, 1, 0x1E,
    0x4D, 1, 0x1E,
    0x4E, 1, 0x1F,
    0x4F, 1, 0x49,
    0x50, 1, 0x49,
    0x51, 1, 0x4B,
    0x52, 1, 0x4B,
    0x53, 1, 0x45,
    0x54, 1, 0x45,
    0x55, 1, 0x47,
    0x56, 1, 0x47,
    0x57, 1, 0x1F,
    0x58, 1, 0x40,
    0x5B, 1, 0x30,
    0x5C, 1, 0x03,
    0x5D, 1, 0x30,
    0x5E, 1, 0x01,
    0x5F, 1, 0x02,
    0x63, 1, 0x14,
    0x64, 1, 0x6A,
    0x67, 1, 0x73,
    0x68, 1, 0x05,
    0x69, 1, 0x14,
    0x6A, 1, 0x6A,
    0x6B, 1, 0x08,
    0x6C, 1, 0x00,
    0x6D, 1, 0x00,
    0x6E, 1, 0x00,
    0x6F, 1, 0x88,
    0x77, 1, 0xDD,
    0x79, 1, 0x0E,
    0x7A, 1, 0x03,
    0x7D, 1, 0x14,
    0x7E, 1, 0x6A,
    0xE0, 1, 0x04,
    0x09, 1, 0x11,
    0x0E, 1, 0x48,
    0x2B, 1, 0x2B,
    0x2D, 1, 0x03,
    0x2E, 1, 0x44,
    0xE0, 1, 0x00,
    0xE6, 1, 0x02,
    0xE7, 1, 0x0C,

    0x11, 0, 0x00,
    FLAG_DELAY, FLAG_DELAY, 120,
    0x29, 0, 0x00,
    FLAG_DELAY, FLAG_DELAY, 200,

    FLAG_END_OF_TABLE, FLAG_END_OF_TABLE,
};

MI_PANEL_MipiDsiConfig_t stMipiDsiConfig =
{
    //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
    0x05,   0x03,  0x05,  0x0A,     0x0E,     0x03,    0x0B,   0x0A,     0x05,    0x00,
    //Lpx   TaGet  TaSure  TaGo
    0x10, 0x1a,  0x16,   0x32,

    //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
    800,    20,    20,  20,   1280, 4,   10,   30,   0,    60,

    E_MI_PNL_MIPI_DSI_LANE_4,      // MIPnlMipiDsiLaneMode_e enLaneNum;
    E_MI_PNL_MIPI_DSI_RGB888,      // MIPnlMipiDsiFormat_e enFormat;
    E_MI_PNL_MIPI_DSI_SYNC_PULSE,  // MIPnlMipiDsiCtrlMode_e enCtrl;

    JD9365_CMD,
    sizeof(JD9365_CMD),
    1, 0x01AF, 0x01B9, 0x80D2, 9,
};

