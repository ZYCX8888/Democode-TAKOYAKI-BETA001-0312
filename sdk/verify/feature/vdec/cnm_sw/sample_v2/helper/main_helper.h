/* 
 * Copyright (c) 2018, Chips&Media
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _MAIN_HELPER_H_
#define _MAIN_HELPER_H_

#include "config.h"
#include "vpuapifunc.h"
#include "vpuapi.h"
#include "vputypes.h"
#ifdef PLATFORM_QNX
    #include <sys/stat.h>
#endif

#define MATCH_OR_MISMATCH(_expected, _value, _ret)        ((_ret=(_expected == _value)) ? "MATCH" : "MISMATCH")

#if defined(WIN32) || defined(WIN64)
/*
 ( _MSC_VER => 1200 )     6.0     vs6
 ( _MSC_VER => 1310 )     7.1     vs2003
 ( _MSC_VER => 1400 )     8.0     vs2005
 ( _MSC_VER => 1500 )     9.0     vs2008
 ( _MSC_VER => 1600 )    10.0     vs2010
 */
#if (_MSC_VER == 1200)
#define strcasecmp          stricmp
#define strncasecmp         strnicmp
#else
#define strcasecmp          _stricmp
#define strncasecmp         _strnicmp
#endif
#define inline              _inline
#if (_MSC_VER == 1600)
#define strdup              _strdup
#endif
#endif

#define MAX_GETOPT_OPTIONS 100
//extension of option struct in getopt
struct OptionExt
{
    const char *name;
    int has_arg;
    int *flag;
    int val;
    const char *help;
};

#define MAX_FILE_PATH               256
#define MAX_PIC_SKIP_NUM            5



#define EXTRA_SRC_BUFFER_NUM            0
#define VPU_WAIT_TIME_OUT               10  //should be less than normal decoding time to give a chance to fill stream. if this value happens some problem. we should fix VPU_WaitInterrupt function
#define VPU_WAIT_TIME_OUT_CQ            1
#define MAX_NOT_DEC_COUNT               2000
#define COMPARE_RESOLUTION(_src, _dst)  (_src->width == _dst->width && _src->height == _dst->height)

extern char* productNameList[];

typedef union {
    struct {
        Uint32  ctu_force_mode  :  2; //[ 1: 0]
        Uint32  ctu_coeff_drop  :  1; //[    2]
        Uint32  reserved        :  5; //[ 7: 3]
        Uint32  sub_ctu_qp_0    :  6; //[13: 8]
        Uint32  sub_ctu_qp_1    :  6; //[19:14]
        Uint32  sub_ctu_qp_2    :  6; //[25:20]
        Uint32  sub_ctu_qp_3    :  6; //[31:26]

        Uint32  lambda_sad_0    :  8; //[39:32]
        Uint32  lambda_sad_1    :  8; //[47:40]
        Uint32  lambda_sad_2    :  8; //[55:48]
        Uint32  lambda_sad_3    :  8; //[63:56]
    } field;
} EncCustomMap; // for wave5xx custom map (1 CTU = 64bits)

typedef union {
    struct {
        Uint8  mb_force_mode  :  2; //lint !e46 [ 1: 0]
        Uint8  mb_qp          :  6; //lint !e46 [ 7: 2]
    } field;
} AvcEncCustomMap; // for AVC custom map on wave  (1 MB = 8bits)

typedef enum {
    MODE_YUV_LOAD = 0,
    MODE_COMP_JYUV,
    MODE_SAVE_JYUV,

    MODE_COMP_CONV_YUV,
    MODE_SAVE_CONV_YUV,

    MODE_SAVE_LOAD_YUV,

    MODE_COMP_RECON,
    MODE_SAVE_RECON,

    MODE_COMP_ENCODED,
    MODE_SAVE_ENCODED
} CompSaveMode;



void replace_character(char* str,
    char  c,
    char  r);

extern Uint32 randomSeed;

/* yuv & md5 */
#define NO_COMPARE         0
#define YUV_COMPARE        1
#define MD5_COMPARE        2
#define STREAM_COMPARE     3

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Performance report */
typedef void*   PFCtx;

PFCtx PFMonitorSetup(
    Uint32  coreIndex,
    Uint32  instanceIndex,
    Uint32  referenceClkInMHz,
    Uint32  fps,
    char*   strLogDir
    );

