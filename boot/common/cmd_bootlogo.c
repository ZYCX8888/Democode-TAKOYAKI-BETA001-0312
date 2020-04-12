

//-------------------------------------------------------------------------------------------------
//  Include Files
//-------------------------------------------------------------------------------------------------
#include <common.h>
#include <command.h>
#include <malloc.h>
#include <stdlib.h>
#include "asm/arch/mach/ms_types.h"
#include "asm/arch/mach/platform.h"
#include "asm/arch/mach/io.h"

#include <ubi_uboot.h>
#include <cmd_osd.h>

#if defined(CONFIG_SSTAR_DISP)
#include "mhal_common.h"
#include "mhal_disp_datatype.h"
#include "mhal_disp.h"
#endif

#if defined(CONFIG_SSTAR_PNL)
#include "mhal_pnl_datatype.h"
#include "mhal_pnl.h"
#endif

#if defined(CONFIG_SSTAR_HDMITX)
#include "mhal_hdmitx_datatype.h"
#include "mhal_hdmitx.h"
#endif

#if defined(CONFIG_SSTAR_JPD)
#include "jinclude.h"
#include "jpeglib.h"
#endif

#if defined(CONFIG_MS_PARTITION)
#include "part_mxp.h"
#endif

#if defined(CONFIG_SSTAR_RGN)
#include "mhal_rgn_datatype.h"
#include "mhal_rgn.h"
#endif
//-------------------------------------------------------------------------------------------------
//  Defines & Macro
//-------------------------------------------------------------------------------------------------
#define PNL_TEST_MD_EN             0

#define BOOTLOGO_DBG_LEVEL_ERR     0x01
#define BOOTLOGO_DBG_LEVEL_INFO    0x02
#define BOOTLOGO_DBG_LEVEL_JPD     0x04

#define FLAG_DELAY            0xFE
#define FLAG_END_OF_TABLE     0xFF   // END OF REGISTERS MARKER

#define BOOTLOGO_DBG_LEVEL          0 // BOOTLOGO_DBG_LEVEL_INFO

