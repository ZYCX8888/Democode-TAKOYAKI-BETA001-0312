#include "mid_utils.h"
#include "module_common.h"
#include "mi_venc.h"
#include "mid_VideoEncoder.h"
#include "mid_vif.h"
#include "mi_sensor.h"
#include "mid_sys.h"
#include <stdlib.h>
#include "mid_system_config.h"
#include "module_private_memory_pool.h"

extern MI_U32 g_rotation;
extern MixerVideoParam g_videoParam[MAX_VIDEO_NUMBER];
extern MixerAudioInParam g_audioInParam[MIXER_AI_MAX_NUMBER];
extern MixerAudioOutParam g_audioOutParam[MIXER_AO_MAX_NUMBER];

extern MI_S32 g_s32DivpChnIndex;

CMmaPrivateManager::CMmaPrivateManager()
{

}

CMmaPrivateManager::~CMmaPrivateManager()
{

}

CMmaPrivateManager *CMmaPrivateManager::instance()
{
    static CMmaPrivateManager _instance;

    return &_instance;
}


MI_S32 CMmaPrivateManager::Config(MI_S32 videoNum, MI_S32 alloc_free_flag)
{
    MI_S32 ret = 0x0;
    BOOL vpePort[4];
    MI_U32 divpChnIndex = 0;
#if TARGET_CHIP_I6B0
    BOOL realDivpFix = FALSE;
#endif
    memset(vpePort, 0, sizeof(vpePort));
    if(videoNum <= 0  || videoNum > MAX_VIDEO_NUMBER)
    {
        MIXER_ERR(" videoNum is err.\n");
        return -1;
    }
#if TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    NEW_USER_MMA_INFO MmaParam;
    {
        //vpe-isp
        memset(&MmaParam,0,sizeof(NEW_USER_MMA_INFO));
        MmaParam.eModule = MODULE_ID_ISP_VPE;
        MmaParam.u32ChnId = 0;
        MmaParam.u32Devid = 0;
#if !TARGET_CHIP_I6B0
        MmaParam.u32PrivateHeapSize = CalcIspVpeSize(g_videoParam[0].stVifInfo.SensorWidth, \
                                        g_videoParam[0].stVifInfo.SensorHeight, \
                                        2, \
                                        TRUE,g_videoParam[0].stVifInfo.HdrType);
#else
                MmaParam.u32PrivateHeapSize = CalcIspVpeSize(g_videoParam[0].stVifInfo.SensorWidth, \
                                                g_videoParam[0].stVifInfo.SensorHeight, \
                                                2, \
                                                g_rotation==0?FALSE:TRUE,g_videoParam[0].stVifInfo.HdrType);
                //I6B0 rotate and hdr is conflict!
#endif

        MmaParam.allocMmaFlag = !!alloc_free_flag;
        memcpy(MmaParam.u8MMAHeapName, "mma_heap_name0", strlen("mma_heap_name0"));
        ConfigMMAHeap(MmaParam);
    }

    for(int i=0x0; i<videoNum; i++) //config vpe port
    {
        //vpe-port
        memset(&MmaParam,0,sizeof(NEW_USER_MMA_INFO));

        if(VE_AVC == g_videoParam[i].encoderType || VE_H265 == g_videoParam[i].encoderType)    // h264/5
        {
            if(VpePort1 == g_videoParam[i].FromPort  && (0 == vpePort[VpePort1]))
            {
                if(Mixer_Venc_Bind_Mode_FRAME == g_videoParam[i].eBindMode)
                {
                    MmaParam.eModule = MODULE_ID_VPE_FRAME_OUT;
                    MmaParam.u32ChnId = 0;
                    MmaParam.u32Devid = 0;
                    MmaParam.port = g_videoParam[i].stVpeChnPort.u32PortId;
                    MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].MaxWidth, \
                                                            g_videoParam[i].MaxHeight,\
                                                            g_videoParam[i].vpeBufCntQuota,\
                                                            Yuv420Sp);
                    if(MmaParam.u32PrivateHeapSize)
                    {
                      vpePort[VpePort1] = 1;
                    }

                }
                else if(Mixer_Venc_Bind_Mode_HW_RING == g_videoParam[i].eBindMode)
                {
                    MmaParam.eModule = MODULE_ID_VPE_RING_OUT;
                    MmaParam.u32ChnId = 0;
                    MmaParam.u32Devid = 0;
                    MmaParam.port = g_videoParam[i].stVpeChnPort.u32PortId;
                    MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].MaxWidth, \
                                                            g_videoParam[i].MaxHeight,\
                                                            1,\
                                                            Yuv420Sp);
                    if(MmaParam.u32PrivateHeapSize)
                    {
                      vpePort[VpePort1] = 1;
                    }
                }
                #if TARGET_CHIP_I6B0
                else if(Mixer_Venc_Bind_Mode_HW_HALF_RING == g_videoParam[i].eBindMode)
                {
                    MmaParam.eModule = MODULE_ID_VPE_RING_OUT;
                    MmaParam.u32ChnId = 0;
                    MmaParam.u32Devid = 0;
                    MmaParam.port = g_videoParam[i].stVpeChnPort.u32PortId;
                    MmaParam.u32PrivateHeapSize = CalcYuvFrameModeHalfBufSize(g_videoParam[i].MaxWidth, \
                                                            g_videoParam[i].MaxHeight,\
                                                            1,2,\
                                                            Yuv420Sp);
                    if(MmaParam.u32PrivateHeapSize)
                    {
                      vpePort[VpePort1] = 1;
                    }
                }
                else
                {
                    MIXER_ERR("port1  only support frame_mode/hw_ring_mode/half_ring_mode @h264/5.\n");
                    continue;
                }
                #else
                else
                {
                    MIXER_ERR("port1  only support frame_mode/hw_ring_mode @h264/5.\n");
                    continue;
                }
                #endif
            }
            #if TARGET_CHIP_I5
            else if(VpePort3 == g_videoParam[i].FromPort && (0 == vpePort[VpePort3]))
            {
                if(Mixer_Venc_Bind_Mode_FRAME == g_videoParam[i].eBindMode)
                {
                    MmaParam.eModule = MODULE_ID_VPE_FRAME_OUT;
                    MmaParam.u32ChnId = 0;
                    MmaParam.u32Devid = 0;
                    MmaParam.port = g_videoParam[i].stVpeChnPort.u32PortId;
                    MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].MaxWidth, \
                                                            g_videoParam[i].MaxHeight,\
                                                            g_videoParam[i].vpeBufCntQuota,\
                                                            Yuv420Sp);
                   if(MmaParam.u32PrivateHeapSize)
                   {
                     vpePort[VpePort3] = 1;
                   }
                }
                else
                {
                    MIXER_ERR("port3  only support frame_mode @h264/5/jpeg/divp.\n");
                    continue;
                }
            }
            #elif TARGET_CHIP_I6B0
            else if(VpePort3 == g_videoParam[i].FromPort && (0 == vpePort[VpePort3]))
            {
                //MIXER_DBG("I6B0 vpe port3  used for realtime mode,not need Private Pool.\n");
                vpePort[VpePort3] = 1;
                continue;
            }
            #endif
            else        //vpe port0/2 -> codec.  vpe port0/2 -> divp
            {
                if(Mixer_Venc_Bind_Mode_FRAME == g_videoParam[i].eBindMode)
                {
                       MmaParam.eModule = MODULE_ID_VPE_FRAME_OUT;
                    MmaParam.u32ChnId = 0;
                    MmaParam.u32Devid = 0;
                    if(VpePort0 == g_videoParam[i].FromPort  && (0 == vpePort[VpePort0]))
                    {
                      MmaParam.port = g_videoParam[i].stVpeChnPort.u32PortId;
                      MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].MaxWidth, \
                                                            g_videoParam[i].MaxHeight,\
                                                            g_videoParam[i].vpeBufCntQuota,\
                                                            Yuv420Sp);
                      if(MmaParam.u32PrivateHeapSize)
                      {
                          vpePort[VpePort0] = 1;
                      }
                    }
                    else  if(VpePort2 == Mixer_coverVi2Vpe(g_videoParam[i].FromPort)  && (0 == vpePort[VpePort2]))
                    {
                      MmaParam.port = g_videoParam[i].stVpeChnPort.u32PortId;
                      #if !TARGET_CHIP_I6B0
                      MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].MaxWidth, \
                                                            g_videoParam[i].MaxHeight,\
                                                            g_videoParam[i].vpeBufCntQuota,\
                                                            Yuv420Sp);
                      #else
                      MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].width, \
                                                            g_videoParam[i].height,\
                                                            g_videoParam[i].vpeBufCntQuota,\
                                                            Yuv420Sp);
                      #endif
                      if(MmaParam.u32PrivateHeapSize)
                      {
                        vpePort[VpePort2] = 1;
                      }
                    }
                    else
                    {
                        continue;
                    }
                }
                else
                {
                    MIXER_ERR("port2  only support frame_mode @h264/5/jpeg/divp.\n");
                    continue;
                }
            }
        }
        else if(VE_JPG == g_videoParam[i].encoderType)    //jpeg
        {
            if(VpePort0 == g_videoParam[i].FromPort  && (0 == vpePort[VpePort0]))
            {
                if(Mixer_Venc_Bind_Mode_FRAME == g_videoParam[i].eBindMode)
                {
                    MmaParam.eModule = MODULE_ID_VPE_FRAME_OUT;
                    MmaParam.u32ChnId = 0;
                    MmaParam.u32Devid = 0;
                    MmaParam.port = g_videoParam[i].stVpeChnPort.u32PortId;
                    MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].width, \
                                                                    g_videoParam[i].height,\
                                                                    g_videoParam[i].vpeBufCntQuota,\
                                                                    Yuv420Sp);
                    if(MmaParam.u32PrivateHeapSize)
                    {
                      vpePort[VpePort0] = 1;
                    }
                }
                else if(Mixer_Venc_Bind_Mode_REALTIME == g_videoParam[i].eBindMode)
                {
                    MIXER_DBG("realtime_mode don't need buf @ vpe -> divp. and only jpg support realtime_mode\n");
                    continue;
                }
                else
                {
                    MIXER_ERR("port0 only support frame mode@h264/5.\n");
                    continue;
                }
            }
            #if TARGET_CHIP_I5
            else if(VpePort3 == g_videoParam[i].FromPort)
            {
                if(Mixer_Venc_Bind_Mode_FRAME == g_videoParam[i].eBindMode (0 == vpePort[VpePort3]))
                {
                    MmaParam.eModule = MODULE_ID_VPE_FRAME_OUT;
                    MmaParam.u32ChnId = 0;
                    MmaParam.u32Devid = 0;
                    MmaParam.port = g_videoParam[i].stVpeChnPort.u32PortId;
                    MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].MaxWidth, \
                                                            g_videoParam[i].MaxHeight,\
                                                            g_videoParam[i].vpeBufCntQuota,\
                                                            Yuv420Sp);
                    if(MmaParam.u32PrivateHeapSize)
                    {
                      vpePort[VpePort3] = 1;
                    }
                }
                else
                {
                    MIXER_ERR("port3  only support frame_mode @h264/5/jpeg/divp.\n");
                    continue;
                }
            }
            #elif TARGET_CHIP_I6B0
            else if(VpePort3 == g_videoParam[i].FromPort)
            {
                MIXER_DBG("I6B0 vpe port3  used for realtime mode,not need Private Pool.\n");
                continue;
            }
            #endif
            else
            {
                //vpe port1/2 -> jpeg codec.  divp -> jpeg codec.
                 if(Mixer_Venc_Bind_Mode_FRAME == g_videoParam[i].eBindMode)
                  {
                    MmaParam.eModule = MODULE_ID_VPE_FRAME_OUT;
                    MmaParam.u32ChnId = 0;
                    MmaParam.u32Devid = 0;
                    if(VpePort1 == g_videoParam[i].FromPort  && (0 == vpePort[VpePort1]))
                    {
                      MmaParam.port = g_videoParam[i].stVpeChnPort.u32PortId;
                      #if !TARGET_CHIP_I6B0
                      MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].MaxWidth, \
                                                            g_videoParam[i].MaxHeight,\
                                                            g_videoParam[i].vpeBufCntQuota,\
                                                            Yuv420Sp);
                      #else
                      MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].width, \
                                                                                  g_videoParam[i].height,\
                                                                                  g_videoParam[i].vpeBufCntQuota,\
                                                                                  Yuv420Sp);

                      #endif
                      if(MmaParam.u32PrivateHeapSize)
                      {
                        vpePort[VpePort1] = 1;
                      }
                    }
                    else if(VpePort2 == Mixer_coverVi2Vpe(g_videoParam[i].FromPort)  && (0 == vpePort[VpePort2]))
                    {
                      MmaParam.port = g_videoParam[i].stVpeChnPort.u32PortId;
                      #if !TARGET_CHIP_I6B0
                      MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].MaxWidth, \
                                                            g_videoParam[i].MaxHeight,\
                                                            g_videoParam[i].vpeBufCntQuota,\
                                                            Yuv420Sp);
                      #else
                      MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].width, \
                                                                                  g_videoParam[i].height,\
                                                                                  g_videoParam[i].vpeBufCntQuota,\
                                                                                  Yuv420Sp);

                      #endif
                      if(MmaParam.u32PrivateHeapSize)
                      {
                        vpePort[VpePort2] = 1;
                      }
                    }
                }
                else
                {
                    MIXER_ERR("port2  only support frame_mode @h264/5/jpeg/divp.\n");
                    continue;
                }
            }
        }
        else if(VE_YUV420 == g_videoParam[i].encoderType)    //yuv420
        {
            //all port use frame_mode;
            if(DivpCPort0 > g_videoParam[i].FromPort &&(0 == vpePort[g_videoParam[i].FromPort]))
            {
                MmaParam.eModule = MODULE_ID_VPE_FRAME_OUT;
                MmaParam.u32ChnId = 0;
                MmaParam.u32Devid = 0;
                MmaParam.port = g_videoParam[i].stVpeChnPort.u32PortId;
                MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].MaxWidth, \
                                                        g_videoParam[i].MaxHeight,\
                                                        g_videoParam[i].vpeBufCntQuota,\
                                                        Yuv420Sp);
                if(MmaParam.u32PrivateHeapSize)
                {
                  vpePort[g_videoParam[i].FromPort] = 1;
                }
            }
            else
            {
                MIXER_DBG("this case is not init divp.\n");
                continue;
            }
        }

        MmaParam.allocMmaFlag = !!alloc_free_flag;
        memcpy(MmaParam.u8MMAHeapName, "mma_heap_name0", strlen("mma_heap_name0"));
        printf("==g_videoParam[%d].FromPort=%d=\n",i,g_videoParam[i].FromPort);
    //    if(MmaParam.u32PrivateHeapSize)
            ConfigMMAHeap(MmaParam);
    }

