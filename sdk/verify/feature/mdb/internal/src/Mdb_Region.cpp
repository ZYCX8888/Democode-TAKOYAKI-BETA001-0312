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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sstream>

#include "mi_sys.h"
#include "mi_rgn.h"

#include "st_common.h"
#include "st_rgn.h"

#include "Mdb_Region.h"

#define MDB_RGN_BMP_MAXNUM     56
#define MDB_RGN_OSD_MAX_NUM 4
#define MDB_RGN_COVER_MAX_NUM 4
#define MDB_RGN_VPE_PORT_MAXNUM 4
#define MDB_RGN_DIVP_PORT_MAXNUM 1
#define MDB_RGN_VPE_MAX_CH_NUM 64
#define MDB_RGN_DIVP_MAX_CH_NUM 64

#define DOT_FONT_FILE            "gb2312.hzk"
#define ASCII_FONT_FILE             "ascii.bin"

#define MDB_RGN_PIXELFMT_BITCOUNT(pixelType, bits)   do {    \
                                        switch (pixelType)    \
                                        {   \
                                            case E_MI_RGN_PIXEL_FORMAT_ARGB1555: \
                                            case E_MI_RGN_PIXEL_FORMAT_ARGB4444: \
                                            case E_MI_RGN_PIXEL_FORMAT_RGB565:   \
                                                bits = 16;  \
                                                break;  \
                                            case E_MI_RGN_PIXEL_FORMAT_I2:   \
                                                bits = 2; \
                                                break;  \
                                            case E_MI_RGN_PIXEL_FORMAT_I4:   \
                                                bits = 4; \
                                                break;  \
                                            case E_MI_RGN_PIXEL_FORMAT_I8:   \
                                                bits = 8; \
                                                break;  \
                                            case E_MI_RGN_PIXEL_FORMAT_ARGB8888:   \
                                                bits = 32; \
                                                break;  \
                                            default:    \
                                                printf("wrong pixel type\n");   \
                                                return; \
                                        }   \
} while(0);
#define RM  0x00FF0000
#define GM  0x0000FF00
#define BM  0x000000FF
#define Rval(data) ( (data & RM) >> 16)
#define Gval(data) ( (data & GM) >> 8)
#define Bval(data) ( (data & BM) )

static MI_RGN_PaletteTable_t _gstPaletteTable =
{
    {{0, 0, 0, 0}, {255, 255, 0, 0}, {255, 0, 255, 0}, {255, 0, 0, 255},
    {255, 255, 255, 0}, {255, 0, 112, 255}, {255, 0, 255, 255}, {255, 255, 255, 255},
    {255, 128, 0, 0}, {255, 128, 128, 0}, {255, 128, 0, 128}, {255, 0, 128, 0},
    {255, 0, 0, 128}, {255, 0, 128, 128}, {255, 128, 128, 128}, {255, 64, 64, 64}}
};

static const char * _gszFileName[MDB_RGN_BMP_MAXNUM] = {"testFile/1555/128X96.argb1555", "testFile/1555/200X301.argb1555", "testFile/1555/300X360.argb1555", "testFile/1555/400X266.argb1555",
                                                        "testFile/1555/200X356.argb1555", "testFile/1555/200X131.argb1555", "testFile/1555/300X224.argb1555", "testFile/1555/200X133.argb1555",
                                                        "testFile/4444/128X96.argb4444", "testFile/4444/200X301.argb4444", "testFile/4444/300X360.argb4444", "testFile/4444/400X266.argb4444",
                                                        "testFile/4444/200X356.argb4444", "testFile/4444/200X131.argb4444", "testFile/4444/300X224.argb4444", "testFile/4444/200X133.argb4444",
                                                        "testFile/I2/64X48.i2", "testFile/I2/80X80.i2", "testFile/I2/200X200.i2", "testFile/I2/304X200.i2",
                                                        "testFile/I2/64X48.i2", "testFile/I2/80X80.i2", "testFile/I2/200X200.i2", "testFile/I2/304X200.i2",
                                                        "testFile/I4/64X48.i4", "testFile/I4/80X80.i4", "testFile/I4/200X200.i4", "testFile/I4/304X200.i4",
                                                        "testFile/I4/64X48.i4", "testFile/I4/80X80.i4", "testFile/I4/200X200.i4", "testFile/I4/304X200.i4",
                                                        "testFile/I8/64X48.i8", "testFile/I8/80X80.i8", "testFile/I8/200X200.i8", "testFile/I8/304X200.i8",
                                                        "testFile/I8/64X48.i8", "testFile/I8/80X80.i8", "testFile/I8/200X200.i8", "testFile/I8/304X200.i8",
                                                        "testFile/565/128X96.rgb565", "testFile/565/200X301.rgb565", "testFile/565/300X360.rgb565", "testFile/565/400X266.rgb565",
                                                        "testFile/565/200X356.rgb565", "testFile/565/200X131.rgb565", "testFile/565/300X224.rgb565", "testFile/565/200X133.rgb565",
                                                        "testFile/8888/128X96.argb8888", "testFile/8888/200X301.argb8888", "testFile/8888/300X360.argb8888", "testFile/8888/400X266.argb8888",
                                                        "testFile/8888/200X356.argb8888", "testFile/8888/200X131.argb8888", "testFile/8888/300X224.argb8888", "testFile/8888/200X133.argb8888"};
static const MI_RGN_Size_t _gstSize[MDB_RGN_BMP_MAXNUM] = {{128, 96}, {200, 301}, {300, 360}, {400, 266},
                                                           {200, 356}, {200, 131}, {300, 224}, {200, 133},
                                                           {128, 96}, {200, 301}, {300, 360}, {400, 266},
                                                           {200, 356}, {200, 131}, {300, 224}, {200, 133},
                                                           {64, 48}, {80, 80}, {200, 200}, {304, 200},
                                                           {64, 48}, {80, 80}, {200, 200}, {304, 200},
                                                           {64, 48}, {80, 80}, {200, 200}, {304, 200},
                                                           {64, 48}, {80, 80}, {200, 200}, {304, 200},
                                                           {64, 48}, {80, 80}, {200, 200}, {304, 200},
                                                           {64, 48}, {80, 80}, {200, 200}, {304, 200},
                                                           {128, 96}, {200, 301}, {300, 360}, {400, 266},
                                                           {200, 356}, {200, 131}, {300, 224}, {200, 133},
                                                           {128, 96}, {200, 301}, {300, 360}, {400, 266},
                                                           {200, 356}, {200, 131}, {300, 224}, {200, 133}};
static const MI_RGN_Point_t _gstPos[MDB_RGN_BMP_MAXNUM] = {{0, 0}, {400, 0}, {100, 200}, {400, 400},
                                                           {0, 510}, {150, 0}, {650, 0}, {650, 400},
                                                           {0, 0}, {400, 0}, {100, 200}, {400, 400},
                                                           {0, 510}, {150, 0}, {650, 0}, {650, 400},
                                                           {100, 200}, {64, 100}, {200, 200}, {400, 400}, //I2
                                                           {0, 510}, {152, 0}, {652, 0}, {752, 500},
                                                           {100, 200}, {64, 100}, {200, 200}, {400, 400}, //I4
                                                           {0, 510}, {152, 0}, {652, 0}, {752, 500},
                                                           {100, 200}, {64, 100}, {200, 200}, {400, 400}, //I8
                                                           {0, 510}, {152, 0}, {652, 0}, {752, 500},
                                                           {0, 0}, {400, 0}, {100, 200}, {400, 400},
                                                           {0, 510}, {150, 0}, {650, 0}, {650, 400},
                                                           {0, 0}, {400, 0}, {100, 200}, {400, 400},
                                                           {0, 510}, {150, 0}, {650, 0}, {650, 400},};
static MI_RGN_ChnPortParam_t _gstCoverChnPortParam[MDB_RGN_COVER_MAX_NUM] =
{
    {TRUE, {10, 10}, {0, {100,100}, 0x0F00}},
    {TRUE, {30, 30}, {1, {100,100}, 0x00F0}},
    {TRUE, {50, 20}, {2, {100,100}, 0x000F}},
    {TRUE, {10, 60}, {3, {100,100}, 0x0FFF}}
};

static MI_HANDLE _ghHandle = 0;
static MI_RGN_Attr_t _gstRgnAttr = {E_MI_RGN_TYPE_OSD, {E_MI_RGN_PIXEL_FORMAT_ARGB1555, {40, 40}}};
static MI_RGN_ChnPort_t _gstChnPort = {E_MI_RGN_MODID_VPE, 0,  0, 0};
static MI_RGN_ChnPortParam_t _gstChnPortParam = {TRUE, {0, 0}, {0, {40,40}, 0}};
static MI_RGN_ChnPortParam_t _gstChnPortParamOsd = {TRUE, {0, 0}};
static MI_BOOL bSelectMode = 0; //1 for: SetBitMap, 1 for: Update canvas.

static std::vector<std::string> _gvecStrIn;

Mdb_Region::Mdb_Region()
{
    printf("%s\n", __FUNCTION__);
    PREPARE_COMMAND("init", &Mdb_Region::Init, 0);
    PREPARE_COMMAND("deinit", &Mdb_Region::Deinit, 0);
    PREPARE_COMMAND("create", &Mdb_Region::Create, 5);
    PREPARE_COMMAND("destroy", &Mdb_Region::Destroy, 1);
    PREPARE_COMMAND("attachcover", &Mdb_Region::AttachCover, 12);
    PREPARE_COMMAND("attachosd", &Mdb_Region::AttachOsd, 17);
    PREPARE_COMMAND("dettach", &Mdb_Region::Dettach, 5);
    PREPARE_COMMAND("getattr", &Mdb_Region::GetAttr, 5);
    PREPARE_COMMAND("setcoverdisplayattr", &Mdb_Region::SetCoverDisplayAttr, 12);
    PREPARE_COMMAND("setosddisplayattr", &Mdb_Region::SetOsdDisplayAttr, 17);
    PREPARE_COMMAND("setbitmap", &Mdb_Region::SetBitMap, 3);
    PREPARE_COMMAND("setcanvas", &Mdb_Region::SetCanvas, 3);
    PREPARE_COMMAND("getcanvas", &Mdb_Region::GetCanvas, 1);
    PREPARE_COMMAND("updatecanvas", &Mdb_Region::UpdateCanvas, 1);
    PREPARE_COMMAND("onergnoneattach", &Mdb_Region::OneRegionOneAttach, 0);
    PREPARE_COMMAND("onergnmaxattach", &Mdb_Region::OneRegionMaxAttach, 0);
    PREPARE_COMMAND("maxrgnmaxattach", &Mdb_Region::MaxRegionMaxAttach, 0);
    PREPARE_COMMAND("clearregion", &Mdb_Region::ClearRegion, 0);
    PREPARE_COMMAND("createattachdettachdestroy", &Mdb_Region::CreateAttachDetechDestroy, 0);
    PREPARE_COMMAND("maxrgn", &Mdb_Region::MaxRegion, 0);
    PREPARE_COMMAND("onechnoneportsetbitmap", &Mdb_Region::OneChnOnePortSetBitmap, 4);
    PREPARE_COMMAND("draw", &Mdb_Region::Init, 2);
    PREPARE_COMMAND("autotest", &Mdb_Region::AutoTest, 1);
    PREPARE_COMMAND("test1", &Mdb_Region::TestCase001, 0);
    PREPARE_COMMAND("test2", &Mdb_Region::TestCase002, 0);
    PREPARE_COMMAND("test3", &Mdb_Region::TestCase003, 2);
    PREPARE_COMMAND("test4", &Mdb_Region::TestCase004, 2);
    PREPARE_COMMAND("test5", &Mdb_Region::TestCase005, 2);
    PREPARE_COMMAND("test6", &Mdb_Region::TestCase006, 4);
    PREPARE_COMMAND("test7", &Mdb_Region::TestCase007, 2);
    PREPARE_COMMAND("test8", &Mdb_Region::TestCase008, 1);
    PREPARE_COMMAND("test9", &Mdb_Region::TestCase009, 1);
    PREPARE_COMMAND("test10", &Mdb_Region::TestCase010, 1);
    PREPARE_COMMAND("test11", &Mdb_Region::TestCase011, 4);
    PREPARE_COMMAND("processstart", &Mdb_Region::VpeDivpProcess, 6);
    PREPARE_COMMAND("adddata", &Mdb_Region::VpeDivpAddData, 1);
    PREPARE_COMMAND("attachdata", &Mdb_Region::VpeDivpAttachData, 6);
    PREPARE_COMMAND("rmdata", &Mdb_Region::VpeDivpRmData, 0);
    PREPARE_COMMAND("selectfilldatamode", &Mdb_Region::SelectFillDataMode, 1);
    //injectpic xxx_filename handle width height pitch fmt mod channel port posx posy
    PREPARE_COMMAND("injectpic", &Mdb_Region::InjectPic, 12);
    PREPARE_COMMAND("injectword", &Mdb_Region::InjectWord, 10);
    PREPARE_COMMAND("elicitword", &Mdb_Region::ElicitWord, 1);
    PREPARE_COMMAND("updateword", &Mdb_Region::UpdateWord, 2);
    PREPARE_COMMAND("injectcover", &Mdb_Region::InjectCover, 10);
    PREPARE_COMMAND("setpos", &Mdb_Region::SetOsdPos, 6);
    PREPARE_COMMAND("setonoff", &Mdb_Region::SetOnOff, 5);
    PREPARE_COMMAND("setosdlayer", &Mdb_Region::SetOsdLayer, 5);
    PREPARE_COMMAND("setosdalpha", &Mdb_Region::SetOsdAlpha, 7);
    PREPARE_COMMAND("setcovercolor", &Mdb_Region::SetCoverColor, 5);
    PREPARE_COMMAND("setcoverlayer", &Mdb_Region::SetCoverLayer, 5);
    PREPARE_COMMAND("setramdompos", &Mdb_Region::SetRamdomPos, 8);

    intVpeChCnt = 0;
    intDivpChCnt = 0;
    intCurFormat = 0;
}
Mdb_Region::~Mdb_Region()
{
    printf("%s\n", __FUNCTION__);
}

unsigned int Mdb_Region::Rgb2Yuv(unsigned int u32RgbColor)
{
    MI_FLOAT fY,fU,fV;
    MI_U32 u32Y,u32Cr,u32Cb;
    MI_U32 u32RetColor;

    fY = (0.299 * Rval(u32RgbColor)) + (0.587 * Gval(u32RgbColor)) + (0.114 * Bval(u32RgbColor));
    fU = (-0.168736 * Rval(u32RgbColor)) - (0.331264 * Gval(u32RgbColor)) + (0.5000 * Bval(u32RgbColor)) + 128;
    fV = (0.5000 * Rval(u32RgbColor)) - (0.418688 * Gval(u32RgbColor)) - (0.081312 * Bval(u32RgbColor)) + 128;
    u32Y = fY;
    u32Cb = fU;
    u32Cr = fV;
    u32RetColor = ((u32Cr << 16) & 0xFF0000) | ((u32Y << 8) & 0xFF00) | (u32Cb & 0xFF);

    return u32RetColor;
}