#define BOOTLOGO_DBG(dbglv, _fmt, _args...)    \
    do                                          \
    if(dbglv & u32BootlogDvgLevel)              \
    {                                           \
            printf(_fmt, ## _args);             \
    }while(0)



#define DISP_DEVICE_NULL     0
#define DISP_DEVICE_HDMI     1
#define DISP_DEVICE_VGA      2
#define DISP_DEVICE_LCD      4

#define BOOTLOGO_TIMING_NUM  14


#define BOOTLOGO_NOT_ZOOM     0

#define BOOTLOGO_VIRTUAL_ADDRESS_OFFSET 0x20000000

//-------------------------------------------------------------------------------------------------
//  structure & Enu
//-------------------------------------------------------------------------------------------------
typedef struct
{
    u32 u32PnlParamCfgSize;
    u8 *pPnlParamCfg;
    u32 u32MipiDsiCfgSize;
    u8 *pMipiDsiCfg;
}PnlConfig_t;


typedef struct
{
    void *pInBuff;
    u64  u64InBuffAddr;
    u32  u32InBuffSize;
    u64  u64OutBuffAddr;
    u32  u32OutBuffSize;
    u16  u16DisplayWidth;
    u16  u16DisplayHeight;
    u8   u8DisplayRate;
    u8   u8Interface;
#if defined(CONFIG_SSTAR_PNL)
    u8 panelname[20];
    MhalPnlParamConfig_t stPnlPara;
    MhalPnlMipiDsiConfig_t stMipiDsiCfg;
#endif
}BootlogoImgConfig_t;


typedef struct
{
    MHAL_DISP_DeviceTiming_e enTiminId;
    u16 u16HsyncWidht;
    u16 u16HsyncBacPorch;

    u16 u16VsyncWidht;
    u16 u16VsyncBacPorch;

    u16 u16Hstart;
    u16 u16Vstart;
    u16 u16Hactive;
    u16 u16Vactive;

    u16 u16Htotal;
    u16 u16Vtotal;
    u16 u16DclkMhz;
}DisplayLogoTimingConfig_t;

//-------------------------------------------------------------------------------------------------
//  Variable
//-------------------------------------------------------------------------------------------------
u32 u32BootlogDvgLevel = BOOTLOGO_DBG_LEVEL_ERR | BOOTLOGO_DBG_LEVEL_INFO | BOOTLOGO_DBG_LEVEL_JPD;

DisplayLogoTimingConfig_t stTimingTable[BOOTLOGO_TIMING_NUM] =
{
    {   E_MHAL_DISP_OUTPUT_1080P60,
        44, 148,  5,  36, 192, 41, 1920, 1080, 2200, 1125, 148 },

    {   E_MHAL_DISP_OUTPUT_1080P50,
        44,  148, 5, 36, 192, 41, 1920, 1080, 2640, 1125, 148 },

    {   E_MHAL_DISP_OUTPUT_720P50,
        40, 220, 5, 20, 260, 25, 1280, 720, 1980, 750, 74},

    {   E_MHAL_DISP_OUTPUT_720P60,
        40, 220,5,20, 260, 25, 1280, 720, 1650, 750, 74},

    {   E_MHAL_DISP_OUTPUT_480P60,
        62, 60, 6, 30, 122, 36, 720, 480, 858, 525, 27},

    {   E_MHAL_DISP_OUTPUT_576P50,
        64, 68, 4, 39, 132, 44, 720, 5760, 864, 625, 27},

    {   E_MHAL_DISP_OUTPUT_1024x768_60,
        136, 160, 6, 29, 296, 35, 1024, 768, 1344, 806, 65},

    {   E_MHAL_DISP_OUTPUT_1366x768_60,
        143, 215, 3, 24, 358, 27, 1366, 768, 1792, 798, 86},

    {   E_MHAL_DISP_OUTPUT_1440x900_60,
        152, 232, 6, 25, 384, 31, 1440, 900, 1904, 934, 106},

    {   E_MHAL_DISP_OUTPUT_1280x800_60,
        128, 200, 6, 22, 328, 28, 1280, 800, 1680, 831, 84},

    {   E_MHAL_DISP_OUTPUT_1280x1024_60,
        112, 248, 3, 38, 360, 41, 1280, 1024, 1688, 1066, 108},

    {   E_MHAL_DISP_OUTPUT_1680x1050_60,
        176, 280, 6, 30, 456, 36, 1680, 1050, 2240, 1089, 146},

    {   E_MHAL_DISP_OUTPUT_1600x1200_60,
        192, 304, 3, 46, 496, 49, 1600, 1200, 2160, 1250, 162},

    {   E_MHAL_DISP_OUTPUT_USER,
        48, 46,  4,  23, 98, 27, 800, 480, 928, 525, 43},
};

#if PNL_TEST_MD_EN
MhalPnlParamConfig_t stTtl00x480Param =
{
    "TTL_800x480_60",         // m_pPanelName
    0,                        //
    0,    // m_bPanelDither
    0,    // m_ePanelLinkType  0:TTL,1:LVDS,8:DAC_P

    1,    // m_bPanelDualPort
    0,    // m_bPanelSwapPort          //
    0,    // m_bPanelSwapOdd_ML
    0,    // m_bPanelSwapEven_ML
    0,    // m_bPanelSwapOdd_RB
    0,    // m_bPanelSwapEven_RB


    0,    // u8SwapLVDS_POL;          ///<  Swap LVDS Channel Polarity
    0,    // u8SwapLVDS_CH;           ///<  Swap LVDS channel
    0,    // u8PDP10BIT;              ///<  PDP 10bits on/off
    0,    // u8LVDS_TI_MODE;          ///<  Ti Mode On/Off

    0,    // m_ucPanelDCLKDelay
    0,    // m_bPanelInvDCLK
    0,    // m_bPanelInvDE
    0,    // m_bPanelInvHSync
    0,    // m_bPanelInvVSync
          //
    1,    // m_ucPanelDCKLCurrent
    1,    // m_ucPanelDECurrent
    1,    // m_ucPanelODDDataCurrent
    1,    // _ucPanelEvenDataCurrent
          //
    30,   // m_wPanelOnTiming1
    400,  // m_wPanelOnTiming2
    80,   // _wPanelOffTiming1
    30,   // m_wPanelOffTiming2
          //
    48,   // m_ucPanelHSyncWidth
    46,   // m_ucPanelHSyncBackPorch
          //
    4,    // m_ucPanelVSyncWidth
    23,   // m_ucPanelVBackPorch
          //
    98,   // m_wPanelHStart
    27,   // m_wPanelVStart
          //
    800,  // m_wPanelWidth
    480,  // m_wPanelHeight
          //
    978,  // m_wPanelMaxHTotal
    928,  // m_wPanelHTotal
    878,  // m_wPanelMinHTotal
          //
    818,  // m_wPanelMaxVTotal
    525,  // m_wPanelVTotal
    718,  // m_wPanelMinVTotal
          //
    49,   // m_dwPanelMaxDCLK
    29,   // m_dwPanelDCLK
    37,   // m_dwPanelMinDCLK
          //
    25,   // m_wSpreadSpectrumStep
    192,  // m_wSpreadSpectrumSpan
          //
    160,  // m_ucDimmingCtl
    255,  // m_ucMaxPWMVal
    80,   // m_ucMinPWMVal
          //
    0,    // m_bPanelDeinterMode

    1,    // m_ucPanelAspectRatio

    0,   //u16LVDSTxSwapValue
    2,   // m_ucTiBitMode      TI_10BIT_MODE = 0    TI_8BIT_MODE  = 2    TI_6BIT_MODE  = 3

    2, // m_ucOutputFormatBitMode  10BIT_MODE = 0  6BIT_MODE  = 1   8BIT_MODE  = 2 565BIT_MODE =3

    0, // m_bPanelSwapOdd_RG
    0, // m_bPanelSwapEven_RG
    0, // m_bPanelSwapOdd_GB
    0, // m_bPanelSwapEven_GB

    1, //m_bPanelDoubleClk
    0x001c848e,  //m_dwPanelMaxSET
    0x0018eb59,  //m_dwPanelMinSET

    2, // m_ucOutTimingMode   DCLK=0, HTOTAL=1, VTOTAL=2

    0, // m_bPanelNoiseDith

    0, // m_bPanelChannelSwap0
    1, // m_bPanelChannelSwap1
    2, // m_bPanelChannelSwap2
    3, // m_bPanelChannelSwap3
    4, // m_bPanelChannelSwap4
};


MhalPnlParamConfig_t stRm6820Param =
{
    "Rm6820",         // m_pPanelName
    0,                        //
    0,    // m_bPanelDither
    11,    // m_ePanelLinkType  0:TTL,1:LVDS,8:DAC_P, 11 MIPI_DSI

    1,    // m_bPanelDualPort
    0,    // m_bPanelSwapPort          //
    0,    // m_bPanelSwapOdd_ML
    0,    // m_bPanelSwapEven_ML
    0,    // m_bPanelSwapOdd_RB
    0,    // m_bPanelSwapEven_RB


    0,    // u8SwapLVDS_POL;          ///<  Swap LVDS Channel Polarity
    0,    // u8SwapLVDS_CH;           ///<  Swap LVDS channel
    0,    // u8PDP10BIT;              ///<  PDP 10bits on/off
    0,    // u8LVDS_TI_MODE;          ///<  Ti Mode On/Off

    0,    // m_ucPanelDCLKDelay
    0,    // m_bPanelInvDCLK
    0,    // m_bPanelInvDE
    0,    // m_bPanelInvHSync
    0,    // m_bPanelInvVSync
          //
    1,    // m_ucPanelDCKLCurrent
    1,    // m_ucPanelDECurrent
    1,    // m_ucPanelODDDataCurrent
    1,    // _ucPanelEvenDataCurrent

    30,   // m_wPanelOnTiming1
    400,  // m_wPanelOnTiming2
    80,   // _wPanelOffTiming1
    30,   // m_wPanelOffTiming2

    6,    // m_ucPanelHSyncWidth
    60,  // m_ucPanelHSyncBackPorch

    40,    // m_ucPanelVSyncWidth
    220,   // m_ucPanelVBackPorch
          //
    66,   // m_wPanelHStart
    260,   // m_wPanelVStart
          //
    720,  // m_wPanelWidth
    1280,  // m_wPanelHeight
          //
    850,  // m_wPanelMaxHTotal
    830,  // m_wPanelHTotal
    750,  // m_wPanelMinHTotal
          //
    1750,  // m_wPanelMaxVTotal
    1650,  // m_wPanelVTotal
    1550,  // m_wPanelMinVTotal
          //
    89,   // m_dwPanelMaxDCLK
    79,   // m_dwPanelDCLK
    69,   // m_dwPanelMinDCLK
          //
    25,   // m_wSpreadSpectrumStep
    192,  // m_wSpreadSpectrumSpan
          //
    160,  // m_ucDimmingCtl
    255,  // m_ucMaxPWMVal
    80,   // m_ucMinPWMVal
          //
    0,    // m_bPanelDeinterMode

    1,    // m_ucPanelAspectRatio

    0,   //u16LVDSTxSwapValue
    2,   // m_ucTiBitMode      TI_10BIT_MODE = 0    TI_8BIT_MODE  = 2    TI_6BIT_MODE  = 3

    2, // m_ucOutputFormatBitMode  10BIT_MODE = 0  6BIT_MODE  = 1   8BIT_MODE  = 2 565BIT_MODE =3

    0, // m_bPanelSwapOdd_RG
    0, // m_bPanelSwapEven_RG
    0, // m_bPanelSwapOdd_GB
    0, // m_bPanelSwapEven_GB

    1, //m_bPanelDoubleClk
    0x001c848e,  //m_dwPanelMaxSET
    0x0018eb59,  //m_dwPanelMinSET

    2, // m_ucOutTimingMode   DCLK=0, HTOTAL=1, VTOTAL=2

    0, // m_bPanelNoiseDith

    2, // m_bPanelChannelSwap0
    4, // m_bPanelChannelSwap1
    3, // m_bPanelChannelSwap2
    1, // m_bPanelChannelSwap3
    0, // m_bPanelChannelSwap4
};

u8 Rm6820TestCmd[] =
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


u8 Rm6820Cmd[] =
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


MhalPnlMipiDsiConfig_t stRm6820MipiCfg =
{
    5,     // HsTrail
    3,     // HsPrpr
    5,     // HsZero
    10,    // ClkHsPrpr
    14,    // ClkHsExit
    3,     // ClkTrail
    12,    // ClkZero
    10,    // ClkHsPost
    5,     // DaHsExit
    0,     // ContDet

    16,    // Lpx
    26,    // TaGet
    24,    // TaSure
    50,    // TaGo

    720,   // Hactive
    6,     // Hpw
    60,    // Hbp
    44,    // Hfp
    1280,  // Vactive
    40,    // Vpw
    220,   // Vbp
    110,   // Vfp
    0,     // Bllp
    60,    // Fps

    4,     // LaneNum
    3,     // Format   0:RGB565, 1:RGB666, 2:Loosely_RGB666, 3:RGB888
    1,     // CtrlMode 1:Sync pulse 2:Sync_event, 3:Burst
    Rm6820TestCmd,
    sizeof(Rm6820TestCmd),
};

#endif

#if defined(CONFIG_CMD_MTDPARTS)
#include <jffs2/jffs2.h>
/* partition handling routines */
int mtdparts_init(void);
int find_dev_and_part(const char *id, struct mtd_device **dev,
		u8 *part_num, struct part_info **part);
#endif


//-------------------------------------------------------------------------------------------------
//  Functions
//-------------------------------------------------------------------------------------------------
MS_S32 BootLogoMemAlloc(MS_U8 *pu8Name, MS_U32 size, unsigned long long *pu64PhyAddr)
{
    return 0;
}

MS_S32 BootLogoMemRelease(unsigned long long u64PhyAddr)
{
    return 0;
}

#if defined(CONFIG_SSTAR_DISP)
MHAL_DISP_DeviceTiming_e _BootLogoGetTiminId(u16 u16Width, u16 u16Height, u8 u8Rate)
{
    MHAL_DISP_DeviceTiming_e enTiming;
    enTiming =  ((u16Width) == 1920 && (u16Height) == 1080 && (u8Rate) == 24) ? E_MHAL_DISP_OUTPUT_1080P24 :
                ((u16Width) == 1920 && (u16Height) == 1080 && (u8Rate) == 25) ? E_MHAL_DISP_OUTPUT_1080P25 :
                ((u16Width) == 1920 && (u16Height) == 1080 && (u8Rate) == 30) ? E_MHAL_DISP_OUTPUT_1080P30 :
                ((u16Width) == 1920 && (u16Height) == 1080 && (u8Rate) == 50) ? E_MHAL_DISP_OUTPUT_1080P50 :
                ((u16Width) == 1920 && (u16Height) == 1080 && (u8Rate) == 60) ? E_MHAL_DISP_OUTPUT_1080P60 :
                ((u16Width) == 1280 && (u16Height) == 720  && (u8Rate) == 50) ? E_MHAL_DISP_OUTPUT_720P50  :
                ((u16Width) == 1280 && (u16Height) == 720  && (u8Rate) == 60) ? E_MHAL_DISP_OUTPUT_720P60  :
                ((u16Width) == 720  && (u16Height) == 480  && (u8Rate) == 60) ? E_MHAL_DISP_OUTPUT_480P60  :
                ((u16Width) == 720  && (u16Height) == 576  && (u8Rate) == 60) ? E_MHAL_DISP_OUTPUT_576P50  :
                ((u16Width) == 640  && (u16Height) == 480  && (u8Rate) == 60) ? E_MHAL_DISP_OUTPUT_640x480_60   :
                ((u16Width) == 800  && (u16Height) == 600  && (u8Rate) == 60) ? E_MHAL_DISP_OUTPUT_800x600_60   :
                ((u16Width) == 1280 && (u16Height) == 1024 && (u8Rate) == 60) ? E_MHAL_DISP_OUTPUT_1280x1024_60 :
                ((u16Width) == 1366 && (u16Height) == 768  && (u8Rate) == 60) ? E_MHAL_DISP_OUTPUT_1366x768_60  :
                ((u16Width) == 1440 && (u16Height) == 800  && (u8Rate) == 60) ? E_MHAL_DISP_OUTPUT_1440x900_60  :
                ((u16Width) == 1280 && (u16Height) == 800  && (u8Rate) == 60) ? E_MHAL_DISP_OUTPUT_1280x800_60  :
                ((u16Width) == 3840 && (u16Height) == 2160 && (u8Rate) == 30) ? E_MHAL_DISP_OUTPUT_3840x2160_30 :
                                                                                E_MHAL_DISP_OUTPUT_MAX;
    return enTiming;
}
#endif

#if defined(CONFIG_SSTAR_HDMITX)
MhaHdmitxTimingResType_e _BootLogoGetHdmitxTimingId(u16 u16Width, u16 u16Height, u8 u8Rate)
{
    MhaHdmitxTimingResType_e enTiming;
    enTiming =  ((u16Width) == 1920 && (u16Height) == 1080 && (u8Rate) == 24) ? E_MHAL_HDMITX_RES_1920X1080P_24HZ :
                ((u16Width) == 1920 && (u16Height) == 1080 && (u8Rate) == 25) ? E_MHAL_HDMITX_RES_1920X1080P_25HZ :
                ((u16Width) == 1920 && (u16Height) == 1080 && (u8Rate) == 30) ? E_MHAL_HDMITX_RES_1920X1080P_30HZ :
                ((u16Width) == 1920 && (u16Height) == 1080 && (u8Rate) == 50) ? E_MHAL_HDMITX_RES_1920X1080P_50HZ :
                ((u16Width) == 1920 && (u16Height) == 1080 && (u8Rate) == 60) ? E_MHAL_HDMITX_RES_1920X1080P_60HZ :
                ((u16Width) == 1280 && (u16Height) == 720  && (u8Rate) == 50) ? E_MHAL_HDMITX_RES_1280X720P_50HZ  :
                ((u16Width) == 1280 && (u16Height) == 720  && (u8Rate) == 60) ? E_MHAL_HDMITX_RES_1280X720P_60HZ  :
                ((u16Width) == 720  && (u16Height) == 480  && (u8Rate) == 60) ? E_MHAL_HDMITX_RES_720X480P_60HZ  :
                ((u16Width) == 720  && (u16Height) == 576  && (u8Rate) == 60) ? E_MHAL_HDMITX_RES_720X576P_50HZ  :
                ((u16Width) == 640  && (u16Height) == 480  && (u8Rate) == 60) ? E_MHAL_HDMITX_RES_MAX   :
                ((u16Width) == 800  && (u16Height) == 600  && (u8Rate) == 60) ? E_MHAL_HDMITX_RES_MAX   :
                ((u16Width) == 1280 && (u16Height) == 1024 && (u8Rate) == 60) ? E_MHAL_HDMITX_RES_1280X1024P_60HZ :
                ((u16Width) == 1366 && (u16Height) == 768  && (u8Rate) == 60) ? E_MHAL_HDMITX_RES_1366X768P_60HZ  :
                ((u16Width) == 1440 && (u16Height) == 900  && (u8Rate) == 60) ? E_MHAL_HDMITX_RES_1440X900P_60HZ  :
                ((u16Width) == 1280 && (u16Height) == 800  && (u8Rate) == 60) ? E_MHAL_HDMITX_RES_1280X800P_60HZ  :
                ((u16Width) == 3840 && (u16Height) == 2160 && (u8Rate) == 30) ? E_MHAL_HDMITX_RES_MAX :
                                                                                E_MHAL_HDMITX_RES_MAX;
    return enTiming;
}
#endif




int _BootLogoGetImageConfig(BootlogoImgConfig_t *pCfg)
{
    //Length
    #define LOGO_HEADER_MAGIC_PREFIX_LEN                (4)
    #define LOGO_HEADER_OUT_BUFFER_SIZE_LEN             (4)
    #define LOGO_HEADER_OUT_BUFFER_ADDR_LEN             (8)
    #define LOGO_HEADER_DISPLAY_WIDTH_LEN               (2)
    #define LOGO_HEADER_DISPLAY_HEIGHT_LEN              (2)
    #define LOGO_HEADER_DISPLAY_RATE_LEN                (1)
    #define LOGO_HEADER_DISPLAY_INTERFACE_LEN           (1)
    #define LOGO_HEADER_RESERVED_LEN                    (10)

#if defined(CONFIG_SSTAR_PNL)
    #define LOGO_HEADER_PANEL_NAME_LEN                  (20)
    #define LOGO_HEADER_PANEL_INIT_PARA_LEN             (sizeof(MhalPnlParamConfig_t))
    #define LOGO_HEADER_PANEL_MIPI_DST_CONFIG_LEN       (sizeof(MhalPnlMipiDsiConfig_t))
    #define LOGO_HEADER_SIZE                            (52 + LOGO_HEADER_PANEL_INIT_PARA_LEN + LOGO_HEADER_PANEL_MIPI_DST_CONFIG_LEN)
#else
    #define LOGO_HEADER_SIZE                            (32)
#endif

    // Offset
    #define LOGO_HEADER_MAGIC_PREFIX_OFFSET             (0)
    #define LOGO_HEADER_OUT_BUFFER_SIZE_OFFSET          (LOGO_HEADER_MAGIC_PREFIX_OFFSET + LOGO_HEADER_MAGIC_PREFIX_LEN)
    #define LOGO_HEADER_OUT_BUFFER_ADDR_OFFSET          (LOGO_HEADER_OUT_BUFFER_SIZE_OFFSET + LOGO_HEADER_OUT_BUFFER_SIZE_LEN)
    #define LOGO_HEADER_DISPLAY_WIDTH_OFFSET            (LOGO_HEADER_OUT_BUFFER_ADDR_OFFSET + LOGO_HEADER_OUT_BUFFER_ADDR_LEN)
    #define LOGO_HEADER_DISPLAY_HEIGHT_OFFSET           (LOGO_HEADER_DISPLAY_WIDTH_OFFSET + LOGO_HEADER_DISPLAY_WIDTH_LEN)
    #define LOGO_HEADER_DISPLAY_FPS_OFFSET              (LOGO_HEADER_DISPLAY_HEIGHT_OFFSET + LOGO_HEADER_DISPLAY_HEIGHT_LEN)
    #define LOGO_HEADER_INTERFACE_ID_OFFSET             (LOGO_HEADER_DISPLAY_FPS_OFFSET + LOGO_HEADER_DISPLAY_RATE_LEN)
    #define LOGO_HEADER_RESERVED_OFFSET                 (LOGO_HEADER_INTERFACE_ID_OFFSET + LOGO_HEADER_DISPLAY_INTERFACE_LEN)
#if defined(CONFIG_SSTAR_PNL)
    #define LOGO_HEADER_PANEL_NAME_OFFSET               (LOGO_HEADER_RESERVED_OFFSET + LOGO_HEADER_RESERVED_LEN)
    #define LOGO_HEADER_PANEL_INIT_PARA_OFFSET          (LOGO_HEADER_PANEL_NAME_OFFSET + LOGO_HEADER_PANEL_NAME_LEN)
    #define LOGO_HEADER_PANEL_MIPI_DST_CONFIG_OFFSET    (LOGO_HEADER_PANEL_INIT_PARA_OFFSET + LOGO_HEADER_PANEL_INIT_PARA_LEN)
#endif

    #define LOGO_FLAHS_BASE     0x14000000

    u64     start, size;
	char strENVName[] = "LOGO";
    int idx;
#if defined(CONFIG_CMD_MTDPARTS) || defined(CONFIG_MS_SPINAND)
    struct mtd_device *dev;
    struct part_info *part;
    u8 pnum;
    int ret;

    ret = mtdparts_init();
    if (ret)
        return ret;

    ret = find_dev_and_part(strENVName, &dev, &pnum, &part);
    if (ret)
    {
        return ret;
    }

    if (dev->id->type != MTD_DEV_TYPE_NAND)
    {
        puts("not a NAND device\n");
        return -1;
    }

    start = part->offset;
    size = part->size;
#elif defined(CONFIG_MS_PARTITION)
    mxp_record rec;
	mxp_load_table();
	idx=mxp_get_record_index(strENVName);
	if(idx<0)
	{
        BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "can not found mxp record: %s\n", strENVName);
        return FALSE;
	}

	if(0 != mxp_get_record_by_index(idx,&rec))
    {
        BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "failed to get MXP record with name: %s\n", strENVName);
        return 0;
    }
    start = rec.start;
    size = rec.size;
