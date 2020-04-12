#ifndef __MID_SYSTEM_CONFIG_H_
#define __MID_SYSTEM_CONFIG_H_
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>

#include "mid_video_type.h"
#include "mid_audio_type.h"
#include "mid_uvc_datatype.h"

#include "mi_aio_datatype.h"
#include "mid_json.h"
#include "mid_common.h"

#define APPEND_MAX_LEN  1024
#define CREAT_OK   0
#define NOT_FOUND_FILE  -1
#define ERR_JSON_FILE   -2
#define INVALID_FILEPATH -3
#define CREAT_FILE_FAIL  -4
#define CREATE_ITEM_FAIL  -5
#define ERR_JSON_ITEM  -6
#define BUFFER_LEN      (10*1024)

#if TARGET_CHIP_I6

#define BIND_DIVP_MAX  3
#define VPE_MAX_PORT      3
#define SENSOR_MAX_RES_INDEX 2
#define MAX_RGN_NUM   8
#define MAX_DIVP_NUM   6
#define MIXER_MAX_YUV2IE_NUM   1
#define JSON_ITEM_NUM 10

#define SENSOR_REINDEX_NUM 2

#elif TARGET_CHIP_I5

#define JSON_ITEM_NUM 12
#define BIND_DIVP_MAX  3
#define SENSOR_MAX_RES_INDEX 2
#define MAX_RGN_NUM   8
#define MAX_DIVP_NUM   6
#define MIXER_MAX_YUV2IE_NUM   2
#define VPE_MAX_PORT      4
#define SENSOR_REINDEX_NUM 2

#elif  TARGET_CHIP_I6E

#define BIND_DIVP_MAX  3
#define VPE_MAX_PORT      3
#define SENSOR_MAX_RES_INDEX 2
#define MAX_RGN_NUM   8
#define MAX_DIVP_NUM   6
#define MIXER_MAX_YUV2IE_NUM   2
#define JSON_ITEM_NUM 10
#define SENSOR_REINDEX_NUM 2
#elif  TARGET_CHIP_I6B0

#define BIND_DIVP_MAX  3
#define VPE_MAX_PORT      MAX_VPE_PORT_NUMBER
#define SENSOR_MAX_RES_INDEX 1
#define MAX_DIVP_NUM   6
#define MIXER_MAX_YUV2IE_NUM   2
#define JSON_ITEM_NUM 10
#define SENSOR_REINDEX_NUM 2

#endif
/*typedef  int  MI_S32;
typedef  unsigned char uchar;
typedef  unsigned int  uint;*/