void Mdb_Region::ShowWelcome(std::string &strOut)
{
    strOut.assign("Welcome to Region module\n");
}

// osd:0 cover:1; vpe:0 divp:1; dettach:0 not dettach:1;
void Mdb_Region::AutoTest(std::vector<std::string> &inStrings, std::string &strOut)
{
    std::vector<std::string> vecStrIn;
    MI_U8 u8ExeCnt = (MI_U8)Atoi(inStrings[0]);

    while(u8ExeCnt--)
    {
        // testcase001
        strOut += "create[x1] -> destroy[x1]\n";
        TestCase001(inStrings, strOut);

        // testcase002
        strOut += "create[x1024(cover&osd)] -> destroy[x1024]\n";
        TestCase002(inStrings, strOut);

        // testcase003
        strOut += "create[osd] -> attach -> dettach -> destroy\n";
        vecStrIn.clear();
        ParseStrings("0 0", vecStrIn);
        TestCase003(vecStrIn, strOut);

        strOut +=
"create[osd] -> attach -> destroy\n";
        vecStrIn.clear();
        ParseStrings("0 1", vecStrIn);
        TestCase003(vecStrIn, strOut);

        strOut +=
"create[cover] -> attach -> dettach -> destroy\n";
        vecStrIn.clear();
        ParseStrings("1 0", vecStrIn);
        TestCase003(vecStrIn, strOut);

        strOut +=
"create[cover] -> attach -> destroy\n";
        vecStrIn.clear();
        ParseStrings("1 1", vecStrIn);
        TestCase003(vecStrIn, strOut);

        // testcase004
        strOut += "create[osd] -> attach[vpe MxN] -> dettach -> destroy\n";
        vecStrIn.clear();
        ParseStrings("0 0", vecStrIn);
        TestCase004(vecStrIn, strOut);

        strOut += "create[osd] -> attach[divp MxN] -> dettach -> destroy\n";
        vecStrIn.clear();
        ParseStrings("0 1", vecStrIn);
        TestCase004(vecStrIn, strOut);

        strOut += "create[cover] -> attach[vpe MxN] -> dettach -> destroy\n";
        vecStrIn.clear();
        ParseStrings("1 0", vecStrIn);
        TestCase004(vecStrIn, strOut);

        strOut +=
"create[cover] -> attach[divp MxN] -> dettach -> destroy\n";
        vecStrIn.clear();
        ParseStrings("1 1", vecStrIn);
        TestCase004(vecStrIn, strOut);

        // testcase005
        strOut += "create[osdx4] -> attach[vpe] -> dettach -> destroy\n";
        vecStrIn.clear();
        ParseStrings("0 0", vecStrIn);
        TestCase005(vecStrIn, strOut);

        strOut += "create[osdx4] -> attach[divp] -> dettach -> destroy\n";
        vecStrIn.clear();
        ParseStrings("0 1", vecStrIn);
        TestCase005(vecStrIn, strOut);

        strOut +=
"create[coverx4] -> attach[vpe] -> dettach -> destroy\n";
        vecStrIn.clear();
        ParseStrings("1 0", vecStrIn);
        TestCase005(vecStrIn, strOut);

        strOut += "create[coverx4] -> attach[divp] -> dettach -> destroy\n";
        vecStrIn.clear();
        ParseStrings("1 1", vecStrIn);
        TestCase005(vecStrIn, strOut);
        continue;

        // testcase006
        strOut += "create[osd] -> attach[vpe] -> setbitmap -> destroy\n";
        vecStrIn.clear();
        ParseStrings("0 0", vecStrIn);
        TestCase006(vecStrIn, strOut);

        strOut +=
"create[osd] -> setbitmap -> attach[vpe] -> destroy\n";
        vecStrIn.clear();
        ParseStrings("0 1", vecStrIn);
        TestCase006(inStrings, strOut);

        strOut +=
"create[osd] -> attach[divp] -> setbitmap -> destroy\n";
        vecStrIn.clear();
        ParseStrings("1 0", vecStrIn);
        TestCase006(vecStrIn, strOut);

        strOut +=
"create[osd] -> setbitmap -> attach[divp] -> destroy\n";
        vecStrIn.clear();
        ParseStrings("1 1", vecStrIn);
        TestCase006(inStrings, strOut);

        // testcase007
        strOut +=
"create[osd] -> getcanvas -> update -> attach -> destroy\n";
        vecStrIn.clear();
        ParseStrings("0 0", vecStrIn);
        TestCase007(inStrings, strOut);

        strOut += "create[osd] -> attach -> getcanvas -> update -> destroy\n";
        vecStrIn.clear();
        ParseStrings("1 0", vecStrIn);
        TestCase007(inStrings, strOut);

        // testcase008
        strOut += "create[osdx4] -> setbitmap[x4] -> attach[vpex4] -> (getbitmap -> dettach[x1])x3 -> destroy[x4]\n";
        vecStrIn.clear();
        ParseStrings("0", vecStrIn);
        TestCase008(inStrings, strOut);

        strOut +=
"create[osdx4] -> setbitmap[x4] -> attach[divpx4] -> (getbitmap -> dettach[x1])x3 -> destroy[x4]\n";
        vecStrIn.clear();
        ParseStrings("1", vecStrIn);
        TestCase008(inStrings, strOut);

        // testcase09
        strOut += "create[osdx4] -> setbitmap[x4] -> (attach[vpex4+divpx1] -> init[vpe] -> init[divp])x16 -> destroy[x4]";
        TestCase009(inStrings, strOut);
    }
}

void Mdb_Region::Init(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_PaletteTable_t stPaletteTable;

    if (!inStrings.empty())
    {
        stPaletteTable.astElement[15].u8Alpha = (MI_U8)Atoi(inStrings[0]);
        stPaletteTable.astElement[15].u8Green = (MI_U8)Atoi(inStrings[1]);
        MDB_EXPECT_OK("MI_RGN_Init", strOut, MI_RGN_Init(&stPaletteTable), MI_RGN_OK);

    }
    else
    {
        MDB_EXPECT_OK("MI_RGN_Init", strOut, MI_RGN_Init(&_gstPaletteTable), MI_RGN_OK);
    }
}
void Mdb_Region::Deinit(std::vector<std::string> &inStrings, std::string &strOut)
{
    MDB_EXPECT_OK("MI_RGN_Deinit", strOut, MI_RGN_DeInit(), MI_RGN_OK);
}
void Mdb_Region::Create(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hHandle;
    MI_RGN_Attr_t stRegion;

    hHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);
    stRegion.eType = (MI_RGN_Type_e)Atoi(inStrings[1]);
    stRegion.stOsdInitParam.ePixelFmt = (MI_RGN_PixelFormat_e)Atoi(inStrings[2]);
    stRegion.stOsdInitParam.stSize.u32Width = (MI_U32)Atoi(inStrings[3]);
    stRegion.stOsdInitParam.stSize.u32Height = (MI_U32)Atoi(inStrings[4]);

    MDB_EXPECT_OK("MI_RGN_Create", strOut, MI_RGN_Create(hHandle, &stRegion), MI_RGN_OK);
}
void Mdb_Region::Destroy(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hHandle;
    hHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);

    MDB_EXPECT_OK("MI_RGN_Destroy", strOut, MI_RGN_Destroy(hHandle), MI_RGN_OK);
}
void Mdb_Region::AttachCover(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hHandle;
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnAttr;

    hHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);
    stChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);
    stChnPort.s32DevId = (MI_S32)Atoi(inStrings[2]);
    stChnPort.s32ChnId = (MI_S32)Atoi(inStrings[3]);
    stChnPort.s32OutputPortId = (MI_U32)Atoi(inStrings[4]);
    stChnAttr.bShow = (MI_BOOL)Atoi(inStrings[5]);
    stChnAttr.stPoint.u32X = (MI_U32)Atoi(inStrings[6]);
    stChnAttr.stPoint.u32Y = (MI_U32)Atoi(inStrings[7]);
    stChnAttr.unPara.stCoverChnPort.u32Layer = (MI_U32)Atoi(inStrings[8]);
    stChnAttr.unPara.stCoverChnPort.stSize.u32Width = (MI_U32)Atoi(inStrings[9]);
    stChnAttr.unPara.stCoverChnPort.stSize.u32Height = (MI_U32)Atoi(inStrings[10]);
    stChnAttr.unPara.stCoverChnPort.u32Color = Rgb2Yuv((MI_U32)Atoi(inStrings[11]));

    MDB_EXPECT_OK("MI_RGN_AttachToChn", strOut, MI_RGN_AttachToChn(hHandle, &stChnPort, &stChnAttr), MI_RGN_OK);
}
void Mdb_Region::AttachOsd(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hHandle;
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnAttr;

    hHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);
    memset(&stChnAttr, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);
    stChnPort.s32DevId = (MI_S32)Atoi(inStrings[2]);
    stChnPort.s32ChnId = (MI_S32)Atoi(inStrings[3]);
    stChnPort.s32OutputPortId = (MI_U32)Atoi(inStrings[4]);
    stChnAttr.bShow = (MI_BOOL)Atoi(inStrings[5]);
    stChnAttr.stPoint.u32X = (MI_U32)Atoi(inStrings[6]);
    stChnAttr.stPoint.u32Y = (MI_U32)Atoi(inStrings[7]);
    stChnAttr.unPara.stOsdChnPort.u32Layer = (MI_U32)Atoi(inStrings[8]);
    stChnAttr.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = (MI_BOOL)Atoi(inStrings[9]);
    stChnAttr.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode = (MI_RGN_InvertColorMode_e)Atoi(inStrings[10]);
    stChnAttr.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold = (MI_U16)Atoi(inStrings[11]);
    stChnAttr.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum = (MI_U16)Atoi(inStrings[12]);
    stChnAttr.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum = (MI_U16)Atoi(inStrings[13]);
    stChnAttr.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode = (MI_RGN_AlphaMode_e)Atoi(inStrings[14]);
    if (stChnAttr.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode == E_MI_RGN_PIXEL_ALPHA)
    {
        stChnAttr.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8BgAlpha = (MI_U8)Atoi(inStrings[15]);
        stChnAttr.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8FgAlpha = (MI_U8)Atoi(inStrings[16]);

    }
    else if (stChnAttr.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode == E_MI_RGN_CONSTANT_ALPHA)
    {
        stChnAttr.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.u8ConstantAlpha = (MI_U8)Atoi(inStrings[15]);
    }


    MDB_EXPECT_OK("MI_RGN_AttachToChn", strOut, MI_RGN_AttachToChn(hHandle, &stChnPort, &stChnAttr), MI_RGN_OK);
}

void Mdb_Region::Dettach(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hHandle;
    MI_RGN_ChnPort_t stChnPort;

    hHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);
    stChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);
    stChnPort.s32DevId = (MI_S32)Atoi(inStrings[2]);
    stChnPort.s32ChnId = (MI_S32)Atoi(inStrings[3]);
    stChnPort.s32OutputPortId = (MI_U32)Atoi(inStrings[4]);

    MDB_EXPECT_OK("MI_RGN_DetachFromChn", strOut, MI_RGN_DetachFromChn(hHandle, &stChnPort), MI_RGN_OK);
}

void Mdb_Region::GetAttr(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hHandle;
    MI_RGN_Attr_t stRegion;

    hHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);
    stRegion.eType = (MI_RGN_Type_e)Atoi(inStrings[1]);
    stRegion.stOsdInitParam.ePixelFmt = (MI_RGN_PixelFormat_e)Atoi(inStrings[2]);
    stRegion.stOsdInitParam.stSize.u32Width = (MI_S32)Atoi(inStrings[3]);
    stRegion.stOsdInitParam.stSize.u32Height = (MI_S32)Atoi(inStrings[4]);

    MDB_EXPECT_OK("MI_RGN_GetAttr", strOut, MI_RGN_GetAttr(hHandle, &stRegion), MI_RGN_OK);
}

void Mdb_Region::SetCoverDisplayAttr(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hHandle;
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortAttr;

    hHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);
    memset(&stChnPortAttr, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);
    stChnPort.s32DevId = (MI_S32)Atoi(inStrings[2]);
    stChnPort.s32ChnId = (MI_S32)Atoi(inStrings[3]);
    stChnPort.s32OutputPortId = (MI_U32)Atoi(inStrings[4]);
    stChnPortAttr.bShow = (MI_BOOL)Atoi(inStrings[5]);
    stChnPortAttr.stPoint.u32X = (MI_U32)Atoi(inStrings[6]);
    stChnPortAttr.stPoint.u32Y = (MI_U32)Atoi(inStrings[7]);
    stChnPortAttr.unPara.stCoverChnPort.u32Layer = (MI_U32)Atoi(inStrings[8]);
    stChnPortAttr.unPara.stCoverChnPort.stSize.u32Width = (MI_U32)Atoi(inStrings[9]);
    stChnPortAttr.unPara.stCoverChnPort.stSize.u32Height = (MI_U32)Atoi(inStrings[10]);
    stChnPortAttr.unPara.stCoverChnPort.u32Color = Rgb2Yuv((MI_U32)Atoi(inStrings[11]));

    MDB_EXPECT_OK("MI_RGN_SetDisplayAttr", strOut, MI_RGN_SetDisplayAttr(hHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK);
}
void Mdb_Region::SetOsdDisplayAttr(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hHandle;
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortAttr;

    hHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);
    memset(&stChnPortAttr, 0, sizeof(MI_RGN_ChnPortParam_t));
    stChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);
    stChnPort.s32DevId = (MI_S32)Atoi(inStrings[2]);
    stChnPort.s32ChnId = (MI_S32)Atoi(inStrings[3]);
    stChnPort.s32OutputPortId = (MI_U32)Atoi(inStrings[4]);
    stChnPortAttr.bShow = (MI_BOOL)Atoi(inStrings[5]);
    stChnPortAttr.stPoint.u32X = (MI_U32)Atoi(inStrings[6]);
    stChnPortAttr.stPoint.u32Y = (MI_U32)Atoi(inStrings[7]);
    stChnPortAttr.unPara.stOsdChnPort.u32Layer = (MI_U32)Atoi(inStrings[8]);
    stChnPortAttr.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = (MI_BOOL)Atoi(inStrings[9]);
    stChnPortAttr.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode = (MI_RGN_InvertColorMode_e)Atoi(inStrings[10]);
    stChnPortAttr.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold = (MI_U16)Atoi(inStrings[11]);
    stChnPortAttr.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum = (MI_U16)Atoi(inStrings[12]);
    stChnPortAttr.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum = (MI_U16)Atoi(inStrings[13]);
    stChnPortAttr.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode = (MI_RGN_AlphaMode_e)Atoi(inStrings[14]);
    if (stChnPortAttr.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode == E_MI_RGN_PIXEL_ALPHA)
    {
        stChnPortAttr.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8BgAlpha = (MI_U8)Atoi(inStrings[15]);
        stChnPortAttr.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8FgAlpha = (MI_U8)Atoi(inStrings[16]);

    }
    else if (stChnPortAttr.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode == E_MI_RGN_CONSTANT_ALPHA)
    {
        stChnPortAttr.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.u8ConstantAlpha = (MI_U8)Atoi(inStrings[15]);
    }
    MDB_EXPECT_OK("MI_RGN_SetDisplayAttr", strOut, MI_RGN_SetDisplayAttr(hHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK);
}

