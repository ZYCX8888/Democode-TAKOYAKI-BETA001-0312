#ifndef _MODULE_PRIVATE_MEMORY_POOL_H_
#define _MODULE_PRIVATE_MEMORY_POOL_H_
#include "mid_system_config.h"
#include "mid_common.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define MAX_CHN 3

typedef enum
{
    RgnI2 = 0x0,
    RgnI4,
    RgnI8,
    RgnArgb
}MIXER_RGN_FORMAT;

typedef enum
{
    Yuv420Sp = 0x0,
    Yuv422,
}MIXER_YUV_FORMAT;

class CMmaPrivateManager
{
public:
    static CMmaPrivateManager * instance();
    CMmaPrivateManager();
    ~CMmaPrivateManager();

    MI_S32 Config(MI_S32 videoNum, MI_S32 alloc_free_flag);
    MI_S32 Stop();

private:
    MI_S32 ConfigMMAHeap(const NEW_USER_MMA_INFO &MmaParam);
    MI_U32 CalcIspVpeSize(MI_U16 sensor_w, MI_U16 sensor_h, MI_U8  _3dnrlevel = 2 , MI_BOOL rot = TRUE,MI_U8 _hdr = 1);
    MI_U32 CalcVencSize(MI_U16 venc_w, MI_U16 venc_h, MI_BOOL EnableLtr = TRUE);
    MI_U32 CalcRgnSize(MI_U16 mainstream_max_w, MI_U16 mainstream_max_h, MIXER_RGN_FORMAT pRgnFormat = RgnI4 );
    MI_U32 CalcYuvFrameModeBufSize(MI_U16 yuv_w, MI_U16 yuv_h, MI_U8 depth,MIXER_YUV_FORMAT pYuvFormat = Yuv420Sp);
#if TARGET_CHIP_I6B0
    MI_U32 CalcVencVPUCommonBufsize(MI_U16 venc_w, MI_U16 venc_h);
    MI_U32 CalcYuvFrameModeHalfBufSize(MI_U16 yuv_w, MI_U16 yuv_h, MI_U8 depth_mum, MI_U8 depth_den, MIXER_YUV_FORMAT pYuvFormat = Yuv420Sp);
#endif
    MI_U32 CalcHwRingModeBufSize(MI_U16 yuv_w, MI_U16 yuv_h);
    MI_U32 CalcAudioInDevbufSize(MI_U16 OutputDepth, MI_U8 BitWidthByte, MI_U16 pktnum  = 2048);
    MI_U32 CalcAudioInChnbufSize(MI_U16 SampleRate, MI_U16 BitWidthByte, AudioSoundMode_e chnt = AUDIO_SOUND_MODE_MONO );
    MI_U32 CalcAudioOutbufSize(MI_U16 SampleRate, MI_U16 BitWidthByte);
    MI_U32 CalcJpegOutbufSize(MI_U16 venc_w, MI_U16 venc_h);
};

#define g_PriMmaManager (CMmaPrivateManager::instance())
#ifdef __cplusplus
}
#endif
#endif