#if TARGET_CHIP_I6B0
    divpChnIndex = g_s32DivpChnIndex;
#else
    divpChnIndex = Mixer_Divp_GetChannleNum();
#endif

    for(MI_U8 i=0x0; i<videoNum; i++)    //init divp
    {
        //MIXER_DBG("FromPort:%d\n", g_videoParam[i].FromPort);
        memset(&MmaParam,0,sizeof(NEW_USER_MMA_INFO));

        
        if(DivpCPort0 <= g_videoParam[i].FromPort)    // divp -> codec
        {
            MmaParam.eModule = MODULE_ID_DIVP_FRAME_OUT;
            MmaParam.u32ChnId = divpChnIndex;
            MmaParam.u32Devid = 0;
            MmaParam.port = 0;
            #if !TARGET_CHIP_I6B0
            MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].MaxWidth, \
                                                g_videoParam[i].MaxHeight,\
                                                g_videoParam[i].divpBufCntQuota,\
                                                Yuv420Sp);
            #else
            MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].width, \
                                                            g_videoParam[i].height,\
                                                            g_videoParam[i].divpBufCntQuota,\
                                                            Yuv420Sp);
            #endif
            divpChnIndex ++;
        }
        #if TARGET_CHIP_I6B0
        else if(g_videoParam[i].FromPort == VpePort3 && realDivpFix==FALSE)
        {
            MmaParam.eModule = MODULE_ID_DIVP_FRAME_OUT;
            MmaParam.u32ChnId = divpChnIndex;
            MmaParam.u32Devid = 0;
            MmaParam.port = 0;
            MmaParam.u32PrivateHeapSize = CalcYuvFrameModeBufSize(g_videoParam[i].width, \
                                                            g_videoParam[i].height,\
                                                            g_videoParam[i].divpBufCntQuota,\
                                                            Yuv420Sp);
            divpChnIndex ++;
        }
        #endif
        else
        {
            //MIXER_DBG("this case is not init vpe.\n");
            continue;
        }

        MmaParam.allocMmaFlag = !!alloc_free_flag;
        memcpy(MmaParam.u8MMAHeapName, "mma_heap_name0", strlen("mma_heap_name0"));
        ConfigMMAHeap(MmaParam);
    }

