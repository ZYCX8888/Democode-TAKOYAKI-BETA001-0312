#include "mi_panel.h"
#include "mi_panel_datatype.h"

#define FLAG_DELAY            0xFE
#define FLAG_END_OF_TABLE     0xFF   // END OF REGISTERS MARKER

MI_PANEL_ParamConfig_t stPanelParam =
{
    "FC070JDI4001_1200x1920",    //const char *pPanelName; ///<  PanelName
    0,                    //MI_U8             MI_U8Dither;         ///<  Diether On?off
    E_MI_PNL_LINK_MIPI_DSI, //MIPnlLinkType_e eLinkType;     ///<  Panel LinkType

    1,        //MI_U8 MI_U8DualPort      :1;          ///<  DualPort on/off
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

    1,     //MI_U8 MI_U8DCKLCurrent;              ///< PANEL_DCLK_CURRENT
    1,     //MI_U8 MI_U8DECurrent;                ///< PANEL_DE_CURRENT
    1,     //MI_U8 MI_U8ODDDataCurrent;           ///< PANEL_ODD_DATA_CURRENT
    1,     //MI_U8 MI_U8EvenDataCurrent;          ///< PANEL_EVEN_DATA_CURRENT

    30,       //u16 u16OnTiming1;                ///<  time between panel & data while turn on power
    400,      //u16 u16OnTiming2;                ///<  time between data & back light while turn on power
    80,       //u16 u16OffTiming1;               ///<  time between back light & data while turn off power
    30,       //u16 u16OffTiming2;               ///<  time between data & panel while turn off power

    45,        //u16 u16HSyncWidth;               ///<  Hsync Width
    10,        //u16 u16HSyncBackPorch;           ///<  Hsync back porch

    2,       //u16 u16VSyncWidth;               ///<  Vsync width
    4,      //u16 u16VSyncBackPorch;           ///<  Vsync back porch

    0,       //u16 u16HStart;                   ///<  HDe start
    0,        //u16 u16VStart;                   ///<  VDe start
    1200,      //u16 u16Width;                    ///<  Panel Width
    1920,     //u16 u16Height;                   ///<  Panel Height

    0,      //u16 u16MaxHTotal;                ///<  Max H Total
    1360,      //u16 u16HTotal;                   ///<  H Total
    0,      //u16 u16MinHTotal;                ///<  Min H Total

    0,     //u16 u16MaxVTotal;                ///<  Max V Total
    1928,     //u16 u16VTotal;                   ///<  V Total
    0,     //u16 u16MinVTotal;                ///<  Min V Total

    0,       //u16 u16MaxDCLK;                  ///<  Max DCLK
    157,       //u16 u16DCLK;                     ///<  DCLK ( Htt * Vtt * Fps)
    0,       //u16 u16MinDCLK;                  ///<  Min DCLK

    25,    //u16 u16SpreadSpectrumStep;       ///<  Step of SSC
    192,    //u16 u16SpreadSpectrumSpan;       ///<  Span of SSC

    160,      //MI_U8 MI_U8DimmingCtl;                 ///<  Dimming Value
    255,      //MI_U8 MI_U8MaxPWMVal;                  ///<  Max Dimming Value
    80,      //MI_U8 MI_U8MinPWMVal;                  ///<  Min Dimming Value

    0,                            //MI_U8 MI_U8DeinterMode   :1;                  ///<  DeInter Mode
    E_MI_PNL_ASPECT_RATIO_WIDE, //MIPnlAspectRatio_e ePanelAspectRatio; ///<  Aspec Ratio

    0,                            //u16 u16LVDSTxSwapValue;         // LVDS Swap Value
    E_MI_PNL_TI_8BIT_MODE,      //MIPnlTiBitMode_e eTiBitMode;  // Ti Bit Mode
    E_MI_PNL_OUTPUT_8BIT_MODE, //MIPnlOutputFormatBitMode_e eOutputFormatBitMode;

    0,        //MI_U8 MI_U8SwapOdd_RG    :1;          ///<  Swap Odd RG
    0,        //MI_U8 MI_U8SwapEven_RG   :1;          ///<  Swap Even RG
    0,        //MI_U8 MI_U8SwapOdd_GB    :1;          ///<  Swap Odd GB
    0,        //MI_U8 MI_U8SwapEven_GB   :1;          ///<  Swap Even GB

    1,        //MI_U8 MI_U8DoubleClk     :1;                      ///<  Double CLK On/off
    0x1C848E, //u32 u32MaxSET;                              ///<  Max Lpll Set
    0x18EB59, //u32 u32MinSET;                              ///<  Min Lpll Set
    E_MI_PNL_CHG_VTOTAL, //MIPnlOutputTimingMode_e eOutTimingMode;   ///<  Define which panel output timing change mode is used to change VFreq for same panel
    0,                     //MI_U8 MI_U8NoiseDith     :1;                      ///<  Noise Dither On/Off
    E_MI_PNL_CH_SWAP_2,
    E_MI_PNL_CH_SWAP_4,
    E_MI_PNL_CH_SWAP_3,
    E_MI_PNL_CH_SWAP_1,
    E_MI_PNL_CH_SWAP_0,//H2->2 4 3 1 0
};