void Mdb_Region::SetBitMap(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_HANDLE hHandle = 0;
    MI_U8 u8FileId = 0;
    MI_U8 u8PixelType = 0;
    MI_U8 u8BitCount = 0;
    MI_RGN_Bitmap_t stBitmap;
    MI_U32 u32FileSize = 0;
    MI_U8 *pu8FileBuffer = NULL;

    hHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);
    u8FileId = (MI_U8)Atoi(inStrings[1]);
    u8PixelType = (MI_U8)Atoi(inStrings[2]);

    if (u8FileId >= MDB_RGN_BMP_MAXNUM)
    {
        printf("file %d is not exsit\n", u8FileId);
        return;
    }

    FILE *pFile = fopen(_gszFileName[u8FileId], "rb");
    if (pFile == NULL)
    {
        printf("open file %s failed \n", _gszFileName[u8FileId]);
        return;
    }

    MDB_RGN_PIXELFMT_BITCOUNT((MI_RGN_PixelFormat_e)u8PixelType, u8BitCount);

    u32FileSize = _gstSize[u8FileId].u32Height * MDB_ALIGN_UP((u8BitCount*_gstSize[u8FileId].u32Width) , 8) / 8;
    pu8FileBuffer = (MI_U8*)malloc(u32FileSize);
    if (pu8FileBuffer == NULL)
    {
        printf("malloc failed fileSize=%d\n", u32FileSize);
        fclose(pFile);
        return;
    }

    memset(pu8FileBuffer, 0, u32FileSize);
    fread(pu8FileBuffer, 1,  u32FileSize, pFile);
    fclose(pFile);

    stBitmap.stSize.u32Width = _gstSize[u8FileId].u32Width;
    stBitmap.stSize.u32Height = _gstSize[u8FileId].u32Height;
    stBitmap.ePixelFormat = (MI_RGN_PixelFormat_e)u8PixelType;
    stBitmap.pData = pu8FileBuffer;

    MDB_EXPECT_OK("MI_RGN_SetBitMap", strOut, MI_RGN_SetBitMap(hHandle, &stBitmap), MI_RGN_OK);
    free(pu8FileBuffer);
}

void Mdb_Region::SetCanvas(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_HANDLE hHandle = 0;
    MI_U8 u8FileId = 0;
    MI_U8 u8PixelType = 0;
    MI_U8 u8BitCount = 0;
    MI_U32 u32FileSize = 0;
    MI_U8 *pu8FileBuffer = NULL;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    MI_U16 i = 0;
    MI_U32 u32CopyWidthBytes = 0;
    MI_U16 u16CopyWidth = 0;
    MI_U16 u16CopyHeight = 0;

    hHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);
    u8FileId = (MI_U8)Atoi(inStrings[1]);
    u8PixelType = (MI_U8)Atoi(inStrings[2]);

    if (u8FileId >= MDB_RGN_BMP_MAXNUM)
    {
        printf("file %d is not exsit\n", u8FileId);
        return;
    }

    FILE *pFile = fopen(_gszFileName[u8FileId], "rb");
    if (pFile == NULL)
    {
        printf("open file %s failed \n", _gszFileName[u8FileId]);
        return;
    }

    MDB_RGN_PIXELFMT_BITCOUNT((MI_RGN_PixelFormat_e)u8PixelType, u8BitCount);
    u32FileSize = _gstSize[u8FileId].u32Height * MDB_ALIGN_UP((u8BitCount*_gstSize[u8FileId].u32Width) , 8) / 8;
    pu8FileBuffer = (MI_U8*)malloc(u32FileSize);
    if (pu8FileBuffer == NULL)
    {
        printf("malloc failed fileSize=%d\n", u32FileSize);
        fclose(pFile);
        return;
    }

    memset(pu8FileBuffer, 0, u32FileSize);
    fread(pu8FileBuffer, 1,  u32FileSize, pFile);
    fclose(pFile);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_GetCanvasInfo", strOut, MI_RGN_GetCanvasInfo(hHandle, &stCanvasInfo), MI_RGN_OK, free(pu8FileBuffer);return;);
    u16CopyHeight = stCanvasInfo.stSize.u32Height > _gstSize[u8FileId].u32Height ? _gstSize[u8FileId].u32Height : stCanvasInfo.stSize.u32Height;
    u16CopyWidth = stCanvasInfo.stSize.u32Width > _gstSize[u8FileId].u32Width ? _gstSize[u8FileId].u32Width: stCanvasInfo.stSize.u32Width;
    u32CopyWidthBytes = MDB_ALIGN_UP((u8BitCount * u16CopyWidth), 8) / 8;
    for (i = 0; i < u16CopyHeight; i++)
    {
        memcpy((void *)(stCanvasInfo.virtAddr + i * stCanvasInfo.u32Stride), pu8FileBuffer + i * u32CopyWidthBytes, u32CopyWidthBytes);
    }
    MDB_EXPECT_OK_ERRCASE("MI_RGN_UpdateCanvas", strOut, MI_RGN_UpdateCanvas(hHandle), MI_RGN_OK, free(pu8FileBuffer);return;);

    free(pu8FileBuffer);
}


void Mdb_Region::GetCanvas(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);
    MI_RGN_CanvasInfo_t stCanvasInfo;

    memset(&stCanvasInfo, 0, sizeof(MI_RGN_CanvasInfo_t));

    MDB_EXPECT_OK("MI_RGN_GetCanvasInfo", strOut, MI_RGN_GetCanvasInfo(hHandle, &stCanvasInfo), MI_RGN_OK);
}
void Mdb_Region::UpdateCanvas(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hHandle;
    hHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);

    MDB_EXPECT_OK("MI_RGN_UpdateCanvas", strOut, MI_RGN_UpdateCanvas(hHandle), MI_RGN_OK);
}

// create 1 -> destroy 1
void Mdb_Region::TestCase001(std::vector<std::string> &inStrings, std::string &strOut)
{
    MDB_EXPECT_OK_ERRCASE("MI_RGN_Create", strOut, MI_RGN_Create(_ghHandle, &_gstRgnAttr), MI_RGN_OK, {});
    MDB_EXPECT_OK_ERRCASE("MI_RGN_Destroy", strOut, MI_RGN_Destroy(_ghHandle), MI_RGN_OK, {});

    SetDefaultParam();
}

// create 1024 (cover&osd) -> destroy 1024
void Mdb_Region::TestCase002(std::vector<std::string> &inStrings, std::string &strOut)
{
    std::stringstream ss;
    MI_S32 s32ErrorCode;

    while(1)
    {
        s32ErrorCode = MI_RGN_Create(_ghHandle, &_gstRgnAttr);
        if (s32ErrorCode != MI_RGN_OK)
        {
            ss.str("");
            ss << "MI_RGN_Create[" << _ghHandle << "]";
            MDB_EXPECT(ss.str(), strOut, s32ErrorCode);
            if (_ghHandle >= MI_RGN_MAX_HANDLE)
            {
                break;
            }
            else
            {
                SetDefaultParam();
                ClearRegion(inStrings, strOut);
                return;
            }
        }
        _ghHandle++;
    }

    while(_ghHandle > 0)
    {
        s32ErrorCode = MI_RGN_Destroy(_ghHandle-1);
        if (s32ErrorCode != MI_RGN_OK)
        {
            ss.str("");
            ss << "MI_RGN_Destroy[" << _ghHandle-1 << "]";
            MDB_EXPECT(ss.str(), strOut, s32ErrorCode);
        }
        _ghHandle--;
    }

    SetDefaultParam();
}

// create cover -> attach ->dettach ->destroy
// create osd -> attach ->dettach ->destroy
// create cover -> attach ->destroy
// create osd -> attach ->destroy
void Mdb_Region::TestCase003(std::vector<std::string> &inStrings, std::string &strOut)
{
    std::stringstream ss;
    std::string strRgnType;
    std::string strDettachInfo;
    MI_BOOL bNotDettach = FALSE;

    _gstRgnAttr.eType = (MI_RGN_Type_e)Atoi(inStrings[0]);
    bNotDettach = (MI_BOOL)Atoi(inStrings[1]);

    switch(_gstRgnAttr.eType)
    {
        case E_MI_RGN_TYPE_OSD:
            strRgnType = "Osd";
            break;
        case E_MI_RGN_TYPE_COVER:
            strRgnType = "Cover";
            break;
        default:
            printf("wrong region type\n");
            SetDefaultParam();
            return;
    }

    if (bNotDettach)
    {
        strDettachInfo = " directly destroy";
    }
    else
    {
        strDettachInfo = "";
    }

    ss.str("");
    ss << "MI_RGN_Create" << "(" << strRgnType << strDettachInfo <<")";
    MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Create(_ghHandle, &_gstRgnAttr), MI_RGN_OK,
            {SetDefaultParam();ClearRegion(inStrings, strOut);return;};);
    ss.str("");
    ss << "MI_RGN_AttachToChn" << "(" << strRgnType << " " << strDettachInfo <<")";
    if (E_MI_RGN_TYPE_COVER == _gstRgnAttr.eType)
    {
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParam), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;};);
    }
    else
    {
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParamOsd), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;};);
    }

    if (!bNotDettach)
    {
        ss.str("");
        ss << "MI_RGN_DetachFromChn" << "(" << strRgnType << " " << strDettachInfo <<")";
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_DetachFromChn(_ghHandle, &_gstChnPort), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
    }
    ss.str("");
    ss << "MI_RGN_Destroy" << "(" << strRgnType << " " << strDettachInfo <<")";
    MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Destroy(_ghHandle), MI_RGN_OK,
            {SetDefaultParam();ClearRegion(inStrings, strOut);return;};);

    SetDefaultParam();
}

// create osd&cover -> attach VPE&DIVP chl(0~M)port(0~N) -> dettach -> destroy
void Mdb_Region::TestCase004(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U32 u32MaxChl = 0;
    MI_U32 u32MaxOutPort = 0;
    std::string strRgnType;
    std::string strPortType;
    std::stringstream ss;

    _gstRgnAttr.eType = (MI_RGN_Type_e)Atoi(inStrings[0]);
    _gstChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);

    switch(_gstRgnAttr.eType)
    {
        case E_MI_RGN_TYPE_OSD:
            strRgnType = "Osd";
            break;
        case E_MI_RGN_TYPE_COVER:
            strRgnType = "Cover";
            break;
        default:
            printf("wrong region type\n");
            SetDefaultParam();
            return;
    }

    switch(_gstChnPort.eModId)
    {
        case E_MI_RGN_MODID_VPE:
            u32MaxChl = (MI_U32)MDB_RGN_VPE_MAX_CH_NUM;
            u32MaxOutPort = (MI_U32)MDB_RGN_VPE_PORT_MAXNUM;
            strPortType = "Vpe";
            break;
        case E_MI_RGN_MODID_DIVP:
            u32MaxChl = (MI_U32)MDB_RGN_DIVP_MAX_CH_NUM;
            u32MaxOutPort = (MI_U32)MDB_RGN_DIVP_PORT_MAXNUM;
            strPortType = "Divp";
            break;
        default:
            printf("wrong chnPort ModId\n");
            SetDefaultParam();
            return;
    }

    // create
    ss.str("");
    ss << "MI_RGN_Create" << "(" << strRgnType << ")";
    MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Create(_ghHandle, &_gstRgnAttr), MI_RGN_OK,
            {SetDefaultParam();ClearRegion(inStrings, strOut);return;});

    // attach
    for (MI_U32 chl = 0; chl < u32MaxChl; chl++)
    {
        _gstChnPort.s32ChnId = chl;
        for (MI_U32 port = 0; port < u32MaxOutPort; port++)
        {
            _gstChnPort.s32OutputPortId = port;
            ss.str("");
            ss << "MI_RGN_AttachToChn" << "(" << strRgnType << "_" << strPortType << "_" << chl << "_" << port << ")";
            if (E_MI_RGN_TYPE_COVER == _gstRgnAttr.eType)
            {
                MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParam), MI_RGN_OK,
                    SetDefaultParam();ClearRegion(inStrings, strOut);return;);
            }
            else
            {
                MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParamOsd), MI_RGN_OK,
                    SetDefaultParam();ClearRegion(inStrings, strOut);return;);
            }
        }
    }

    // dettach
    for (MI_U32 chl = 0; chl < u32MaxChl; chl++)
    {
        _gstChnPort.s32ChnId = chl;
        for (MI_U32 port = 0; port < u32MaxOutPort; port++)
        {
            _gstChnPort.s32OutputPortId = port;
            ss.str("");
            ss << "MI_RGN_DetachFromChn" << "(" << strRgnType << "_" << strPortType << "_" << chl << "_" << port << ")";
            MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_DetachFromChn(_ghHandle, &_gstChnPort), MI_RGN_OK,
                SetDefaultParam();ClearRegion(inStrings, strOut);return;);
        }
    }

    // destroy
    ss.str();
    ss << "MI_RGN_Destroy" << "(" << strRgnType << ")";
    MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Destroy(_ghHandle), MI_RGN_OK,
        SetDefaultParam();ClearRegion(inStrings, strOut);return;);

    SetDefaultParam();
}

