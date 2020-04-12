/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
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

#define FLAG_DELAY            0xFE
#define FLAG_END_OF_TABLE     0xFF   // END OF REGISTERS MARKER

const MI_PANEL_ParamConfig_t gstPanelParam =
{
    "RM68200_720x1280",    //const char *pPanelName; ///<  PanelName
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

    6,        //u16 u16HSyncWidth;               ///<  Hsync Width
    60,        //u16 u16HSyncBackPorch;           ///<  Hsync back porch

    40,       //u16 u16VSyncWidth;               ///<  Vsync width
    220,      //u16 u16VSyncBackPorch;           ///<  Vsync back porch

    66,       //u16 u16HStart;                   ///<  HDe start
    0,        //u16 u16VStart;                   ///<  VDe start
    720,      //u16 u16Width;                    ///<  Panel Width
    1280,     //u16 u16Height;                   ///<  Panel Height

    800,      //u16 u16MaxHTotal;                ///<  Max H Total
    800,      //u16 u16HTotal;                   ///<  H Total
    700,      //u16 u16MinHTotal;                ///<  Min H Total

    1750,     //u16 u16MaxVTotal;                ///<  Max V Total
    1650,     //u16 u16VTotal;                   ///<  V Total
    1550,     //u16 u16MinVTotal;                ///<  Min V Total

    84,       //u16 u16MaxDCLK;                  ///<  Max DCLK
    75,       //u16 u16DCLK;                     ///<  DCLK ( Htt * Vtt * Fps)
    65,       //u16 u16MinDCLK;                  ///<  Min DCLK

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
    E_MI_PNL_CH_SWAP_0,
    E_MI_PNL_CH_SWAP_1,
    E_MI_PNL_CH_SWAP_3,
    E_MI_PNL_CH_SWAP_4,//H2->2 4 3 1 0
};