MI_U8 FC070JDI4001_INIT_CMD[] =
{
	FLAG_DELAY, FLAG_DELAY, 50,
	0x01, 0, 0x00,
	FLAG_DELAY, FLAG_DELAY, 50,
	0xB0, 1, 0x04,
	0xD6, 1, 0x01,
	0x51, 1, 0xFF,
	0x53, 1, 0x0C,
	0x55, 1, 0x00,
	0xB3, 6, 0x14,0x08,0x00,0x00,0x00,0x00,
	0xB4, 1, 0x0C,
	0xB6, 2, 0x3A,0xD3,
	0xB7, 1, 0x00,
	0xB8, 6, 0x07,0x90,0x1E,0x10,0x1E,0x32,
	0xB9, 6, 0x07,0x82,0x3C,0x10,0x3C,0x87,
	0xBA, 6, 0x07,0x78,0x64,0x10,0x64,0xB4,
	0xC0, 1, 0xFF,
	0xC1, 36,0x04,0x61,0x00,0x20,0x8C,0xA4,0xD6,0xFF,0xFF,0xFF,0xFF,0x7F,0x73,0xEF,0xB9,0xF6,0xFF,0xFF,0xFF,0xFF,0xBF,0x54,0x22,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x62,0x03,0x00,0x22,0x00,0x01,
	0xC2, 9, 0x31,0xF7,0x80,0x00,0x09,0x00,0x04,0x00,0x00,
	0xC3, 3, 0x00,0x00,0x00,
	0xC4, 12,0x70,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x04,0x00,
	0xC6, 20,0x78,0x73,0x75,0x0A,0x14,0x0B,0x16,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1C,0x17,0x09,
	0xC7, 30,0x06,0x0D,0x16,0x20,0x2F,0x3E,0x48,0x58,0x3C,0x44,0x4F,0x5C,0x65,0x6D,0x76,0x06,0x0D,0x16,0x20,0x2F,0x3E,0x48,0x58,0x3C,0x44,0x4F,0x5C,0x65,0x6D,0x76,
	0xC8, 19,0x01,0x00,0x00,0x00,0x00,0xFC,0x00,0x00,0x00,0x00,0x00,0xFC,0x00,0x00,0x00,0x00,0x00,0xFC,0x00,
	0xCA, 32,0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x08,0x20,0x80,0x80,0x0A,0x4A,0x37,0xA0,0x55,0xF8,0x0C,0x0C,0x20,0x10,0x3F,0x3F,0x00,0x00,0x10,0x10,0x3F,0x3F,0x3F,0x3F,
	0xCB, 12,0x3F,0xC0,0x0F,0xF0,0x03,0x00,0x03,0x00,0x03,0x00,0x00,0xC0,
	0xCC, 1, 0x11,
	0xCE, 24,0x35,0x40,0x43,0x49,0x55,0x62,0x71,0x82,0x94,0xA8,0xB9,0xCB,0xDB,0xE9,0xF5,0xFC,0xFF,0x04,0x00,0x04,0x04,0x00,0x20,0x01,
	0xD0, 5, 0x44,0x81,0xBB,0x15,0x94,
	0xD3, 25,0x0B,0x33,0xBF,0xBB,0xB3,0x33,0x33,0x37,0x00,0x01,0x00,0xA0,0x98,0xA0,0x00,0x39,0x39,0x33,0x3B,0x37,0x72,0x07,0x3D,0xBF,0x99,
	0xD5, 7, 0x06,0x00,0x00,0x01,0x15,0x01,0x15,
	0x29, 0, 0x00,
	0x11, 0, 0x00,
    FLAG_DELAY, FLAG_DELAY, 200,
    FLAG_END_OF_TABLE, FLAG_END_OF_TABLE,
};

MI_PANEL_MipiDsiConfig_t stMipiDsiConfig =
{
    //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
    5,      3,     5,     10,       14,       3,       12,     10,       5,       0,
    //Lpx   TaGet  TaSure  TaGo
    16,   26,    24,     50,

    //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
    1200,   45,    10,  105,  1920,  2,   4,  2,    0,    60,

    E_MI_PNL_MIPI_DSI_LANE_4,      // MIPnlMipiDsiLaneMode_e enLaneNum;
    E_MI_PNL_MIPI_DSI_RGB888,      // MIPnlMipiDsiFormat_e enFormat;
    E_MI_PNL_MIPI_DSI_SYNC_EVENT,  // MIPnlMipiDsiCtrlMode_e enCtrl;

    FC070JDI4001_INIT_CMD,
    sizeof(FC070JDI4001_INIT_CMD),
    1, 0x01AF, 0x01B9, 0x80D2, 7,
};