/*#if TARGET_CHIP_I6B0
    {
        MI_U16 insmod_max_width = 0,insmod_max_height = 0;
        for(int i=0x0; i<videoNum; i++) //config vpe port
        {
            //actually, it most be the param when insmod mi_venc.ko
            if(g_videoParam[i].MaxWidth > insmod_max_width)insmod_max_width = g_videoParam[i].MaxWidth;
            if(g_videoParam[i].MaxHeight > insmod_max_height)insmod_max_height = g_videoParam[i].MaxHeight;
        }
        memset(&MmaParam,0,sizeof(NEW_USER_MMA_INFO));
        MmaParam.eModule = MODULE_ID_VENC_VPU_COMMON;
        MmaParam.u32Devid = 0;
        MmaParam.u32PrivateHeapSize = CalcVencVPUCommonBufsize(insmod_max_width,insmod_max_height);
        MmaParam.allocMmaFlag = !!alloc_free_flag;
        memcpy(MmaParam.u8MMAHeapName, "mma_heap_name0", strlen("mma_heap_name0"));
        ConfigMMAHeap(MmaParam);
    }
#endif*/

    for(MI_U8 i=0x0; i<videoNum; i++)    //init codec
    {
        memset(&MmaParam,0,sizeof(NEW_USER_MMA_INFO));
        if(VE_AVC == g_videoParam[i].encoderType || VE_H265 == g_videoParam[i].encoderType)    //codec
        {
            MmaParam.eModule = MODULE_ID_VENC_H264_5;
            MmaParam.u32Devid = 0;
            MmaParam.u32ChnId = i;
            #if !TARGET_CHIP_I6B0
            MmaParam.u32PrivateHeapSize = CalcVencSize(g_videoParam[i].MaxWidth, \
                                                g_videoParam[i].MaxHeight);
            #else
            MmaParam.u32PrivateHeapSize = CalcVencSize(g_videoParam[i].width, \
                                                g_videoParam[i].height);
            #endif
        }
        else if(VE_JPG == g_videoParam[i].encoderType)
        {
            MmaParam.eModule = MODULE_ID_VENC_JPG;
            MmaParam.u32Devid = 1;    //must notice:jpeg is dev1 ,H264 H265 IS dev 0.
            MmaParam.u32ChnId = i;
            #if !TARGET_CHIP_I6B0
            MmaParam.u32PrivateHeapSize = CalcJpegOutbufSize(g_videoParam[i].MaxWidth, \
                                                g_videoParam[i].MaxHeight);
            #else
            MmaParam.u32PrivateHeapSize = CalcJpegOutbufSize(g_videoParam[i].width, \
                                                g_videoParam[i].height);
            #endif
        }
        else
        {
            MIXER_DBG("this case is only init codec.\n");
            continue;
        }
#if TARGET_CHIP_I6B0
        /*if(MmaParam.u32ChnId == 0)
            MmaParam.u32PrivateHeapSize += CalcVencVPUCommonBufsize(g_videoParam[0].stVifInfo.SensorWidth,g_videoParam[0].stVifInfo.SensorHeight);*/
#endif
        MmaParam.allocMmaFlag = !!alloc_free_flag;
        memcpy(MmaParam.u8MMAHeapName, "mma_heap_name0", strlen("mma_heap_name0"));
        ConfigMMAHeap(MmaParam);
    }

    //init rgn
    memset(&MmaParam,0,sizeof(NEW_USER_MMA_INFO));
    {
        MmaParam.eModule = MODULE_ID_RGN;
        MmaParam.u32Devid = 0;
        #if !TARGET_CHIP_I6B0
        MmaParam.u32PrivateHeapSize = CalcRgnSize(g_videoParam[0].MaxWidth, \
                                            g_videoParam[0].MaxHeight);
        #else
        MmaParam.u32PrivateHeapSize = 2*1024*1024;
        #endif
    }

    MmaParam.allocMmaFlag = !!alloc_free_flag;
    memcpy(MmaParam.u8MMAHeapName, "mma_heap_name0", strlen("mma_heap_name0"));
    ConfigMMAHeap(MmaParam);