#else
    start = 0;
    size = 0;
    return 0;
#endif

	{
		BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_INFO, "%s in flash offset=0x%llx size=0x%llx\n",strENVName , start, size);

		pCfg->pInBuff = malloc(size);
		if(pCfg->pInBuff == NULL)
		{
		    BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "allocate buffer fail\n");
            return 0;
		}
    #if defined(CONFIG_CMD_MTDPARTS) || defined(CONFIG_MS_SPINAND)
        char  cmd_str[128];
        sprintf(cmd_str, "nand read.e 0x%p %s", pCfg->pInBuff, strENVName);
        run_command(cmd_str, 0);
    #else
        //sprintf(cmd_str, "sf probe; sf read 0x%p 0x%p 0x%p", pCfg->pInBuff, start, size);
        //run_command(cmd_str, 0);
        memcpy(pCfg->pInBuff, (void*)(U32)(start | LOGO_FLAHS_BASE), size);
    #endif

        flush_cache((U32)pCfg->pInBuff, size);

        //Parsing Header
        for(idx=0; idx<4; idx++)
        {
            if( strENVName[idx] != *((U8 *)(pCfg->pInBuff+idx)))
            {
                BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "Header check fail\n");
                return 0;
            }
        }

        pCfg->u32OutBuffSize    = *((u32 *)(pCfg->pInBuff +  LOGO_HEADER_OUT_BUFFER_SIZE_OFFSET));
        pCfg->u64OutBuffAddr    = *((u64 *)(pCfg->pInBuff +  LOGO_HEADER_OUT_BUFFER_ADDR_OFFSET));
        pCfg->u16DisplayWidth   = *((u16 *)(pCfg->pInBuff +  LOGO_HEADER_DISPLAY_WIDTH_OFFSET));
        pCfg->u16DisplayHeight  = *((u16 *)(pCfg->pInBuff +  LOGO_HEADER_DISPLAY_HEIGHT_OFFSET));
        pCfg->u8DisplayRate     = *((u8 *) (pCfg->pInBuff +  LOGO_HEADER_DISPLAY_FPS_OFFSET));
        pCfg->u8Interface       = *((u8 *) (pCfg->pInBuff +  LOGO_HEADER_INTERFACE_ID_OFFSET));
