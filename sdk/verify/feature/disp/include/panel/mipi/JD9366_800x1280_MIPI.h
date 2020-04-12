#ifndef _JD9366_800x1280_H
#define JD9366_800x1280_H
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>

#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>

#include "mi_panel.h"
#include "mi_panel_datatype.h"
#include "mi_panel_test.h"

#define FLAG_DELAY            0xFE
#define FLAG_END_OF_TABLE     0xFF   // END OF REGISTERS MARKER

const MI_PANEL_ParamConfig_t gstPanelParam_JD9366_800_1280_mipi =
{
    "JD9366_800x1280",    //const char *pPanelName; ///<  PanelName
    0,                    //MI_U8             MI_U8Dither;         ///<  Diether On?off
    E_MI_PNL_LINK_MIPI_DSI, //MIPnlLinkType_e eLinkType;     ///<  Panel LinkType

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

    0x01,     //MI_U8 MI_U8DCKLCurrent;              ///< PANEL_DCLK_CURRENT
    0x01,     //MI_U8 MI_U8DECurrent;                ///< PANEL_DE_CURRENT
    0x01,     //MI_U8 MI_U8ODDDataCurrent;           ///< PANEL_ODD_DATA_CURRENT
    0x01,     //MI_U8 MI_U8EvenDataCurrent;          ///< PANEL_EVEN_DATA_CURRENT

    30,       //u16 u16OnTiming1;                ///<  time between panel & data while turn on power
    400,      //u16 u16OnTiming2;                ///<  time between data & back light while turn on power
    80,       //u16 u16OffTiming1;               ///<  time between back light & data while turn off power
    30,       //u16 u16OffTiming2;               ///<  time between data & panel while turn off power

    18,        //u16 u16HSyncWidth;               ///<  Hsync Width
    18,       //u16 u16HSyncBackPorch;           ///<  Hsync back porch

    4,       //u16 u16VSyncWidth;               ///<  Vsync width
    10,      //u16 u16VSyncBackPorch;           ///<  Vsync back porch

    66,       //u16 u16HStart;                   ///<  HDe start
    0,        //u16 u16VStart;                   ///<  VDe start
    800,      //u16 u16Width;                    ///<  Panel Width
    1280,     //u16 u16Height;                   ///<  Panel Height

    888,      //u16 u16MaxHTotal;                ///<  Max H Total
    854,      //u16 u16HTotal;                   ///<  H Total
    816,      //u16 u16MinHTotal;                ///<  Min H Total

    1530,     //u16 u16MaxVTotal;                ///<  Max V Total
    1314,     //u16 u16VTotal;                   ///<  V Total
    1288,     //u16 u16MinVTotal;                ///<  Min V Total

    81,       //u16 u16MaxDCLK;                  ///<  Max DCLK
    68,       //u16 u16DCLK;                     ///<  DCLK ( Htt * Vtt * Fps)
    63,       //u16 u16MinDCLK;                  ///<  Min DCLK

    0x0019,    //u16 u16SpreadSpectrumStep;       ///<  Step of SSC
    0x00C0,    //u16 u16SpreadSpectrumSpan;       ///<  Span of SSC

    0xA0,      //MI_U8 MI_U8DimmingCtl;                 ///<  Dimming Value
    0xFF,      //MI_U8 MI_U8MaxPWMVal;                  ///<  Max Dimming Value
    0x50,      //MI_U8 MI_U8MinPWMVal;                  ///<  Min Dimming Value

    0,                            //MI_U8 MI_U8DeinterMode   :1;                  ///<  DeInter Mode
    E_MI_PNL_ASPECT_RATIO_WIDE, //MIPnlAspectRatio_e ePanelAspectRatio; ///<  Aspec Ratio

    0,                            //u16 u16LVDSTxSwapValue;         // LVDS Swap Value
    E_MI_PNL_TI_8BIT_MODE,      //MIPnlTiBitMode_e eTiBitMode;  // Ti Bit Mode
    E_MI_PNL_OUTPUT_10BIT_MODE, //MIPnlOutputFormatBitMode_e eOutputFormatBitMode;

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
    E_MI_PNL_CH_SWAP_4,
    E_MI_PNL_CH_SWAP_3,
    E_MI_PNL_CH_SWAP_1,
    E_MI_PNL_CH_SWAP_0,
};