void PFMonitorRelease(
    PFCtx   context
    );

void PFMonitorUpdate(
    Uint32  coreIndex,
    PFCtx   context,
    Uint32  cycles,
    ...
    );

void PrepareDecoderTest(
    DecHandle decHandle
    );

void byte_swap(
    unsigned char* data, 
    int len
    );

void word_swap(
    unsigned char* data, 
    int len
    );

void dword_swap(
    unsigned char* data, 
    int len
    );

void lword_swap(
    unsigned char* data, 
    int len
    );

Int32 LoadFirmware(
    Int32       productId, 
    Uint8**   retFirmware, 
    Uint32*   retSizeInWord, 
    const char* path
    );

void PrintDecSeqWarningMessages(
    Uint32          productId, 
    DecInitialInfo* seqInfo
    );

void DisplayDecodedInformation(
    DecHandle      handle, 
    CodStd         codec, 
    Uint32         frameNo, 
    DecOutputInfo* decodedInfo,
    ...
    );


void PrintEncSppStatus(
    Uint32 coreIdx,
    Uint32 productId
    );
/*
 * VPU Helper functions
 */
/************************************************************************/
/* Video                                                                */
/************************************************************************/

#define PUT_BYTE(_p, _b) \
    *_p++ = (unsigned char)_b; 

#define PUT_BUFFER(_p, _buf, _len) \
    osal_memcpy(_p, _buf, _len); \
    (_p) += (_len);

#define PUT_LE32(_p, _var) \
    *_p++ = (unsigned char)((_var)>>0);  \
    *_p++ = (unsigned char)((_var)>>8);  \
    *_p++ = (unsigned char)((_var)>>16); \
    *_p++ = (unsigned char)((_var)>>24); 

#define PUT_BE32(_p, _var) \
    *_p++ = (unsigned char)((_var)>>24);  \
    *_p++ = (unsigned char)((_var)>>16);  \
    *_p++ = (unsigned char)((_var)>>8); \
    *_p++ = (unsigned char)((_var)>>0); 

#define PUT_LE16(_p, _var) \
    *_p++ = (unsigned char)((_var)>>0);  \
    *_p++ = (unsigned char)((_var)>>8);  

#define PUT_BE16(_p, _var) \
    *_p++ = (unsigned char)((_var)>>8);  \
    *_p++ = (unsigned char)((_var)>>0);  

Int32 ConvFOURCCToMp4Class(
    Int32   fourcc
    );

Int32 ConvFOURCCToCodStd(
    Uint32 fourcc
    );

Int32 ConvCodecIdToMp4Class(
    Uint32 codecId
    );

Int32 ConvCodecIdToCodStd(
    Int32   codecId
    );

Int32 ConvCodecIdToFourcc(
    Int32   codecId
    );

/*!
 * \brief       wrapper function of StoreYuvImageBurstFormat()
 */
Uint8* GetYUVFromFrameBuffer(
    DecHandle       decHandle,
    FrameBuffer*    fb,
    VpuRect         rcFrame,
	Uint32*       retWidth,
    Uint32*       retHeight,
    Uint32*       retBpp,
	size_t*		retSize
    );

/************************************************************************/
/* Queue                                                                */
/************************************************************************/
typedef struct {
    void*   data;
} QueueData;
typedef struct {
    Uint8*          buffer;
    Uint32          size;
    Uint32          itemSize;
    Uint32          count;
    Uint32          front;
    Uint32          rear;
    osal_mutex_t    lock;
} Queue;

Queue* Queue_Create(
    Uint32    itemCount,
    Uint32    itemSize
    );

Queue* Queue_Create_With_Lock(
    Uint32    itemCount,
    Uint32    itemSize
    );

void Queue_Destroy(
    Queue*      queue
    );

/**
 * \brief       Enqueue with deep copy
 */
BOOL Queue_Enqueue(
    Queue*      queue, 
    void*       data
    );

/**
 * \brief       Caller has responsibility for releasing the returned data
 */
void* Queue_Dequeue(
    Queue*      queue
    );

void Queue_Flush(
    Queue*      queue
    );

void* Queue_Peek(
    Queue*      queue
    );

Uint32 Queue_Get_Cnt(
    Queue*      queue
    );

/**
 * \brief       @dstQ is NULL, it allocates Queue structure and then copy from @srcQ.
 */