// create osdx4 coverx4 -> attach vpe&divp -> dettachx8 -> destroyx8
void Mdb_Region::TestCase005(std::vector<std::string> &inStrings, std::string &strOut)
{
    std::string strRgnType;
    std::string strOutPortType;
    std::stringstream ss;
    MI_U32 u32OffsetX = 0;
    MI_U32 u32OffsetY = 0;
    MI_U32 u32RgnWidth = 0;
    MI_U32 u32RgnHeight = 0;
    MI_U8 u8RgnMaxNum = 0;

    _gstRgnAttr.eType = (MI_RGN_Type_e)Atoi(inStrings[0]);
    _gstChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);

    switch(_gstRgnAttr.eType)
    {
        case E_MI_RGN_TYPE_OSD:
            strRgnType = "Osd";
            u8RgnMaxNum = MDB_RGN_OSD_MAX_NUM;
            u32RgnWidth = _gstRgnAttr.stOsdInitParam.stSize.u32Width;
            u32RgnHeight = _gstRgnAttr.stOsdInitParam.stSize.u32Height;
            break;
        case E_MI_RGN_TYPE_COVER:
            strRgnType = "Cover";
            u8RgnMaxNum = MDB_RGN_COVER_MAX_NUM;
            u32RgnWidth = _gstChnPortParam.unPara.stCoverChnPort.stSize.u32Width;
            u32RgnHeight = _gstChnPortParam.unPara.stCoverChnPort.stSize.u32Height;
            break;
        default:
            printf("wrong region type\n");
            SetDefaultParam();
            return;
    }

    switch(_gstChnPort.eModId)
    {
        case E_MI_RGN_MODID_VPE:
            strOutPortType = "Vpe";
            break;
        case E_MI_RGN_MODID_DIVP:
            strOutPortType = "Divp";
            break;
        default:
            printf("wrong chnPort ModId\n");
            SetDefaultParam();
            return;
    }

    for (MI_U8 i = 0; i < u8RgnMaxNum+1; i++)
    {
        if (u32OffsetX > 1920)
        {
            u32OffsetX = 0;
            u32OffsetY += u32RgnHeight;
        }

        if (E_MI_RGN_TYPE_COVER == _gstRgnAttr.eType)
        {
            _gstChnPortParam.stPoint.u32X = u32OffsetX;
            _gstChnPortParam.stPoint.u32Y = u32OffsetY;
        }
        else
        {
            _gstChnPortParamOsd.stPoint.u32X = u32OffsetX;
            _gstChnPortParamOsd.stPoint.u32Y = u32OffsetY;
        }

        ss.str("");
        ss << "MI_RGN_Create" << "(" << strRgnType << "_" << _ghHandle << "_" << strOutPortType << ")";
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Create(_ghHandle, &_gstRgnAttr), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});

        ss.str("");
        ss << "MI_RGN_AttachToChn" << "(" << strRgnType << "_" << _ghHandle << "_" << strOutPortType << ")";

        if (u8RgnMaxNum == i)
        {
            if (E_MI_RGN_TYPE_COVER == _gstRgnAttr.eType)
            {
                MDB_EXPECT_FAIL_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParam), MI_RGN_OK, SetDefaultParam();ClearRegion(inStrings, strOut);return;);
            }
            else
            {
                MDB_EXPECT_FAIL_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParamOsd), MI_RGN_OK, SetDefaultParam();ClearRegion(inStrings, strOut);return;);
            }
            break;
        }
        else
        {
            if (E_MI_RGN_TYPE_COVER == _gstRgnAttr.eType)
            {
                MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParam), MI_RGN_OK, SetDefaultParam();ClearRegion(inStrings, strOut);return;);
            }
            else
            {
                MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParamOsd), MI_RGN_OK, SetDefaultParam();ClearRegion(inStrings, strOut);return;);
            }
        }
        u32OffsetX += u32RgnWidth;
        _ghHandle++;
        if (_gstRgnAttr.eType == E_MI_RGN_TYPE_COVER)
        {
            _gstChnPortParam.unPara.stCoverChnPort.u32Layer++;
        }
    }

    // destroy all rgn
    while(_ghHandle >= 0)
    {
        ss.str();
        ss << "MI_RGN_Destroy" << "[" << _ghHandle << "]";
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Destroy(_ghHandle), MI_RGN_OK,
        SetDefaultParam();ClearRegion(inStrings, strOut);return;);
        _ghHandle--;
    }
    SetDefaultParam();
}

// create osd -> attach[vpe&divp] -> setbitmap -> destroy
// create osd -> setbitmap -> attach[vpe&divp] -> destroy
void Mdb_Region::TestCase006(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8FileId = 0;
    std::vector<std::string> vecStrIn;
    std::string strPortType;
    std::stringstream ss;
    char szParamIn[64];
    MI_U8 u8FlowId = 0;
    MI_U8 u8PixelType = 0;

    _gstChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[0]);
    u8FlowId = (MI_U8)Atoi(inStrings[1]);
    u8FileId = (MI_U8)Atoi(inStrings[2]);
    u8PixelType = (MI_U8)Atoi(inStrings[3]);

    memset(szParamIn, 0, sizeof(szParamIn));
    sprintf(szParamIn, "%d %d %d", (MI_U8)_ghHandle, u8FileId, u8PixelType);
    ParseStrings(szParamIn, vecStrIn);

    switch(_gstChnPort.eModId)
    {
        case E_MI_RGN_MODID_VPE:
            strPortType = "Vpe";
            break;
        case E_MI_RGN_MODID_DIVP:
            strPortType = "Divp";
            break;
        default:
            printf("wrong chnPort ModId\n");
            SetDefaultParam();
            return;
    }

    _gstRgnAttr.stOsdInitParam.stSize.u32Width = _gstSize[u8FileId].u32Width;
    _gstRgnAttr.stOsdInitParam.stSize.u32Height = _gstSize[u8FileId].u32Height;

    switch(u8FlowId)
    {
        case 0:
            // create osd -> attach -> setbitmap -> destroy
            ss.str("");
            ss << "MI_RGN_Create" << "(" << strPortType << ")";
            MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Create(_ghHandle, &_gstRgnAttr), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            ss.str("");
            ss << "MI_RGN_AttachToChn" << "(" << strPortType << ")";
            MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParamOsd), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            MDB_RGN_USE_SingleTestCase(SetBitMap, vecStrIn, strOut);
            vecStrIn.clear();
            memset(szParamIn, 0, sizeof(szParamIn));
            sprintf(szParamIn, "%d %d", (MI_U8)_gstChnPort.eModId, (MI_U8)_gstChnPort.s32ChnId);
            printf("Init %s\n", szParamIn);
            ParseStrings(szParamIn, vecStrIn);
            MDB_RGN_USE_SingleTestCase(Init, vecStrIn, strOut);
            ss.str("");
            ss << "MI_RGN_Destroy" << "(" << strPortType << ")";
            MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Destroy(_ghHandle), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            break;
        case 1:
            // create osd -> setbitmap -> attach -> destroy
            ss.str("");
            ss << "MI_RGN_Create" << "(" << strPortType << ")";
            MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Create(_ghHandle, &_gstRgnAttr), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            MDB_RGN_USE_SingleTestCase(SetBitMap, vecStrIn, strOut);
            ss.str("");
            ss << "MI_RGN_AttachToChn" << "(" << strPortType << ")";
            MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParamOsd), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            memset(szParamIn, 0, sizeof(szParamIn));
            sprintf(szParamIn, "%d %d", (MI_U8)_gstChnPort.eModId, (MI_U8)_gstChnPort.s32ChnId);
            ParseStrings(szParamIn, vecStrIn);
            MDB_RGN_USE_SingleTestCase(Init, vecStrIn, strOut);
            ss.str("");
            ss << "MI_RGN_Destroy" << "(" << strPortType << ")";
            MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Destroy(_ghHandle), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            break;
        default:
            printf("wrong flow id\n");
            SetDefaultParam();
            return;
    }

    SetDefaultParam();
}