#if defined(CONFIG_SSTAR_PNL)
        memcpy(pCfg->panelname, pCfg->pInBuff + LOGO_HEADER_PANEL_NAME_OFFSET, LOGO_HEADER_PANEL_NAME_LEN);
        memcpy(&pCfg->stPnlPara, pCfg->pInBuff + LOGO_HEADER_PANEL_INIT_PARA_OFFSET, LOGO_HEADER_PANEL_INIT_PARA_LEN);
        memcpy(&pCfg->stMipiDsiCfg, pCfg->pInBuff + LOGO_HEADER_PANEL_MIPI_DST_CONFIG_OFFSET, LOGO_HEADER_PANEL_MIPI_DST_CONFIG_LEN);
        pCfg->u64InBuffAddr     =  (U32)pCfg->pInBuff + LOGO_HEADER_SIZE + pCfg->stMipiDsiCfg.u32CmdBufSize;
        pCfg->u32InBuffSize     =  (U32)(size - LOGO_HEADER_SIZE + pCfg->stMipiDsiCfg.u32CmdBufSize);
        pCfg->stMipiDsiCfg.pu8CmdBuf = malloc(pCfg->stMipiDsiCfg.u32CmdBufSize);
        if (pCfg->stMipiDsiCfg.pu8CmdBuf == NULL)
        {
            printf("mipi dii cfg malloc error!!\n");
            return -1;
        }
        memcpy(pCfg->stMipiDsiCfg.pu8CmdBuf, pCfg->pInBuff + LOGO_HEADER_SIZE, pCfg->stMipiDsiCfg.u32CmdBufSize);
        printf("Panel name %s\n", pCfg->panelname);
        printf("mipi cmd buf size %d\n", pCfg->stMipiDsiCfg.u32CmdBufSize);
        printf("PNL para size %d\n", sizeof(MhalPnlParamConfig_t));
        printf("PNL mipi size %d\n", sizeof(MhalPnlMipiDsiConfig_t));
#else
        pCfg->u64InBuffAddr     =  (U32)pCfg->pInBuff + LOGO_HEADER_SIZE;
        pCfg->u32InBuffSize     =  (U32)(size - LOGO_HEADER_SIZE);