Queue* Queue_Copy(
    Queue*  dstQ,
    Queue*  srcQ
    );

/**
 * \brief       Check the queue is full or not.
 */
BOOL Queue_IsFull(
    Queue*      queue
    );

/************************************************************************/
/* ETC                                                                  */
/************************************************************************/
Uint32 GetRandom(
    Uint32 start,
    Uint32 end
    );

/************************************************************************/
/* MD5                                                                  */
/************************************************************************/

typedef struct MD5state_st {
    Uint32 A,B,C,D;
    Uint32 Nl,Nh;
    Uint32 data[16];
    Uint32 num;
} MD5_CTX;

Int32 MD5_Init(
    MD5_CTX *c
    );

Int32 MD5_Update(
    MD5_CTX*    c, 
    const void* data, 
    size_t      len);

Int32 MD5_Final(
    Uint8*      md, 
    MD5_CTX*    c
    );

Uint8* MD5(
    const Uint8*  d, 
    size_t        n, 
    Uint8*        md
    );

void plane_md5(MD5_CTX *md5_ctx, 
    Uint8  *src,
    int    src_x, 
    int    src_y,
    int    out_x, 
    int    out_y,
    int    stride, 
    int    bpp, 
    Uint16 zero
);

/************************************************************************/
/* Comparator                                                           */
/************************************************************************/
#define COMPARATOR_SKIP 0xF0F0F0F0
typedef enum {
    COMPARATOR_CONF_SET_GOLDEN_DATA_SIZE,
    COMPARATOR_CONF_SKIP_GOLDEN_DATA,       /*!<< 2nd parameter pointer of Queue 
                                                  containing skip command */
    COMPARATOR_CONF_SET_PICINFO,            //!<< This command is followed by YUVInfo structure.
} ComparatorConfType;

typedef void*   Comparator;
typedef struct ComparatorImpl {
    void*       context;
    char*       filename;
    Uint32      curIndex;
    Uint32      numOfFrames;
    BOOL        (*Create)(struct ComparatorImpl* impl, char* path);
    BOOL        (*Destroy)(struct ComparatorImpl* impl);
    BOOL        (*Compare)(struct ComparatorImpl* impl, void* data, Uint32 size);
    BOOL        (*Configure)(struct ComparatorImpl* impl, ComparatorConfType type, void* val);
    BOOL        (*Rewind)(struct ComparatorImpl* impl);
    BOOL        eof;
    BOOL        enableScanMode;
    BOOL        usePrevDataOneTime;
} ComparatorImpl;

typedef struct {
    Uint32          totalFrames;
    ComparatorImpl* impl;
} AbstractComparator;

// YUV Comparator 
typedef struct {
    Uint32            width;
    Uint32            height;
    FrameBufferFormat   format;
    BOOL                cbcrInterleave;
    BOOL                isVp9;
} PictureInfo;

Comparator Comparator_Create(
    Uint32    type,               //!<<   1: yuv
    char* goldenPath,
    ...
    );

BOOL Comparator_Destroy(
    Comparator  comp
    );

BOOL Comparator_Act(
    Comparator  comp,
    void*       data,
    Uint32    size
    );

BOOL Comparator_CheckFrameCount(
    Comparator  comp
    );

BOOL Comparator_SetScanMode(
    Comparator  comp,
    BOOL        enable
    );

BOOL Comparator_Rewind(
    Comparator  comp
    );

BOOL Comparator_CheckEOF(
    Comparator  comp
    );

Uint32 Comparator_GetFrameCount(
    Comparator comp
    );

BOOL IsEndOfFile(
    FILE* fp
    );

/************************************************************************/
/* Bitstream Feeder                                                     */
/************************************************************************/
typedef enum {
    FEEDING_METHOD_FIXED_SIZE,
    FEEDING_METHOD_FRAME_SIZE,
    FEEDING_METHOD_SIZE_PLUS_ES,
    FEEDING_METHOD_MAX
} FeedingMethod;

typedef struct {
    void*       data;
    Uint32    size;
    BOOL        eos;        //!<< End of stream
	int seqHeaderSize;
} BSChunk;

typedef void* BSFeeder;

typedef void (*BSFeederHook)(BSFeeder feeder, void* data, Uint32 size, void* arg);