#if !TARGET_CHIP_I6B0
    //audioIn dev
    memset(&MmaParam,0,sizeof(NEW_USER_MMA_INFO));
    {
        MmaParam.eModule = MODULE_ID_AI;
        MmaParam.u32Devid = g_audioInParam[0].stAudioInChnPort.u32DevId;
        MmaParam.u32PrivateHeapSize = CalcAudioInDevbufSize(g_audioInParam[0].stAudioAttr.u32FrmNum,\
                                                    g_audioInParam[0].stAudioAttr.eBitwidth, \
                                                    g_audioInParam[0].stAudioAttr.u32PtNumPerFrm);
    }

    MmaParam.allocMmaFlag = !!alloc_free_flag;
    memcpy(MmaParam.u8MMAHeapName, "mma_heap_name0", strlen("mma_heap_name0"));
    ConfigMMAHeap(MmaParam);

    //audioIn dev
    memset(&MmaParam,0,sizeof(NEW_USER_MMA_INFO));
    {
        MmaParam.eModule = MODULE_ID_AICHN;
        MmaParam.u32Devid = g_audioInParam[0].stAudioInChnPort.u32DevId;
        MmaParam.u32ChnId = g_audioInParam[0].stAudioInChnPort.u32ChnId;
        MmaParam.u32PrivateHeapSize = CalcAudioInChnbufSize(g_audioInParam[0].stAudioAttr.eSamplerate,\
                                                    g_audioInParam[0].stAudioAttr.eBitwidth, \
                                                    (AudioSoundMode_e)g_audioInParam[0].stAudioAttr.eSoundmode);
    }

    MmaParam.allocMmaFlag = !!alloc_free_flag;
    memcpy(MmaParam.u8MMAHeapName, "mma_heap_name0", strlen("mma_heap_name0"));
    ConfigMMAHeap(MmaParam);

    //audioOut dev
    memset(&MmaParam,0,sizeof(NEW_USER_MMA_INFO));
    {
        MmaParam.eModule = MODULE_ID_AO;
        MmaParam.u32Devid = g_audioInParam[0].stAudioInChnPort.u32DevId;
        MmaParam.u32PrivateHeapSize = CalcAudioOutbufSize(g_audioInParam[0].stAudioAttr.eSamplerate,\
                                                    g_audioInParam[0].stAudioAttr.eBitwidth);
    }

    MmaParam.allocMmaFlag = !!alloc_free_flag;
    memcpy(MmaParam.u8MMAHeapName, "mma_heap_name0", strlen("mma_heap_name0"));
    ConfigMMAHeap(MmaParam);
#endif
#endif

    return ret;
}


MI_S32 CMmaPrivateManager::Stop()
{
    MI_S32 ret = 0x0;

    return ret;
}

MI_U32 CMmaPrivateManager::CalcIspVpeSize(MI_U16 sensor_w, MI_U16 sensor_h, MI_U8  _3dnrlevel /* =2 */, MI_BOOL rot /* = TRUE*/,MI_U8  _hdr)
{
#if !TARGET_CHIP_I6B0
    MI_U32 size = 0x0;
    MI_U8 count = 0x0;
    MI_U16 _w = 0x0, _h = 0x0;
    _hdr = _hdr;
    //MIXER_DBG("w:%d, h:%d, rot:%d, 3dnr:%d.\n", sensor_w, sensor_h, rot, _3dnrlevel);
    if((FALSE == rot))  // no rot
    {
        if(2 == _3dnrlevel) // 3DNR 12bit
        {
            _w = (sensor_w+ 79)/80;
            _h = (sensor_h + 15)/16;
            count = 1;
        }
        else if(1 == _3dnrlevel) // 3DNR 8bit
        {
            _w = (sensor_w+ 127)/128;
            _h = (sensor_h + 15)/16;
            count = 1;
        }
        else if(7 == _3dnrlevel)    //two level 3dnr buf@12bit
        {
            _w = (sensor_w+ 79)/80;
            _h = (sensor_h + 15)/16;
            count = 2;
        }
          size = (_w  * _h ) * 2048 * count + \
                _w * _h * 0.5 +\
                0x2d000 + \
                0x9000 ;
    }
    else if((TRUE == rot))      // rot
    {
        if(2 == _3dnrlevel) // 3DNR 12bit
        {
            _w = (sensor_w+ 79)/80;
            _h = (sensor_h + 15)/16;
            count = 1 + 1;
        }
        else if(1 == _3dnrlevel) // 3DNR 8bit
        {
            _w = (sensor_w+ 127)/128;
            _h = (sensor_h + 15)/16;
            count = 1 + 1;
        }
        else if(7 == _3dnrlevel)    //two level 3dnr buf@12bit
        {
            _w = (sensor_w+ 79)/80;
            _h = (sensor_h + 15)/16;
            count = 1 + 2;
        }
        size = (_w  * _h ) * 2048 * count + \
                _w * _h * 0.5 +\
                0x2d000 + \
                0x9000 ;
    }
    else
    {
        MIXER_ERR("wrong param\n");
    }
    return size;
#else
    MI_U32 tmp1,tmp2;
    //according to <i6b0_i6e_ISP_memory_calculator.xlsx>
    MI_BOOL bIRsensor = FALSE; //such as ar0237.
    MI_U16 u16height_block_rot = (sensor_h + 1)>> 1;
    MI_U16 u16width_dn = (sensor_w>>1) + 1;
    MI_U16 width_dn_block = (u16width_dn + 3) >> 2;
    MI_U16 u16height_block_motion = (sensor_h + 7) >> 3;

    MI_U32 u32ae_0     = 8192;
    MI_U32 u32af     = 13824;
    MI_U32 u32awb_short_frame     = 138240;
    MI_U32 u32histogram_0        = 1024;
    MI_U32 u32extra64    = 128;
    MI_U32 u32ir_acc    = 1024;
    MI_U32 u32HDR_mode    = 0;
    MI_U32 u32IR_image    = sensor_w * sensor_h;
    MI_U32 u32Rot        = 0;
    MI_U32 u32motion_map = 0;
    MI_U32 u32Reference = 0;
    MI_U32 u32History = 0;
    MI_U32 u32Reference_ROT = 0;
    MI_U32 u32History_ROT =0;
    MI_U32 u32mload = 11456;
    MI_U32 u32cmdq    = 65536;
    MI_U32 u32wdr    = 2048;
    MI_U32 total_size = 0;
    MIXER_DBG("sensor_w=%d,sensor_h=%d,_3dnrlevel=%d,rot=%d,_hrd=%d\n",sensor_w,sensor_h,_3dnrlevel,rot,_hdr);
    if(_hdr)
    {
        u32HDR_mode = ((sensor_w+31)/32 *3 * 16 * sensor_h) / 8;  //sensor HDR 12BIT precision
    }
    if(rot==TRUE)
    {

        if(_3dnrlevel <= E_MI_VPE_3DNR_LEVEL1)
        {
            tmp1 = ((sensor_w+63)>>6)*2*2048;
            tmp2 = (u16height_block_rot+31)>>5;
            u32Rot = tmp1 * tmp2;
            u32Reference_ROT = u32Rot;
            u32History_ROT = ((sensor_w/8+31)>>5) *sensor_h*16;
        }
        else if(_3dnrlevel == E_MI_VPE_3DNR_LEVEL2)
        {
            tmp1 = ((sensor_w+39)/40)*2*2048;
            tmp2 = (u16height_block_rot+31)>>5;
            u32Rot = tmp1 * tmp2;
            u32Reference_ROT = u32Rot;
            u32History_ROT = ((sensor_w/8+31)>>5) *sensor_h*16;
        }
        else
        {
            MIXER_ERR("wrong param\n");
        }
        tmp1 = ((width_dn_block+15)>>4) * 4 * 2048;
        tmp2 = ((u16height_block_motion+31)>>5) * 2;
        u32motion_map = tmp1 * tmp2;
    }
    else
    {
        if(_3dnrlevel <= E_MI_VPE_3DNR_LEVEL1)
        {
            u32Reference = ((sensor_w+15)>>4 )*sensor_h*16;
            u32History = ((sensor_w/8+31)>>5) *sensor_h*16;
        }
        else if(_3dnrlevel == E_MI_VPE_3DNR_LEVEL2)
        {
            u32Reference = ((sensor_w+9)/10)*sensor_h*16;
            u32History = ((sensor_w/8+31)>>5 )*sensor_h*16;
        }
        else
        {
            MIXER_ERR("wrong param\n");
        }
    }

    if(bIRsensor==FALSE)
    {
        total_size = u32ae_0 + u32af + u32awb_short_frame + u32histogram_0 + u32extra64 + u32HDR_mode \
                + u32Rot + u32motion_map + u32Reference + u32History + u32Reference_ROT + u32History_ROT \
                + u32mload + u32cmdq + u32wdr;

    }
    else
    {
        total_size = u32ae_0 + u32af + u32awb_short_frame + u32histogram_0 + u32extra64 + u32ir_acc + u32HDR_mode \
            + u32IR_image + u32Rot + u32motion_map + u32Reference + u32History + u32Reference_ROT + u32History_ROT \
            + u32mload + u32cmdq + u32wdr;
    }


    /*MIXER_DBG("total_size=%d u32ae_0=%d u32af=%d u32awb_short_frame=%d u32histogram_0=%d u32extra64=%d u32ir_acc=%d u32HDR_mode=%d "\
        "u32IR_image=%d u32Rot=%d u32motion_map=%d u32Reference=%d u32History=%d u32Reference_ROT=%d u32History_ROT=%d "\
        "u32mload=%d u32cmdq=%d  u32wdr=%d\n",total_size, u32ae_0, u32af, u32awb_short_frame, u32histogram_0, u32extra64, u32ir_acc, u32HDR_mode, \
        u32IR_image, u32Rot, u32motion_map, u32Reference, u32History, u32Reference_ROT, u32History_ROT,  \
        u32mload, u32cmdq, u32wdr);*/

    return ALIGN_4096xUP(total_size)+0x4000;  //16k redundancy
#endif

}