#endif
        BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_INFO, "InBuf:(%x), IN(%llx %x), OUT:(%llx, %x), DISP(%x %x %x), Interface:%x\n",
            (u32)pCfg->pInBuff,
            pCfg->u64InBuffAddr, pCfg->u32InBuffSize,
            pCfg->u64OutBuffAddr, pCfg->u32OutBuffSize,
            pCfg->u16DisplayWidth, pCfg->u16DisplayHeight,
            pCfg->u8DisplayRate, pCfg->u8Interface);
    }

    return 1;
}

void _BootLogoDispPnlInit(void)
{
#if defined(CONFIG_SSTAR_DISP)
    MHAL_DISP_PanelConfig_t stPnlCfg[BOOTLOGO_TIMING_NUM];
    u16 i;


    if( sizeof(stTimingTable)/sizeof(DisplayLogoTimingConfig_t) > BOOTLOGO_TIMING_NUM)
    {
        BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "%s %d:: Timing Talbe is bigger than %d\n",
            __FUNCTION__, __LINE__, BOOTLOGO_TIMING_NUM);
        return;
    }
    memset(stPnlCfg, 0, sizeof(MHAL_DISP_PanelConfig_t)*BOOTLOGO_TIMING_NUM);
    for(i=0; i<BOOTLOGO_TIMING_NUM; i++)
    {
        stPnlCfg[i].bValid = 1;
        stPnlCfg[i].eTiming = stTimingTable[i].enTiminId;
        stPnlCfg[i].stPanelAttr.m_ucPanelHSyncWidth     = stTimingTable[i].u16HsyncWidht;
        stPnlCfg[i].stPanelAttr.m_ucPanelHSyncBackPorch = stTimingTable[i].u16HsyncBacPorch;
        stPnlCfg[i].stPanelAttr.m_ucPanelVSyncWidth     = stTimingTable[i].u16VsyncWidht;
        stPnlCfg[i].stPanelAttr.m_ucPanelVBackPorch     = stTimingTable[i].u16VsyncBacPorch;
        stPnlCfg[i].stPanelAttr.m_wPanelHStart          = stTimingTable[i].u16Hstart;
        stPnlCfg[i].stPanelAttr.m_wPanelVStart          = stTimingTable[i].u16Vstart;
        stPnlCfg[i].stPanelAttr.m_wPanelWidth           = stTimingTable[i].u16Hactive;
        stPnlCfg[i].stPanelAttr.m_wPanelHeight          = stTimingTable[i].u16Vactive;
        stPnlCfg[i].stPanelAttr.m_wPanelHTotal          = stTimingTable[i].u16Htotal;
        stPnlCfg[i].stPanelAttr.m_wPanelVTotal          = stTimingTable[i].u16Vtotal;
        stPnlCfg[i].stPanelAttr.m_dwPanelDCLK           = stTimingTable[i].u16DclkMhz;
    }


    MHAL_DISP_InitPanelConfig(stPnlCfg, BOOTLOGO_TIMING_NUM);
#endif
}

void _BootLogoYuv444ToYuv420(u8 *pu8InBuf, u8 *pu8OutBuf, u16 u16Width, u16 u16Height)
{
    u16 i, j;
    u32 u32YDesIdx, u32UVDesIdx;
    u32 u32YSrcIdx;;
    u8 *pu8DesY = NULL, *pu8DesUV = NULL;;
    u8 *pu8SrcYUV = NULL;

    pu8SrcYUV = pu8InBuf;

    pu8DesY = pu8OutBuf;
    pu8DesUV = pu8DesY + u16Width * u16Height;

    u32UVDesIdx = 0;
    u32YDesIdx = 0;
    u32YSrcIdx = 0;

	BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_JPD,"%s %d:: 444 To 422, In:%x, Out:%x, Width:%d, Height:%d\n",
        __FUNCTION__, __LINE__,
	    (u32)pu8InBuf, (u32)pu8OutBuf, u16Width, u16Height);

    for(i=0; i<u16Height; i++)
    {
        for(j=0; j<u16Width; j++)
        {
            //printf("(%d %d):: SrcY:%d, DstY:%d\n", i, j, u32YSrcIdx,u32YDesIdx);
            pu8DesY[u32YDesIdx++] = pu8SrcYUV[u32YSrcIdx];

            if((i & 0x01) && (j & 0x01))
            {
                //printf("(%d %d)::SrcUV:%d, %d DstUV:%d\n", i, j, u32YSrcIdx+1,u32YSrcIdx+2, u32UVDesIdx);
                pu8DesUV[u32UVDesIdx++] = pu8SrcYUV[u32YSrcIdx+1];
                pu8DesUV[u32UVDesIdx++] = pu8SrcYUV[u32YSrcIdx+2];
            }

            u32YSrcIdx += 3;
        }
    }

}


void _BootLogoJpdCtrl(BootlogoImgConfig_t  *pstBootlogoImgCfg, u16 *pu16OutWidth, u16 *pu16OutHeight)
{
#if defined(CONFIG_SSTAR_JPD)
    // Variables for the source jpg
    u32 u32JpgSize;
    u8 *pu8JpgBuffer;

    // Variables for the decompressor itself
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    // Variables for the output buffer, and how long each row is
    u32 u32BmpSize;
    u8 *pu8BmpBuffer;

    u32 u32Yuv420Size;
    u8 *pu8Yuv420Buffer;

    u16 u16RowStride, u16Width, u16Height, u16PixelSize;

    int rc; //, i, j;

    u32JpgSize = pstBootlogoImgCfg->u32InBuffSize;
    pu8JpgBuffer = (unsigned char *)((u32)pstBootlogoImgCfg->u64InBuffAddr);

	BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_JPD,"%s %d::  Create Decompress struct\n", __FUNCTION__, __LINE__);
	// Allocate a new decompress struct, with the default error handler.
	// The default error handler will exit() on pretty much any issue,
	// so it's likely you'll want to replace it or supplement it with
	// your own.
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_JPD,"%s %d::  Set memory buffer as source\n", __FUNCTION__, __LINE__);
    // Configure this decompressor to read its data from a memory
    // buffer starting at unsigned char *pu8JpgBuffer, which is u32JpgSize
    // long, and which must contain a complete jpg already.
    //
    // If you need something fancier than this, you must write your
    // own data source manager, which shouldn't be too hard if you know
    // what it is you need it to do. See jpeg-8d/jdatasrc.c for the
    // implementation of the standard jpeg_mem_src and jpeg_stdio_src
    // managers as examples to work from.
    jpeg_mem_src(&cinfo, pu8JpgBuffer, u32JpgSize);

    BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_JPD, "%s %d::  Read the JPEG header\n", __FUNCTION__, __LINE__);
    // Have the decompressor scan the jpeg header. This won't populate
    // the cinfo struct output fields, but will indicate if the
    // jpeg is valid.
    rc = jpeg_read_header(&cinfo, TRUE);

    if (rc != 1)
    {
        BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "%s %d:: File does not seem to be a normal JPEG\n", __FUNCTION__, __LINE__);
        return;
    }


    BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_JPD,"%s %d::  Initiate JPEG decompression\n", __FUNCTION__, __LINE__);

    // output color space is yuv444 packet
	cinfo.out_color_space = JCS_YCbCr;

    jpeg_start_decompress(&cinfo);

    u16Width = cinfo.output_width;
    u16Height = cinfo.output_height;
    u16PixelSize = cinfo.output_components;
    *pu16OutWidth = u16Width;
    *pu16OutHeight = u16Height;

    BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_JPD, "%s %d::  Image is %d by %d with %d components\n",
        __FUNCTION__, __LINE__, u16Width, u16Height, u16PixelSize);


	u32BmpSize = u16Width * u16Height * u16PixelSize;
	pu8BmpBuffer =(u8 *) malloc(u32BmpSize);

    if(pu8BmpBuffer == NULL)
    {
        BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "%s %d:: malloc fail\n", __FUNCTION__, __LINE__);
        return;
    }

    BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_JPD, "%s %d:: BmpBuffer: 0x%x\n", __FUNCTION__, __LINE__, (u32)pu8BmpBuffer);
    u32Yuv420Size = u16Width * u16Height * 3 / 2;
    pu8Yuv420Buffer = (unsigned char *)((u32)pstBootlogoImgCfg->u64OutBuffAddr + BOOTLOGO_VIRTUAL_ADDRESS_OFFSET);

    if( u32Yuv420Size > pstBootlogoImgCfg->u32OutBuffSize)
    {
        BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR,"%s %d:: Output buffer is too big, %d\n",
            __FUNCTION__, __LINE__, u16Width * u16Height * u16PixelSize);
        return;
    }

	u16RowStride = u16Width * u16PixelSize;

    BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_JPD,"%s %d:: Start reading scanlines\n", __FUNCTION__, __LINE__);
	while (cinfo.output_scanline < cinfo.output_height)
	{
		unsigned char *buffer_array[1];
		buffer_array[0] = pu8BmpBuffer + \
						   (cinfo.output_scanline) * u16RowStride;

		jpeg_read_scanlines(&cinfo, buffer_array, 1);
	}

    BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_JPD,"%s %d:: Done reading scanlines\n", __FUNCTION__, __LINE__);
    jpeg_finish_decompress(&cinfo);

    jpeg_destroy_decompress(&cinfo);

    BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_JPD,"%s %d:: End of decompression\n", __FUNCTION__, __LINE__);
    _BootLogoYuv444ToYuv420(pu8BmpBuffer, pu8Yuv420Buffer, u16Width, u16Height);
    flush_dcache_all();

    free(pu8BmpBuffer);