/**
 * \brief           BitstreamFeeder consumes bitstream and updates information of bitstream buffer of VPU.
 * \param handle    handle of decoder
 * \param path      bitstream path
 * \param method    feeding method. see FeedingMethod.
 * \param loopCount If @loopCount is greater than 1 then BistreamFeeder reads the start of bitstream again
 *                  when it encounters the end of stream @loopCount times.
 * \param ...       FEEDING_METHOD_FIXED_SIZE:
 *                      This value of parameter is size of chunk at a time.
 *                      If the size of chunk is equal to zero than the BitstreamFeeder reads bistream in random size.(1Byte ~ 4MB)
 * \return          It returns the pointer of handle containing the context of the BitstreamFeeder.
 */
void* BitstreamFeeder_Create(
    Uint32          coreIdx,
    const char*     path,
    CodStd          codecId,
    FeedingMethod   method,
    EndianMode      endian
    );

/**
 * \brief           This is helper function set to simplify the flow that update bit-stream
 *                  to the VPU.
 */
Uint32 BitstreamFeeder_Act(
    BSFeeder        feeder,
    vpu_buffer_t*   bsBuffer,
    PhysicalAddress wrPtr,
    Uint32          room,
    PhysicalAddress* newWrPtr
    );

BOOL BitstreamFeeder_SetFeedingSize(
    BSFeeder    feeder,
    Uint32      size
    );
/**
 * \brief           Set filling bitstream as ringbuffer mode or linebuffer mode. 
 * \param   mode    0 : auto 
 *                  1 : ringbuffer
 *                  2 : linebuffer.
 */
#define BSF_FILLING_AUTO                    0
#define BSF_FILLING_RINGBUFFER              1
#define BSF_FILLING_LINEBUFFER              2
/* BSF_FILLING_RINBGUFFER_WITH_ENDFLAG:
 * Scenario: 
 * - Application writes 1 ~ 10 frames into bitstream buffer.
 * - Set stream end flag by using VPU_DecUpdateBitstreamBuffer(handle, 0).
 * - Application clears stream end flag by using VPU_DecUpdateBitstreamBuffer(handle, -1).
 *   when indexFrameDisplay is equal to -1.
 * NOTE:
 * - Last frame cannot be a complete frame.
 */
#define BSF_FILLING_RINGBUFFER_WITH_ENDFLAG 3
void BitstreamFeeder_SetFillMode(
    BSFeeder    feeder,
    Uint32      mode
    );

BOOL BitstreamFeeder_IsEos(
    BSFeeder    feeder
    );


Uint32 BitstreamFeeder_GetSeqHeaderSize(
	BSFeeder    feeder
	);


BOOL BitstreamFeeder_Destroy(
    BSFeeder    feeder
    );

BOOL BitstreamFeeder_Rewind(
    BSFeeder feeder
    );

BOOL BitstreamFeeder_SetHook(
    BSFeeder        feeder,
    BSFeederHook    hookFunc,
    void*           arg
    );


BOOL LoadTiledImageYuvBurst(
    Uint32          coreIdx,
    BYTE*           pYuv, 
    size_t          picWidth, 
    size_t          picHeight, 
    FrameBuffer*    fb, 
    TiledMapConfig  mapCfg
    );

Uint32 StoreYuvImageBurstFormat(
    Uint32          coreIndex, 
    FrameBuffer*    fbSrc, 
    TiledMapConfig  mapCfg, 
    Uint8*          pDst, 
    VpuRect         cropRect, 
    BOOL            enableCrop
    );


/************************************************************************/
/* Simple Renderer                                                      */
/************************************************************************/
typedef void*       Renderer;

typedef enum {
    RENDER_DEVICE_NULL,
    RENDER_DEVICE_FBDEV,
    RENDER_DEVICE_HDMI,
    RENDER_DEVICE_MAX
} RenderDeviceType;

typedef struct RenderDevice {
    void*       context;
    DecHandle   decHandle;
    BOOL (*Open)(struct RenderDevice* device);
    void (*Render)(struct RenderDevice* device, DecOutputInfo* fbInfo, Uint8* yuv, Uint32 width, Uint32 height);
    BOOL (*Close)(struct RenderDevice* device);
} RenderDevice;

Renderer SimpleRenderer_Create(
    DecHandle           decHandle,
    RenderDeviceType    deviceType,
    const char*         yuvPath            //!<< path to store yuv iamge. 
    );