MI_U32 CMmaPrivateManager::CalcVencSize(MI_U16 venc_w, MI_U16 venc_h, MI_BOOL EnableLtr)
{
    MI_U32 size = 0x0;
    MI_U32 VencOutputSize = 0x0;

    #if TARGET_CHIP_I6 || TARGET_CHIP_I5

    MI_U8 count = 0x1;

    MI_U32 VencRefListSize = 0x0;
    MI_U32 VencRefHistSize = 0x0;
    MI_U32 RefYuvSize = 0x0;

    if(EnableLtr)
        count = 2;

    RefYuvSize = (venc_w) * (ALIGN_32xUP(venc_h) + 64) * 3/2 * count;

    VencOutputSize = (venc_w) * (venc_h) /2;

    count  = 2;
    if(EnableLtr)
        count = 3;
    VencRefListSize = ((venc_w) * ALIGN_32xUP(venc_h)/64 + 1)*4*count;

    if(EnableLtr)
        count = 3;
    VencRefHistSize = ((venc_w) * ALIGN_32xUP(venc_h)/32)*2*count;


    size = RefYuvSize + VencOutputSize + VencRefListSize + VencRefHistSize + 0x10000;
    #elif TARGET_CHIP_I6E

    MI_U8 count = 0x2;

    MI_U32 VpuCommonBuf = 0x0;
    MI_U32 WorkBuf = 0x0;
    MI_U32 CinfoBuf = 0x0;
    MI_U32 FbcReconBuf = 0x0;
    MI_U32 MvBuf = 0x0;
    MI_U32 FbcYtblBuf = 0x0;
    MI_U32 FbcCtblBuf = 0x0;
    MI_U32 FbcBuf = 0x0;
    MI_U32 SubSamBuf = 0x0;
    MI_U32 TaskBuf = 0x0;
    MI_U32 RoiMapBuf = 0x0;
    MI_U32 SecAxiBuf = 0x0;
    MI_U32 VlcBuf = 0x0;
    MI_BOOL VlcState = 0x0;

    if(VlcState)
    {
        VlcBuf = ALIGN_4096xUP(0x80000);
    }
    else
    {
        if((venc_w * venc_h) < 0x220000)
        {
            VlcBuf = ALIGN_4096xUP((2048 * 1088 * 3 /8) * 3/2);
        }
        else
        {
            VlcBuf = ALIGN_4096xUP((venc_w * venc_h * 3 /8) * 3/2);
        }
    }
    CinfoBuf = 8 * (ALIGN_16xUP(venc_w)/16) * (ALIGN_16xUP(venc_h)/16);

    VpuCommonBuf =  0x80000 + 0x100000 + VlcBuf * 2 +CinfoBuf * 2;
    WorkBuf = 128 * 1024;

    if(EnableLtr)
        count = 3;

    FbcReconBuf = count * (ALIGN_32xUP(venc_w)) * (ALIGN_16xUP(venc_h)) * 3/2;
    MvBuf = ALIGN_4096xUP((128 * (ALIGN_64xUP(venc_w)/64) * (ALIGN_64xUP(venc_h)/64)) * count);

    FbcYtblBuf = ALIGN_16xUP(ALIGN_256xUP(venc_w) * ALIGN_16xUP(venc_h)/32);
    FbcCtblBuf = ALIGN_16xUP(ALIGN_256xUP(venc_w) * ALIGN_16xUP(venc_h/2)/32);
    FbcBuf = (ALIGN_4096xUP(FbcYtblBuf*count) + 0x1000 + ALIGN_4096xUP(FbcCtblBuf*count)) + 0x1000;

    SubSamBuf = ALIGN_4096xUP((ALIGN_16xUP((venc_w/4) * ALIGN_8xUP(venc_h)/4)) * count) + 0x1000;

    TaskBuf = 0x3940 * 4;
    RoiMapBuf = 4 * (ALIGN_64xUP(venc_w)/64) * (ALIGN_64xUP(venc_h)/64) ;

    SecAxiBuf = 8 * ALIGN_16xUP(venc_w);

    VencOutputSize = (venc_w) * (venc_h) /2;

    size = VpuCommonBuf + WorkBuf + FbcReconBuf + MvBuf + FbcBuf+ SubSamBuf + TaskBuf + RoiMapBuf + SecAxiBuf + 0x10000 + VencOutputSize;
    #elif TARGET_CHIP_I6B0

    MI_U8 inplace_mode=1;         //0:off 1:on
    MI_U8 u8refFrame_num=2;
    MI_U8 u8refBuffer_num = 1;
    MI_U32 WorkBuf = 0x0;
    MI_U32 FbcReconBuf = 0x0;
    MI_U32 MvBuf = 0x0;
    MI_U32 FbcYtblBuf = 0x0;
    MI_U32 FbcCtblBuf = 0x0;
    MI_U32 FbcBuf = 0x0;
    MI_U32 SubSamBuf = 0x0;
    MI_U32 TaskBuf = 0x0;
    MI_U32 instance_common = 4096;
    MI_U32 RoiMapBuf = 0x0;
    MI_U32 SecAxiBuf = 0x0;

    WorkBuf = 128 * 1024;

    if(EnableLtr)
        u8refFrame_num = 3;
    else
        u8refFrame_num = 2;
    u8refBuffer_num = u8refFrame_num - inplace_mode;

    FbcReconBuf = (ALIGN_32xUP(venc_w)) * (ALIGN_16xUP(venc_h) + 160*inplace_mode)  * 3/2;
    FbcReconBuf = ALIGN_4096xUP(FbcReconBuf * u8refBuffer_num);

    MvBuf = 128 * (ALIGN_64xUP(venc_w)/64) * (ALIGN_64xUP(venc_h)/64);
    MvBuf = ALIGN_4096xUP(MvBuf*u8refFrame_num) + 4096;

    FbcYtblBuf = ALIGN_16xUP(ALIGN_256xUP(venc_w) * ALIGN_16xUP(venc_h)/32);
    FbcCtblBuf = ALIGN_16xUP(ALIGN_256xUP(venc_w) * ALIGN_16xUP(venc_h/2)/32);
    FbcBuf = (ALIGN_4096xUP(FbcYtblBuf*u8refFrame_num) + 0x1000 + ALIGN_4096xUP(FbcCtblBuf*u8refFrame_num)) + 0x1000;

    SubSamBuf = ALIGN_4096xUP((ALIGN_16xUP((venc_w/4) * ALIGN_8xUP(venc_h)/4)) * u8refFrame_num) + 0x1000;

    TaskBuf = ALIGN_4096xUP(58624); //depth of cmdq = 4
    RoiMapBuf = (ALIGN_16xUP(venc_w)/16) * (ALIGN_16xUP(venc_h)/16);
    RoiMapBuf = ALIGN_4096xUP(RoiMapBuf);

    SecAxiBuf = 8 * ALIGN_16xUP(venc_w);
    SecAxiBuf = ALIGN_4096xUP(SecAxiBuf);

    VencOutputSize = (venc_w) * (venc_h) /2;
    if(venc_w*venc_h <= 640*480) VencOutputSize *=2;
    VencOutputSize = ALIGN_4096xUP(VencOutputSize);

    size = ALIGN_4096xUP(WorkBuf + FbcReconBuf + MvBuf + FbcBuf+ SubSamBuf + TaskBuf + instance_common + RoiMapBuf + SecAxiBuf +VencOutputSize);
    /*MIXER_DBG("w=%d,h=%d,WorkBuf=%d,FbcReconBuf=%d,MvBuf=%d,FbcBuf=%d,SubSamBuf=%d,TaskBuf=%d,instance_common=%d,RoiMapBuf=%d,SecAxiBuf=%d,VencOutputSize=%d,size=%d\n",
        venc_w,venc_h,WorkBuf,FbcReconBuf,MvBuf,FbcBuf,SubSamBuf,TaskBuf,instance_common,RoiMapBuf,SecAxiBuf,VencOutputSize,size);*/
    #endif
    return size;
}
#if TARGET_CHIP_I6B0
MI_U32 CMmaPrivateManager::CalcVencVPUCommonBufsize(MI_U16 venc_w, MI_U16 venc_h)
{
    MI_BOOL boolVLCRingOn = FALSE;
    MI_U8   u8VLC_bufnum = 2;
    MI_U8   u8Cinfo_bufnum = 2;

    MI_U32 tmp1,tmp2;
    MI_U32 u32CodeBase = 524288;
    MI_U32 u32TempBuf = 65536;
    MI_U32 u32VLC = 0;
    MI_U32 u32Cinfo = 0;
    MI_U32 u32Ramlog = 16384;
    MI_U32 sum = 0;
    tmp1 = (venc_w * venc_h)<2228224?(((2048*1088)*3/8)*3/2):(((venc_w * venc_h)*3/8)*3/2);
    tmp2 = boolVLCRingOn?524288:(tmp1);
    u32VLC = ALIGN_4096xUP(tmp2);
    tmp1 = ALIGN_16xUP(venc_w)/16;
    tmp2 = ALIGN_16xUP(venc_h)/16;
    u32Cinfo = 8*tmp1*tmp2;
    sum = ALIGN_4096xUP(u32CodeBase + u32TempBuf + u32VLC*u8VLC_bufnum + u32Cinfo*u8Cinfo_bufnum + u32Ramlog + 64*1024);
    MIXER_DBG("u32CodeBase=%d,u32TempBuf=%d,u32VLC=%d,u32Cinfo=%d,VPUCommon sum =%d\n",u32CodeBase,u32TempBuf,u32VLC,u32Cinfo,sum);
    return (sum);
}
#endif
MI_U32 CMmaPrivateManager::CalcRgnSize(MI_U16 mainstream_max_w, MI_U16 mainstream_max_h, MIXER_RGN_FORMAT pRgnFormat /* = RgnI4 */)
{
    //
    MI_U32 size = 0x0;

    switch(pRgnFormat)
    {
        case RgnI2:
                size = mainstream_max_w * mainstream_max_h/2;
            break;

        case RgnI4:
                size = mainstream_max_w * mainstream_max_h*2;
            break;

        case RgnI8:
                size = mainstream_max_w * mainstream_max_h * 3;
            break;

        case RgnArgb:
        default:
                size = mainstream_max_w * mainstream_max_h * 6;
            break;

    }

    return size;
}