#endif
}


void _BootLogoDispCtrl(BootlogoImgConfig_t  *pstBootlogoImgCfg, u16 u16ImgWidht, u16 u16ImgHeight)
{
#if defined(CONFIG_SSTAR_DISP)
    MHAL_DISP_AllocPhyMem_t stPhyMem;
    MHAL_DISP_DeviceTimingInfo_t stTimingInfo;
    MHAL_DISP_InputPortAttr_t stInputAttr;
    MHAL_DISP_VideoFrameData_t stVideoFrameBuffer;
    static void *pDevCtx = NULL;
    static void *pVidLayerCtx = NULL;
    static void *pInputPortCtx = NULL;
    u32 u32Interface = 0;
    u32 u32DispDbgLevel;

    stPhyMem.alloc = BootLogoMemAlloc;
    stPhyMem.free  = BootLogoMemRelease;

    u32DispDbgLevel = 0x00;//0x1F;
    MHAL_DISP_DbgLevel(&u32DispDbgLevel);

    //Inint Pnl Tbl
    _BootLogoDispPnlInit();

    if(pstBootlogoImgCfg->u8Interface & DISP_DEVICE_LCD)
    {
        u32Interface = MHAL_DISP_INTF_LCD;
    }
    else
    {
        u32Interface |= (pstBootlogoImgCfg->u8Interface & DISP_DEVICE_HDMI) ? MHAL_DISP_INTF_HDMI : 0;
        u32Interface |= (pstBootlogoImgCfg->u8Interface & DISP_DEVICE_VGA)  ? MHAL_DISP_INTF_VGA : 0;
    }

    if(pDevCtx == NULL)
    {
        if(MHAL_DISP_DeviceCreateInstance(&stPhyMem, 0, &pDevCtx) == FALSE)
        {
            BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "%s %d, CreateDevice Fail\n", __FUNCTION__, __LINE__);
            return;
        }
    }

    if(pVidLayerCtx == NULL)
    {
        if(MHAL_DISP_VideoLayerCreateInstance(&stPhyMem, 0, &pVidLayerCtx) == FALSE)
        {
            BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "%s %d, CreateVideoLayer Fail\n", __FUNCTION__, __LINE__);
            return;

        }
    }

    if(pInputPortCtx == NULL)
    {
        if(MHAL_DISP_InputPortCreateInstance(&stPhyMem, pVidLayerCtx, 0, &pInputPortCtx) == FALSE)
        {
            BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "%s %d, CreaetInputPort Fail\n", __FUNCTION__, __LINE__);
            return;
        }
    }

    MHAL_DISP_DeviceSetBackGroundColor(pDevCtx, 0x800080);
    MHAL_DISP_DeviceEnable(pDevCtx, 0);
    MHAL_DISP_DeviceAddOutInterface(pDevCtx, u32Interface);

#if defined(CONFIG_SSTAR_PNL)
    if(u32Interface == MHAL_DISP_INTF_LCD)
    {
        MHAL_DISP_SyncInfo_t stSyncInfo;

        if(pstBootlogoImgCfg->panelname[0] != 0)
        {
            stSyncInfo.u16Vact = pstBootlogoImgCfg->stPnlPara.u16Height;
            stSyncInfo.u16Vbb  = pstBootlogoImgCfg->stPnlPara.u16VSyncBackPorch;
            stSyncInfo.u16Vpw  = pstBootlogoImgCfg->stPnlPara.u16VSyncWidth;
            stSyncInfo.u16Vfb  = pstBootlogoImgCfg->stPnlPara.u16VTotal - stSyncInfo.u16Vact - stSyncInfo.u16Vbb - stSyncInfo.u16Vpw;
            stSyncInfo.u16Hact = pstBootlogoImgCfg->stPnlPara.u16Width;
            stSyncInfo.u16Hbb  = pstBootlogoImgCfg->stPnlPara.u16HSyncBackPorch;
            stSyncInfo.u16Hpw  = pstBootlogoImgCfg->stPnlPara.u16HSyncWidth;
            stSyncInfo.u16Hfb  = pstBootlogoImgCfg->stPnlPara.u16HTotal - stSyncInfo.u16Hact - stSyncInfo.u16Hbb - stSyncInfo.u16Hpw;
            stSyncInfo.u32FrameRate = pstBootlogoImgCfg->u8DisplayRate;

            BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_INFO, "%s %d, H(%d %d %d %d) V(%d %d %d %d) Fps:%d\n",
                __FUNCTION__, __LINE__,
                stSyncInfo.u16Hfb, stSyncInfo.u16Hpw, stSyncInfo.u16Hbb, stSyncInfo.u16Hact,
                stSyncInfo.u16Vfb, stSyncInfo.u16Vpw, stSyncInfo.u16Vbb, stSyncInfo.u16Vact,
                stSyncInfo.u32FrameRate);

            stTimingInfo.eTimeType = E_MHAL_DISP_OUTPUT_USER;
            stTimingInfo.pstSyncInfo = &stSyncInfo;
            MHAL_DISP_DeviceSetOutputTiming(pDevCtx, MHAL_DISP_INTF_LCD, &stTimingInfo);
        }
        else
        {
            BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "%s %d, No stPnlPara Fail\n", __FUNCTION__, __LINE__);
            return;
        }
    }
    else
#endif
    {
        stTimingInfo.eTimeType = _BootLogoGetTiminId(pstBootlogoImgCfg->u16DisplayWidth,
                                                     pstBootlogoImgCfg->u16DisplayHeight,
                                                     pstBootlogoImgCfg->u8DisplayRate);
        stTimingInfo.pstSyncInfo = NULL;
        MHAL_DISP_DeviceSetOutputTiming(pDevCtx, u32Interface, &stTimingInfo);
    }
    MHAL_DISP_DeviceEnable(pDevCtx, 1);

    memset(&stInputAttr, 0, sizeof(MHAL_DISP_InputPortAttr_t));
    memset(&stVideoFrameBuffer, 0, sizeof(MHAL_DISP_VideoFrameData_t));