MI_U8 FITI_OTA7001A_Test_Pattern[] =
{
    0xFE, 1, 0x01,
    0x27, 1, 0x0A,
    0x29, 1, 0x0A,
    0x2B, 1, 0xE5,
    0x24, 1, 0xC0,
    0x25, 1, 0x53,
    0x26, 1, 0x00,
    0x16, 1, 0x52, //wrong
    0x2F, 1, 0x54,
    0x34, 1, 0x57,
    0x1B, 1, 0x00,
    0x12, 1, 0x0A,
    0x1A, 1, 0x06,
    0x46, 1, 0x4D,
    0x52, 1, 0x90,
    0x53, 1, 0x00,
    0x54, 1, 0x90,
    0x55, 1, 0x00,
    FLAG_DELAY, FLAG_DELAY, 200,
    0xFE, 1, 0x03,
    0x00, 1, 0x05,
    0x01, 1, 0x16,
    0x02, 1, 0x09,
    0x03, 1, 0x0D,
    0x04, 1, 0x00,
    0x05, 1, 0x00,
    0x06, 1, 0x50,
    0x07, 1, 0x05,
    0x08, 1, 0x16,
    0x09, 1, 0x0B,
    0x0A, 1, 0x0F,
    0x0B, 1, 0x00,
    0x0C, 1, 0x00,
    0x0D, 1, 0x50,
    0x0E, 1, 0x03,
    0x0F, 1, 0x04,
    0x10, 1, 0x05,
    0x11, 1, 0x06,
    0x12, 1, 0x00,
    0x13, 1, 0x54,
    0x14, 1, 0x00,
    0x15, 1, 0xC5,
    0x16, 1, 0x08,
    0x17, 1, 0x07,
    0x18, 1, 0x08,
    0x19, 1, 0x09,
    0x1A, 1, 0x0A,
    0x1B, 1, 0x00,
    0x1C, 1, 0x54,
    0x1D, 1, 0x00,
    0x1E, 1, 0x85,
    0x1F, 1, 0x08,
    0x20, 1, 0x00,
    0x21, 1, 0x00,
    0x22, 1, 0x03,
    0x23, 1, 0x1F,
    0x24, 1, 0x00,
    0x25, 1, 0x28,
    0x26, 1, 0x00,
    0x27, 1, 0x1F,
    0x28, 1, 0x00,
    0x29, 1, 0x28,
    0x2A, 1, 0x00,
    0x2B, 1, 0x00,
    0x2D, 1, 0x00,
    0x2F, 1, 0x00,
    0x30, 1, 0x00,
    0x31, 1, 0x00,
    0x32, 1, 0x00,
    0x33, 1, 0x00,
    0x34, 1, 0x00,
    0x35, 1, 0x00,
    0x36, 1, 0x00,
    0x37, 1, 0x00,
    0x38, 1, 0x00,
    0x39, 1, 0x00,
    0x3A, 1, 0x00,
    0x3B, 1, 0x00,
    0x3D, 1, 0x00,
    0x3F, 1, 0x00,
    0x40, 1, 0x00,
    0x3F, 1, 0x00,
    0x41, 1, 0x00,
    0x42, 1, 0x00,
    0x43, 1, 0x00,
    0x44, 1, 0x00,
    0x45, 1, 0x00,
    0x46, 1, 0x00,
    0x47, 1, 0x00,
    0x48, 1, 0x00,
    0x49, 1, 0x00,
    0x4A, 1, 0x00,
    0x4B, 1, 0x00,
    0x4C, 1, 0x00,
    0x4D, 1, 0x00,
    0x4E, 1, 0x00,
    0x4F, 1, 0x00,
    0x50, 1, 0x00,
    0x51, 1, 0x00,
    0x52, 1, 0x00,
    0x53, 1, 0x00,
    0x54, 1, 0x00,
    0x55, 1, 0x00,
    0x56, 1, 0x00,
    0x58, 1, 0x00,
    0x59, 1, 0x00,
    0x5A, 1, 0x00,
    0x5B, 1, 0x00,
    0x5C, 1, 0x00,
    0x5D, 1, 0x00,
    0x5E, 1, 0x00,
    0x5F, 1, 0x00,
    0x60, 1, 0x00,
    0x61, 1, 0x00,
    0x62, 1, 0x00,
    0x63, 1, 0x00,
    0x64, 1, 0x00,
    0x65, 1, 0x00,
    0x66, 1, 0x00,
    0x67, 1, 0x00,
    0x68, 1, 0x00,
    0x69, 1, 0x00,
    0x6A, 1, 0x00,
    0x6B, 1, 0x00,
    0x6C, 1, 0x00,
    0x6D, 1, 0x00,
    0x6E, 1, 0x00,
    0x6F, 1, 0x00,
    0x70, 1, 0x00,
    0x71, 1, 0x00,
    0x72, 1, 0x00,
    0x73, 1, 0x00,
    0x74, 1, 0x04,
    0x75, 1, 0x04,
    0x76, 1, 0x04,
    0x77, 1, 0x04,
    0x78, 1, 0x00,
    0x79, 1, 0x00,
    0x7A, 1, 0x00,
    0x7B, 1, 0x00,
    0x7C, 1, 0x00,
    0x7D, 1, 0x00,
    0x7E, 1, 0x86,
    0x7F, 1, 0x02,
    0x80, 1, 0x0E,
    0x81, 1, 0x0C,
    0x82, 1, 0x0A,
    0x83, 1, 0x08,
    0x84, 1, 0x3F,
    0x85, 1, 0x3F,
    0x86, 1, 0x3F,
    0x87, 1, 0x3F,
    0x88, 1, 0x3F,
    0x89, 1, 0x3F,
    0x8A, 1, 0x3F,
    0x8B, 1, 0x3F,
    0x8C, 1, 0x3F,
    0x8D, 1, 0x3F,
    0x8E, 1, 0x3F,
    0x8F, 1, 0x3F,
    0x90, 1, 0x00,
    0x91, 1, 0x04,
    0x92, 1, 0x3F,
    0x93, 1, 0x3F,
    0x94, 1, 0x3F,
    0x95, 1, 0x3F,
    0x96, 1, 0x05,
    0x97, 1, 0x01,
    0x98, 1, 0x3F,
    0x99, 1, 0x3F,
    0x9A, 1, 0x3F,
    0x9B, 1, 0x3F,
    0x9C, 1, 0x3F,
    0x9D, 1, 0x3F,
    0x9E, 1, 0x3F,
    0x9F, 1, 0x3F,
    0xA0, 1, 0x3F,
    0xA2, 1, 0x3F,
    0xA3, 1, 0x3F,
    0xA4, 1, 0x3F,
    0xA5, 1, 0x09,
    0xA6, 1, 0x0B,
    0xA7, 1, 0x0D,
    0xA9, 1, 0x0F,
    FLAG_DELAY, FLAG_DELAY, 10,
    0xAA, 1, 0x03, // wrong
    FLAG_DELAY, FLAG_DELAY, 10,
    0xAB, 1, 0x07, //wrong
    FLAG_DELAY, FLAG_DELAY, 10,
    0xAC, 1, 0x01,
    0xAD, 1, 0x05,
    0xAE, 1, 0x0D,
    0xAF, 1, 0x0F,
    0xB0, 1, 0x09,
    0xB1, 1, 0x0B,
    0xB2, 1, 0x3F,
    0xB3, 1, 0x3F,
    0xB4, 1, 0x3F,
    0xB5, 1, 0x3F,
    0xB6, 1, 0x3F,
    0xB7, 1, 0x3F,
    0xB8, 1, 0x3F,
    0xB9, 1, 0x3F,
    0xBA, 1, 0x3F,
    0xBB, 1, 0x3F,
    0xBC, 1, 0x3F,
    0xBD, 1, 0x3F,
    0xBE, 1, 0x07,
    0xBF, 1, 0x03,
    0xC0, 1, 0x3F,
    0xC1, 1, 0x3F,
    0xC2, 1, 0x3F,
    0xC3, 1, 0x3F,
    0xC4, 1, 0x02,
    0xC5, 1, 0x06,
    0xC6, 1, 0x3F,
    0xC7, 1, 0x3F,
    0xC8, 1, 0x3F,
    0xC9, 1, 0x3F,
    0xCA, 1, 0x3F,
    0xCB, 1, 0x3F,
    0xCC, 1, 0x3F,
    0xCD, 1, 0x3F,
    0xCE, 1, 0x3F,
    0xCF, 1, 0x3F,
    0xD0, 1, 0x3F,
    0xD1, 1, 0x3F,
    0xD2, 1, 0x0A,
    0xD3, 1, 0x08,
    0xD4, 1, 0x0E,
    0xD5, 1, 0x0C,
    0xD6, 1, 0x04,
    0xD7, 1, 0x00,
    0xDC, 1, 0x02,
    0xDE, 1, 0x10,
    FLAG_DELAY, FLAG_DELAY, 200,
    0xFE, 1, 0x04,
    0x60, 1, 0x00,
    0x61, 1, 0x0C,
    0x62, 1, 0x14,
    0x63, 1, 0x0F,
    0x64, 1, 0x08,
    0x65, 1, 0x15,
    0x66, 1, 0x0F,
    0x67, 1, 0x0B,
    0x68, 1, 0x17,
    0x69, 1, 0x0D,
    0x6A, 1, 0x10,
    0x6B, 1, 0x09,
    0x6C, 1, 0x0F,
    0x6D, 1, 0x11,
    0x6E, 1, 0x0B,
    0x6F, 1, 0x00,
    0x70, 1, 0x00,
    0x71, 1, 0x0C,
    0x72, 1, 0x14,
    0x73, 1, 0x0F,
    0x74, 1, 0x08,
    0x75, 1, 0x15,
    0x76, 1, 0x0F,
    0x77, 1, 0x0B,
    0x78, 1, 0x17,
    0x79, 1, 0x0D,
    0x7A, 1, 0x10,
    0x7B, 1, 0x09,
    0x7C, 1, 0x0F,
    0x7D, 1, 0x11,
    0x7E, 1, 0x0B,
    0x7F, 1, 0x00,
    FLAG_DELAY, FLAG_DELAY, 200,
    0xFE, 1, 0x0E,
    0x01, 1, 0x75,
    0x49, 1, 0x56,
    FLAG_DELAY, FLAG_DELAY, 200,
    0xFE, 1, 0x00,
    0x58, 1, 0xA9,
    0x11, 0, 0x00,
    FLAG_DELAY, FLAG_DELAY, 200,
    0x29, 0, 0x00,
    FLAG_DELAY, FLAG_DELAY, 200,
    0xFE, 1, 0x0E,
    0x35, 1, 0x80,
    FLAG_END_OF_TABLE, FLAG_END_OF_TABLE,
};