MI_U32 CMmaPrivateManager::CalcYuvFrameModeBufSize(MI_U16 yuv_w, MI_U16 yuv_h, MI_U8 depth, MIXER_YUV_FORMAT pYuvFormat /* =Yuv420Sp */)
{
    MI_U32 size = 0x0;

    //MIXER_DBG("w:%d, h:%d, depth:%d, format:%d\n", yuv_w, yuv_h, depth, pYuvFormat);
#if !TARGET_CHIP_I6B0
    switch(pYuvFormat)
    {

        case Yuv422:
                size = (((yuv_w * yuv_h*2)+4095)&(~4095)) * depth;
            break;

        case Yuv420Sp:
        default:
                size = (((yuv_w * yuv_h *  3/2)+4095)&(~4095)) * depth;
            break;
    }
#else
    switch(pYuvFormat)
    {

        case Yuv422:
                size = (((ALIGN_32xUP(yuv_w) * ALIGN_32xUP(yuv_h)*2)+4095)&(~4095)) * depth;
            break;

        case Yuv420Sp:
        default:
                size = (((ALIGN_32xUP(yuv_w) * ALIGN_32xUP(yuv_h) *  3/2)+4095)&(~4095)) * depth;
            break;
    }
#endif
    return size;
}
#if TARGET_CHIP_I6B0
MI_U32 CMmaPrivateManager::CalcYuvFrameModeHalfBufSize(MI_U16 yuv_w, MI_U16 yuv_h, MI_U8 depth_mum, MI_U8 depth_den, MIXER_YUV_FORMAT pYuvFormat /* =Yuv420Sp */)
{
    MI_U32 size = 0x0;

    //MIXER_ERR("w:%d, h:%d, depth:%d %d, format:%d\n", yuv_w, yuv_h, depth_mum,depth_den, pYuvFormat);
    switch(pYuvFormat)
    {

        case Yuv422:
                size = (((ALIGN_32xUP(yuv_w) * ALIGN_32xUP(yuv_h)*2)+4095)&(~4095)) *depth_mum/depth_den;
            break;

        case Yuv420Sp:
        default:
                size = (((ALIGN_32xUP(yuv_w) * ALIGN_32xUP(yuv_h) *  3/2)+4095)&(~4095)) *depth_mum/depth_den;
            break;
    }

    return size;
}