#if (BOOTLOGO_NOT_ZOOM)
    stInputAttr.stDispWin.u16X = (pstBootlogoImgCfg->u16DisplayWidth - u16ImgWidht) / 2;
    stInputAttr.stDispWin.u16Y = (pstBootlogoImgCfg->u16DisplayHeight - u16ImgHeight) / 2;
    stInputAttr.stDispWin.u16Width = u16ImgWidht;
    stInputAttr.stDispWin.u16Height = u16ImgHeight;
#else
    stInputAttr.stDispWin.u16X = 0;
    stInputAttr.stDispWin.u16Y = 0;
    stInputAttr.stDispWin.u16Width = pstBootlogoImgCfg->u16DisplayWidth;
    stInputAttr.stDispWin.u16Height = pstBootlogoImgCfg->u16DisplayHeight;
#endif

    stInputAttr.u16SrcWidth = u16ImgWidht;
    stInputAttr.u16SrcHeight = u16ImgHeight;

    stVideoFrameBuffer.ePixelFormat = E_MHAL_PIXEL_FRAME_YUV_MST_420;
    stVideoFrameBuffer.aPhyAddr[0] = pstBootlogoImgCfg->u64OutBuffAddr;
    stVideoFrameBuffer.aPhyAddr[1] = pstBootlogoImgCfg->u64OutBuffAddr + u16ImgWidht * u16ImgHeight;
    stVideoFrameBuffer.au32Stride[0] = u16ImgWidht;
#if !defined(CONFIG_SSTAR_RGN)
    MHAL_DISP_InputPortSetAttr(pInputPortCtx, &stInputAttr);
    MHAL_DISP_InputPortFlip(pInputPortCtx, &stVideoFrameBuffer);
    MHAL_DISP_InputPortEnable(pInputPortCtx, TRUE);
#endif

#endif
}

void _BootLogoPnlCtrl(BootlogoImgConfig_t *pstBootLogoImgCfg)
{
#if defined(CONFIG_SSTAR_PNL)
    void *pPnlDev;
    u32 u32DbgLevel;

    if(pstBootLogoImgCfg->u8Interface == DISP_DEVICE_LCD && pstBootLogoImgCfg->panelname[0] != 0)
    {
        BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_INFO, "%s %d, PnlLink:%d\n",
            __FUNCTION__, __LINE__, pstBootLogoImgCfg->stPnlPara.eLinkType);

        u32DbgLevel = 0x0F;
        MhalPnlSetDebugLevel((void *)&u32DbgLevel);

        if(MhalPnlCreateInstance(&pPnlDev, pstBootLogoImgCfg->stPnlPara.eLinkType) == FALSE)
        {
            BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "%s %d, PnlCreateInstance Fail\n", __FUNCTION__, __LINE__);
            return;
        }

        MhalPnlSetParamConfig(pPnlDev, &pstBootLogoImgCfg->stPnlPara);

        if(pstBootLogoImgCfg->stPnlPara.eLinkType == E_MHAL_PNL_LINK_MIPI_DSI)
        {
            MhalPnlSetMipiDsiConfig(pPnlDev, &pstBootLogoImgCfg->stMipiDsiCfg);
        }
    }
#endif
}

void _BootLogoHdmitxCtrl(BootlogoImgConfig_t *pstBootLogoImgCfg)
{
#if defined(CONFIG_SSTAR_HDMITX)
    static void *pHdmitxCtx = NULL;
    MhalHdmitxAttrConfig_t stAttrCfg;
    MhalHdmitxSignalConfig_t stSignalCfg;
    MhalHdmitxMuteConfig_t stMuteCfg;
    MhalHdmitxHpdConfig_t stHpdCfg;

    if(pstBootLogoImgCfg->u8Interface & DISP_DEVICE_HDMI)
    {
        printf("%s %d\n", __FUNCTION__, __LINE__);
        if(pHdmitxCtx == NULL)
        {
            if(MhalHdmitxCreateInstance(&pHdmitxCtx, 0) != E_MHAL_HDMITX_RET_SUCCESS)
            {
                printf("%s %d, CreateInstance Fail\n", __FUNCTION__, __LINE__);
                return;
            }
        }

        //MhalHdmitxSetDebugLevel(pHdmitxCtx, 0x3F);

        stHpdCfg.u8GpioNum = 26;
        MhalHdmitxSetHpdConfig(pHdmitxCtx, &stHpdCfg);

        stMuteCfg.enType = E_MHAL_HDMITX_MUTE_AUDIO | E_MHAL_HDMITX_MUTE_VIDEO | E_MHAL_HDMITX_MUTE_AVMUTE;
        stMuteCfg.bMute = 1;
        MhalHdmitxSetMute(pHdmitxCtx, &stMuteCfg);

        stSignalCfg.bEn = 0;
        MhalHdmitxSetSignal(pHdmitxCtx, &stSignalCfg);

        stAttrCfg.bVideoEn = 1;
        stAttrCfg.enInColor    = E_MHAL_HDMITX_COLOR_RGB444;
        stAttrCfg.enOutColor   = E_MHAL_HDMITX_COLOR_RGB444;
        stAttrCfg.enOutputMode = E_MHAL_HDMITX_OUTPUT_MODE_HDMI;
        stAttrCfg.enColorDepth = E_MHAL_HDMITX_CD_24_BITS;
        stAttrCfg.enTiming     = _BootLogoGetHdmitxTimingId(pstBootLogoImgCfg->u16DisplayWidth, pstBootLogoImgCfg->u16DisplayHeight, pstBootLogoImgCfg->u8DisplayRate);

        stAttrCfg.bAudioEn = 1;
        stAttrCfg.enAudioFreq = E_MHAL_HDMITX_AUDIO_FREQ_48K;
        stAttrCfg.enAudioCh   = E_MHAL_HDMITX_AUDIO_CH_2;
        stAttrCfg.enAudioFmt  = E_MHAL_HDMITX_AUDIO_FORMAT_PCM;
        stAttrCfg.enAudioCode = E_MHAL_HDMITX_AUDIO_CODING_PCM;
        MhalHdmitxSetAttr(pHdmitxCtx, &stAttrCfg);

        stSignalCfg.bEn = 1;
        MhalHdmitxSetSignal(pHdmitxCtx, &stSignalCfg);

        stMuteCfg.enType = E_MHAL_HDMITX_MUTE_AUDIO | E_MHAL_HDMITX_MUTE_VIDEO | E_MHAL_HDMITX_MUTE_AVMUTE;
        stMuteCfg.bMute = 0;
        MhalHdmitxSetMute(pHdmitxCtx, &stMuteCfg);
    }
#endif
}