Uint32 SimpleRenderer_Act(
    Renderer        renderer,
    DecOutputInfo*  fbInfo,
    Uint8*          pYuv,
    Uint32        width,
    Uint32        height
    );

void* SimpleRenderer_GetFreeFrameInfo(
    Renderer        renderer
    );

/* \brief       Flush display queues and clear display indexes 
 */
void SimpleRenderer_Flush(
    Renderer        renderer
    );

BOOL SimpleRenderer_Destroy(
    Renderer    renderer
    );

BOOL SimpleRenderer_SetFrameRate(
    Renderer        renderer,
    Uint32          fps
    );


BOOL MkDir(
    char* path
    );
/*******************************************************************************
 * DATATYPES AND FUNCTIONS RELATED TO REPORT
 *******************************************************************************/
typedef struct 
{
    osal_file_t     fpPicDispInfoLogfile;
    osal_file_t     fpPicTypeLogfile;
    osal_file_t     fpSeqDispInfoLogfile;
    osal_file_t     fpUserDataLogfile;
    osal_file_t     fpSeqUserDataLogfile;


    // Report Information
    BOOL            reportOpened;
    Int32           decIndex;
    vpu_buffer_t    vb_rpt;
    BOOL            userDataEnable;
    BOOL            userDataReportMode;

    Int32           profile;
    Int32           level;
} vpu_rpt_info_t;

typedef struct VpuReportConfig_t {
    PhysicalAddress userDataBufAddr;
    BOOL            userDataEnable;
    Int32           userDataReportMode; // (0 : Int32errupt mode, 1 Int32errupt disable mode)
    Int32           userDataBufSize;

} VpuReportConfig_t;

void OpenDecReport(
    Uint32              core_idx, 
    VpuReportConfig_t*  cfg
    );

void CloseDecReport(
    Uint32 core_idx
    );

void ConfigDecReport(
    Uint32      core_idx, 
    DecHandle   handle, 
    CodStd      bitstreamFormat
    );

void SaveDecReport(
    Uint32          core_idx, 
    DecHandle       handle, 
    DecOutputInfo*  pDecInfo, 
    CodStd          bitstreamFormat, 
    Uint32          mbNumX, 
    Uint32          mbNumY
    );

void CheckUserDataInterrupt(
    Uint32      core_idx, 
    DecHandle   handle, 
    Int32       decodeIdx, 
    CodStd      bitstreamFormat, 
    Int32       int_reason
    );

RetCode VPU_GetFBCOffsetTableSize(
    CodStd  codStd,
    int     width,
    int     height,
    int*     ysize,
    int*     csize
    );
extern Int32 ProductCalculateAuxBufferSize(
    AUX_BUF_TYPE    type, 
    CodStd          codStd, 
    Int32           width, 
    Int32           height
    );

void PrintVpuVersionInfo(
    Uint32 coreIdx
    );

void ChangePathStyle(
    char *str
    );

BOOL CalcYuvSize(
    Int32   format,
    Int32   picWidth,
    Int32   picHeight,
    Int32   cbcrInterleave,
    size_t  *lumaSize,
    size_t  *chromaSize,
    size_t  *frameSize,
    Int32   *bitDepth,
    Int32   *packedFormat,
    Int32   *yuv3p4b
    );

FrameBufferFormat GetPackedFormat (
    int srcBitDepth, 
    int packedType,
    int p10bits,
    int msb
    );

char* GetDirname(
    const char* path
    );

char* GetBasename(
    const char* pathname
    );

char* GetFileExtension(
    const char* filename 
    );

#ifdef __cplusplus
}
#endif /* __cplusplus */