#endif
MI_U32 CMmaPrivateManager::CalcHwRingModeBufSize(MI_U16 yuv_w, MI_U16 yuv_h)
{
    //
    MI_U32 size = 0x0;

    {
        size = yuv_w * ALIGN_32xUP(yuv_h) *  3/2;    // this mode only support yuv420sp.
    }

    return size;
}

MI_U32 CMmaPrivateManager::CalcAudioInDevbufSize(MI_U16 OutputDepth, MI_U8 BitWidthByte, MI_U16 pktnum /* = 2048*/)
{
    // SampleRate: (8k/16k/32k/48k)
    // BitWidthByte: 8bit/16bit
    // chnt: MONO/STREO   (1/2)
    MI_U32 size = 0x0;

    size = BitWidthByte * pktnum * OutputDepth * 2;

    return size;
}

MI_U32 CMmaPrivateManager::CalcAudioInChnbufSize(MI_U16 SampleRate, MI_U16 BitWidthByte, AudioSoundMode_e chnt /*= AUDIO_SOUND_MODE_MONO */)
{
    // SampleRate: (8k/16k/32k/48k)
    // BitWidthByte: 8bit/16bit
    // chnt: MONO/STREO   (1/2)
    MI_U32 size = 0x0;

    size = BitWidthByte * chnt * SampleRate;

    return size;
}

MI_U32 CMmaPrivateManager::CalcAudioOutbufSize(MI_U16 SampleRate, MI_U16 BitWidthByte)
{
    // SampleRate: (8k/16k/32k/48k)
    // BitWidthByte: 8bit/16bit
    MI_U32 size = 0x0;

    size = BitWidthByte * SampleRate /2;

    return size;
}

MI_U32 CMmaPrivateManager::CalcJpegOutbufSize(MI_U16 venc_w, MI_U16 venc_h)
{
    // SampleRate: (8k/16k/32k/48k)
    // BitWidthByte: 8bit/16bit
    MI_U32 size = 0x0;
    MI_U32 tmp  = 0x0;

    tmp = venc_w * venc_h;

    if(tmp < 352*288)
        size = tmp * 3/2;
    else if(tmp < 704*576)
        size = tmp;
    else if(tmp < 1280*736)
        size = tmp * 3/4;
    else if(tmp < 1920*1088)
        size = tmp/2;
    else
        size = tmp/2;

    return size;
}

