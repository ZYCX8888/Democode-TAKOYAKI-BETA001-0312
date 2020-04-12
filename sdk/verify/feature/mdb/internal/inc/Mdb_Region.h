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
#ifndef MDB_REGION
#define MDB_REGION
#include "Mdb_Base.h"


#define MDB_RGN_USE_SingleTestCase(testcaseName, vecStrIn, strOut)  do{ \
    MI_U32 idx; \
    testcaseName(vecStrIn, strOut); \
    idx=strOut.find("Test Fail"); \
    if(idx != std::string::npos ) \
    { \
        SetDefaultParam(); \
        ClearRegion(vecStrIn, strOut); \
        return; \
    } \
}while(0);

typedef struct MDB_REGION_DrawPara_s
{
    int intMaxChannelNum;
    int intModId; //0 vpe, 1 divp
}MDB_REGION_DrawPara_t;

class Mdb_Region : public Mdb_Base
{
    public:
        Mdb_Region();
        virtual ~Mdb_Region();
        virtual void ShowWelcome(std::string &strOut);
    private:
        void AutoTest(std::vector<std::string> &inStrings, std::string &strOut);
        void Init(std::vector<std::string> &inStrings, std::string &strOut);
        void Deinit(std::vector<std::string> &inStrings, std::string &strOut);
        void Create(std::vector<std::string> &inStrings, std::string &strOut);
        void Destroy(std::vector<std::string> &inStrings, std::string &strOut);
        void AttachCover(std::vector<std::string> &inStrings, std::string &strOut);
        void AttachOsd(std::vector<std::string> &inStrings, std::string &strOut);
        void Dettach(std::vector<std::string> &inStrings, std::string &strOut);
        void GetAttr(std::vector<std::string> &inStrings, std::string &strOut);
        void SetCoverDisplayAttr(std::vector<std::string> &inStrings, std::string &strOut);
        void SetOsdDisplayAttr(std::vector<std::string> &inStrings, std::string &strOut);
        void SetBitMap(std::vector<std::string> &inStrings, std::string &strOut);
        void GetCanvas(std::vector<std::string> &inStrings, std::string &strOut);
        void UpdateCanvas(std::vector<std::string> &inStrings, std::string &strOut);
        void TestCase001(std::vector<std::string> &inStrings, std::string &strOut);
        void TestCase002(std::vector<std::string> &inStrings, std::string &strOut);
        void TestCase003(std::vector<std::string> &inStrings, std::string &strOut);
        void TestCase004(std::vector<std::string> &inStrings, std::string &strOut);
        void TestCase005(std::vector<std::string> &inStrings, std::string &strOut);
        void TestCase006(std::vector<std::string> &inStrings, std::string &strOut);
        void TestCase007(std::vector<std::string> &inStrings, std::string &strOut);
        void TestCase008(std::vector<std::string> &inStrings, std::string &strOut);
        void TestCase009(std::vector<std::string> &inStrings, std::string &strOut);
        void TestCase010(std::vector<std::string> &inStrings, std::string &strOut);
        void TestCase011(std::vector<std::string> &inStrings, std::string &strOut);
        void OneRegionOneAttach(std::vector<std::string> &inStrings, std::string &strOut);
        void OneRegionMaxAttach(std::vector<std::string> &inStrings, std::string &strOut);
        void MaxRegionMaxAttach(std::vector<std::string> &inStrings, std::string &strOut);
        void OneChnOnePortSetBitmap(std::vector<std::string> &inStrings, std::string &strOut);
        void ClearRegion(std::vector<std::string> &inStrings, std::string &strOut);
        void MaxRegion(std::vector<std::string> &inStrings, std::string &strOut);
        void CreateAttachDetechDestroy(std::vector<std::string> &inStrings, std::string &strOut);
        void ParseStrings(const char *pStr, std::vector<std::string> &strInStrings);
        void SetDefaultParam();
        void RunProcess(void *pArg);
        void VpeDivpProcess(std::vector<std::string> &inStrings, std::string &strOut);
        void VpeDivpAddData(std::vector<std::string> &inStrings, std::string &strOut);
        void VpeDivpRmData(std::vector<std::string> &inStrings, std::string &strOut);
        void VpeDivpAttachData(std::vector<std::string> &inStrings, std::string &strOut);
        void SetCanvas(std::vector<std::string> &inStrings, std::string &strOut);
        void SelectFillDataMode(std::vector<std::string> &inStrings, std::string &strOut);
        void InjectPic(std::vector<std::string> &inStrings, std::string &strOut);
        void InjectWord(std::vector<std::string> &inStrings, std::string &strOut);
        void ElicitWord(std::vector<std::string> &inStrings, std::string &strOut);
        void UpdateWord(std::vector<std::string> &inStrings, std::string &strOut);
        void InjectCover(std::vector<std::string> &inStrings, std::string &strOut);
        void SetOsdPos(std::vector<std::string> &inStrings, std::string &strOut);
        void SetOnOff(std::vector<std::string> &inStrings, std::string &strOut);
        void SetOsdLayer(std::vector<std::string> &inStrings, std::string &strOut);
        void SetOsdAlpha(std::vector<std::string> &inStrings, std::string &strOut);
        void SetCoverColor(std::vector<std::string> &inStrings, std::string &strOut);
        void SetCoverLayer(std::vector<std::string> &inStrings, std::string &strOut);
        void SetRamdomPos(std::vector<std::string> &inStrings, std::string &strOut);

        unsigned int Rgb2Yuv(unsigned int u32RgbColor);

        int intVpeChCnt;
        int intDivpChCnt;
        int intCurFormat;

};
#endif
