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
#ifndef __MI_VPE_TEST_HEADER__
#define __MI_VPE_TEST_HEADER__
#include "mi_vpe.h"
#include "mi_sys.h"
#include "mi_sys_datatype.h"
#include "mi_hdmi.h"
#include "mi_disp.h"
#define TEST_ERROR (1)
#define TEST_INFO  (2)
#define TEST_DBG   (3)
#define SHOW_LEVEL TEST_INFO
#define TEST_MAX_SRC_FRAME_SIZE  (98)

#define VPE_TEST_001_DESC ("MI_VPE base1: one channel and one output port")
#define VPE_TEST_002_DESC ("MI_VPE base2: Multi Instance")
#define VPE_TEST_003_DESC ("MI_VPE switch resolution")
#define VPE_TEST_004_DESC ("MI_VPE user set customer crop")
#define VPE_TEST_005_DESC ("MI_VPE set vpe port Disp Window")
#define VPE_TEST_006_DESC ("MI_VPE set 3DNR")
#define VPE_TEST_007_DESC ("MI_VPE get luma")
#define VPE_TEST_008_DESC ("MI_VPE IQ menual test")
#define VPE_TEST_009_DESC ("MI_VPE 64Channel test")


#define MI_VPE_TEST_PRINT(level, fmt, args ...) do { if (level <= SHOW_LEVEL) {printf("[VPE_TEST] %s [%d]: ", __func__, __LINE__); printf(fmt, ##args);}} while(0)
//#define MI_VPE_TEST_PRINT(fmt, args ...)

#define MI_VPE_TEST_INFO(fmt, args ...) MI_VPE_TEST_PRINT(TEST_INFO, fmt, ##args)
#define MI_VPE_TEST_DBG(fmt, args ...)  MI_VPE_TEST_PRINT(TEST_DBG, fmt, ##args)