MI_S32 CMmaPrivateManager::ConfigMMAHeap(const NEW_USER_MMA_INFO &MmaParam)
{
    MI_SYS_GlobalPrivPoolConfig_t  tmp;
    MI_S32 s32Ret = 0x0;
    memset(&tmp, 0x0, sizeof(tmp));
    switch(MmaParam.eModule)
    {
        case MODULE_ID_ISP_VPE:

            tmp.eConfigType = E_MI_SYS_PER_DEV_PRIVATE_POOL;
            tmp.bCreate = !!MmaParam.allocMmaFlag;

            tmp.uConfig.stPreDevPrivPoolConfig.eModule = E_MI_MODULE_ID_VPE;
            tmp.uConfig.stPreDevPrivPoolConfig.u32Devid = 0;
            memcpy(tmp.uConfig.stPreDevPrivPoolConfig.u8MMAHeapName, (const char*)MmaParam.u8MMAHeapName, strlen( (const char*)MmaParam.u8MMAHeapName));
               tmp.uConfig.stPreDevPrivPoolConfig.u32PrivateHeapSize = MmaParam.u32PrivateHeapSize;

               s32Ret = MI_SYS_ConfigPrivateMMAPool(&tmp);

            break;

        case  MODULE_ID_VPE_RING_OUT :
            tmp.eConfigType = E_MI_SYS_VPE_TO_VENC_PRIVATE_RING_POOL;
            tmp.bCreate = !!MmaParam.allocMmaFlag;

            tmp.uConfig.stPreVpe2VencRingPrivPoolConfig.u32VencInputRingPoolStaticSize = MmaParam.u32PrivateHeapSize;
                memcpy(tmp.uConfig.stPreVpe2VencRingPrivPoolConfig.u8MMAHeapName,  (const char*)MmaParam.u8MMAHeapName, strlen( (const char*)MmaParam.u8MMAHeapName));

               s32Ret = MI_SYS_ConfigPrivateMMAPool(&tmp);
            break;

        case MODULE_ID_VPE_FRAME_OUT:

            tmp.eConfigType = E_MI_SYS_PER_CHN_PORT_OUTPUT_POOL;
            tmp.bCreate = !!MmaParam.allocMmaFlag;

            tmp.uConfig.stPreChnPortOutputPrivPool.eModule = E_MI_MODULE_ID_VPE;
            tmp.uConfig.stPreChnPortOutputPrivPool.u32Devid = MmaParam.u32Devid;
            tmp.uConfig.stPreChnPortOutputPrivPool.u32Channel = MmaParam.u32ChnId;
            tmp.uConfig.stPreChnPortOutputPrivPool.u32Port = MmaParam.port;

            tmp.uConfig.stPreChnPortOutputPrivPool.u32PrivateHeapSize = MmaParam.u32PrivateHeapSize;
            //MIXER_DBG("source name: %s. port :%d\n", MmaParam.u8MMAHeapName, tmp.uConfig.stPreChnPortOutputPrivPool.u32Port);
            memcpy(tmp.uConfig.stPreChnPortOutputPrivPool.u8MMAHeapName,  (const char*)MmaParam.u8MMAHeapName, strlen( (const char*)MmaParam.u8MMAHeapName));
            //MIXER_DBG("target name: %s\n", tmp.uConfig.stPreChnPortOutputPrivPool.u8MMAHeapName);
            s32Ret = MI_SYS_ConfigPrivateMMAPool(&tmp);

            break;

        case MODULE_ID_DIVP_FRAME_OUT:

            tmp.eConfigType = E_MI_SYS_PER_CHN_PORT_OUTPUT_POOL;
            tmp.bCreate = !!MmaParam.allocMmaFlag;
            tmp.uConfig.stPreChnPortOutputPrivPool.eModule = E_MI_MODULE_ID_DIVP;
            tmp.uConfig.stPreChnPortOutputPrivPool.u32Devid = MmaParam.u32Devid;
            tmp.uConfig.stPreChnPortOutputPrivPool.u32Channel = MmaParam.u32ChnId;
            tmp.uConfig.stPreChnPortOutputPrivPool.u32Port = MmaParam.port;

            tmp.uConfig.stPreChnPortOutputPrivPool.u32PrivateHeapSize = MmaParam.u32PrivateHeapSize;
            memcpy(tmp.uConfig.stPreChnPortOutputPrivPool.u8MMAHeapName,  (const char*)MmaParam.u8MMAHeapName, strlen( (const char*)MmaParam.u8MMAHeapName));
            s32Ret = MI_SYS_ConfigPrivateMMAPool(&tmp);

            break;

        case MODULE_ID_VENC_H264_5:
        case MODULE_ID_VENC_JPG:
            tmp.eConfigType = E_MI_SYS_PER_CHN_PRIVATE_POOL;
            tmp.bCreate = !!MmaParam.allocMmaFlag;

            tmp.uConfig.stPreChnPrivPoolConfig.eModule = E_MI_MODULE_ID_VENC;
            tmp.uConfig.stPreChnPrivPoolConfig.u32Devid = MmaParam.u32Devid;
            tmp.uConfig.stPreChnPrivPoolConfig.u32Channel = MmaParam.u32ChnId;

            tmp.uConfig.stPreChnPrivPoolConfig.u32PrivateHeapSize = MmaParam.u32PrivateHeapSize;
            memcpy(tmp.uConfig.stPreChnPrivPoolConfig.u8MMAHeapName,  (const char*)MmaParam.u8MMAHeapName, strlen( (const char*)MmaParam.u8MMAHeapName));
            s32Ret = MI_SYS_ConfigPrivateMMAPool(&tmp);

            break;
/*#if TARGET_CHIP_I6B0
        case MODULE_ID_VENC_VPU_COMMON:
            tmp.eConfigType = E_MI_SYS_PER_DEV_PRIVATE_POOL;
            tmp.bCreate = !!MmaParam.allocMmaFlag;

            tmp.uConfig.stPreDevPrivPoolConfig.eModule = E_MI_MODULE_ID_VENC;
            tmp.uConfig.stPreDevPrivPoolConfig.u32Devid = MmaParam.u32Devid;
            tmp.uConfig.stPreChnPrivPoolConfig.u32PrivateHeapSize = MmaParam.u32PrivateHeapSize;
            memcpy(tmp.uConfig.stPreDevPrivPoolConfig.u8MMAHeapName,  (const char*)MmaParam.u8MMAHeapName, strlen( (const char*)MmaParam.u8MMAHeapName));
            s32Ret = MI_SYS_ConfigPrivateMMAPool(&tmp);
            break;
#endif*/
        case MODULE_ID_RGN:
            tmp.eConfigType = E_MI_SYS_PER_DEV_PRIVATE_POOL;
            tmp.bCreate = !!MmaParam.allocMmaFlag;

            tmp.uConfig.stPreDevPrivPoolConfig.eModule = E_MI_MODULE_ID_RGN;
            tmp.uConfig.stPreDevPrivPoolConfig.u32Devid = MmaParam.u32Devid;
            tmp.uConfig.stPreDevPrivPoolConfig.u32PrivateHeapSize = MmaParam.u32PrivateHeapSize;
            memcpy(tmp.uConfig.stPreDevPrivPoolConfig.u8MMAHeapName,  (const char*)MmaParam.u8MMAHeapName, strlen( (const char*)MmaParam.u8MMAHeapName));
            s32Ret = MI_SYS_ConfigPrivateMMAPool(&tmp);

            break;

        case MODULE_ID_AI:
            tmp.eConfigType = E_MI_SYS_PER_DEV_PRIVATE_POOL;
            tmp.bCreate = !!MmaParam.allocMmaFlag;

            tmp.uConfig.stPreDevPrivPoolConfig.eModule = E_MI_MODULE_ID_AI;
            tmp.uConfig.stPreDevPrivPoolConfig.u32Devid = MmaParam.u32Devid;

            tmp.uConfig.stPreDevPrivPoolConfig.u32PrivateHeapSize = MmaParam.u32PrivateHeapSize;
            memcpy(tmp.uConfig.stPreDevPrivPoolConfig.u8MMAHeapName,  (const char*)MmaParam.u8MMAHeapName, strlen( (const char*)MmaParam.u8MMAHeapName));
            s32Ret = MI_SYS_ConfigPrivateMMAPool(&tmp);

            break;

        /*case MODULE_ID_AICHN:
            tmp.eConfigType = E_MI_SYS_PER_CHN_PRIVATE_POOL;
            tmp.bCreate = !!MmaParam.allocMmaFlag;

            tmp.uConfig.stPreDevPrivPoolConfig.eModule = E_MI_MODULE_ID_AI;
            tmp.uConfig.stPreDevPrivPoolConfig.u32Devid = MmaParam.u32Devid;
            tmp.uConfig.stPreChnPrivPoolConfig.u32Channel = MmaParam.u32ChnId;

            tmp.uConfig.stPreChnPrivPoolConfig.u32PrivateHeapSize = MmaParam.u32PrivateHeapSize;
            memcpy(tmp.uConfig.stPreDevPrivPoolConfig.u8MMAHeapName,  (const char*)MmaParam.u8MMAHeapName, strlen( (const char*)MmaParam.u8MMAHeapName));
            s32Ret = MI_SYS_ConfigPrivateMMAPool(&tmp);
            break;*/

        case MODULE_ID_AO:
            tmp.eConfigType = E_MI_SYS_PER_DEV_PRIVATE_POOL;
            tmp.bCreate = !!MmaParam.allocMmaFlag;

            tmp.uConfig.stPreDevPrivPoolConfig.eModule = E_MI_MODULE_ID_AO;
            tmp.uConfig.stPreDevPrivPoolConfig.u32Devid = MmaParam.u32Devid;

            tmp.uConfig.stPreDevPrivPoolConfig.u32PrivateHeapSize = MmaParam.u32PrivateHeapSize;
            memcpy(tmp.uConfig.stPreDevPrivPoolConfig.u8MMAHeapName,  (const char*)MmaParam.u8MMAHeapName, strlen( (const char*)MmaParam.u8MMAHeapName));
            s32Ret = MI_SYS_ConfigPrivateMMAPool(&tmp);
            break;

        default:
            MIXER_ERR("module id is err.\n");
            break;
    }
    MIXER_DBG("eModule:%d,  HeapSize:0x%x, HeapName:%s. eConfigType:%d, Config.eModule:%d, s32Ret:%x.\n", \
                MmaParam.eModule, \
                MmaParam.u32PrivateHeapSize, \
                MmaParam.u8MMAHeapName,\
                tmp.eConfigType,\
                tmp.uConfig.stPreDevPrivPoolConfig.eModule,\
                s32Ret);
    return s32Ret;
}