MI_U8 FITI_OTA7001A_CMD[] =
{
    0xFE, 1, 0x01,
    0x27, 1, 0x0A,
    0x29, 1, 0x0A,
    0x2B, 1, 0xE5,
    0x24, 1, 0xC0,
    0x25, 1, 0x53,
    0x26, 1, 0x00,
    0x16, 1, 0x52, //wrong
    0x2F, 1, 0x54,
    0x34, 1, 0x57,
    0x1B, 1, 0x00,
    0x12, 1, 0x0A,
    0x1A, 1, 0x06,
    0x46, 1, 0x4D,
    0x52, 1, 0x90,
    0x53, 1, 0x00,
    0x54, 1, 0x90,
    0x55, 1, 0x00,
    FLAG_DELAY, FLAG_DELAY, 200,
    0xFE, 1, 0x03,
    0x00, 1, 0x05,
    0x01, 1, 0x16,
    0x02, 1, 0x09,
    0x03, 1, 0x0D,
    0x04, 1, 0x00,
    0x05, 1, 0x00,
    0x06, 1, 0x50,
    0x07, 1, 0x05,
    0x08, 1, 0x16,
    0x09, 1, 0x0B,
    0x0A, 1, 0x0F,
    0x0B, 1, 0x00,
    0x0C, 1, 0x00,
    0x0D, 1, 0x50,
    0x0E, 1, 0x03,
    0x0F, 1, 0x04,
    0x10, 1, 0x05,
    0x11, 1, 0x06,
    0x12, 1, 0x00,
    0x13, 1, 0x54,
    0x14, 1, 0x00,
    0x15, 1, 0xC5,
    0x16, 1, 0x08,
    0x17, 1, 0x07,
    0x18, 1, 0x08,
    0x19, 1, 0x09,
    0x1A, 1, 0x0A,
    0x1B, 1, 0x00,
    0x1C, 1, 0x54,
    0x1D, 1, 0x00,
    0x1E, 1, 0x85,
    0x1F, 1, 0x08,
    0x20, 1, 0x00,
    0x21, 1, 0x00,
    0x22, 1, 0x03,
    0x23, 1, 0x1F,
    0x24, 1, 0x00,
    0x25, 1, 0x28,
    0x26, 1, 0x00,
    0x27, 1, 0x1F,
    0x28, 1, 0x00,
    0x29, 1, 0x28,
    0x2A, 1, 0x00,
    0x2B, 1, 0x00,
    0x2D, 1, 0x00,
    0x2F, 1, 0x00,
    0x30, 1, 0x00,
    0x31, 1, 0x00,
    0x32, 1, 0x00,
    0x33, 1, 0x00,
    0x34, 1, 0x00,
    0x35, 1, 0x00,
    0x36, 1, 0x00,
    0x37, 1, 0x00,
    0x38, 1, 0x00,
    0x39, 1, 0x00,
    0x3A, 1, 0x00,
    0x3B, 1, 0x00,
    0x3D, 1, 0x00,
    0x3F, 1, 0x00,
    0x40, 1, 0x00,
    0x3F, 1, 0x00,
    0x41, 1, 0x00,
    0x42, 1, 0x00,
    0x43, 1, 0x00,
    0x44, 1, 0x00,
    0x45, 1, 0x00,
    0x46, 1, 0x00,
    0x47, 1, 0x00,
    0x48, 1, 0x00,
    0x49, 1, 0x00,
    0x4A, 1, 0x00,
    0x4B, 1, 0x00,
    0x4C, 1, 0x00,
    0x4D, 1, 0x00,
    0x4E, 1, 0x00,
    0x4F, 1, 0x00,
    0x50, 1, 0x00,
    0x51, 1, 0x00,
    0x52, 1, 0x00,
    0x53, 1, 0x00,
    0x54, 1, 0x00,
    0x55, 1, 0x00,
    0x56, 1, 0x00,
    0x58, 1, 0x00,
    0x59, 1, 0x00,
    0x5A, 1, 0x00,
    0x5B, 1, 0x00,
    0x5C, 1, 0x00,
    0x5D, 1, 0x00,
    0x5E, 1, 0x00,
    0x5F, 1, 0x00,
    0x60, 1, 0x00,
    0x61, 1, 0x00,
    0x62, 1, 0x00,
    0x63, 1, 0x00,
    0x64, 1, 0x00,
    0x65, 1, 0x00,
    0x66, 1, 0x00,
    0x67, 1, 0x00,
    0x68, 1, 0x00,
    0x69, 1, 0x00,
    0x6A, 1, 0x00,
    0x6B, 1, 0x00,
    0x6C, 1, 0x00,
    0x6D, 1, 0x00,
    0x6E, 1, 0x00,
    0x6F, 1, 0x00,
    0x70, 1, 0x00,
    0x71, 1, 0x00,
    0x72, 1, 0x00,
    0x73, 1, 0x00,
    0x74, 1, 0x04,
    0x75, 1, 0x04,
    0x76, 1, 0x04,
    0x77, 1, 0x04,
    0x78, 1, 0x00,
    0x79, 1, 0x00,
    0x7A, 1, 0x00,
    0x7B, 1, 0x00,
    0x7C, 1, 0x00,
    0x7D, 1, 0x00,
    0x7E, 1, 0x86,
    0x7F, 1, 0x02,
    0x80, 1, 0x0E,
    0x81, 1, 0x0C,
    0x82, 1, 0x0A,
    0x83, 1, 0x08,
    0x84, 1, 0x3F,
    0x85, 1, 0x3F,
    0x86, 1, 0x3F,
    0x87, 1, 0x3F,
    0x88, 1, 0x3F,
    0x89, 1, 0x3F,
    0x8A, 1, 0x3F,
    0x8B, 1, 0x3F,
    0x8C, 1, 0x3F,
    0x8D, 1, 0x3F,
    0x8E, 1, 0x3F,
    0x8F, 1, 0x3F,
    0x90, 1, 0x00,
    0x91, 1, 0x04,
    0x92, 1, 0x3F,
    0x93, 1, 0x3F,
    0x94, 1, 0x3F,
    0x95, 1, 0x3F,
    0x96, 1, 0x05,
    0x97, 1, 0x01,
    0x98, 1, 0x3F,
    0x99, 1, 0x3F,
    0x9A, 1, 0x3F,
    0x9B, 1, 0x3F,
    0x9C, 1, 0x3F,
    0x9D, 1, 0x3F,
    0x9E, 1, 0x3F,
    0x9F, 1, 0x3F,
    0xA0, 1, 0x3F,
    0xA2, 1, 0x3F,
    0xA3, 1, 0x3F,
    0xA4, 1, 0x3F,
    0xA5, 1, 0x09,
    0xA6, 1, 0x0B,
    0xA7, 1, 0x0D,
    0xA9, 1, 0x0F,
    FLAG_DELAY, FLAG_DELAY, 10,
    0xAA, 1, 0x03, // wrong
    FLAG_DELAY, FLAG_DELAY, 10,
    0xAB, 1, 0x07, //wrong
    FLAG_DELAY, FLAG_DELAY, 10,
    0xAC, 1, 0x01,
    0xAD, 1, 0x05,
    0xAE, 1, 0x0D,
    0xAF, 1, 0x0F,
    0xB0, 1, 0x09,
    0xB1, 1, 0x0B,
    0xB2, 1, 0x3F,
    0xB3, 1, 0x3F,
    0xB4, 1, 0x3F,
    0xB5, 1, 0x3F,
    0xB6, 1, 0x3F,
    0xB7, 1, 0x3F,
    0xB8, 1, 0x3F,
    0xB9, 1, 0x3F,
    0xBA, 1, 0x3F,
    0xBB, 1, 0x3F,
    0xBC, 1, 0x3F,
    0xBD, 1, 0x3F,
    0xBE, 1, 0x07,
    0xBF, 1, 0x03,
    0xC0, 1, 0x3F,
    0xC1, 1, 0x3F,
    0xC2, 1, 0x3F,
    0xC3, 1, 0x3F,
    0xC4, 1, 0x02,
    0xC5, 1, 0x06,
    0xC6, 1, 0x3F,
    0xC7, 1, 0x3F,
    0xC8, 1, 0x3F,
    0xC9, 1, 0x3F,
    0xCA, 1, 0x3F,
    0xCB, 1, 0x3F,
    0xCC, 1, 0x3F,
    0xCD, 1, 0x3F,
    0xCE, 1, 0x3F,
    0xCF, 1, 0x3F,
    0xD0, 1, 0x3F,
    0xD1, 1, 0x3F,
    0xD2, 1, 0x0A,
    0xD3, 1, 0x08,
    0xD4, 1, 0x0E,
    0xD5, 1, 0x0C,
    0xD6, 1, 0x04,
    0xD7, 1, 0x00,
    0xDC, 1, 0x02,
    0xDE, 1, 0x10,
    FLAG_DELAY, FLAG_DELAY, 200,
    0xFE, 1, 0x04,
    0x60, 1, 0x00,
    0x61, 1, 0x0C,
    0x62, 1, 0x14,
    0x63, 1, 0x0F,
    0x64, 1, 0x08,
    0x65, 1, 0x15,
    0x66, 1, 0x0F,
    0x67, 1, 0x0B,
    0x68, 1, 0x17,
    0x69, 1, 0x0D,
    0x6A, 1, 0x10,
    0x6B, 1, 0x09,
    0x6C, 1, 0x0F,
    0x6D, 1, 0x11,
    0x6E, 1, 0x0B,
    0x6F, 1, 0x00,
    0x70, 1, 0x00,
    0x71, 1, 0x0C,
    0x72, 1, 0x14,
    0x73, 1, 0x0F,
    0x74, 1, 0x08,
    0x75, 1, 0x15,
    0x76, 1, 0x0F,
    0x77, 1, 0x0B,
    0x78, 1, 0x17,
    0x79, 1, 0x0D,
    0x7A, 1, 0x10,
    0x7B, 1, 0x09,
    0x7C, 1, 0x0F,
    0x7D, 1, 0x11,
    0x7E, 1, 0x0B,
    0x7F, 1, 0x00,
    FLAG_DELAY, FLAG_DELAY, 200,
    0xFE, 1, 0x0E,
    0x01, 1, 0x75,
    0x49, 1, 0x56,
    FLAG_DELAY, FLAG_DELAY, 200,
    0xFE, 1, 0x00,
    0x58, 1, 0xA9,
    0x11, 0, 0x00,
    FLAG_DELAY, FLAG_DELAY, 200,
    0x29, 0, 0x00,
    FLAG_DELAY, FLAG_DELAY, 200,
    FLAG_END_OF_TABLE, FLAG_END_OF_TABLE,
};