/************************************************************************/
/* Structure                                                            */
/************************************************************************/
typedef struct TestDecConfig_struct {
    char                outputPath[MAX_FILE_PATH];
    char                inputPath[MAX_FILE_PATH];
    Int32               forceOutNum;
    CodStd              bitFormat;
    Int32               reorder;
    TiledMapType        mapType;
    BitStreamMode       bitstreamMode;
    BOOL                enableWTL;
    FrameFlag           wtlMode;
    FrameBufferFormat   wtlFormat;
    Int32               coreIdx;
    ProductId           productId;
    BOOL                enableCrop;                 //!<< option for saving yuv 
    BOOL                cbcrInterleave;             //!<< 0: None, 1: NV12, 2: NV21 
    BOOL                nv21;                       //!<< FALSE: NV12, TRUE: NV21, 
                                                    //!<< This variable is valid when cbcrInterleave is TRUE 
    EndianMode          streamEndian;
    EndianMode          frameEndian;
    Uint32              secondaryAXI;
    Int32               compareType;
    char                md5Path[MAX_FILE_PATH];
    char                fwPath[MAX_FILE_PATH];
    char                refYuvPath[MAX_FILE_PATH];
    RenderDeviceType    renderType;
    BOOL                thumbnailMode;
    Int32               skipMode;
    size_t              bsSize;
    BOOL                streamEndFlag;
    BOOL                scenarioTest;
    Uint32              scaleDownWidth;
    Uint32              scaleDownHeight;
    Uint32              scaleDownStepX;
    Uint32              scaleDownStepY;
    Uint32              scaleDownListW[64];
    Uint32              scaleDownListH[64];
    Uint32              numScaleDownList;
    struct {
        BOOL        enableMvc;                      //!<< H.264 MVC
        BOOL        enableTiled2Linear;
        FrameFlag   tiled2LinearMode;
        BOOL        enableBWB;
        Uint32      rotate;                         //!<< 0, 90, 180, 270
        Uint32      mirror;
        BOOL        enableDering;                   //!<< MPEG-2/4
        BOOL        enableDeblock;                  //!<< MPEG-2/4
        Uint32      mp4class;                       //!<< MPEG_4
        Uint32      frameCacheBypass;
        Uint32      frameCacheBurst;
        Uint32      frameCacheMerge;
        Uint32      frameCacheWayShape;
        LowDelayInfo    lowDelay;                   //!<< H.264
    } coda9;
    struct {
        Uint32      numVCores;                      //!<< This numVCores is valid on PRODUCT_ID_4102 multi-core version 
        BOOL        bwOptimization;                 //!<< On/Off bandwidth optimization function
        BOOL        craAsBla;
    } wave;
    Uint32          pfClock;                        //!<< performance clock in Hz
    BOOL            performance;
    BOOL            bandwidth;
    Uint32          fps;
    Uint32          enableUserData;
    Uint32              testEnvOptions;             /*!<< See debug.h */
    /* FEEDER */
    FeedingMethod       feedingMode;
    Uint32              feedingSize;
    Uint32              loopCount;                  
    BOOL                errorInject;

    InplaceBufMode      inPlaceBufferMode;
    Int32               maxVerticalMV;
    Uint32              customDisableFlag;
} TestDecConfig;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void SetDefaultDecTestConfig(
    TestDecConfig* testConfig
    );

struct option* ConvertOptions(
    struct OptionExt*   cnmOpt,
    Uint32              nItems
    );

void ReleaseVideoMemory(
    Uint32        coreIndex,
    vpu_buffer_t*   memoryArr,
    Uint32        count
    );

BOOL AllocateDecLinearFrameBuffer(
    DecHandle           handle, 
    Uint32              width, 
    Uint32              height, 
    Uint32              align,
    FrameBufferFormat   fmt, 
    BOOL                semiPlanar, 
    FrameBuffer*        fb
    );

BOOL AllocateDecFrameBuffer(
    DecHandle       decHandle,
    TestDecConfig*  config,
    Uint32          tiledFbCount,
    Uint32          linearFbCount,
    FrameBuffer*    retFbArray,
    vpu_buffer_t*   retFbAddrs,
    Uint32*         retStride
    );

#define OUTPUT_FP_NUMBER 4
BOOL OpenDisplayBufferFile(
    CodStd  codec,
    char *outputPath, 
    VpuRect rcDisplay, 
    TiledMapType mapType,
    FILE *fp[]
    );

void CloseDisplayBufferFile(
    FILE *fp[]
    );

void SaveDisplayBufferToFile(
    DecHandle handle, 
    CodStd codStd, 
    FrameBuffer dispFrame, 
    VpuRect rcDisplay, 
    FILE *fp[]
    );


void GetUserData(
    Int32 coreIdx,
    Uint8* pBase,
    vpu_buffer_t vbUserData,
    DecOutputInfo outputInfo
    );

Uint32 CalcScaleDown(
    Uint32 origin,
    Uint32 scaledValue
    );


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif	/* _MAIN_HELPER_H_ */
 