#ifdef __cplusplus
extern "C"{
#endif

typedef enum
{
    DevSensor = 0x00,
    DevVif = 0x1,
    DevVpeTop,
    DevVpeBase,
    DevLdc,
    DevVenc,
    DevRgn,
    DevAi,
    DevAo,
    DevAiChn,
    DevRec,
    DevMax
}DevCfg;

typedef enum
{
    VpePort0 = 0x0,
    VpePort1 = 0x01,
    VpePort2 = 0x2,
#if TARGET_CHIP_I5 || TARGET_CHIP_I6B0
    VpePort3,
#endif
    DivpCPort0,
    DivpCPort1,
    DivpCPort2,
    DivpCPort3,
    DivpCPort4,
    DivpCPort5,
    PortMax
}MixerPortDef;

typedef enum _USER_MMA_Module
{
       MODULE_ID_ISP_VPE = 0x0,
       MODULE_ID_VPE_RING_OUT,
       MODULE_ID_VPE_FRAME_OUT,
       MODULE_ID_DIVP_FRAME_OUT,
       MODULE_ID_VENC_H264_5,
       MODULE_ID_VENC_JPG,
#if TARGET_CHIP_I6B0
       MODULE_ID_VENC_VPU_COMMON,
#endif
       MODULE_ID_RGN,
       MODULE_ID_AI,
          MODULE_ID_AICHN,
       MODULE_ID_AO,
       E_MMAP_ID_VPE_MLOAD,
}USER_MMA_Module;
 typedef struct
 {
     USER_MMA_Module eModule;
#if TARGET_CHIP_I6
     MI_SYS_BindType_e eModType;
#endif
     MI_U32 u32Devid ;
     MI_U32 u32ChnId ;
     MI_U8  port;
     MI_U8 u8MMAHeapName[32];
     MI_U8  allocMmaFlag;
     MI_U32 u32PrivateHeapSize;
}NEW_USER_MMA_INFO;

typedef struct {
    MI_U8 maxFps;
    MI_U8 minFps;
    MI_U16 cropX;
    MI_U16 cropY;
    MI_U16 sensor_w;
    MI_U16 sensor_h;
}sensor_PadArry_s;
typedef struct _ModuleSensorInfo_s
{
    MI_S8 * sensor_name;
    MI_U32   workResIndex;
    sensor_PadArry_s  ResIndex[SENSOR_MAX_RES_INDEX];
}ModuleSensorInfo_s ;

typedef struct _ModuleVifInfo_s
{
 MI_U8 MaxFps;
 MI_U8 MinFps;
  MI_U16  MaxHeight;
 MI_U16  MaxWidth;
 MI_U16 CurWidth;
 MI_U16 CurHeight;
  MI_U8 IsHDR;
 MI_U8 VifChn;
}ModuleVifInfo_s;

typedef struct _ModuleVpeTopInfo_s
{
   MI_U8 MaxFps;
   MI_U8 MinFps;
   MI_U8 PortId;
   MI_U8 DevId;
   MI_U8 ChnId;
   MI_U8 _3DnrLevel;
   MI_U16 Rotation;
   MI_U16 MaxWidth;
   MI_U16 MaxHeight;
}ModuleVpeTopInfo_s;

typedef struct _ModuleLdcInfo_s
{
   MI_U8 maxFps;
   MI_U8 minFps;
   MI_U8 portId;
   MI_U8 devId;
   MI_U8 chnId;
   MI_U8 _3dnr_level;
   MI_U16 rotation;
   MI_U16 maxWidth;
   MI_U16 maxHeight;
}ModuleLdcInfo_s;

typedef struct _ModuleVpeButtomInfo_s
{

}ModuleVpeButtomInfo_s;

typedef struct ModuleVpeInfo_s
{
    MI_U8   Channel;
    MI_U8   Port[MAX_VIDEO_NUMBER];
    MI_U8   Outdepth[MAX_VPE_PORT_NUMBER];

    MI_U8   fps[MAX_VPE_PORT_NUMBER];
    MI_U8   minFps[MAX_VPE_PORT_NUMBER];
       MI_U8   isHdr;
    MI_U8   bindMoudleType[MAX_VIDEO_NUMBER];
    MI_U16    roatValue;
    MI_U8   _3dnrLevel;

    MI_U16    in_modle_width[MAX_VPE_PORT_NUMBER];
    MI_U16    in_modle_height[MAX_VPE_PORT_NUMBER];
    MI_U16    CurWidth[MAX_VPE_PORT_NUMBER];
    MI_U16    CurHeight[MAX_VPE_PORT_NUMBER];

    MI_U32    out_moule_size;
    MI_S8    attachRgnNum[MAX_DIVP_NUM];
    MI_S8    bindDivpNum[MAX_DIVP_NUM];
    MI_U8   bindDest[MAX_DIVP_NUM];
    MI_S8    bindMoudle[MAX_VIDEO_NUMBER];
    MI_S8    bindVencNum[MAX_VIDEO_NUMBER];
    MI_S8    bindYuvNum[MAX_DIVP_NUM];
    MI_U8   Vpe_OutPortNum;
    MI_U8   Vpe_OutChnNum;
    MI_U8   bindVencMaxNum;
    MI_U8   bindYuvMaxNum;
}ModuleVpeInfo_s;

typedef struct _ModuleVencInfo_s
{
    MI_U8        EncodeType[MAX_VIDEO_NUMBER];
    MI_U8        FromPort[MAX_VIDEO_NUMBER];

    MI_BOOL         IsPraM[MAX_VIDEO_NUMBER];
    MI_U32      MaxBitrate[MAX_VIDEO_NUMBER];
    MI_U32      Bitrate[MAX_VIDEO_NUMBER];
    MI_U32      MinBitrate[MAX_VIDEO_NUMBER];
    MI_U32       Fps[MAX_VIDEO_NUMBER];
    MI_U32         Minfps[MAX_VIDEO_NUMBER];

    MI_U16       StreamWidth[MAX_VIDEO_NUMBER];
    MI_U16       StreamHeight[MAX_VIDEO_NUMBER];
    MI_U16       StreamMaxWidth[MAX_VIDEO_NUMBER];
    MI_U16       StreamMaxHeight[MAX_VIDEO_NUMBER];

    MI_U8        MmaModuleId[MAX_VIDEO_NUMBER];
    MI_U8       BindModuleType[MAX_VIDEO_NUMBER];
}ModuleVencInfo_s;

typedef struct _ModuleRgnInfo_s
{
    MI_U8     IsPraM;
}ModuleRgnInfo_s;

typedef struct _ModuleAiInfo_s
{
    MI_U32                BitWidthByte;
    MI_U32                 Samplerate;
    MI_U8                AiMediaType[MI_AUDIO_MAX_CHN_NUM/2];
    MI_U8                VolumeInDb;
    MI_U8            Chnt;
}ModuleAiInfo_s;

typedef struct _ModuleAiChnInfo_s
{
    MI_U8               out_depath;
    MI_U32                BitWidthByte;
    MI_U32                   Samplerate;
    MI_U32                pktnum;
}ModuleAiChnInfo_s;

typedef struct _ModuleAoInfo_s
{
    MI_S32                BitWidthByte;
    MI_U8               AoMediaType[MI_AUDIO_MAX_CHN_NUM/2];
    MI_S32                   Samplerate;
    MI_U8               VolumeOutDb;
}ModuleAoInfo_s;

typedef struct _ModuleRec_s
{
    MI_U8    RecPath[64];
    MI_U8     Storage;
}ModuleRecInfo_s;

int Get_AllCustomerConfigInfo(MI_S8 *MixerConfigPath);

ModuleAoInfo_s * GetAoDevInfo();
ModuleAiChnInfo_s * GetAiCHNInfo();
ModuleAiInfo_s *GetAiDevInfo();
ModuleRgnInfo_s *GetRgnInfo();
ModuleVencInfo_s *GetVencInfo();
ModuleVpeInfo_s  *GetVpeBaseInfo();
ModuleVpeTopInfo_s *GetVpeTopInfo();
ModuleLdcInfo_s  *GetLdcInfo();
ModuleVifInfo_s  *GetVifInfo();
ModuleSensorInfo_s  *GetSensorInfo();
ModuleRecInfo_s *GetRecInfo();

#ifdef __cplusplus
}
#endif
#endif