MI_U8 JD9366_CMD[] =
{
    0xE0, 0x01, 0x00,
    0xE1, 0x01, 0x93,
    0xE2, 0x01, 0x65,
    0xE3, 0x01, 0xF8,
    0x80, 0x01, 0x03,
    0xE0, 0x01, 0x02,
    0x00, 0x01, 0x09,
    0x01, 0x01, 0x05,
    0x02, 0x01, 0x08,
    0x03, 0x01, 0x04,
    0x04, 0x01, 0x06,
    0x05, 0x01, 0x0A,
    0x06, 0x01, 0x07,
    0x07, 0x01, 0x0B,
    0x08, 0x01, 0x00,
    0x0F, 0x01, 0x17,
    0x10, 0x01, 0x37,
    0x12, 0x01, 0x1F,
    0x13, 0x01, 0x1E,
    0x14, 0x01, 0x10,
    0x16, 0x01, 0x09,
    0x17, 0x01, 0x05,
    0x18, 0x01, 0x08,
    0x19, 0x01, 0x04,
    0x1A, 0x01, 0x06,
    0x1B, 0x01, 0x0A,
    0x1C, 0x01, 0x07,
    0x1D, 0x01, 0x0B,
    0x1E, 0x01, 0x00,
    0x25, 0x01, 0x17,
    0x26, 0x01, 0x37,
    0x28, 0x01, 0x1F,
    0x29, 0x01, 0x1E,
    0x2A, 0x01, 0x10,
    0x58, 0x01, 0x01,
    0x59, 0x01, 0x00,
    0x5A, 0x01, 0x00,
    0x5B, 0x01, 0x00,
    0x5C, 0x01, 0x01,
    0x5D, 0x01, 0x70,
    0x5E, 0x01, 0x00,
    0x5F, 0x01, 0x00,
    0x60, 0x01, 0x40,
    0x61, 0x01, 0x00,
    0x62, 0x01, 0x00,
    0x63, 0x01, 0x65,
    0x64, 0x01, 0x65,
    0x65, 0x01, 0x45,
    0x66, 0x01, 0x09,
    0x67, 0x01, 0x73,
    0x68, 0x01, 0x05,
    0x69, 0x01, 0x00,
    0x6A, 0x01, 0x64,
    0x6B, 0x01, 0x00,
    0x6C, 0x01, 0x00,
    0x6D, 0x01, 0x00,
    0x6E, 0x01, 0x00,
    0x6F, 0x01, 0x88,
    0x70, 0x01, 0x00,
    0x71, 0x01, 0x00,
    0x72, 0x01, 0x06,
    0x73, 0x01, 0x7B,
    0x74, 0x01, 0x00,
    0x75, 0x01, 0x80,
    0x76, 0x01, 0x00,
    0x77, 0x01, 0x05,
    0x78, 0x01, 0x18,
    0x79, 0x01, 0x00,
    0x7A, 0x01, 0x00,
    0x7B, 0x01, 0x00,
    0x7C, 0x01, 0x00,
    0x7D, 0x01, 0x03,
    0x7E, 0x01, 0x7B,
    0xE0, 0x01, 0x01,
    0x01, 0x01, 0x7B,
    0x17, 0x01, 0x00,
    0x18, 0x01, 0xC2,
    0x19, 0x01, 0x01,
    0x1A, 0x01, 0x00,
    0x1B, 0x01, 0xC2,
    0x1C, 0x01, 0x01,
    0x1F, 0x01, 0x3F,
    0x20, 0x01, 0x24,
    0x21, 0x01, 0x24,
    0x22, 0x01, 0x0E,
    0x37, 0x01, 0x09,
    0x38, 0x01, 0x04,
    0x39, 0x01, 0x00,
    0x3A, 0x01, 0x01,
    0x3C, 0x01, 0x78,
    0x3D, 0x01, 0xFF,
    0x3E, 0x01, 0xFF,
    0x3F, 0x01, 0xFF,
    0x40, 0x01, 0x06,
    0x41, 0x01, 0xA0,
    0x43, 0x01, 0x16,
    0x44, 0x01, 0x07,
    0x45, 0x01, 0x24,
    0x55, 0x01, 0x01,
    0x56, 0x01, 0x01,
    0x57, 0x01, 0x69,
    0x58, 0x01, 0x0A,
    0x59, 0x01, 0x2A,
    0x5A, 0x01, 0x28,
    0x5B, 0x01, 0x0F,
    0x5D, 0x01, 0x7C,
    0x5E, 0x01, 0x67,
    0x5F, 0x01, 0x55,
    0x60, 0x01, 0x48,
    0x61, 0x01, 0x3E,
    0x62, 0x01, 0x2D,
    0x63, 0x01, 0x30,
    0x64, 0x01, 0x16,
    0x65, 0x01, 0x2C,
    0x66, 0x01, 0x28,
    0x67, 0x01, 0x26,
    0x68, 0x01, 0x42,
    0x69, 0x01, 0x30,
    0x6A, 0x01, 0x38,
    0x6B, 0x01, 0x29,
    0x6C, 0x01, 0x26,
    0x6D, 0x01, 0x1C,
    0x6E, 0x01, 0x0D,
    0x6F, 0x01, 0x00,
    0x70, 0x01, 0x7C,
    0x71, 0x01, 0x69,
    0x72, 0x01, 0x55,
    0x73, 0x01, 0x48,
    0x74, 0x01, 0x3E,
    0x75, 0x01, 0x2D,
    0x76, 0x01, 0x30,
    0x77, 0x01, 0x16,
    0x78, 0x01, 0x2C,
    0x79, 0x01, 0x28,
    0x7A, 0x01, 0x26,
    0x7B, 0x01, 0x42,
    0x7C, 0x01, 0x30,
    0x7D, 0x01, 0x38,
    0x7E, 0x01, 0x29,
    0x7F, 0x01, 0x26,
    0x80, 0x01, 0x1C,
    0x81, 0x01, 0x0D,
    0x82, 0x01, 0x00,
    0xE0, 0x01, 0x04,
    0x2D, 0x01, 0x03,
    0x2B, 0x01, 0x2B,
    0x2E, 0x01, 0x44,
    0xE0, 0x01, 0x00,
    0xE6, 0x01, 0x02,
    0xE7, 0x01, 0x02,
    0x11, 0, 0x00,
    FLAG_DELAY, FLAG_DELAY, 120,
    0x29, 0, 0x00,
    FLAG_DELAY, FLAG_DELAY, 5,
    FLAG_END_OF_TABLE, FLAG_END_OF_TABLE,
};



const MI_PANEL_MipiDsiConfig_t astMipiDsiConfig_JD9366_800_1280_mipi[]=
{
    [Sync_Pulse_RGB888] = {
        //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
        0x05,   0x03,  0x05,  0x0A,     0x0E,     0x03,    0x0B,   0x0A,     0x05,    0x00,
        //Lpx   TaGet  TaSure  TaGo
        0x10, 0x1a,  0x16,   0x32,

        //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
        800,  18,    488,  55,   1280, 6,   4,   29,   0,    60,

        E_MI_PNL_MIPI_DSI_LANE_4,      // MIPnlMipiDsiLaneMode_e enLaneNum;
        E_MI_PNL_MIPI_DSI_RGB888,      // MIPnlMipiDsiFormat_e enFormat;
        E_MI_PNL_MIPI_DSI_SYNC_PULSE,  // MIPnlMipiDsiCtrlMode_e enCtrl;

        //FITI_OTA7001A_Test_Pattern,
        //sizeof(JD9366_CMD),
        JD9366_CMD,
        sizeof(JD9366_CMD),
    },
};

#endif