void _BootLogoRgnCtrl(BootlogoImgConfig_t *pstBootLogoImgCfg, u16 u16ImgWidth, u16 u16ImgHeight)
{
#if defined(CONFIG_SSTAR_RGN)
    MHAL_RGN_GopType_e eGopId = E_MHAL_GOP_VPE_PORT1;
    MHAL_RGN_GopGwinId_e eGwinId = E_MHAL_GOP_GWIN_ID_0;
    MHAL_RGN_GopWindowConfig_t stSrcWinCfg;
    MHAL_RGN_GopWindowConfig_t stDstWinCfg;


#if (BOOTLOGO_NOT_ZOOM)
    stSrcWinCfg.u32X = (pstBootLogoImgCfg->u16DisplayWidth - u16ImgWidth) / 2;
    stSrcWinCfg.u32Y = (pstBootLogoImgCfg->u16DisplayHeight - u16ImgHeight) / 2;
    stSrcWinCfg.u32Width  = u16ImgWidth;
    stSrcWinCfg.u32Height = u16ImgHeight;
    stDstWinCfg.u32X = 0;
    stDstWinCfg.u32Y = 0;
    stDstWinCfg.u32Width  = u16ImgWidth;
    stDstWinCfg.u32Height = u16ImgHeight;
#else
    stSrcWinCfg.u32X = 0;
    stSrcWinCfg.u32Y = 0;
    stSrcWinCfg.u32Width  = u16ImgWidth;
    stSrcWinCfg.u32Height = u16ImgHeight;
    stDstWinCfg.u32X = 0;
    stDstWinCfg.u32Y = 0;
    stDstWinCfg.u32Width  = pstBootLogoImgCfg->u16DisplayWidth;
    stDstWinCfg.u32Height = pstBootLogoImgCfg->u16DisplayHeight;
#endif

    MHAL_RGN_GopInit();

    MHAL_RGN_GopSetBaseWindow(eGopId, &stSrcWinCfg, &stDstWinCfg);

    MHAL_RGN_GopGwinSetPixelFormat(eGopId, eGwinId, E_MHAL_RGN_PIXEL_FORMAT_ARGB8888);

    MHAL_RGN_GopGwinSetWindow(eGopId, eGwinId,
        u16ImgWidth, u16ImgHeight, u16ImgWidth*4, 0, 0);

    MHAL_RGN_GopGwinSetBuffer(eGopId, eGwinId, pstBootLogoImgCfg->u64OutBuffAddr);

    MHAL_RGN_GopSetAlphaZeroOpaque(eGopId, FALSE, TRUE, E_MHAL_RGN_PIXEL_FORMAT_ARGB8888);

    MHAL_RGN_GopSetAlphaType(eGopId, eGwinId, E_MHAL_GOP_GWIN_ALPHA_CONSTANT, 0xFF);

    MHAL_RGN_GopGwinEnable(eGopId,eGwinId);
#endif
}

int do_display (cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
    #define BUFFERSIZE (720*480*3)

    BootlogoImgConfig_t  stBootlogoImgCfg;
    u16 u16ImgWidht = 720;
    u16 u16ImgHeight = 480;
    bool bParamSet = 0;
    static u8 *pu8InBuf = NULL;
    static u8 *pu8OutBuf = NULL;
    static u32 u32InBufSize = BUFFERSIZE;
    static u32 u32OutBufSize = BUFFERSIZE;

    BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_INFO,"%s %d, argc =%d\n", __FUNCTION__, __LINE__, argc);

    memset(&stBootlogoImgCfg, 0, sizeof(BootlogoImgConfig_t));
    if(argc == 1)
    {
        if(_BootLogoGetImageConfig(&stBootlogoImgCfg) == FALSE)
        {
            bParamSet = 0;
            BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "%s %d, GetImage fail\n", __FUNCTION__, __LINE__);
        }
        else
        {
            bParamSet = 1;
        }
    }
    else if(argc == 2)
    {
        if(strcmp(argv[1], "malloc") == 0)
        {
            pu8InBuf = (u8 *) malloc(u32InBufSize);
            pu8OutBuf = (u8 *) malloc(u32OutBufSize);
            printf("InBuffer: 0x%x Size:0x%x OutBuf: 0x%x, Size: 0x%x\n",
                (u32)pu8InBuf, u32InBufSize, (u32)pu8OutBuf, u32OutBufSize);
        }
    }
    else if(argc ==3)
    {
        if(strcmp(argv[1], "insize") == 0)
        {
            u32InBufSize = simple_strtoul(argv[2], NULL, 16);
        }
        else if(strcmp(argv[1], "outsize") == 0)
        {
            u32OutBufSize = simple_strtoul(argv[2], NULL, 16);
        }
    }
    else if(argc >= 5)
    {
        stBootlogoImgCfg.u64InBuffAddr    = (u32)pu8InBuf;
        stBootlogoImgCfg.u32InBuffSize    = u32InBufSize;

        stBootlogoImgCfg.u64OutBuffAddr   = (u32)pu8OutBuf;
        stBootlogoImgCfg.u64OutBuffAddr   &= 0xFFFFFFF;
        stBootlogoImgCfg.u32OutBuffSize   = u32OutBufSize;

        if(stBootlogoImgCfg.u64OutBuffAddr & 0x0F)
        {   // 16 align
            u64 u64NewAddr;
            u64NewAddr = (stBootlogoImgCfg.u64OutBuffAddr + 0x0F) & 0xFFFFFFF0;
            stBootlogoImgCfg.u32OutBuffSize = u32OutBufSize - (u64NewAddr - stBootlogoImgCfg.u64OutBuffAddr);
            stBootlogoImgCfg.u64OutBuffAddr = u64NewAddr;
        }

        stBootlogoImgCfg.u16DisplayWidth  = simple_strtoul(argv[1], NULL, 10);
        stBootlogoImgCfg.u16DisplayHeight = simple_strtoul(argv[2], NULL, 10);
        stBootlogoImgCfg.u8DisplayRate    = simple_strtoul(argv[3], NULL, 10);
        stBootlogoImgCfg.u8Interface      = simple_strtoul(argv[4], NULL, 10);

        bParamSet = 1;

        if(stBootlogoImgCfg.u8Interface == 4 && argc == 6)
        {
        #if PNL_TEST_MD_EN
            stBootlogoImgCfg.panelname[0] = 1;
            if(strcmp(argv[5], "ttl800x480") == 0)
            {
                memcpy(&stBootlogoImgCfg.stPnlPara, &stTtl00x480Param, sizeof(MhalPnlParamConfig_t));
                memset(&stBootlogoImgCfg.stMipiDsiCfg, 0, sizeof(MhalPnlMipiDsiConfig_t));
            }
            else if(strcmp(argv[5], "rm6820") == 0)
            {
                memcpy(&stBootlogoImgCfg.stPnlPara, &stRm6820Param, sizeof(MhalPnlParamConfig_t));
                memcpy(&stBootlogoImgCfg.stMipiDsiCfg, &stRm6820MipiCfg, sizeof(MhalPnlMipiDsiConfig_t));
            }
            else
            {
                bParamSet = 0;
                BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "Wrong Pnl Name: %s\n", argv[5]);

            }
        #endif
        }
    }
    else
    {
        BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, "bootlogo [Disp Width] [Disp Height] [Disp Rate] [Interface] [[PNL NAME]]\n");
        BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, " [Interface] 1: HDMI, 2:VGA, 4:LCD\n");
        BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_ERR, " [[PNL NAME]] rm6820, ttl800x480\n");
        bParamSet = 0;
    }

    if(bParamSet)
    {
        BOOTLOGO_DBG(BOOTLOGO_DBG_LEVEL_INFO, "%s %d:: In(%llx %x), Out(%llx %x), Disp(%d %d %d) Interface:%d\n",
            __FUNCTION__, __LINE__,
            stBootlogoImgCfg.u64InBuffAddr, stBootlogoImgCfg.u32InBuffSize,
            stBootlogoImgCfg.u64OutBuffAddr, stBootlogoImgCfg.u32OutBuffSize,
            stBootlogoImgCfg.u16DisplayWidth, stBootlogoImgCfg.u16DisplayHeight, stBootlogoImgCfg.u8DisplayRate,
            stBootlogoImgCfg.u8Interface);

        if(stBootlogoImgCfg.u64InBuffAddr && stBootlogoImgCfg.u64InBuffAddr)
        {
            _BootLogoJpdCtrl(&stBootlogoImgCfg, &u16ImgWidht, &u16ImgHeight);
        }

        _BootLogoDispCtrl(&stBootlogoImgCfg, u16ImgWidht, u16ImgHeight);
        _BootLogoHdmitxCtrl(&stBootlogoImgCfg);
        _BootLogoPnlCtrl(&stBootlogoImgCfg);
        _BootLogoRgnCtrl(&stBootlogoImgCfg, u16ImgWidht, u16ImgHeight);

        return 0;
    }
    else
    {
        return 1;
    }
}

U_BOOT_CMD(
	bootlogo, CONFIG_SYS_MAXARGS, 1,    do_display,
	"show bootlogo",
	NULL
);