#define ExecFunc(func, _ret_) \
    MI_VPE_TEST_DBG("%d Start test: %s\n", __LINE__, #func);\
    if (func != _ret_)\
    {\
        MI_VPE_TEST_INFO("exec function failed\n");\
        return 1;\
    }\
    else\
    {\
        MI_VPE_TEST_INFO("exec function pass\n");\
    }\
    MI_VPE_TEST_DBG("%d End test: %s\n", __LINE__, #func)

//#define TEST_VPE_MAIN(case) int test_vpe_TestCase#case_main(int argc, const char *argv[])

#define TEST_VPE_FUNC(number, test_func, desc) { .testCaseName = "mi_vpe_testCase"#number, .func = (test_func), .testDecription = desc,}
#define TEST_VPE_CHNN_FILE(case, chnn, size) ("mi_vpe_test_case"#case"_"#chnn"_"#size"_yuv422.yuv")
#define TEST_VPE_CHNN_FILE420(case, chnn, size) ("mi_vpe_test_case"#case"_"#chnn"_"#size"_yuv420.yuv")
#define TEST_VPE_PORT_OUT_FILE(case, chnn, port, size) ("mi_vpe_test_case"#case"_"#chnn"_outport_"#port"_"#size"_yuv422.yuv")
#define YUV422_PIXEL_PER_BYTE (2)
typedef int(*test_func)(int);

typedef struct {
    const char *desc;
    test_func  func;
    int        next_index;
    MI_BOOL    bEnd;
} test_vpe_TestIqMenu_t;

#define TEST_VPE_VALID_INDEX (0xffff)
#define ENTER_TEST() printf("Enty test.\n")
#define EXIT_TEST()  printf("Exit test.\n")
enum{
    // Main menu
    E_VPE_TEST_IQ_IQ_ON_OFF = 0,
    E_VPE_TEST_IQ_IQ_PARAM,
    E_VPE_TEST_IQ_SET_CROP,
    E_VPE_TEST_ISP_PARAM,
    E_VPE_TEST_IQ_IQ_SHOW_ALL,

    // IQ ON/OFF
    E_VPE_TEST_IQ_ENABLE_NR,
    E_VPE_TEST_IQ_ENABLE_EDGE,
    E_VPE_TEST_IQ_ENABLE_ES,
    E_VPE_TEST_IQ_ENABLE_CONTRAST,
    E_VPE_TEST_IQ_ENABLE_UVINVERT,
    E_VPE_TEST_IQ_ENABLE_SHOW_ON_OFF,
    E_VPE_TEST_IQ_ENABLE_RET,

    // IQ Parameters
    E_VPE_TEST_IQ_SET_NRC_SF_STR,
    E_VPE_TEST_IQ_SET_NRC_TF_STR,
    E_VPE_TEST_IQ_SET_NRY_SF_STR,
    E_VPE_TEST_IQ_SET_NRY_TF_STR,
    E_VPE_TEST_IQ_SET_NRY_BLEND_MOTION_TH,
    E_VPE_TEST_IQ_SET_NRY_BLEND_STILL_TH,
    E_VPE_TEST_IQ_SET_NRY_BLEND_MOTION_WEI,
    E_VPE_TEST_IQ_SET_NRY_BLEND_OTHER_WEI,
    E_VPE_TEST_IQ_SET_NRY_BLEND_STILL_WEI,
    E_VPE_TEST_IQ_SET_EDGE_GAIN,
    E_VPE_TEST_IQ_SET_CONTRAST,
    E_VPE_TEST_IQ_SET_SHOW_PARA,
    E_VPE_TEST_IQ_SET_RET,

    // IQ set crop
    E_VPE_TEST_IQ_SET_CROP_INFO,
    E_VPE_TEST_IQ_GET_CROP_INFO,
    E_VPE_TEST_IQ_CROP_RET,

    // IQ set crop
    E_VPE_TEST_SET_ISP_PARAM,
    E_VPE_TEST_GET_ISP_PARAM,
    E_VPE_TEST_ISP_PARAM_RET,

    E_VPE_TEST_IQ_MAX,
} test_vpe_TestIqOp_e;

enum {
    E_VPE_TEST_IQ_RET_PASS,
    E_VPE_TEST_IQ_RET_FAIL,
    E_VPE_TEST_IQ_RET_EXIT,
} test_vpe_TestIqRet_e;

typedef struct {
    MI_SYS_WindowRect_t stPortWin;
    char *outputFile;
    MI_BOOL bEnable;
    int dest_fd;
    int dest_offset;
} test_vpe_OutPortConfig;

typedef struct {
    char *inputFile;
    int src_fd;
    int count;
    int src_offset;
    int src_count;
    MI_SYS_WindowRect_t stSrcWin;
    MI_SYS_WindowRect_t stCropWin;
    test_vpe_OutPortConfig stOutPort[4];
    int product;
} test_vpe_Config;

MI_S32 test_vpe_CreatChannel_MaxSize(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort, MI_SYS_WindowRect_t *pstSrcWin,MI_SYS_WindowRect_t *pstCropWin, MI_SYS_WindowRect_t *pstDispWin,MI_SYS_PixelFormat_e ePixFmt);
MI_S32 test_vpe_CreatChannel(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort, MI_SYS_WindowRect_t *pstCropWin, MI_SYS_WindowRect_t *pstDispWin, MI_SYS_PixelFormat_e InePixFmt, MI_SYS_PixelFormat_e OutePixFmt);
MI_S32 test_vpe_DestroyChannel(MI_VPE_CHANNEL VpeChannel, MI_VPE_PORT VpePort);;
void test_vpe_PrintfData(MI_SYS_BufInfo_t *pstBufInfo);
MI_S32 test_vpe_GetOneFrame(int srcFd, int offset, char *pData, int yuvSize);
MI_S32 test_vpe_PutOneFrame(int dstFd, int offset, char *pDataFrame, int line_offset, int line_size, int lineNumber);

void test_vpe_ShowFrameInfo (const char *s, MI_SYS_FrameData_t *pstFrameInfo);
MI_BOOL test_vpe_OpenSourceFile(const char *pFileName, int *pSrcFd);
MI_BOOL test_vpe_OpenDestFile(const char *pFileName, int *pDestFd);
void test_vpe_CloseFd(int fd);
MI_BOOL test_vpe_SysEnvInit(void);
void test_vpe_WriteBonderYuv422(MI_U16 u16Color, int font, MI_SYS_WindowRect_t *pstRect, MI_U16 *pData,  int line_offset);
void testVpeFdRewind(int srcFd);
MI_S32 test_vpe_HdmiInit(void);
MI_S32 test_vpeUnBinderDisp(MI_U32 VpeOutputPort, MI_U32 DispInputPort);
MI_S32 test_vpeBinderDisp(MI_U32 VpeOutputPort, MI_U32 DispInputPort);
MI_S32 test_vpe_DeinitDisp(MI_DISP_DEV DispDev, MI_DISP_LAYER DispLayer, MI_DISP_INPUTPORT LayerInputPort);
MI_S32 test_vpe_GetOneFrameYUV420ByStride(int srcFd, char *pYData, char *pUvData, int ySize, int uvSize, int height, int width, int yStride, int uvStride);

MI_S32 test_vpe_GetOneFrameYUV422ByStride(int srcFd, char *pYUVData, int height, int width, int Stride);


// Test case
int test_vpe_TestCase001_main(int argc, const char *argv[]);
int test_vpe_TestCase002_main(int argc, const char *argv[]);
int test_vpe_TestCase003_main(int argc, const char *argv[]);
int test_vpe_TestCase004_main(int argc, const char *argv[]);
int test_vpe_TestCase005_main(int argc, const char *argv[]);
int test_vpe_TestCase006_main(int argc, const char *argv[]);
int test_vpe_TestCase007_main(int argc, const char *argv[]);
int test_vpe_TestCase008_main(int argc, const char *argv[]);

#endif // __MI_VPE_TEST_HEADER__