typedef enum
{
    Sync_Pulse_RGB888 = 0,
    Sync_Pulse_RGB565,
    Sync_Pulse_RGB666,
    Sync_Pulse_Loosely_RGB666,
    Sync_Event_RGB888,
    Sync_Event_RGB565,
    Sync_Event_RGB666,
    Sync_Event_Loosely_RGB666,
    Burst_RGB888,
    Burst_RGB565,
    Burst_RGB666,
    Burst_Loosely_RGB666,
}eMipiPanelConfigIndex;

const MI_PANEL_MipiDsiConfig_t astMipiDsiConfig[]=
{
    [Sync_Pulse_RGB888] = {
        //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
          0x05,   0x03,  0x05,  0x0A,     0x0E,     0x03,    0x0B,   0x0A,     0x05,    0x00,
        //Lpx   TaGet  TaSure  TaGo
          0x10, 0x1a,  0x16,   0x32,

        //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
          720,  5,    25,   5,    1280,  40, 220, 110,  0,    60,

        E_MI_PNL_MIPI_DSI_LANE_4,      // MIPnlMipiDsiLaneMode_e enLaneNum;
        E_MI_PNL_MIPI_DSI_RGB888,      // MIPnlMipiDsiFormat_e enFormat;
        E_MI_PNL_MIPI_DSI_SYNC_PULSE,  // MIPnlMipiDsiCtrlMode_e enCtrl;

        //FITI_OTA7001A_Test_Pattern,
        //sizeof(FITI_OTA7001A_CMD),
        FITI_OTA7001A_CMD,
        sizeof(FITI_OTA7001A_CMD),
        1, 0x01AF, 0x01B9, 0x80D2,
    },
    [Sync_Pulse_RGB565] = {
        //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
        0x05,   0x03,  0x05,  0x0A,     0x0E,     0x03,    0x0B,   0x0A,     0x05,    0x00,
        //Lpx   TaGet  TaSure  TaGo
        0x10, 0x1a,  0x16,   0x32,

        //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
        720,  6,    630,  90,   1280, 6,   4,   29,   0,    60,

        E_MI_PNL_MIPI_DSI_LANE_4,      // MIPnlMipiDsiLaneMode_e enLaneNum;
        E_MI_PNL_MIPI_DSI_RGB565,      // MIPnlMipiDsiFormat_e enFormat;
        E_MI_PNL_MIPI_DSI_SYNC_PULSE,  // MIPnlMipiDsiCtrlMode_e enCtrl;

        //FITI_OTA7001A_Test_Pattern,
        //sizeof(FITI_OTA7001A_CMD),
        FITI_OTA7001A_CMD,
        sizeof(FITI_OTA7001A_CMD),
    },
    [Sync_Pulse_RGB666] = {
        //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
        0x05,   0x03,  0x05,  0x0A,     0x0E,     0x03,    0x0B,   0x0A,     0x05,    0x00,
        //Lpx   TaGet  TaSure  TaGo
        0x10, 0x1a,  0x16,   0x32,

        //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
        720,  6,    638,   90,   1280, 6,   4,   18,   0,    60,

        E_MI_PNL_MIPI_DSI_LANE_4,      // MIPnlMipiDsiLaneMode_e enLaneNum;
        E_MI_PNL_MIPI_DSI_RGB666,      // MIPnlMipiDsiFormat_e enFormat;
        E_MI_PNL_MIPI_DSI_SYNC_PULSE,  // MIPnlMipiDsiCtrlMode_e enCtrl;

        //FITI_OTA7001A_Test_Pattern,
        //sizeof(FITI_OTA7001A_CMD),
        FITI_OTA7001A_CMD,
        sizeof(FITI_OTA7001A_CMD),
    },
    [Sync_Pulse_Loosely_RGB666] = {
        //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
        0x05,   0x03,  0x05,  0x0A,     0x0E,     0x03,    0x0B,   0x0A,     0x05,    0x00,
        //Lpx   TaGet  TaSure  TaGo
        0x10, 0x1a,  0x16,   0x32,

        //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
        720,  6,    654,  90,   1280, 6,   4,   29,   0,    60,

        E_MI_PNL_MIPI_DSI_LANE_4,        // MIPnlMipiDsiLaneMode_e enLaneNum;
        E_MI_PNL_MIPI_DSI_LOOSELY_RGB666,// MIPnlMipiDsiFormat_e enFormat;
        E_MI_PNL_MIPI_DSI_SYNC_PULSE,    // MIPnlMipiDsiCtrlMode_e enCtrl;

        //FITI_OTA7001A_Test_Pattern,
        //sizeof(FITI_OTA7001A_CMD),
        FITI_OTA7001A_CMD,
        sizeof(FITI_OTA7001A_CMD),
    },
    [Sync_Event_RGB888] = {
        //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
        0x05,   0x03,  0x05,  0x0A,     0x0E,     0x03,    0x0B,   0x0A,     0x05,    0x00,
        //Lpx   TaGet  TaSure  TaGo
        0x10, 0x1a,  0x16,   0x32,

        //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
        720,  6,    76,   90,   1280, 6,   4,   29,   0,    60,

        E_MI_PNL_MIPI_DSI_LANE_4,      // MIPnlMipiDsiLaneMode_e enLaneNum;
        E_MI_PNL_MIPI_DSI_RGB888,      // MIPnlMipiDsiFormat_e enFormat;
        E_MI_PNL_MIPI_DSI_SYNC_EVENT,  // MIPnlMipiDsiCtrlMode_e enCtrl;

        //FITI_OTA7001A_Test_Pattern,
        //sizeof(FITI_OTA7001A_CMD),
        FITI_OTA7001A_CMD,
        sizeof(FITI_OTA7001A_CMD),
    },
    [Sync_Event_RGB565] = {
        //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
        0x05,   0x03,  0x05,  0x0A,     0x0E,     0x03,    0x0B,   0x0A,     0x05,    0x00,
        //Lpx   TaGet  TaSure  TaGo
        0x10, 0x1a,  0x16,   0x32,

        //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
        720,  6,    74,   90,   1280, 6,   4,   29,   0,    60,

        E_MI_PNL_MIPI_DSI_LANE_4,      // MIPnlMipiDsiLaneMode_e enLaneNum;
        E_MI_PNL_MIPI_DSI_RGB565,      // MIPnlMipiDsiFormat_e enFormat;
        E_MI_PNL_MIPI_DSI_SYNC_EVENT,  // MIPnlMipiDsiCtrlMode_e enCtrl;

        //FITI_OTA7001A_Test_Pattern,
        //sizeof(FITI_OTA7001A_CMD),
        FITI_OTA7001A_CMD,
        sizeof(FITI_OTA7001A_CMD),
    },
    [Sync_Event_RGB666] = {
        //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
        0x05,   0x03,  0x05,  0x0A,     0x0E,     0x03,    0x0B,   0x0A,     0x05,    0x00,
        //Lpx   TaGet  TaSure  TaGo
        0x10, 0x1a,  0x16,   0x32,

        //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
        720,  6,    74,   80,   1280, 6,   4,   18,   0,    60,

        E_MI_PNL_MIPI_DSI_LANE_4,      // MIPnlMipiDsiLaneMode_e enLaneNum;
        E_MI_PNL_MIPI_DSI_RGB666,      // MIPnlMipiDsiFormat_e enFormat;
        E_MI_PNL_MIPI_DSI_SYNC_EVENT,  // MIPnlMipiDsiCtrlMode_e enCtrl;

        //FITI_OTA7001A_Test_Pattern,
        //sizeof(FITI_OTA7001A_CMD),
        FITI_OTA7001A_CMD,
        sizeof(FITI_OTA7001A_CMD),
    },
    [Sync_Event_Loosely_RGB666] = {
        //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
        0x05,   0x03,  0x05,  0x0A,     0x0E,     0x03,    0x0B,   0x0A,     0x05,    0x00,
        //Lpx   TaGet  TaSure  TaGo
        0x10, 0x1a,  0x16,   0x32,

        //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
        720,  6,    76,   90,   1280, 6,   4,   29,   0,    60,

        E_MI_PNL_MIPI_DSI_LANE_4,        // MIPnlMipiDsiLaneMode_e enLaneNum;
        E_MI_PNL_MIPI_DSI_LOOSELY_RGB666,// MIPnlMipiDsiFormat_e enFormat;
        E_MI_PNL_MIPI_DSI_SYNC_EVENT,    // MIPnlMipiDsiCtrlMode_e enCtrl;

        //FITI_OTA7001A_Test_Pattern,
        //sizeof(FITI_OTA7001A_CMD),
        FITI_OTA7001A_CMD,
        sizeof(FITI_OTA7001A_CMD),
    },
    [Burst_RGB888] = {
        //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
        0x05,   0x03,  0x05,  0x0A,     0x0E,     0x03,    0x0B,   0x0A,     0x05,    0x00,
        //Lpx   TaGet  TaSure  TaGo
        0x10, 0x1a,  0x16,   0x32,

        //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
        720,  6,    76,   90,   1280, 6,   4,   29,   0,    60,

        E_MI_PNL_MIPI_DSI_LANE_4,      // MIPnlMipiDsiLaneMode_e enLaneNum;
        E_MI_PNL_MIPI_DSI_RGB888,      // MIPnlMipiDsiFormat_e enFormat;
        E_MI_PNL_MIPI_DSI_BURST_MODE,  // MIPnlMipiDsiCtrlMode_e enCtrl;

        //FITI_OTA7001A_Test_Pattern,
        //sizeof(FITI_OTA7001A_CMD),
        FITI_OTA7001A_CMD,
        sizeof(FITI_OTA7001A_CMD),
    },
    [Burst_RGB565] = {
        //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
        0x05,   0x03,  0x05,  0x0A,     0x0E,     0x03,    0x0B,   0x0A,     0x05,    0x00,
        //Lpx   TaGet  TaSure  TaGo
        0x10, 0x1a,  0x16,   0x32,

        //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
        720,  6,    74,   90,   1280, 6,   4,   29,   0,    60,

        E_MI_PNL_MIPI_DSI_LANE_4,      // MIPnlMipiDsiLaneMode_e enLaneNum;
        E_MI_PNL_MIPI_DSI_RGB565,      // MIPnlMipiDsiFormat_e enFormat;
        E_MI_PNL_MIPI_DSI_BURST_MODE,  // MIPnlMipiDsiCtrlMode_e enCtrl;

        //FITI_OTA7001A_Test_Pattern,
        //sizeof(FITI_OTA7001A_CMD),
        FITI_OTA7001A_CMD,
        sizeof(FITI_OTA7001A_CMD),
    },
    [Burst_RGB666] = {
        //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
        0x05,   0x03,  0x05,  0x0A,     0x0E,     0x03,    0x0B,   0x0A,     0x05,    0x00,
        //Lpx   TaGet  TaSure  TaGo
        0x10, 0x1a,  0x16,   0x32,

        //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
        720,  6,    74,   70,   1280, 6,   4,   18,   0,    60,

        E_MI_PNL_MIPI_DSI_LANE_4,      // MIPnlMipiDsiLaneMode_e enLaneNum;
        E_MI_PNL_MIPI_DSI_RGB666,      // MIPnlMipiDsiFormat_e enFormat;
        E_MI_PNL_MIPI_DSI_BURST_MODE,  // MIPnlMipiDsiCtrlMode_e enCtrl;

        //FITI_OTA7001A_Test_Pattern,
        //sizeof(FITI_OTA7001A_CMD),
        FITI_OTA7001A_CMD,
        sizeof(FITI_OTA7001A_CMD),
    },
    [Burst_Loosely_RGB666] = {
        //HsTrail HsPrpr HsZero ClkHsPrpr ClkHsExit ClkTrail ClkZero ClkHsPost DaHsExit ContDet
        0x05,   0x03,  0x05,  0x0A,     0x0E,     0x03,    0x0B,   0x0A,     0x05,    0x00,
        //Lpx   TaGet  TaSure  TaGo
        0x10, 0x1a,  0x16,   0x32,

        //Hac,  Hpw,  Hbp,  Hfp,  Vac,  Vpw, Vbp, Vfp,  Bllp, Fps
        720,  6,    76,   90,   1280, 6,   4,   29,   0,    60,

        E_MI_PNL_MIPI_DSI_LANE_4,        // MIPnlMipiDsiLaneMode_e enLaneNum;
        E_MI_PNL_MIPI_DSI_LOOSELY_RGB666,// MIPnlMipiDsiFormat_e enFormat;
        E_MI_PNL_MIPI_DSI_BURST_MODE,    // MIPnlMipiDsiCtrlMode_e enCtrl;

        //FITI_OTA7001A_Test_Pattern,
        //sizeof(FITI_OTA7001A_CMD),
        FITI_OTA7001A_CMD,
        sizeof(FITI_OTA7001A_CMD),
    },
};