// create osd -> getcanvas -> update -> attach -> destroy
// create osd -> attach -> getcanvas -> update -> destroy
void Mdb_Region::TestCase007(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_U8 u8FlowId = (MI_U8)Atoi(inStrings[0]);
    MI_U8 u8FileId = (MI_U8)Atoi(inStrings[1]);
    std::vector<std::string> vecStrIn;
    char szParamIn[64];
    MI_U32 u32FileSize;
    MI_U32 u32CopyBytes;
    MI_U8 *pu8FileBuffer = NULL;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    MI_RGN_Attr_t stRgnAttr;

    memset(&stCanvasInfo, 0, sizeof(MI_RGN_CanvasInfo_t));

    _gstRgnAttr.stOsdInitParam.stSize.u32Width = _gstSize[u8FileId].u32Width;
    _gstRgnAttr.stOsdInitParam.stSize.u32Height = _gstSize[u8FileId].u32Height;

    if (u8FileId >= MDB_RGN_BMP_MAXNUM)
    {
        printf("file %d is not exsit\n", u8FileId);
        return;
    }

    FILE *pFile = fopen(_gszFileName[u8FileId], "rb");
    if (pFile == NULL)
    {
        printf("open file %s failed \n", _gszFileName[u8FileId]);
        return;
    }

    u32FileSize = _gstSize[u8FileId].u32Width * _gstSize[u8FileId].u32Height * 2;
    pu8FileBuffer = (MI_U8*)malloc(u32FileSize);
    if (pu8FileBuffer == NULL)
    {
        printf("malloc failed fileSize=%d\n", u32FileSize);
        fclose(pFile);
        return;
    }

    memset(pu8FileBuffer, 0, u32FileSize);
    fread(pu8FileBuffer, 1,  u32FileSize, pFile);
    fclose(pFile);


    switch(u8FlowId)
    {
        case 0:
            stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
            stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_ARGB1555;
            stRgnAttr.stOsdInitParam.stSize.u32Height = _gstSize[u8FileId].u32Height;
            stRgnAttr.stOsdInitParam.stSize.u32Width = _gstSize[u8FileId].u32Width;
            MDB_EXPECT_OK_ERRCASE("MI_RGN_Create", strOut, MI_RGN_Create(_ghHandle, &stRgnAttr), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            MDB_EXPECT_OK_ERRCASE("MI_RGN_GetCanvasInfo", strOut, MI_RGN_GetCanvasInfo(_ghHandle, &stCanvasInfo), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            u32CopyBytes = MDB_MIN(stCanvasInfo.stSize.u32Width, _gstSize[u8FileId].u32Width) * 2;
            for (MI_U32 i = 0; i < MDB_MIN(stCanvasInfo.stSize.u32Height, _gstSize[u8FileId].u32Height); i++)
            {
                memcpy((MI_U8*)stCanvasInfo.virtAddr+i*stCanvasInfo.u32Stride,
                        pu8FileBuffer+i*_gstSize[u8FileId].u32Width*2, u32CopyBytes);
            }
            free(pu8FileBuffer);

            MDB_EXPECT_OK_ERRCASE("MI_RGN_UpdateCanvas", strOut, MI_RGN_UpdateCanvas(_ghHandle), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            MDB_EXPECT_OK_ERRCASE("MI_RGN_AttachToChn", strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParamOsd), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            vecStrIn.clear();
            memset(szParamIn, 0, sizeof(szParamIn));
            sprintf(szParamIn, "%d %d", (MI_U8)_gstChnPort.eModId, (MI_U8)_gstChnPort.s32ChnId);
            printf("Init %s\n", szParamIn);
            ParseStrings(szParamIn, vecStrIn);
            MDB_RGN_USE_SingleTestCase(Init, vecStrIn, strOut);
            MDB_EXPECT_OK_ERRCASE("MI_RGN_Destroy", strOut, MI_RGN_Destroy(_ghHandle), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            break;
        case 1:
            stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
            stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_ARGB1555;
            stRgnAttr.stOsdInitParam.stSize.u32Height = _gstSize[u8FileId].u32Height;
            stRgnAttr.stOsdInitParam.stSize.u32Width = _gstSize[u8FileId].u32Width;
            MDB_EXPECT_OK_ERRCASE("MI_RGN_Create", strOut, MI_RGN_Create(_ghHandle, &stRgnAttr), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            MDB_EXPECT_OK_ERRCASE("MI_RGN_AttachToChn", strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParamOsd), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            MDB_EXPECT_OK_ERRCASE("MI_RGN_GetCanvasInfo", strOut, MI_RGN_GetCanvasInfo(_ghHandle, &stCanvasInfo), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            u32CopyBytes = MDB_MIN(stCanvasInfo.stSize.u32Width, _gstSize[u8FileId].u32Width) * 2;
            for (MI_U32 i = 0; i < MDB_MIN(stCanvasInfo.stSize.u32Height, _gstSize[u8FileId].u32Height); i++)
            {
                memcpy((MI_U8*)(stCanvasInfo.virtAddr+i*stCanvasInfo.u32Stride),
                        pu8FileBuffer+i*_gstSize[u8FileId].u32Width*2, u32CopyBytes);
            }
            free(pu8FileBuffer);

            MDB_EXPECT_OK_ERRCASE("MI_RGN_UpdateCanvas", strOut, MI_RGN_UpdateCanvas(_ghHandle), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            vecStrIn.clear();
            memset(szParamIn, 0, sizeof(szParamIn));
            sprintf(szParamIn, "%d %d", (MI_U8)_gstChnPort.eModId, (MI_U8)_gstChnPort.s32ChnId);
            ParseStrings(szParamIn, vecStrIn);
            MDB_RGN_USE_SingleTestCase(Init, vecStrIn, strOut);
            MDB_EXPECT_OK_ERRCASE("MI_RGN_Destroy", strOut, MI_RGN_Destroy(_ghHandle), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            break;
        default:
            printf("wrong flow id\n");
            SetDefaultParam();
            return;
    }

    SetDefaultParam();
}

// create osdx4 -> setbitmap ->attach -> (getbitmap -> dettach)x3 -> destroyx4
void Mdb_Region::TestCase008(std::vector<std::string> &inStrings, std::string &strOut)
{
    char szParamIn[64];
    MI_U8 u8PixelType = 0;
    std::vector<std::string> vecStrIn;

    memset(szParamIn, 0, sizeof(szParamIn));
    u8PixelType = (MI_U8)Atoi(inStrings[0]);
    vecStrIn.clear();

    sprintf(szParamIn, "0 0 0 %d", u8PixelType);
    ParseStrings(szParamIn, vecStrIn);
    MDB_RGN_USE_SingleTestCase(OneChnOnePortSetBitmap, vecStrIn, strOut);

    SetDefaultParam();
}

void Mdb_Region::TestCase009(std::vector<std::string> &inStrings, std::string &strOut)
{
    std::vector<std::string> vecStrIn;
    std::stringstream ss;
    std::string strPortType;
    MI_U32 u32OffsetX = 0;
    MI_U32 u32OffsetY = 0;
    MI_U8 u8PixelType = 0;
    char szParamIn[64];

    u8PixelType = (MI_U8)Atoi(inStrings[0]);

    for (MI_U8 i = 0; i < MDB_RGN_OSD_MAX_NUM; i++)
    {
        MI_U8 u8FileId = (MI_U8)_ghHandle;
        vecStrIn.clear();
        memset(szParamIn, 0, sizeof(szParamIn));
        sprintf(szParamIn, "%d %d %d", (MI_U8)_ghHandle, u8FileId, u8PixelType);
        ParseStrings(szParamIn, vecStrIn);

        _gstRgnAttr.stOsdInitParam.stSize.u32Width = _gstSize[i].u32Width;
        _gstRgnAttr.stOsdInitParam.stSize.u32Height = _gstSize[i].u32Height;

        ss.str("");
        ss << "MI_RGN_Create" << "(OsdRgn[" << _ghHandle << "])";
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Create(_ghHandle, &_gstRgnAttr), MI_RGN_OK,
            {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
        MDB_RGN_USE_SingleTestCase(SetBitMap, vecStrIn, strOut);

        // attach 16chn x 4port
        if (u32OffsetX > 1920)
        {
            u32OffsetX = 0;
            u32OffsetY += _gstRgnAttr.stOsdInitParam.stSize.u32Height;
        }

        _gstChnPortParamOsd.stPoint.u32X = u32OffsetX;
        _gstChnPortParamOsd.stPoint.u32Y = u32OffsetY;


        for (MI_U8 u8ChnId = 0; u8ChnId < 16; u8ChnId++)
        {
            for (MI_U8 u8VpePort = 0; u8VpePort < MDB_RGN_COVER_MAX_NUM; u8VpePort++)
            {
                _gstChnPort.eModId = E_MI_RGN_MODID_VPE;
                _gstChnPort.s32ChnId = u8ChnId;
                _gstChnPort.s32OutputPortId = u8VpePort;
                ss.str("");
                ss << "MI_RGN_AttachToChn" << "(OsdRgn[" << _ghHandle << "]_" << "Vpe_Chn" << u8ChnId << "_Port" << u8VpePort;
                MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParamOsd), MI_RGN_OK,
                    {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            }

            for (MI_U8 u8DivpPort = 0; u8DivpPort < MDB_RGN_COVER_MAX_NUM; u8DivpPort++)
            {
                _gstChnPort.eModId = E_MI_RGN_MODID_DIVP;
                _gstChnPort.s32ChnId = u8ChnId;
                _gstChnPort.s32OutputPortId = u8DivpPort;
                ss << "MI_RGN_AttachToChn" << "(OsdRgn[" << _ghHandle << "]_" "Divp_Chn" << u8ChnId << "_Port" << u8DivpPort;
                MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParamOsd), MI_RGN_OK,
                    {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
            }
        }

        u32OffsetX += _gstRgnAttr.stOsdInitParam.stSize.u32Width;
        _ghHandle++;
    }

    // blt
    for (MI_U8 u8ChnId = 0; u8ChnId < 16; u8ChnId++)
    {
        vecStrIn.clear();
        memset(szParamIn, 0, sizeof(szParamIn));
        sprintf(szParamIn, "%d %d", (MI_U8)E_MI_RGN_MODID_VPE, u8ChnId);
        printf("Init %s\n", szParamIn);
        ParseStrings(szParamIn, vecStrIn);
        MDB_RGN_USE_SingleTestCase(Init, vecStrIn, strOut);

        vecStrIn.clear();
        memset(szParamIn, 0, sizeof(szParamIn));
        sprintf(szParamIn, "%d %d", (MI_U8)E_MI_RGN_MODID_DIVP, u8ChnId);
        printf("Init %s\n", szParamIn);
        ParseStrings(szParamIn, vecStrIn);
        MDB_RGN_USE_SingleTestCase(Init, vecStrIn, strOut);
    }

    // destroy all rgn
    while(_ghHandle > 0)
    {
        ss.str();
        ss << "MI_RGN_Destroy" << "(OsdRgn[" << _ghHandle-1 << "]";
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Destroy(_ghHandle-1), MI_RGN_OK,
        {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
        _ghHandle--;
    }

    SetDefaultParam();
}

void Mdb_Region::TestCase010(std::vector<std::string> &inStrings, std::string &strOut)
{
    std::vector<std::string> vecStrIn;
    std::stringstream ss;
    char szParamIn[64];
    MI_U8 u8PixelType = (MI_U8)Atoi(inStrings[0]);

    // vpe
    for (MI_U8 i = 0; i < MDB_RGN_COVER_MAX_NUM; i++)
    {
        for (MI_U8 j = 0; j < 16; j++)
        {
            vecStrIn.clear();
            memset(szParamIn, 0, sizeof(szParamIn));
            sprintf(szParamIn, "%d %d %d %d", (MI_U8)E_MI_RGN_MODID_VPE, j, i, u8PixelType);
            ParseStrings(szParamIn, vecStrIn);
            MDB_RGN_USE_SingleTestCase(OneChnOnePortSetBitmap, vecStrIn, strOut);
        }
    }

    // divp
    for (MI_U8 i = 0; i < MDB_RGN_COVER_MAX_NUM; i++)
    {
        for (MI_U8 j = 0; j < 16; j++)
        {
            vecStrIn.clear();
            memset(szParamIn, 0, sizeof(szParamIn));
            sprintf(szParamIn, "%d %d %d %d", (MI_U8)E_MI_RGN_MODID_DIVP, j, i, u8PixelType);
            ParseStrings(szParamIn, vecStrIn);
            MDB_RGN_USE_SingleTestCase(OneChnOnePortSetBitmap, vecStrIn, strOut);
        }
    }

    SetDefaultParam();
}

void Mdb_Region::TestCase011(std::vector<std::string> &inStrings, std::string &strOut)
{
    std::string strRgnType;
    std::string strOutPortType;
    std::stringstream ss;
    std::vector<std::string> vecStrIn;
    MI_U32 u32OffsetX = 0;
    MI_U32 u32OffsetY = 0;
    MI_U32 u32RgnMaxHeight = 0;
    char szParamIn[64];
    MI_U8 u8PixelType;

    _gstChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[0]);
    _gstChnPort.s32ChnId = (MI_U32)Atoi(inStrings[1]);
    _gstChnPort.s32OutputPortId = (MI_U32)Atoi(inStrings[2]);
    u8PixelType = (MI_U32)Atoi(inStrings[3]);

    switch(_gstChnPort.eModId)
    {
        case E_MI_RGN_MODID_VPE:
            strOutPortType = "Vpe";
            break;
        case E_MI_RGN_MODID_DIVP:
            strOutPortType = "Divp";
            break;
        default:
            printf("wrong chnPort ModId\n");
            SetDefaultParam();
            return;
    }

    printf("osd \n");
    for (MI_U8 i = 0; i < MDB_RGN_OSD_MAX_NUM; i++)
    {
        MI_U8 u8FileId = (MI_U8)_ghHandle;
        vecStrIn.clear();
        memset(szParamIn, 0, sizeof(szParamIn));
        sprintf(szParamIn, "%d %d %d", (MI_U8)_ghHandle, u8FileId, u8PixelType);
        ParseStrings(szParamIn, vecStrIn);

        if (u32OffsetX > 1920)
        {
            u32OffsetX = 0;
            u32OffsetY += u32RgnMaxHeight;
            u32RgnMaxHeight = 0;
        }
        else
        {
            u32RgnMaxHeight = MDB_MAX(u32RgnMaxHeight, _gstSize[i].u32Height);
        }

        _gstChnPortParamOsd.stPoint.u32X = u32OffsetX;
        _gstChnPortParamOsd.stPoint.u32Y = u32OffsetY;
        _gstRgnAttr.stOsdInitParam.stSize.u32Width = _gstSize[i].u32Width;
        _gstRgnAttr.stOsdInitParam.stSize.u32Height = _gstSize[i].u32Height;

        ss.str("");
        ss << "MI_RGN_Create" << "(" << "Osd" << "_" << _ghHandle << "_" << strOutPortType << ")";
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Create(_ghHandle, &_gstRgnAttr), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
        MDB_RGN_USE_SingleTestCase(SetBitMap, vecStrIn, strOut);
        ss.str("");
        ss << "MI_RGN_AttachToChn" << "(" << strRgnType << "_" << _ghHandle << "_" << strOutPortType << ")";
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParamOsd), MI_RGN_OK, SetDefaultParam();ClearRegion(inStrings, strOut);return;);

        u32OffsetX += _gstRgnAttr.stOsdInitParam.stSize.u32Width;
        _ghHandle++;
    }

    // cover can only attach to port0&port3 of vpe and port0 of divp
    printf("cover\n");
    for (MI_U8 i = 0; i < MDB_RGN_COVER_MAX_NUM; i++)
    {
        _gstRgnAttr.eType = E_MI_RGN_TYPE_COVER;

        ss.str("");
        ss << "MI_RGN_Create" << "(" << "Cover" << "_" << _ghHandle << "_" << strOutPortType << ")";
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Create(_ghHandle, &_gstRgnAttr), MI_RGN_OK,
                {SetDefaultParam();ClearRegion(inStrings, strOut);return;});

        ss.str("");
        ss << "MI_RGN_AttachToChn" << "(" << strRgnType << "_" << _ghHandle << "_" << strOutPortType << ")";
        memcpy(&_gstChnPortParam, &_gstCoverChnPortParam[i], sizeof(MI_RGN_ChnPortParam_t));
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParam), MI_RGN_OK, SetDefaultParam();ClearRegion(inStrings, strOut);return;);
        _ghHandle++;
    }

    vecStrIn.clear();
    memset(szParamIn, 0, sizeof(szParamIn));
    sprintf(szParamIn, "%d %d", (MI_U8)_gstChnPort.eModId, (MI_U8)_gstChnPort.s32ChnId);
    printf("Init %s\n", szParamIn);
    ParseStrings(szParamIn, vecStrIn);
    MDB_RGN_USE_SingleTestCase(Init, vecStrIn, strOut);


    // destroy all rgn
    printf("destroy all rgn\n");
    _ghHandle--;

    while(_ghHandle >= 0)
    {
        ss.str();
        ss << "MI_RGN_Destroy" << "[" << _ghHandle << "]";
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Destroy(_ghHandle), MI_RGN_OK,
        SetDefaultParam();ClearRegion(inStrings, strOut);return;);
        _ghHandle--;
    }
    SetDefaultParam();
}

void Mdb_Region::OneRegionOneAttach(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hHandle = 233;
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnAttr;
    MI_RGN_Attr_t stRgnAttr;

    stRgnAttr.eType = E_MI_RGN_TYPE_OSD;
    stRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_ARGB1555;
    stRgnAttr.stOsdInitParam.stSize.u32Height = 300;
    stRgnAttr.stOsdInitParam.stSize.u32Width = 300;

    stChnPort.eModId = E_MI_RGN_MODID_VPE;
    stChnPort.s32ChnId = 11;
    stChnPort.s32OutputPortId =2;
    stChnPort.s32DevId = 0;

    stChnAttr.bShow = TRUE;
    stChnAttr.stPoint.u32X = 27;
    stChnAttr.stPoint.u32Y = 25;
    stChnAttr.unPara.stCoverChnPort.stSize.u32Height = 300;
    stChnAttr.unPara.stCoverChnPort.stSize.u32Width = 300;
    stChnAttr.unPara.stCoverChnPort.u32Color = Rgb2Yuv(0x0333);
    stChnAttr.unPara.stCoverChnPort.u32Layer = 1;

    MDB_EXPECT_OK_ERRCASE("MI_RGN_Create", strOut, MI_RGN_Create(hHandle, &stRgnAttr), MI_RGN_OK, return;);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_AttachToChn", strOut, MI_RGN_AttachToChn(hHandle, &stChnPort, &stChnAttr), MI_RGN_OK, return;);

}
void Mdb_Region::OneRegionMaxAttach(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hHandle = 233;
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortParam;
    MI_U8 i = 0, j = 0;
    std::stringstream ss;
    MI_S32 s32ErrorCode;
    MI_BOOL bRun = TRUE;

    memcpy(&stRgnAttr, &_gstRgnAttr, sizeof(MI_RGN_Attr_t));
    memcpy(&stChnPortParam, &_gstChnPortParamOsd, sizeof(MI_RGN_ChnPortParam_t));

    stChnPort.eModId = E_MI_RGN_MODID_VPE;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = 0;
    stChnPort.s32OutputPortId = 0;

    MDB_EXPECT_OK_ERRCASE("MI_RGN_Create", strOut, MI_RGN_Create(hHandle, &stRgnAttr), MI_RGN_OK, return);
    while (bRun)
    {
        stChnPort.s32ChnId = i;
        j = 0;
        while (1)
        {
            stChnPort.s32OutputPortId = j;
            s32ErrorCode = MI_RGN_AttachToChn(hHandle, &stChnPort, &stChnPortParam);
            if (s32ErrorCode != MI_RGN_OK)
            {
                ss.str("");
                ss << "Error case MI_RGN_AttachToChn(" << "VPE" << "ChlId[" << (int)i << "] OutPortId[" << (int)stChnPort.s32OutputPortId << "])";
                MDB_EXPECT(ss.str(), strOut, (j == MDB_RGN_COVER_MAX_NUM) || (i == MDB_RGN_COVER_MAX_NUM));
                if (j == 4)
                {
                    break;
                }
                else if (i == MDB_RGN_COVER_MAX_NUM)
                {
                    bRun = FALSE;
                    break;
                }
                else
                {
                    return;
                }

            }
            j++;
        }
        i++;
    }
    stChnPort.eModId = E_MI_RGN_MODID_DIVP;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = 0;
    stChnPort.s32OutputPortId = 0;
    bRun = TRUE;
    i = 0;
    j = 0;
    while (bRun)
    {
        stChnPort.s32ChnId = i;
        j = 0;
        while (1)
        {
            stChnPort.s32OutputPortId = j;
            s32ErrorCode = MI_RGN_AttachToChn(hHandle, &stChnPort, &stChnPortParam);
            if (s32ErrorCode != MI_RGN_OK)
            {
                ss.str("");
                ss << "Error case MI_RGN_AttachToChn(" << "DIVP" << "ChlId[" << (int)i << "] OutPortId[" << (int)stChnPort.s32OutputPortId << "])";
                MDB_EXPECT(ss.str(), strOut, (j == MDB_RGN_COVER_MAX_NUM) || (i == MDB_RGN_DIVP_MAX_CH_NUM));
                if (j == 1)
                {
                    break;
                }
                else if (i == MDB_RGN_COVER_MAX_NUM)
                {
                    bRun = FALSE;
                    break;
                }
                else
                {
                    return;
                }

            }
            j++;
        }
        i++;
    }

}

void Mdb_Region::MaxRegionMaxAttach(std::vector<std::string> &inStrings, std::string &strOut)
{

}

void Mdb_Region::OneChnOnePortSetBitmap(std::vector<std::string> &inStrings, std::string &strOut)
{
    std::vector<std::string> vecStrIn;
    std::stringstream ss;
    std::string strPortType;
    MI_U32 u32OffsetX = 0;
    MI_U32 u32OffsetY = 0;
    char szParamIn[64];
    MI_U8 u8PixelType = 0;
    MI_U8 i = 0;

    _gstChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[0]);
    _gstChnPort.s32ChnId = (MI_S32)Atoi(inStrings[1]);
    _gstChnPort.s32OutputPortId = (MI_S32)Atoi(inStrings[2]);
    u8PixelType = (MI_S32)Atoi(inStrings[3]);

    switch(_gstChnPort.eModId)
    {
        case E_MI_RGN_MODID_VPE:
            strPortType = "Vpe";
            break;
        case E_MI_RGN_MODID_DIVP:
            strPortType = "Divp";
            break;
        default:
            printf("wrong chnPort ModId\n");
            SetDefaultParam();
            return;
    }

    for (i = 0; i < MDB_RGN_OSD_MAX_NUM; i++)
    {
        MI_U8 u8FileId = (MI_U8)_ghHandle;
        vecStrIn.clear();
        memset(szParamIn, 0, sizeof(szParamIn));
        sprintf(szParamIn, "%d %d %d", (MI_U8)_ghHandle, u8FileId, u8PixelType);
        ParseStrings(szParamIn, vecStrIn);

        if (u32OffsetX > 1920)
        {
            u32OffsetX = 0;
            u32OffsetY += _gstRgnAttr.stOsdInitParam.stSize.u32Height;
        }

        _gstChnPortParamOsd.stPoint.u32X = u32OffsetX;
        _gstChnPortParamOsd.stPoint.u32Y = u32OffsetY;
        _gstRgnAttr.stOsdInitParam.stSize.u32Width = _gstSize[i].u32Width;
        _gstRgnAttr.stOsdInitParam.stSize.u32Height = _gstSize[i].u32Height;

        ss.str("");
        ss << "MI_RGN_Create" << "(OsdRgn[" << _ghHandle << "])";
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Create(_ghHandle, &_gstRgnAttr), MI_RGN_OK,
            {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
        MDB_RGN_USE_SingleTestCase(SetBitMap, vecStrIn, strOut);
        ss.str("");
        ss << "MI_RGN_AttachToChn" << "(OsdRgn[" << _ghHandle << "]_" << strPortType << "_Chn" << _gstChnPort.s32ChnId << "_Port" << _gstChnPort.s32OutputPortId;
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_AttachToChn(_ghHandle, &_gstChnPort, &_gstChnPortParamOsd), MI_RGN_OK,
            {SetDefaultParam();ClearRegion(inStrings, strOut);return;});

        u32OffsetX += _gstRgnAttr.stOsdInitParam.stSize.u32Width;
        _ghHandle++;
    }

    vecStrIn.clear();
    memset(szParamIn, 0, sizeof(szParamIn));
    sprintf(szParamIn, "%d %d", (MI_U8)_gstChnPort.eModId, (MI_U8)_gstChnPort.s32ChnId);
    printf("Init %s\n", szParamIn);
    ParseStrings(szParamIn, vecStrIn);
    MDB_RGN_USE_SingleTestCase(Init, vecStrIn, strOut);

    //getchar();
    for (i = 0; i < MDB_RGN_OSD_MAX_NUM-1; i++)
    {
        ss.str("");
        ss << "MI_RGN_DetachFromChn" << "(OsdRgn[" << (MI_U8)_ghHandle-1-i << "])";
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_DetachFromChn(_ghHandle-1-i, &_gstChnPort), MI_RGN_OK,
            {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
        vecStrIn.clear();
        memset(szParamIn, 0, sizeof(szParamIn));
        sprintf(szParamIn, "%d %d", (MI_U8)_gstChnPort.eModId, (MI_U8)_gstChnPort.s32ChnId);
        printf("Init %s\n", szParamIn);
        ParseStrings(szParamIn, vecStrIn);
        MDB_RGN_USE_SingleTestCase(Init, vecStrIn, strOut);
        //getchar();
    }

    // destroy all rgn
    while(_ghHandle > 0)
    {
        ss.str();
        ss << "MI_RGN_Destroy" << "(OsdRgn[" << _ghHandle-1 << "]";
        MDB_EXPECT_OK_ERRCASE(ss.str(), strOut, MI_RGN_Destroy(_ghHandle-1), MI_RGN_OK,
        {SetDefaultParam();ClearRegion(inStrings, strOut);return;});
        _ghHandle--;
    }

    SetDefaultParam();
}

void Mdb_Region::MaxRegion(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_HANDLE hHandle = 0;
    std::stringstream ss;
    MI_S32 s32ErrorCode;

    memcpy(&stRgnAttr, &_gstRgnAttr, sizeof(MI_RGN_Attr_t));

    while(1)
    {
        s32ErrorCode = MI_RGN_Create(hHandle, &stRgnAttr);
        if (s32ErrorCode != MI_RGN_OK)
        {
            ss << "error case:";
            ss << "MI_RGN_Create(" << (int)hHandle << ")";
            MDB_EXPECT(ss.str(), strOut, (hHandle == MI_RGN_MAX_HANDLE));
            break;
        }
        hHandle++;
    }
}

void Mdb_Region::ClearRegion(std::vector<std::string> &inStrings, std::string &strOut)
{
    std::stringstream ss;
    MI_S32 s32ErrCode;

    for (MI_S16 i = 0; i < MI_RGN_MAX_HANDLE; i++)
    {
        s32ErrCode = MI_RGN_Destroy(MI_RGN_HANDLE(i));
        if (s32ErrCode == MI_RGN_OK)
        {
            ss.str("");
            ss << "MI_RGN_Destroy(" << i << ") OK\n";
            Print(strOut, ss.str(), PRINT_COLOR_GREEN, PRINT_MODE_HIGHTLIGHT);
        }
    }
}
void Mdb_Region::CreateAttachDetechDestroy(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hHandle = 233;
    MI_RGN_Attr_t stRgnAttr;
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortParam;

    // set value
    memcpy(&stRgnAttr, &_gstRgnAttr, sizeof(MI_RGN_Attr_t));
    memcpy(&stChnPortParam, &_gstChnPortParamOsd, sizeof(MI_RGN_ChnPortParam_t));
    stChnPort.eModId = E_MI_RGN_MODID_VPE;
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = 22;
    stChnPort.s32OutputPortId = 1;

    MDB_EXPECT_OK_ERRCASE("MI_RGN_Create", strOut, MI_RGN_Create(hHandle, &stRgnAttr), MI_RGN_OK, return);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_AttachToChn", strOut, MI_RGN_AttachToChn(hHandle, &stChnPort, &stChnPortParam), MI_RGN_OK, return);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_DetachFromChn", strOut, MI_RGN_DetachFromChn(hHandle, &stChnPort), MI_RGN_OK, return);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_Destroy", strOut, MI_RGN_Destroy(hHandle), MI_RGN_OK, return);
}

void Mdb_Region::ParseStrings(const char *pStr, std::vector<std::string> &strInStrings)
{
    char *pStr1 = (char*)malloc(strlen(pStr)+1);
    char *pStr2 = NULL;
    int intCmdSize = 0;
    std::string strTmp;

    memset(pStr1, 0, strlen(pStr)+1);
    strcpy(pStr1, pStr);

    if (pStr == NULL)
    {
        printf("pStr is NULL!\n");
        return;
    }

    while (1)
    {
        while (*pStr1 == ' ' && *pStr1 != 0)
        {
            pStr1++;
        }
        pStr2 = pStr1;
        while (*pStr2 != ' ' && *pStr2 != 0)
        {
            pStr2++;
        }
        intCmdSize = pStr2 - pStr1;
        if (intCmdSize != 0)
        {
            strTmp.assign(pStr1, intCmdSize);
            strInStrings.push_back(strTmp);
            pStr1 = pStr2;
        }
        else
        {
            break;
        }
    }
}

void Mdb_Region::SetDefaultParam()
{
    _ghHandle = 0;

    _gstRgnAttr.eType = E_MI_RGN_TYPE_OSD;
    _gstRgnAttr.stOsdInitParam.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_ARGB1555;
    _gstRgnAttr.stOsdInitParam.stSize.u32Width = 40;
    _gstRgnAttr.stOsdInitParam.stSize.u32Height = 40;

    _gstChnPort.eModId = E_MI_RGN_MODID_VPE;
    _gstChnPort.s32DevId = 0;
    _gstChnPort.s32ChnId = 0;
    _gstChnPort.s32OutputPortId = 0;

    _gstChnPortParam.bShow = TRUE;
    _gstChnPortParam.stPoint.u32X = 0;
    _gstChnPortParam.stPoint.u32Y = 0;
    _gstChnPortParam.unPara.stCoverChnPort.u32Layer = 0;
    _gstChnPortParam.unPara.stCoverChnPort.stSize.u32Width = 40;
    _gstChnPortParam.unPara.stCoverChnPort.stSize.u32Height = 40;
    _gstChnPortParam.unPara.stCoverChnPort.u32Color = 0;

    _gstChnPortParamOsd.bShow = TRUE;
    _gstChnPortParamOsd.stPoint.u32X = 0;
    _gstChnPortParamOsd.stPoint.u32Y = 0;
    _gstChnPortParamOsd.unPara.stOsdChnPort.u32Layer = 0;
    _gstChnPortParamOsd.unPara.stOsdChnPort.stColorInvertAttr.bEnableColorInv = FALSE;
    _gstChnPortParamOsd.unPara.stOsdChnPort.stColorInvertAttr.eInvertColorMode = E_MI_RGN_ABOVE_LUMA_THRESHOLD;
    _gstChnPortParamOsd.unPara.stOsdChnPort.stColorInvertAttr.u16LumaThreshold = 0;
    _gstChnPortParamOsd.unPara.stOsdChnPort.stColorInvertAttr.u16WDivNum = 0;
    _gstChnPortParamOsd.unPara.stOsdChnPort.stColorInvertAttr.u16HDivNum = 0;

    _gvecStrIn.clear();
}
void Mdb_Region::SelectFillDataMode(std::vector<std::string> &inStrings, std::string &strOut)
{
    if (Atoi(inStrings[0]) == 0 || Atoi(inStrings[0]) == 1)
    {
        bSelectMode = Atoi(inStrings[0]);
    }
}
void Mdb_Region::VpeDivpAddData(std::vector<std::string> &inStrings, std::string &strOut)
{
    int intFormat = Atoi(inStrings[0]);
    std::vector<std::string> vecStrIn;
    std::stringstream ss;

    ss << "0 0 " <<intFormat<< " " << _gstSize[intFormat * 8].u32Width << " " << _gstSize[intFormat * 8].u32Height;
    ParseStrings(ss.str().c_str(), vecStrIn);
    Create(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "1 0 " <<intFormat<< " " << _gstSize[intFormat * 8 + 1].u32Width << " " << _gstSize[intFormat * 8 + 1].u32Height;
    ParseStrings(ss.str().c_str(), vecStrIn);
    Create(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "2 0 " <<intFormat<< " " << _gstSize[intFormat * 8 + 2].u32Width << " " << _gstSize[intFormat * 8 + 2].u32Height;
    ParseStrings(ss.str().c_str(), vecStrIn);
    Create(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "3 0 " <<intFormat<< " " << _gstSize[intFormat * 8 + 3].u32Width << " " << _gstSize[intFormat * 8 + 3].u32Height;
    ParseStrings(ss.str().c_str(), vecStrIn);
    Create(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "4 0 " <<intFormat<< " " << _gstSize[intFormat * 8 + 4].u32Width << " " << _gstSize[intFormat * 8 + 4].u32Height;
    ParseStrings(ss.str().c_str(), vecStrIn);
    Create(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "5 0 " <<intFormat<< " " << _gstSize[intFormat * 8 + 5].u32Width << " " << _gstSize[intFormat * 8 + 5].u32Height;
    ParseStrings(ss.str().c_str(), vecStrIn);
    Create(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "6 0 " <<intFormat<< " " << _gstSize[intFormat * 8 + 6].u32Width << " " << _gstSize[intFormat * 8 + 6].u32Height;
    ParseStrings(ss.str().c_str(), vecStrIn);
    Create(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "7 0 " <<intFormat<< " " << _gstSize[intFormat * 8 + 7].u32Width << " " << _gstSize[intFormat * 8 + 7].u32Height;
    ParseStrings(ss.str().c_str(), vecStrIn);
    Create(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "0 " <<intFormat * 8<<" "<<intFormat;
    ParseStrings(ss.str().c_str(), vecStrIn);
    if (bSelectMode)
        SetCanvas(vecStrIn, strOut);
    else
        SetBitMap(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "1 " <<intFormat * 8 + 1<<" "<<intFormat;
    ParseStrings(ss.str().c_str(), vecStrIn);
    if (bSelectMode)
        SetCanvas(vecStrIn, strOut);
    else
        SetBitMap(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "2 " <<intFormat * 8 + 2<<" "<<intFormat;
    ParseStrings(ss.str().c_str(), vecStrIn);
    if (bSelectMode)
        SetCanvas(vecStrIn, strOut);
    else
        SetBitMap(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "3 " <<intFormat * 8 + 3<<" "<<intFormat;
    ParseStrings(ss.str().c_str(), vecStrIn);
    if (bSelectMode)
        SetCanvas(vecStrIn, strOut);
    else
        SetBitMap(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "4 " <<intFormat * 8 + 4<<" "<<intFormat;
    ParseStrings(ss.str().c_str(), vecStrIn);
    if (bSelectMode)
        SetCanvas(vecStrIn, strOut);
    else
        SetBitMap(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "5 " <<intFormat * 8 + 5<<" "<<intFormat;
    ParseStrings(ss.str().c_str(), vecStrIn);
    if (bSelectMode)
        SetCanvas(vecStrIn, strOut);
    else
        SetBitMap(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "6 " <<intFormat * 8 + 6<<" "<<intFormat;
    ParseStrings(ss.str().c_str(), vecStrIn);
    if (bSelectMode)
        SetCanvas(vecStrIn, strOut);
    else
        SetBitMap(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "7 " <<intFormat * 8 + 7<<" "<<intFormat;
    ParseStrings(ss.str().c_str(), vecStrIn);
    if (bSelectMode)
        SetCanvas(vecStrIn, strOut);
    else
        SetBitMap(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "8 1 0 0 0";
    ParseStrings(ss.str().c_str(), vecStrIn);
    Create(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "9 1 0 0 0";
    ParseStrings(ss.str().c_str(), vecStrIn);
    Create(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "10 1 0 0 0";
    ParseStrings(ss.str().c_str(), vecStrIn);
    Create(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << "11 1 0 0 0";
    ParseStrings(ss.str().c_str(), vecStrIn);
    Create(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    intCurFormat = intFormat;
}
void Mdb_Region::VpeDivpAttachData(std::vector<std::string> &inStrings, std::string &strOut)
{
    std::vector<std::string> vecStrIn;
    std::stringstream ss;
    int i = 0, j = 0, intPort[4];
    int intMod = Atoi(inStrings[0]);
    int intMaxChn = Atoi(inStrings[1]);
    intPort[0] = Atoi(inStrings[2]);
    intPort[1] = Atoi(inStrings[3]);
    intPort[2] = Atoi(inStrings[4]);
    intPort[3] = Atoi(inStrings[5]);

    for (i = 0; i < intMaxChn; i++)
    {
        for (j = 0; j < 4; j++)
        {
            if (!intPort[j])
                continue;
            ss << "0 " << intMod << " 0 " << i << " " << j << " 1 " << _gstPos[intCurFormat * 8].u32X <<" " << _gstPos[intCurFormat * 8].u32Y <<" 323 0 0 0 0 0 0 0 255";
            ParseStrings(ss.str().c_str(), vecStrIn);
            AttachOsd(vecStrIn, strOut);
            vecStrIn.clear();
            ss.str("");

            ss << "1 " << intMod << " 0 " << i << " " << j << " 1 " << _gstPos[intCurFormat * 8 + 1].u32X <<" " << _gstPos[intCurFormat * 8 + 1].u32Y <<" 33 0 0 0 0 0 0 0 255";
            ParseStrings(ss.str().c_str(), vecStrIn);
            AttachOsd(vecStrIn, strOut);
            vecStrIn.clear();
            ss.str("");

            ss << "2 " << intMod << " 0 " << i << " " << j << " 1 " << _gstPos[intCurFormat * 8 + 2].u32X <<" " << _gstPos[intCurFormat * 8 + 2].u32Y <<" 22 0 0 0 0 0 0 0 255";
            ParseStrings(ss.str().c_str(), vecStrIn);
            AttachOsd(vecStrIn, strOut);
            vecStrIn.clear();
            ss.str("");

            ss << "3 " << intMod << " 0 " << i << " " << j << " 1 " << _gstPos[intCurFormat * 8 + 3].u32X <<" " << _gstPos[intCurFormat * 8 + 3].u32Y <<" 300 0 0 0 0 0 0 0 255";
            ParseStrings(ss.str().c_str(), vecStrIn);
            AttachOsd(vecStrIn, strOut);
            vecStrIn.clear();
            ss.str("");

            ss << "4 " << intMod << " 0 " << i << " " << j << " 1 " << _gstPos[intCurFormat * 8 + 4].u32X <<" " << _gstPos[intCurFormat * 8 + 4].u32Y <<" 222 0 0 0 0 0 0 0 255";
            ParseStrings(ss.str().c_str(), vecStrIn);
            AttachOsd(vecStrIn, strOut);
            vecStrIn.clear();
            ss.str("");

            ss << "5 " << intMod << " 0 " << i << " " << j << " 1 " << _gstPos[intCurFormat * 8 + 5].u32X <<" " << _gstPos[intCurFormat * 8 + 5].u32Y <<" 333 0 0 0 0 0 0 0 255";
            ParseStrings(ss.str().c_str(), vecStrIn);
            AttachOsd(vecStrIn, strOut);
            vecStrIn.clear();
            ss.str("");

            ss << "6 " << intMod << " 0 " << i << " " << j << " 1 " << _gstPos[intCurFormat * 8 + 6].u32X <<" " << _gstPos[intCurFormat * 8 + 6].u32Y <<" 420 0 0 0 0 0 0 0 255";
            ParseStrings(ss.str().c_str(), vecStrIn);
            AttachOsd(vecStrIn, strOut);
            vecStrIn.clear();
            ss.str("");

            ss << "7 " << intMod << " 0 " << i << " " << j << " 1 " << _gstPos[intCurFormat * 8 + 7].u32X <<" " << _gstPos[intCurFormat * 8 + 7].u32Y <<" 678 0 0 0 0 0 0 0 255";
            ParseStrings(ss.str().c_str(), vecStrIn);
            AttachOsd(vecStrIn, strOut);
            vecStrIn.clear();
            ss.str("");

            ss << "8 " << intMod << " 0 " << i << " " << j << " 1 1200 1200 0 1000 1000 0xFF";
            ParseStrings(ss.str().c_str(), vecStrIn);
            AttachCover(vecStrIn, strOut);
            vecStrIn.clear();
            ss.str("");

            ss << "9 " << intMod << " 0 " << i << " " << j << " 1 1800 1800 1 1000 1000 0xFF00";
            ParseStrings(ss.str().c_str(), vecStrIn);
            AttachCover(vecStrIn, strOut);
            vecStrIn.clear();
            ss.str("");

            ss << "10 " << intMod << " 0 " << i << " " << j << " 1 2400 2400 2 1000 1000 0xFF0000";
            ParseStrings(ss.str().c_str(), vecStrIn);
            AttachCover(vecStrIn, strOut);
            vecStrIn.clear();
            ss.str("");

            ss << "11 " << intMod << " 0 " << i << " " << j << " 1 3000 3000 3 1000 1000 0x1F";
            ParseStrings(ss.str().c_str(), vecStrIn);
            AttachCover(vecStrIn, strOut);
            vecStrIn.clear();
            ss.str("");
        }
    }
}
void Mdb_Region::VpeDivpRmData(std::vector<std::string> &inStrings, std::string &strOut)
{
    std::vector<std::string> vecStrIn;

    ParseStrings("0", vecStrIn);
    Destroy(vecStrIn, strOut);
    vecStrIn.clear();
    ParseStrings("1", vecStrIn);
    Destroy(vecStrIn, strOut);
    vecStrIn.clear();
    ParseStrings("2", vecStrIn);
    Destroy(vecStrIn, strOut);
    vecStrIn.clear();
    ParseStrings("3", vecStrIn);
    Destroy(vecStrIn, strOut);
    vecStrIn.clear();
    ParseStrings("4", vecStrIn);
    Destroy(vecStrIn, strOut);
    vecStrIn.clear();
    ParseStrings("5", vecStrIn);
    Destroy(vecStrIn, strOut);
    vecStrIn.clear();
    ParseStrings("6", vecStrIn);
    Destroy(vecStrIn, strOut);
    vecStrIn.clear();
    ParseStrings("7", vecStrIn);
    Destroy(vecStrIn, strOut);
    vecStrIn.clear();
    ParseStrings("8", vecStrIn);
    Destroy(vecStrIn, strOut);
    vecStrIn.clear();
    ParseStrings("9", vecStrIn);
    Destroy(vecStrIn, strOut);
    vecStrIn.clear();
    ParseStrings("10", vecStrIn);
    Destroy(vecStrIn, strOut);
    vecStrIn.clear();
    ParseStrings("11", vecStrIn);
    Destroy(vecStrIn, strOut);
    vecStrIn.clear();
}
#define PROCESS_STEP 40
#define PROCESS_CNT 25
void Mdb_Region::VpeDivpProcess(std::vector<std::string> &inStrings, std::string &strOut)
{
    std::vector<std::string> vecStrIn;
    std::stringstream ss;
    int i = 0, j = 0, k = 0, intPort[4];
    int intMod = Atoi(inStrings[0]);
    int intMaxChn = Atoi(inStrings[1]);
    intPort[0] = Atoi(inStrings[2]);
    intPort[1] = Atoi(inStrings[3]);
    intPort[2] = Atoi(inStrings[4]);
    intPort[3] = Atoi(inStrings[5]);

    for (i = 0; i < PROCESS_CNT; i++)
    {
        for (j = 0; j < intMaxChn; j++)
        {
            for (k = 0; k < 4; k++)
            {
                if (!intPort[k])
                    continue;
                ss << "0 " << intMod << " 0 "<< j << " " << k << " 1 0 "<< (int)(i * PROCESS_STEP) << " 323 0 0 0 0 0 0 0 255";
                ParseStrings(ss.str().c_str(), vecStrIn);
                SetOsdDisplayAttr(vecStrIn, strOut);
                vecStrIn.clear();
                ss.str("");

                ss << "1 " << intMod << " 0 "<< j << " " << k << " 1 64 "<< (int)(i * PROCESS_STEP) << " 33 0 0 0 0 0 0 0 255";
                ParseStrings(ss.str().c_str(), vecStrIn);
                SetOsdDisplayAttr(vecStrIn, strOut);
                vecStrIn.clear();
                ss.str("");

                ss << "2 " << intMod << " 0 "<< j << " " << k << " 1 200 "<< (int)(i * PROCESS_STEP) << " 22 0 0 0 0 0 0 0 255";
                ParseStrings(ss.str().c_str(), vecStrIn);
                SetOsdDisplayAttr(vecStrIn, strOut);
                vecStrIn.clear();
                ss.str("");

                ss << "3 " << intMod << " 0 "<< j << " " << k << " 1 "<< (int)(i * PROCESS_STEP) << " 400 300 0 0 0 0 0 0 0 255";
                ParseStrings(ss.str().c_str(), vecStrIn);
                SetOsdDisplayAttr(vecStrIn, strOut);
                vecStrIn.clear();
                ss.str("");

                ss << "4 " << intMod << " 0 "<< j << " " << k << " 1 300 "<< (int)(i * PROCESS_STEP) << " 222 0 0 0 0 0 0 0 255";
                ParseStrings(ss.str().c_str(), vecStrIn);
                SetOsdDisplayAttr(vecStrIn, strOut);
                vecStrIn.clear();
                ss.str("");

                ss << "5 " << intMod << " 0 "<< j << " " << k << " 1 364 "<< (int)(i * PROCESS_STEP) << " 333 0 0 0 0 0 0 0 255";
                ParseStrings(ss.str().c_str(), vecStrIn);
                SetOsdDisplayAttr(vecStrIn, strOut);
                vecStrIn.clear();
                ss.str("");

                ss << "6 " << intMod << " 0 "<< j << " " << k << " 1 500 "<< (int)(i * PROCESS_STEP) << " 420 0 0 0 0 0 0 0 255";
                ParseStrings(ss.str().c_str(), vecStrIn);
                SetOsdDisplayAttr(vecStrIn, strOut);
                vecStrIn.clear();
                ss.str("");

                ss << "7 " << intMod << " 0 "<< j << " " << k << " 1 "<< (int)(i * PROCESS_STEP) << " 700 678 0 0 0 0 0 0 0 255";
                ParseStrings(ss.str().c_str(), vecStrIn);
                SetOsdDisplayAttr(vecStrIn, strOut);
                vecStrIn.clear();
                ss.str("");

                ss << "8 " << intMod << " 0 "<< j << " " << k << " 1 "<< (int)(1200 + i * PROCESS_STEP) << " " << (int)(1200 + i * PROCESS_STEP) << " 0 1000 1000 0xFF";
                ParseStrings(ss.str().c_str(), vecStrIn);
                SetCoverDisplayAttr(vecStrIn, strOut);
                vecStrIn.clear();
                ss.str("");

                ss << "9 " << intMod << " 0 "<< j << " " << k << " 1 "<< (int)(1800 + i * PROCESS_STEP) << " " << (int)(1800 + i * PROCESS_STEP) << " 1 1000 1000 0xFF00";
                ParseStrings(ss.str().c_str(), vecStrIn);
                SetCoverDisplayAttr(vecStrIn, strOut);
                vecStrIn.clear();
                ss.str("");

                ss << "10 " << intMod << " 0 "<< j << " " << k << " 1 "<< (int)(2400 + i * PROCESS_STEP) << " " << (int)(2400 + i * PROCESS_STEP) << " 2 1000 1000 0xFF0000";
                ParseStrings(ss.str().c_str(), vecStrIn);
                SetCoverDisplayAttr(vecStrIn, strOut);
                vecStrIn.clear();
                ss.str("");

                ss << "11 " << intMod << " 0 "<< j << " " << k << " 1 "<< (int)(3000 + i * PROCESS_STEP) << " " << (int)(3000 + i * PROCESS_STEP) << " 3 1000 1000 0x1F";
                ParseStrings(ss.str().c_str(), vecStrIn);
                SetCoverDisplayAttr(vecStrIn, strOut);
                vecStrIn.clear();
                ss.str("");
                printf("Run mod %d channel %d port %d\n", intMod, j, k);
                usleep(200 * 1000);
            }
        }
        if (i == 14)
            break;
    }
}
void Mdb_Region::RunProcess(void *pArg)
{
    std::vector<std::string> vecStrIn;
    std::string strStr;
    std::string strOut;
    std::stringstream ss;

    MDB_REGION_DrawPara_t *pstRgnDrawPara = (MDB_REGION_DrawPara_t *)pArg;

    if (!pstRgnDrawPara)
    {
        return;
    }
    if (pstRgnDrawPara->intModId == 0)
    {
        ss << "0 " << (int)intVpeChCnt;
        ParseStrings(ss.str().c_str(), vecStrIn);
        Init(vecStrIn, strOut);
        intVpeChCnt++;
        if (intVpeChCnt == pstRgnDrawPara->intMaxChannelNum)
        {
            intVpeChCnt = 0;
        }
    }
    else
    {
        ss << "1 " << (int)intDivpChCnt;
        ParseStrings(ss.str().c_str(), vecStrIn);
        Init(vecStrIn, strOut);
        intDivpChCnt++;
        if (intDivpChCnt == pstRgnDrawPara->intMaxChannelNum)
        {
            intDivpChCnt = 0;
        }
    }
}
void Mdb_Region::InjectCover(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hRgnHandle = Atoi(inStrings[0]);
    MI_U32 u32Width = Atoi(inStrings[1]);
    MI_U32 u32Height = Atoi(inStrings[2]);
    MI_RGN_ModId_e eMod = (MI_RGN_ModId_e)Atoi(inStrings[3]);
    MI_S32 s32Chn = Atoi(inStrings[4]);
    MI_S32 s32Port = Atoi(inStrings[5]);
    MI_U32 u32PosX = Atoi(inStrings[6]);
    MI_U32 u32PoxY = Atoi(inStrings[7]);
    MI_U32 u32Layer = Atoi(inStrings[8]);
    MI_U32 u32Color = Atoi(inStrings[9]);
    MI_U16 i = 0;
    std::vector<std::string> vecStrIn;
    std::stringstream ss;


    ss << hRgnHandle << " 1 0 0 0";
    ParseStrings(ss.str().c_str(), vecStrIn);
    Create(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    ss << hRgnHandle << " " << eMod << " 0 " << s32Chn << " " << s32Port << " 1 " << u32PosX << " " << u32PoxY << " " << u32Layer << " " << u32Width << " " << u32Height << " " << u32Color;
    printf("%s\n", ss.str().c_str());
    ParseStrings(ss.str().c_str(), vecStrIn);
    AttachCover(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

}


void Mdb_Region::InjectPic(std::vector<std::string> &inStrings, std::string &strOut)
{
    int fd = 0;
    MI_U32 u32FileSize = 0, u32CopyWidthBytes = 0, u32Stride = 0;
    MI_U8 u8BitCount = 0;
    MI_RGN_HANDLE hRgnHandle = Atoi(inStrings[1]);
    MI_U32 u32Width = Atoi(inStrings[2]);
    MI_U32 u32Height = Atoi(inStrings[3]);
    MI_U8 u8Pitch = Atoi(inStrings[4]);
    MI_RGN_PixelFormat_e eFmt = (MI_RGN_PixelFormat_e)Atoi(inStrings[5]);
    MI_RGN_ModId_e eMod = (MI_RGN_ModId_e)Atoi(inStrings[6]);
    MI_S32 s32Chn = Atoi(inStrings[7]);
    MI_S32 s32Port = Atoi(inStrings[8]);
    MI_U32 u32PosX = Atoi(inStrings[9]);
    MI_U32 u32PoxY = Atoi(inStrings[10]);
    MI_U32 u32Layer = Atoi(inStrings[11]);
    MI_U8 * pu8Data = NULL;
    MI_U16 i = 0;
    std::vector<std::string> vecStrIn;
    std::stringstream ss;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    MI_RGN_Bitmap_t stBitmap;

    fd = open(inStrings[0].c_str(), O_RDONLY);
    if (fd < 0)
    {
        Print(strOut, "open file error.\n", PRINT_COLOR_RED, PRINT_MODE_HIGHTLIGHT);
        return;
    }
    MDB_RGN_PIXELFMT_BITCOUNT((MI_RGN_PixelFormat_e)eFmt, u8BitCount);
    u32Stride = (MDB_ALIGN_UP(u8BitCount * MDB_ALIGN_UP(u32Width, u8Pitch) , 8) / 8);
    u32FileSize = u32Height * u32Stride;
    pu8Data = (MI_U8 *)malloc(u32FileSize);
    ASSERT(pu8Data);
    read(fd, pu8Data, u32FileSize);
    close(fd);

    ss << hRgnHandle << " 0 " <<eFmt<< " " << u32Width << " " << u32Height;
    ParseStrings(ss.str().c_str(), vecStrIn);
    Create(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

    u32CopyWidthBytes =  MDB_ALIGN_UP(u8BitCount * u32Width , 8) / 8;
    printf("Copy bytes %d stride %d\n", u32CopyWidthBytes, u32Stride);
    if (bSelectMode)
    {
        MDB_EXPECT_OK_ERRCASE("MI_RGN_GetCanvasInfo", strOut, MI_RGN_GetCanvasInfo(hRgnHandle, &stCanvasInfo), MI_RGN_OK, return);
        for (i = 0; i < u32Height; i++)
        {
            memcpy((void *)(stCanvasInfo.virtAddr + i * stCanvasInfo.u32Stride), pu8Data + i * u32Stride, u32CopyWidthBytes);
        }
        free(pu8Data);
        MDB_EXPECT_OK("MI_RGN_UpdateCanvas", strOut, MI_RGN_UpdateCanvas(hRgnHandle), MI_RGN_OK);
    }
    else
    {
        stBitmap.ePixelFormat = eFmt;
        stBitmap.pData = pu8Data;
        stBitmap.stSize.u32Width = MDB_ALIGN_UP(u32Width, u8Pitch);
        stBitmap.stSize.u32Height = u32Height;
        MDB_EXPECT_OK_ERRCASE("MI_RGN_SetBitMap", strOut, MI_RGN_SetBitMap(hRgnHandle, &stBitmap), MI_RGN_OK, return);        
        free(pu8Data);
    }
    ss << hRgnHandle << " " << eMod << " 0 " << s32Chn << " " << s32Port << " 1 " << u32PosX <<" " << u32PoxY << " " << u32Layer << " 0 0 0 0 0 0 0 255";
    printf("%s\n", ss.str().c_str());
    ParseStrings(ss.str().c_str(), vecStrIn);
    AttachOsd(vecStrIn, strOut);
    vecStrIn.clear();
    ss.str("");

}
void Mdb_Region::ElicitWord(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hRgnHandle;

    hRgnHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);
    MDB_EXPECT_OK("ST_OSD_Init", strOut, ST_OSD_UnInit(hRgnHandle), MI_RGN_OK);
}
void Mdb_Region::InjectWord(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hRgnHandle;
    ST_OSD_Attr_T stOsdAttr;
    ST_Point_T stPoint;
    MI_RGN_CanvasInfo_t *pstCanvasInfo;
    ST_Rect_T stRect;

    memset(&stOsdAttr, 0, sizeof(ST_OSD_Attr_T));
    hRgnHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);
    
    stOsdAttr.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_ARGB1555;
    //stOsdAttr.ePixelFmt = E_MI_RGN_PIXEL_FORMAT_I4;
    
    stOsdAttr.stRgnChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);
    stOsdAttr.stRgnChnPort.s32DevId = 0;
    stOsdAttr.stRgnChnPort.s32ChnId = (MI_U16)Atoi(inStrings[2]);
    stOsdAttr.stRgnChnPort.s32OutputPortId = (MI_U16)Atoi(inStrings[3]);
    stOsdAttr.stRect.u32X = (MI_U32)Atoi(inStrings[4]);
    stOsdAttr.stRect.u32Y = (MI_U32)Atoi(inStrings[5]);
    stOsdAttr.stRect.u16PicW = (MI_U16)Atoi(inStrings[6]);
    stOsdAttr.stRect.u16PicH = (MI_U16)Atoi(inStrings[7]);
    stOsdAttr.u32Layer = (MI_U16)Atoi(inStrings[8]);

    snprintf(stOsdAttr.szBitmapFile, sizeof(stOsdAttr.szBitmapFile) - 1, "%s", DOT_FONT_FILE);
    snprintf(stOsdAttr.szAsciiBitmapFile, sizeof(stOsdAttr.szAsciiBitmapFile) - 1, "%s", ASCII_FONT_FILE);
    MDB_EXPECT_OK("ST_OSD_Init", strOut, ST_OSD_Init(hRgnHandle, &stOsdAttr), MI_RGN_OK);
    stPoint.u32X = 0;
    stPoint.u32Y = 0;
    MDB_EXPECT_OK("ST_OSD_GetCanvasInfo", strOut, ST_OSD_GetCanvasInfo(hRgnHandle, &pstCanvasInfo), MI_RGN_OK);
    stRect.u32X = 0;
    stRect.u32Y = 0;
    stRect.u16PicW = pstCanvasInfo->stSize.u32Width;
    stRect.u16PicH = pstCanvasInfo->stSize.u32Height;
    MDB_EXPECT_OK("ST_OSD_Clear", strOut, ST_OSD_Clear(hRgnHandle, &stRect), MI_RGN_OK);  
    MDB_EXPECT_OK("ST_OSD_DrawText", strOut, ST_OSD_DrawText(hRgnHandle, stPoint, inStrings[9].c_str(), RGB2PIXEL1555(0, 255, 0), DMF_Font_Size_16x16), MI_RGN_OK);
    MDB_EXPECT_OK("ST_OSD_Update", strOut, ST_OSD_Update(hRgnHandle), MI_RGN_OK);
}
void Mdb_Region::UpdateWord(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hRgnHandle;
    ST_Point_T stPoint;
    MI_RGN_CanvasInfo_t *pstCanvasInfo;
    ST_Rect_T stRect;

    hRgnHandle = (MI_RGN_HANDLE)Atoi(inStrings[0]);
    stPoint.u32X = 0;
    stPoint.u32Y = 0;
    MDB_EXPECT_OK("ST_OSD_GetCanvasInfo", strOut, ST_OSD_GetCanvasInfo(hRgnHandle, &pstCanvasInfo), MI_RGN_OK);
    stRect.u32X = 0;
    stRect.u32Y = 0;
    stRect.u16PicW = pstCanvasInfo->stSize.u32Width;
    stRect.u16PicH = pstCanvasInfo->stSize.u32Height;
    MDB_EXPECT_OK("ST_OSD_Clear", strOut, ST_OSD_Clear(hRgnHandle, &stRect), MI_RGN_OK);  
    MDB_EXPECT_OK("ST_OSD_DrawText", strOut, ST_OSD_DrawText(hRgnHandle, stPoint, inStrings[1].c_str(), RGB2PIXEL1555(0, 255, 0), DMF_Font_Size_16x16), MI_RGN_OK);
    MDB_EXPECT_OK("ST_OSD_Update", strOut, ST_OSD_Update(hRgnHandle), MI_RGN_OK);
}
void Mdb_Region::SetOsdPos(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hRgnHandle = Atoi(inStrings[0]);
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortAttr;

    stChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = (MI_S32)Atoi(inStrings[2]);
    stChnPort.s32OutputPortId = (MI_S32)Atoi(inStrings[3]);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_GetDisplayAttr", strOut, MI_RGN_GetDisplayAttr(hRgnHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK, return);
    stChnPortAttr.stPoint.u32X = (MI_U32)Atoi(inStrings[4]);
    stChnPortAttr.stPoint.u32Y = (MI_U32)Atoi(inStrings[5]);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_SetDisplayAttr", strOut, MI_RGN_SetDisplayAttr(hRgnHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK, return);
}
void Mdb_Region::SetOnOff(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hRgnHandle = Atoi(inStrings[0]);
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortAttr;

    stChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = (MI_S32)Atoi(inStrings[2]);
    stChnPort.s32OutputPortId = (MI_S32)Atoi(inStrings[3]);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_GetDisplayAttr", strOut, MI_RGN_GetDisplayAttr(hRgnHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK, return);
    stChnPortAttr.bShow= (MI_U32)Atoi(inStrings[4]);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_SetDisplayAttr", strOut, MI_RGN_SetDisplayAttr(hRgnHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK, return);
}
void Mdb_Region::SetOsdLayer(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hRgnHandle = Atoi(inStrings[0]);
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortAttr;

    stChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = (MI_S32)Atoi(inStrings[2]);
    stChnPort.s32OutputPortId = (MI_S32)Atoi(inStrings[3]);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_GetDisplayAttr", strOut, MI_RGN_GetDisplayAttr(hRgnHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK, return);
    stChnPortAttr.unPara.stOsdChnPort.u32Layer = (MI_U32)Atoi(inStrings[4]);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_SetDisplayAttr", strOut, MI_RGN_SetDisplayAttr(hRgnHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK, return);
}
void Mdb_Region::SetOsdAlpha(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hRgnHandle = Atoi(inStrings[0]);
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortAttr;

    stChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = (MI_S32)Atoi(inStrings[2]);
    stChnPort.s32OutputPortId = (MI_S32)Atoi(inStrings[3]);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_GetDisplayAttr", strOut, MI_RGN_GetDisplayAttr(hRgnHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK, return);
    stChnPortAttr.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode= (MI_RGN_AlphaMode_e)Atoi(inStrings[4]);
    if (stChnPortAttr.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode == E_MI_RGN_PIXEL_ALPHA)
    {
        stChnPortAttr.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8BgAlpha = (MI_U8)Atoi(inStrings[5]);
        stChnPortAttr.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.stArgb1555Alpha.u8FgAlpha = (MI_U8)Atoi(inStrings[6]);

    }
    else if (stChnPortAttr.unPara.stOsdChnPort.stOsdAlphaAttr.eAlphaMode == E_MI_RGN_CONSTANT_ALPHA)
    {
        stChnPortAttr.unPara.stOsdChnPort.stOsdAlphaAttr.stAlphaPara.u8ConstantAlpha = (MI_U8)Atoi(inStrings[5]);
    }
    MDB_EXPECT_OK_ERRCASE("MI_RGN_SetDisplayAttr", strOut, MI_RGN_SetDisplayAttr(hRgnHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK, return);
}
void Mdb_Region::SetCoverColor(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hRgnHandle = Atoi(inStrings[0]);
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortAttr;

    stChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = (MI_S32)Atoi(inStrings[2]);
    stChnPort.s32OutputPortId = (MI_S32)Atoi(inStrings[3]);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_GetDisplayAttr", strOut, MI_RGN_GetDisplayAttr(hRgnHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK, return);
    stChnPortAttr.unPara.stCoverChnPort.u32Color= (MI_U32)Atoi(inStrings[4]);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_SetDisplayAttr", strOut, MI_RGN_SetDisplayAttr(hRgnHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK, return);
}
void Mdb_Region::SetCoverLayer(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hRgnHandle = Atoi(inStrings[0]);
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortAttr;

    stChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = (MI_S32)Atoi(inStrings[2]);
    stChnPort.s32OutputPortId = (MI_S32)Atoi(inStrings[3]);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_GetDisplayAttr", strOut, MI_RGN_GetDisplayAttr(hRgnHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK, return);
    stChnPortAttr.unPara.stCoverChnPort.u32Layer = (MI_U32)Atoi(inStrings[4]);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_SetDisplayAttr", strOut, MI_RGN_SetDisplayAttr(hRgnHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK, return);
}
#define RAMDOM(start, end) ((end > start)?((rand()%(end - start)) + start):0)

void Mdb_Region::SetRamdomPos(std::vector<std::string> &inStrings, std::string &strOut)
{
    MI_RGN_HANDLE hRgnHandle = Atoi(inStrings[0]);
    MI_RGN_ChnPort_t stChnPort;
    MI_RGN_ChnPortParam_t stChnPortAttr;
    MI_U32 posXStart = (MI_U32)Atoi(inStrings[4]);
    MI_U32 posXEnd = (MI_U32)Atoi(inStrings[5]);
    MI_U32 posYStart = (MI_U32)Atoi(inStrings[6]);
    MI_U32 posYEnd = (MI_U32)Atoi(inStrings[7]);

    stChnPort.eModId = (MI_RGN_ModId_e)Atoi(inStrings[1]);
    stChnPort.s32DevId = 0;
    stChnPort.s32ChnId = (MI_S32)Atoi(inStrings[2]);
    stChnPort.s32OutputPortId = (MI_S32)Atoi(inStrings[3]);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_GetDisplayAttr", strOut, MI_RGN_GetDisplayAttr(hRgnHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK, return);
    stChnPortAttr.stPoint.u32X = (MI_U32)RAMDOM(posXStart, posXEnd);
    stChnPortAttr.stPoint.u32Y = (MI_U32)RAMDOM(posYStart, posYEnd);
    printf("POS X %d Y %d\n", stChnPortAttr.stPoint.u32X, stChnPortAttr.stPoint.u32Y);
    MDB_EXPECT_OK_ERRCASE("MI_RGN_SetDisplayAttr", strOut, MI_RGN_SetDisplayAttr(hRgnHandle, &stChnPort, &stChnPortAttr), MI_RGN_OK, return);
}
