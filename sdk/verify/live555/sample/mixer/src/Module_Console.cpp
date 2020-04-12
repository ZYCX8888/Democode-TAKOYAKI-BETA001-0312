/*************************************************
*
* Copyright (c) 2018-2019 SigmaStar Technology Inc.
* All rights reserved.
*
**************************************************
* File name:  Module_Console.cpp
* Author:     fisher.yang@sigmastar.com.cn
* Version:    Initial Draft
* Date:       2019/9/10
* Description: mixer record source file
*
*
*
* History:
*
*    1. Date  :        2019/9/10
*       Author:        fisher.yang@sigmastar.com.cn
*       Modification:  Created file
*
**************************************************/

#include <stdio.h>

#include "Module_Console.h"
#include "mi_common_datatype.h"
#include "module_common.h"
#include "mid_system_config.h"
#include "mid_utils.h"
#include "mid_common.h"
#include "mid_VideoEncoder.h"

extern int gDebug_VideoStreamLog;
extern int gDebug_AudioStreamLog;
extern MixerVideoParam g_videoParam[MAX_VIDEO_NUMBER];
extern MI_AI_VqeConfig_t g_stAiVqeCfg;
extern MI_AO_VqeConfig_t g_stAoVqeCfg;
extern MixerAudioInParam g_audioInParam[MIXER_AI_MAX_NUMBER];
extern MixerAudioOutParam g_audioOutParam[MIXER_AO_MAX_NUMBER];
extern int  gDebug_saveAudioData[MIXER_AI_MAX_NUMBER];
extern MI_S32 g_mdParam;
extern MI_S32 g_odParam;
extern MI_S32 g_uvcParam;
extern MI_S32 g_fdParam;
extern MI_S32 g_vgParam;
extern MI_S32 g_hchdParam;
extern MI_S32 g_dlaParam;
extern MI_S32 g_MD_InitDone;
extern MI_S32 g_displayOsd;
extern MI_S32 gDebug_osdColorInverse;
extern MI_U32 g_rotation;
extern MI_S32 g_ieLogParam;
extern int g_IE_Open;
extern MI_S32 g_ieVpePort;
extern int g_ieWidth;
extern int g_ieHeight;
extern MI_U32 g_videoNumber;
extern MI_S32 gDebug_osdDrawRect;
extern MI_S32 gDebug_OsdTest;
extern MI_S32 g_displayVideoInfo;
extern MI_S32 g_s32OsdFlicker;
extern MI_S32 g_s32OsdHandleCnt;
extern MI_S32 gDebug_osdPrivateMask;
extern MI_S32 g_displayVideoInfo;
extern MI_U32 g_videoMaxWidth;
extern MI_U32 g_videoMaxHeight;
extern BOOL g_ShowFrameInterval;
extern BOOL g_bCusAEenable;
extern MI_U32 g_bInitRotate;
extern MI_S32 g_ieOsdDisplayDisable;
extern MI_S32 g_modeParam;
extern MI_VideoEncoder *g_videoEncoderArray[MAX_VIDEO_NUMBER];
extern void mixerBitrateCheck(MI_U8 i,MI_U32 sTWidth, MI_U32 sTHeight,MI_U32 maxBitRate,MI_U32 minBitRate,MI_U32 maxBitRate1,MI_U32 minBitRate1,MI_U8 currentFps,MI_U32 *currentBitRate);


extern MI_S32 mixerEnableIEModule(MI_S8 enable);
extern int MasktoRect(MI_VENC_CHN VeChn, MI_S32 s32MaskNum, MI_SYS_WindowRect_t *pstRect);
extern void setOsdParam(const MI_S8 *buf[], MI_U8 length);
extern void setIspParam(MI_S8 *buf[], MI_U32 ParamLength);
static char  CharBuf[PARAMLIST][PARAMLENGTH];

static MI_S32 MixerGetModeParam()
{
    return g_modeParam;
}

static MI_S32 MixerDisableOSD(MixerCmdId cmdId, MI_S32 veChn, MI_S32 width, MI_S32 height, MI_S32 IeEnable)
{
    MI_S32 param[3];

    if((g_ieLogParam) && (0 == g_IE_Open))
    {
        printf("%s:%d g_IE_Open has not been set\n", __func__, __LINE__);
    }
    else
    {
        printf("%s:%d Disable mixer OSD\n", __func__, __LINE__);
    }

    if(g_displayOsd || gDebug_osdColorInverse)
    {
        if((g_ieLogParam) && (1 == IeEnable))
        {
            mixerEnableIEModule(0);

            param[0] = g_ieVpePort;
            param[1] = g_ieWidth;
            param[2] = g_ieHeight;
            mixer_send_cmd(CMD_IE_CLOSE, (MI_S8 *) param, sizeof(param));
        }

        if((0 <= veChn) && (veChn < (MI_S32)g_videoNumber))
        {
            param[0] = veChn;
            param[1] = width;
            param[2] = height;
            mixer_send_cmd(cmdId, (MI_S8 *) param, sizeof(param));
        }
        else if(veChn < 0)
        {
            for(MI_U32 i = 0; i < g_videoNumber; i++)
            {
                param[0] = i;
                param[1] = g_videoParam[i].width;
                param[2] = g_videoParam[i].height;
                mixer_send_cmd(cmdId, (MI_S8 *) param, sizeof(param));
            }
        }
    }

    return 0;
}

static MI_S32 MixerEnableOSD(MixerCmdId cmdId, MI_S32 veChn, MI_S32 width, MI_S32 height, MI_S32 IeEnable)
{
    MI_S32 param[3];
    MI_U32 i = 0x0;

    if((g_ieLogParam) && (g_IE_Open))
    {
        MIXER_DBG("%s:%d g_IE_Open has been set\n", __func__, __LINE__);
    }
    else
    {
        MIXER_DBG("%s:%d Enable mixer OSD\n", __func__, __LINE__);
    }

    if((g_ieLogParam) && (1 == IeEnable))
    {
        param[0] = g_ieVpePort;
        param[1] = g_ieWidth;
        param[2] = g_ieHeight;
        if(0 == g_IE_Open)
            mixer_send_cmd(CMD_IE_OPEN, (MI_S8 *) param, sizeof(param));

        mixerEnableIEModule(1);
    }

    if(g_displayOsd || gDebug_osdColorInverse)
    {
        if((0 <= veChn) && (veChn < (MI_S32)g_videoNumber))
        {
            g_videoParam[veChn].width  = width;
            g_videoParam[veChn].height = height;

            param[0] = veChn;
            param[1] = g_videoParam[veChn].width;
            param[2] = g_videoParam[veChn].height;
            mixer_send_cmd(cmdId, (MI_S8 *) param, sizeof(param));
        }
        else if(veChn < 0)
        {
            for(i = 0; i < g_videoNumber; i++)
            {
                if((width < (MI_S32)g_videoParam[i].width) || (height < (MI_S32)g_videoParam[i].height))
                {
                    g_videoParam[i].width  = width;
                    g_videoParam[i].height = height;
                }

                param[0] = i;
                param[1] = g_videoParam[i].width;
                param[2] = g_videoParam[i].height;
                mixer_send_cmd(cmdId, (MI_S8 *) param, sizeof(param));
            }
        }
    }

    return 0;
}


static BOOL mixerResolutionValid(MI_U32 venc,MI_U32 width, MI_U32 height)
{
    if(width < 256 || height < 256)
    {
        MIXER_ERR("width:%d height:%d\n", width, height);
        return FALSE;
    }
#if TARGET_CHIP_I6B0
    {
        MI_U32 i = 0x0;
        if(venc >= g_videoNumber)
        {
            MIXER_ERR("ch out of range. %d\n", venc);
            return FALSE;
        }
        if(g_videoEncoderArray[venc]->m_VpeChnPort.u32PortId==VPE_REALMODE_SUB_PORT)
        {
            //for,I6B0, tow venc can be bind to real divp chn directly.
            //at this situation, you can not change resolution.
            for(i = 0; i < g_videoNumber; i++)
            {
                if(i==venc)continue;
                if(g_videoEncoderArray[i]->m_VpeChnPort.u32PortId==VPE_REALMODE_SUB_PORT && (width!=g_videoEncoderArray[i]->m_width || height!=g_videoEncoderArray[i]->m_height))
                {
                    MIXER_ERR("venc %d and %d bind to Real divp,you can not change resolution one of them!\n",venc,i);
                    return FALSE;
                }

            }
        }
        else if(g_videoEncoderArray[venc]->m_VpeChnPort.u32PortId==2 && g_videoEncoderArray[venc]->m_DivpChnPort.eModId  == E_MI_MODULE_ID_MAX)
        {
            for(i = 0; i < g_videoNumber; i++)
            {
                if(i==venc)continue;
                //for I6B0, two divp chn can be bind to vpe port directly.
                //at this situation, you can not change resolution.
                if(g_videoEncoderArray[i]->m_VpeChnPort.u32PortId==2 && g_videoEncoderArray[i]->m_DivpChnPort.eModId  == E_MI_MODULE_ID_MAX && \
                   (width!=g_videoEncoderArray[i]->m_width || height!=g_videoEncoderArray[i]->m_height))
                {
                    MIXER_ERR("venc %d and %d bind to vpe port 2,you can not change resolution one of them!\n",venc,i);
                    return FALSE;
                }
                //for I6B0, ONE divp chn and one venc can be bind to vpe port directly.
                //at this situation, you can not change vpe port 2 resolution,but you can change divp resolution.
                if(g_videoEncoderArray[i]->m_VpeChnPort.u32PortId==2 && g_videoEncoderArray[i]->m_DivpChnPort.eModId  == E_MI_MODULE_ID_DIVP && \
                   (width!=g_videoEncoderArray[venc]->m_width || height!=g_videoEncoderArray[venc]->m_height))
                {
                    MIXER_ERR("venc %d and divp chn%d bind to vpe port 2,you can not change resolution of vpe port 2!\n",venc,g_videoEncoderArray[i]->m_DivpChnPort.u32ChnId);
                    return FALSE;
                }
            }

        }

    }
#else
    venc = venc;
#endif
    return TRUE;
}


void CConsoleManager::Console_SnrFramerate(void)
{
    MI_S32 param[2];
    char *buf = GetParam(1);

    if( NULL == buf)
    {
        MIXER_ERR("err param buf is NULL\n");
        return;
    }
    param[0] = 0;
    param[1] = atoi(buf);

    mixer_send_cmd(CMD_VIDEO_SET_SENSOR_FRAMERATE, (MI_S8 *) param, sizeof(param));
}

void CConsoleManager::Console_SnrFramerateHelp(void)
{
    printf("\n sensorfps <fps>\n");
    printf("\t fps: sensor min/max fps\n");
    printf("\t example: sensorfps 29\n");
}

void CConsoleManager::Console_AudioVqeMode(void)
{
    MI_S8 param[512]= {0x0};
    MI_U32 WorkMode;
    MI_S32 tmp;
    char *buf[6];

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);
    buf[2] = GetParam(3);
    buf[3] = GetParam(4);
    buf[4] = GetParam(5);
    buf[5] = GetParam(6);

    if(NULL == buf[0] )
    {
        MIXER_ERR("err param\n");
        return;
    }

    WorkMode = atoi(buf[0]);
    if(((WorkMode != 0) && (WorkMode != 1)))
    {
        MIXER_ERR("err Module VQE:0-AO,1-AI\n");
        return;
    }

    if(1 == WorkMode)
    {
        //determine if audioIn is open
        for(int i = 0; i < g_audioInParam[0].u8AudioInNum; i++)
        {
            if(TRUE == g_audioInParam[i].bFlag)
                continue;
            else
            {
                printf("audioIn is not open\n");
                return;
            }
        }

        if(g_audioInParam[0].bAudioInVqe == 0)
        {
            printf("audioIn VQE is not open\n");
            return;
        }

        if(NULL == buf[1]  ||  NULL == buf[2] || NULL == buf[3] || NULL == buf[4] || NULL == buf[5])
        {
            MIXER_ERR("err param\n");
            return;
        }

        tmp = atoi(buf[1]);
        if(((tmp != 0) && (tmp != 1)))
        {
            MIXER_ERR("err.  AI HPF enable or disable: 1-enable,0-disable\n");
            return;
        }

        g_stAiVqeCfg.bHpfOpen = tmp;

        tmp = atoi(buf[2]);
        if(((tmp != 0) && (tmp != 1)))
        {
            MIXER_ERR("err.  AI AEC enable or disable :1-enable,0-disable\n");
            return;
        }

        g_stAiVqeCfg.bAecOpen = tmp;

        tmp = atoi(buf[3]);
        if(((tmp != 0) && (tmp != 1)))
        {
            MIXER_ERR("err.  AI ANR enable or disable :1-enable,0-disable\n");
            return;
        }

        g_stAiVqeCfg.bAnrOpen = tmp;

        tmp = atoi(buf[4]);
        if(((tmp != 0) && (tmp != 1)))
        {
            MIXER_ERR("err.  AI AGC enable or disable :1-enable,0-disable\n");
            return;
        }

        g_stAiVqeCfg.bAgcOpen = tmp;

        tmp = atoi(buf[5]);
        if(((tmp != 0) && (tmp != 1)))
        {
            MIXER_ERR("err.  AI EQ enable or disable :1-enable,0-disable\n");
            return;
        }

        g_stAiVqeCfg.bEqOpen = tmp;

        param[0] = 1;
        memcpy(&param[1], (char *)&g_stAiVqeCfg, sizeof(MI_AI_VqeConfig_t));
    }
    else if(0 == WorkMode)
    {
        //determine if audioOut is open
        for(MI_U32 i = 0; i < g_audioOutParam[0].s32AudioOutNum; i++)
        {
            if(TRUE == g_audioOutParam[i].bFlag)
                continue;
            else
            {
                printf("audioOut is not open\n");
                return;
            }
        }

        if(g_audioOutParam[0].u8AudioOutVqe == 0)
        {
            printf("audioOut VQE is not open\n");
            return;
        }

        if(NULL == buf[1]  ||  NULL == buf[2] || NULL == buf[3] || NULL == buf[4])
        {
            MIXER_ERR("err param\n");
            return;
        }

        tmp = atoi(buf[1]);
        if(((tmp != 0) && (tmp != 1)))
        {
            MIXER_ERR("err.  AO HPF enable or disable :1-enable,0-disable\n");
            return;
        }

        g_stAoVqeCfg.bHpfOpen = tmp;

        tmp = atoi(buf[2]);
        if(((tmp != 0) && (tmp != 1)))
        {
            MIXER_ERR("err.  AO ANR enable or disable :1-enable,0-disable\n");
            return;
        }

        g_stAoVqeCfg.bAnrOpen = tmp;

        tmp = atoi(buf[3]);
        if(((tmp != 0) && (tmp != 1)))
        {
            MIXER_ERR("err.  AO AGC enable or disable :1-enable,0-disable\n");
            return;
        }

        g_stAoVqeCfg.bAgcOpen = tmp;

        tmp = atoi(buf[4]);
        if(((tmp != 0) && (tmp != 1)))
        {
            MIXER_ERR("err.  AO EQ enable or disable :1-enable,0-disable\n");
            return;
        }

        g_stAoVqeCfg.bEqOpen = tmp;

        param[0] = 0;
        memcpy(&param[1], (char *)&g_stAoVqeCfg, sizeof(MI_AO_VqeConfig_t));
    }

    mixer_send_cmd(CDM_AUDIO_SETVQEMODULE, (MI_S8 *)&param, sizeof(param));
}

void CConsoleManager::Console_AudioVqeModeHelp(void)
{
    printf("\n\t audiovqe <module> <HPF> <AEC> <ANR> <AGC> <EQ>\n");
    printf("\n\t\t module: 0/1   0:AO   1:AI\n");
    printf("\n\t\t HPF: 0/1   0:Disable   1:Enable\n");
    printf("\n\t\t AEC: 0/1   0:Disable   1:Enable. it must be worked @AI mode\n");
    printf("\n\t\t ANR: 0/1   0:Disable   1:Enable\n");
    printf("\n\t\t AGC: 0/1   0:Disable   1:Enable\n");
    printf("\n\t\t EQ: 0/1   0:Disable   1:Enable\n");
    printf("\n\t\t example: audiovqe 1 0 0 1 0 0 \n");
}

void CConsoleManager::Console_VideoBitrate(void)
{
    //bitrate control
    MI_S32 param[16], paramLen = 0, control = 0;

    char *buf[16];

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);
    buf[2] = GetParam(3);
    buf[3] = GetParam(4);
    buf[4] = GetParam(5);
    buf[5] = GetParam(6);
    buf[6] = GetParam(7);
    buf[7] = GetParam(8);
    buf[8] = GetParam(9);

    if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("err param\n");
        return;
    }
    param[0] = atoi(buf[0]);
    if(param[0] >=  (MI_S32)g_videoNumber)
    {
        MIXER_ERR("venc err. to change bitrate, ch:%d\n", param[0]);
        return;
    }
    control = atoi(buf[1]);
    if(control < 0 || control > 3)
    {
        MIXER_ERR("control err. ([0]cbr, [1]vbr, [2]fixqp, [3]avbr): %d\n", control);
        return;
    }
    if(0 == control) // cbr
    {
        param[1] = control;
        if((VE_JPG == g_videoParam[param[0]].encoderType) || (VE_MJPEG == g_videoParam[param[0]].encoderType))
        {
            if(NULL == buf[2] || NULL == buf[3] || NULL == buf[4])
            {
                MIXER_ERR("err param\n");
                return;
            }
            param[2] = atoi(buf[2]);
            if(param[2] <=  0)
            {
                MIXER_ERR("bitrate err. bitrate:%d\n", param[2]);
                return;
            }

            param[3] = atoi(buf[3]);
            if(param[3] <  20)
            {
                MIXER_WARN("MaxQfactor err. MaxQfactor:%d\n", param[3]);
                param[3] =  20;
            }
            else if(param[3] > 90)
            {
                MIXER_WARN("MaxQfactor err. MaxQfactor:%d\n", param[3]);
                param[3] =  90;
            }

            param[4] = atoi(buf[4]);
            if(param[4] <  20)
            {
                MIXER_WARN("MinQfactor err. MinQfactor:%d\n", param[4]);
                param[4] =  20;
            }
            else if(param[4] > 90)
            {
                MIXER_WARN("MinQfactor err. MinQfactor:%d\n", param[4]);
                param[4] =  90;
            }

            paramLen = 5;

            g_videoParam[param[0]].u8MaxQfactor = param[3];
            g_videoParam[param[0]].u8MinQfactor = param[4];
        }
        else
        {
            if(NULL == buf[2] || NULL == buf[3] || NULL == buf[4] || \
               NULL == buf[5] ||NULL == buf[6] ||NULL == buf[7] )
            {
                MIXER_ERR("err param\n");
                return;
            }
            param[2] = atoi(buf[2]);
            if(param[2] <=  0)
            {
                MIXER_ERR("bitrate err. bitrate:%d\n", param[2]);
                return;
            }

            param[3] = atoi(buf[3]);
            if(param[3] <  12)
            {
                MIXER_WARN("maxQp err. maxQp:%d\n", param[3]);
                param[3] =  12;
            }
            else if(param[3] > 48)
            {
                MIXER_WARN("maxQp err. maxQp:%d\n", param[3]);
                param[3] =  48;
            }

            param[4] = atoi(buf[4]);
            if(param[4] <  12)
            {
                MIXER_WARN("minQp err. minQp:%d\n", param[4]);
                param[4] =  12;
            }
            else if(param[4] > 48)
            {
                MIXER_WARN("minQp err. minQp:%d\n", param[4]);
                param[4] =  48;
            }

            param[5] = atoi(buf[5]);
            if(param[5] <  12)
            {
                MIXER_WARN("maxIQp err. maxIQp:%d\n", param[5]);
                param[5] =  12;
            }
            else if(param[5] > 48)
            {
                MIXER_WARN("maxIQp err. maxIQp:%d\n", param[5]);
                param[5] =  48;
            }

            param[6] = atoi(buf[6]);
            if(param[6] <  12)
            {
                MIXER_WARN("minIQp err. minIQp:%d\n", param[6]);
                param[6] =  12;
            }
            else if(param[6] > 48)
            {
                MIXER_WARN("minIQp err. minIQp:%d\n", param[6]);
                param[6] =  48;
            }

            param[7] = atoi(buf[7]);
            if(param[7] <  -12)
            {
                MIXER_WARN("minIQp err. minIQp:%d\n", param[7]);
                param[7] =  -12;
            }
            else if(param[7] > 12)
            {
                MIXER_WARN("minIQp err. minIQp:%d\n", param[7]);
                param[7] =  12;
            }

            paramLen = 8;
        }
    }
    else if(1 == control) // vbr
    {
        param[1] = control;

        if(NULL == buf[2] || NULL == buf[3] || NULL == buf[4] || \
           NULL == buf[5] ||NULL == buf[6] ||NULL == buf[7]  || NULL == buf[8])
        {
            MIXER_ERR("err param\n");
            return;
        }
        param[2] = atoi(buf[2]);
        if(param[2] <=  0)
        {
            MIXER_ERR("max bitrate err. max bitrate:%d\n", param[2]);
            return;
        }

        param[3] = atoi(buf[3]);
        if(param[3] <  12)
        {
            MIXER_WARN("maxQp err. maxQp:%d\n", param[3]);
            param[3] =  12;
        }
        else if(param[3] > 48)
        {
            MIXER_WARN("maxQp err. maxQp:%d\n", param[3]);
            param[3] =  48;
        }

        param[4] = atoi(buf[4]);
        if(param[4] <  12)
        {
            MIXER_WARN("minQp err. minQp:%d\n", param[4]);
            param[4] =  12;
        }
        else if(param[4] > 48)
        {
            MIXER_WARN("minQp err. minQp:%d\n", param[4]);
            param[4] =  48;
        }

        param[5] = atoi(buf[5]);
        if(param[5] <  12)
        {
            MIXER_WARN("maxIQp err. maxIQp:%d\n", param[5]);
            param[5] =  12;
        }
        else if(param[5] > 48)
        {
            MIXER_WARN("maxIQp err. maxIQp:%d\n", param[5]);
            param[5] =  48;
        }

        param[6] = atoi(buf[6]);
        if(param[6] <  12)
        {
            MIXER_WARN("minIQp err. minIQp:%d\n", param[6]);
            param[6] =  12;
        }
        else if(param[6] > 48)
        {
            MIXER_WARN("minIQp err. minIQp:%d\n", param[6]);
            param[6] =  48;
        }

        param[7] = atoi(buf[7]);
        if(param[7] <  -12)
        {
            MIXER_WARN("minIQp err. minIQp:%d\n", param[7]);
            param[7] =  -12;
        }
        else if(param[7] > 12)
        {
            MIXER_WARN("minIQp err. minIQp:%d\n", param[7]);
            param[7] =  12;
        }

        param[8] = atoi(buf[8]);
        if(param[8] <  50)
        {
            MIXER_WARN("minIQp err. minIQp:%d\n", param[8]);
            param[8] =  50;
        }
        else if(param[8] > 100)
        {
            MIXER_WARN("minIQp err. minIQp:%d\n", param[8]);
            param[8] =  100;
        }

        paramLen = 9;
    }
    else if(2 == control) // fixqp
    {
        param[1] = control;
        if((VE_JPG == g_videoParam[param[0]].encoderType) || (VE_MJPEG == g_videoParam[param[0]].encoderType))
        {
            if(NULL == buf[2])
            {
                MIXER_ERR("err param\n");
                return;
            }
            param[2] = atoi(buf[2]);
            if(param[2] <  20)
            {
                MIXER_ERR("Qfactor err. Qfactor:%d\n", param[2]);
                param[2] = 20;
            }
            else if(param[2] > 90)
            {
                MIXER_ERR("Qfactor err. Qfactor:%d\n", param[2]);
                param[2] = 90;
            }

            paramLen = 3;
        }
        else
        {
            if(NULL == buf[2] || NULL == buf[3])
            {
                MIXER_ERR("err param\n");
                return;
            }
            param[2] = atoi(buf[2]);
            param[3] = atoi(buf[3]);

            paramLen = 4;
        }
    }
    else if(3 == control) //avbr
    {
        param[1] = control;
     buf[9] = GetParam(10);
     buf[10] = GetParam(11);
     buf[11] = GetParam(12);

        if(NULL == buf[2] || NULL == buf[3] || NULL == buf[4] || \
           NULL == buf[5] ||NULL == buf[6] ||NULL == buf[7]  || NULL == buf[8] ||\
           NULL == buf[9] || NULL == buf[10] || NULL == buf[11])
        {
            MIXER_ERR("err param\n");
            return;
        }
        param[2] = atoi(buf[2]);
        if(param[2] <=  0)
        {
            MIXER_ERR("max bitrate err. max bitrate:%d\n", param[2]);
            return;
        }

        param[3] = atoi(buf[3]);
        if(param[3] <  12)
        {
            MIXER_WARN("maxQp err. maxQp:%d\n", param[3]);
            param[3] =  12;
        }
        else if(param[3] > 48)
        {
            MIXER_WARN("maxQp err. maxQp:%d\n", param[3]);
            param[3] =  48;
        }

        param[4] = atoi(buf[4]);
        if(param[4] <  12)
        {
            MIXER_WARN("minQp err. minQp:%d\n", param[4]);
            param[4] =  12;
        }
        else if(param[4] > 48)
        {
            MIXER_WARN("minQp err. minQp:%d\n", param[4]);
            param[4] =  48;
        }

        param[5] = atoi(buf[5]);
        if(param[5] <  12)
        {
            MIXER_WARN("maxIQp err. maxIQp:%d\n", param[5]);
            param[5] =  12;
        }
        else if(param[5] > 48)
        {
            MIXER_WARN("maxIQp err. maxIQp:%d\n", param[5]);
            param[5] =  48;
        }

        param[6] = atoi(buf[6]);
        if(param[6] <  12)
        {
            MIXER_WARN("minIQp err. minIQp:%d\n", param[6]);
            param[6] =  12;
        }
        else if(param[6] > 48)
        {
            MIXER_WARN("minIQp err. minIQp:%d\n", param[6]);
            param[6] =  48;
        }

        param[7] = atoi(buf[7]);
        if(param[7] <  -12)
        {
            MIXER_WARN("DeltaQp out of range:%d\n", param[7]);
            param[7] =  -12;
        }
        else if(param[7] > 12)
        {
            MIXER_WARN("DeltaQp out of range:%d\n", param[7]);
            param[7] =  12;
        }

        param[8] = atoi(buf[8]);
        if(param[8] <  50)
        {
            MIXER_WARN("change pos:%d\n", param[8]);
            param[8] =  50;
        }
        else if(param[8] > 100)
        {
            MIXER_WARN("change pos:%d\n", param[8]);
            param[8] =  100;
        }

        param[9] = atoi(buf[9]);
        if(param[9] <  5)
        {
            MIXER_WARN("u32MinStillPercen:%d\n", param[9]);
            param[9] =  5;
        }
        else if(param[9] > 100)
        {
            MIXER_WARN("u32MinStillPercent:%d\n", param[9]);
            param[9] =  100;
        }
        param[10] = atoi(buf[10]);
        if(param[10] <  12)
        {
            MIXER_WARN("u32MaxStillQp:%d\n", param[10]);
            param[10] =  12;
        }
        else if(param[10] > 48)
        {
            MIXER_WARN("u32MaxStillQp:%d\n", param[10]);
            param[10] =  48;
        }

        param[11] = atoi(buf[11]);
        if(param[11] <  0)
        {
            MIXER_WARN("u32MotionSensitivity:%d\n", param[11]);
            param[11] =  0;
        }
        else if(param[11] > 100)
        {
            MIXER_WARN("u32MotionSensitivity:%d\n", param[11]);
            param[11] =  100;
        }
        paramLen = 12;
    }
#if TARGET_CHIP_I6
    if(2 != control)
    {
        MI_U32 tmp_width = 0;
        MI_U32 tmp_height = 0;

#if (!TARGET_CHIP_I6E)
        if((90 == (g_rotation & 0xFFFF)) || (270 == (g_rotation & 0xFFFF)))
        {
            tmp_width =  g_videoParam[param[0]].height;
            tmp_height =  g_videoParam[param[0]].width;
            if((2048 <= tmp_height && 3840 >= tmp_height)&&(1536 <= tmp_width && 2160 >= tmp_width))
            {
                mixerBitrateCheck(param[0],2160,3840,12000000,3000000,4000000,1000000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);
                mixerBitrateCheck(param[0],2160,3072,10000000,3000000,3000000,768000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);

                mixerBitrateCheck(param[0],1944,2952,8000000, 2000000,4000000,1000000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);
                mixerBitrateCheck(param[0],1536,2560,6000000, 2000000,2000000,768000,g_videoParam[param[0]].vencframeRate, (MI_U32 *)&param[2]);

                mixerBitrateCheck(param[0],1440,2560,6000000, 2000000,2000000,500000,g_videoParam[param[0]].vencframeRate, (MI_U32 *)&param[2]);
                mixerBitrateCheck(param[0],1536,2048,5000000, 1500000,2000000,500000,g_videoParam[param[0]].vencframeRate, (MI_U32 *)&param[2]);
            }
            else if(((640 <= tmp_height) && (1920 >= tmp_height))&&((480 <= tmp_width) && (1088 >= tmp_width)))
            {
                mixerBitrateCheck(param[0],1088,1920,4000000, 1000000,1500000,300000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);
                mixerBitrateCheck(param[0],1080,1920,4000000, 1000000,1500000,300000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);

                mixerBitrateCheck(param[0],736,1280, 2000000, 768000,1000000,250000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);
                mixerBitrateCheck(param[0],720,1280, 2000000, 768000,1000000,250000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);

                mixerBitrateCheck(param[0],576,720, 1500000, 500000,768000,200000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);
                mixerBitrateCheck(param[0],576,704, 1500000, 500000,768000,200000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);

                mixerBitrateCheck(param[0],480,640, 1000000, 500000,768000,200000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);
            }
        }
        else
        {
            if((2048 <= g_videoParam[param[0]].width && 3840 >= g_videoParam[param[0]].width)&&(1536 <= g_videoParam[param[0]].height && 2160 >= g_videoParam[param[0]].height))
            {
                mixerBitrateCheck(param[0],3840,2160,12000000,3000000,4000000,1000000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);
                mixerBitrateCheck(param[0],3072,2160,10000000,3000000,3000000,768000,g_videoParam[param[0]].vencframeRate, (MI_U32 *)&param[2]);

                mixerBitrateCheck(param[0],2952,1944,8000000, 2000000,4000000,1000000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);
                mixerBitrateCheck(param[0],2560,1536,6000000, 2000000,2000000,768000,g_videoParam[param[0]].vencframeRate, (MI_U32 *)&param[2]);

                mixerBitrateCheck(param[0],2560,1440,6000000, 2000000,2000000,500000,g_videoParam[param[0]].vencframeRate, (MI_U32 *)&param[2]);
                mixerBitrateCheck(param[0],2048,1536,5000000, 1500000,2000000,500000,g_videoParam[param[0]].vencframeRate, (MI_U32 *)&param[2]);

            }
            else if((640 <= g_videoParam[param[0]].width && 1920 >= g_videoParam[param[0]].width)||(480 <= g_videoParam[param[0]].height && 1088 >= g_videoParam[param[0]].height))
            {
                mixerBitrateCheck(param[0],1920,1088,4000000, 1000000,1500000,300000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);
                mixerBitrateCheck(param[0],1920,1080,4000000, 1000000,1500000,300000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);

                mixerBitrateCheck(param[0],1280,736, 2000000, 768000,1000000,250000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);
                mixerBitrateCheck(param[0],1280,720, 2000000, 768000,1000000,250000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);

                mixerBitrateCheck(param[0],720,576,  1500000, 500000,768000,200000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);
                mixerBitrateCheck(param[0],704,576,  1500000, 500000,768000,200000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)&param[2]);

                mixerBitrateCheck(param[0],640,480,  1000000, 500000,768000,200000,g_videoParam[param[0]].vencframeRate,(MI_U32 *)param[2]);
            }
        }
#endif
        g_videoParam[param[0]].bitrate = param[2];
    }
#endif
    mixer_send_cmd(CMD_VIDEO_SET_BITRATE_CONTROL, (MI_S8 *) param, sizeof(MI_S32) * paramLen);
}


void CConsoleManager::Console_VideoBitrateHelp(void)
{
    printf("\n videobitrate <channel> <module> <bitrate> <p1> <p2> <p3> <p4> <p5> <p6>\n");
    printf("\n\t channel[0,%d]\n",g_videoNumber-1);
    printf("\n\t  module: 0/1/2/3  [0]:cbr, [1]:vbr, [2]:fixqp, [3]:avbr\n");
    printf("\n\t  bitrate:  \n");
    printf("\n\t  @cbr&&(VE_JPG||VE_MPJEG)  mode\n");
    printf("\n\t     p1: means MaxQfactor, range:[20,90]\n");
    printf("\n\t     p2: means MinQfactor, range:[20,90]\n");
    printf("\n\t  @cbr&&(H264/5)  mode\n");
    printf("\n\t     p1: means maxQp, range:[12,48]\n");
    printf("\n\t     p2: means minQp, range:[12,48]\n");
    printf("\n\t     p3: means maxIQp, range:[12,48]\n");
    printf("\n\t     p4: means minIQp, range:[12,48]\n");
    printf("\n\t     p5: means IPQPDelta, range:[-12,12]\n");
    printf("\n\t  @vbr&&(H264/5)  mode\n");
    printf("\n\t     p1: means maxQp, range:[12,48]\n");
    printf("\n\t     p2: means minQp, range:[12,48]\n");
    printf("\n\t     p3: means maxIQp, range:[12,48]\n");
    printf("\n\t     p4: means minIQp, range:[12,48]\n");
    printf("\n\t     p5: means IPQPDelta, range:[-12,12]\n");
    printf("\n\t     p6: means ChangePos, range:[50,100]\n");
    printf("\n\t  @fixqp&&(VE_JPG||VE_MPJEG)  mode\n");
    printf("\n\t     p1: means Qfactor, range:[20,90]\n");
    printf("\n\t  @fixqp&&(H264/5)  mode\n");
    printf("\n\t     p1: means IQP, range:[20,90]\n");
    printf("\n\t     p1: means PQP, range:[20,90]\n");
    printf("\n\t  @avbr&&(H264/5)   mode\n");
    printf("\n\t     p1: means u32MinStillPercent, range:[5,100]\n");
    printf("\n\t     p2: means u32MaxStillQp, range:[12,48]\n");
    printf("\n\t     p3: means u32MotionSensitivity, range:[0,100]\n");
    printf("\n\t  example: videobitrate 0 1 5000000 48 20 48 20 -1 80\n");
}


void  CConsoleManager::Console_VideoSuperFrameSize()
{
    //Super I/P-frame
    MI_S32 param[3];
    char *buf[3];

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);
    buf[2] = GetParam(3);
    if(NULL == buf[0] || NULL == buf[1] || NULL == buf[2])
    {
        MIXER_ERR("err param\n");
        return;
    }

    param[0] = atoi(buf[0]);
    if(param[0] < 0 || param[0] >= (MI_S32)g_videoNumber)
    {
        MIXER_ERR("channel err\n");
        return;
    }
    param[1] = atoi(buf[1]);
    param[2] = atoi(buf[2]);

    mixer_send_cmd(CMD_VIDEO_SET_SUPERFRAME, (MI_S8 *) param, sizeof(param));
}

void  CConsoleManager::Console_VideoSuperFrameSizeHelp()
{
    printf("\n superframesize <VencCh> <p1> <p2>\n");
    printf("\n\t VencCh: which venc to change Super frame\n");
    printf("\n\t p1: I-frame super frame bps\n");
    printf("\n\t p2: P-frame super frame bps\n");
    printf("\n\t example: superframesize 0 20 10\n");
}


void CConsoleManager::Console_VideoIrcut(void)
{
    MI_S32 param[1];

    char *buf;

    buf = GetParam(1);
    if(NULL == buf)
    {
        MIXER_ERR("buf is null. err\n");
        return;
    }
    param[0]  = !!atoi(buf);

    if(0 == param[0])
        mixer_send_cmd(CMD_SYSTEM_IRCUT_BLACK, (MI_S8 *) param, sizeof(param));
    else
        mixer_send_cmd(CMD_SYSTEM_IRCUT_WHITE, (MI_S8 *) param, sizeof(param));
}

void CConsoleManager::Console_VideoIrcutHelp(void)
{
    printf("\n\t ircut <Mode>\n");
    printf("\n\t Mode: 0/1. [0] Black Mode, [1] White Mode\n");
    printf("\n\t ircut 0\n");
}

void CConsoleManager::Console_VideoAfMoto(void)
{
    MI_S32 param[1];
    MI_U32 u32Mode = -1;
    char *buf[2];
    buf[0] = GetParam(1);
    buf[1] = GetParam(2);
    if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("buf is null. err\n");
        return;
    }
    u32Mode  = atoi(buf[0]);
    param[0]  = atoi(buf[1]);
    if(u32Mode> 2)
    {
        printf("input set afmoto mode[%d] is error\n",u32Mode);
        return;
    }
    if(u32Mode == 0)
    {
        if(param[0] != 0 && param[0] != 1)
        {
            printf("input set afmoto init/deinit[%d] error,should 0 or 1\n",param[0]);
            return;
        }
        if(param[0] == 0)
        {
            mixer_send_cmd(CMD_MOTOR_INIT, (MI_S8 *) param, sizeof(param));
            printf("AF moto init is finished\n\n");
        }
        else
        {
            mixer_send_cmd(CMD_MOTOR_UNINIT, (MI_S8 *) param, sizeof(param));
            printf("AF moto deinit is finished\n\n");
        }
    }
    else if(u32Mode == 1)
    {
        if(param[0] < 0 || param[0] > 3)
        {
            printf("input set afmoto control_action[%d] error,should 0 to 3\n",param[0]);
            return;
        }
        mixer_send_cmd(CMD_MOTOR_CONTROL, (MI_S8 *) param, sizeof(param));
        printf("Af moto set control step[%d] is finished\n",param[0]);
    }
    else if(u32Mode == 2)
    {
        if(param[0] < 0)
        {
            printf("input set afmoto delayms[%d] error,should > 0\n",param[0]);
            return;
        }
        mixer_send_cmd(CMD_MOTOR_DelayMs, (MI_S8 *) param, sizeof(param));
        printf("Af moto set control delayms[%d] is finished\n",param[0]);
    }
}
void CConsoleManager::Console_VideoAfMotoHelp(void)
{
    printf("\n\t afmoto <Mode> <init/control_action/delayms>\n");
    printf("\n\t V0 Mode: 0/1/2. [0]init/deinit Afmoto,[1] control_action mode, [2] set_delayms\n");
    printf("\n\t V1 \n");
    printf("\n\t when mode =0,init:0 deinit:1\n");
    printf("\n\t when mode =1,control_action: 0 zoom in\n");
    printf("\n\t                              1 zoom out\n");
    printf("\n\t                              2 focus far\n");
    printf("\n\t                              3 focus near\n");
    printf("\n\t when mode =2,set_delayms: 500 default is 500\n");
    printf("\n\t example: afmoto 0 1 or afmoto 1 1 or afmoto 2 300\n");
}
void CConsoleManager::Console_StreamLogOnOff(void)
{
    MI_S32 s32Idx = -1;
    char *buf;

    buf = GetParam(1);
    if(NULL == buf)
    {
        MIXER_ERR("buf is null. err\n");
        return;
    }
    s32Idx  = !!atoi(buf);

    if(0 == s32Idx)
    {
        gDebug_VideoStreamLog = !gDebug_VideoStreamLog;
        printf("gDebug_VideoStreamLog=%d (video stream %s)\n", gDebug_VideoStreamLog, gDebug_VideoStreamLog?"ON":"OFF");
    }
    else if(1 == s32Idx)
    {
        gDebug_AudioStreamLog = !gDebug_AudioStreamLog;
        printf("gDebug_AudioStreamLog=%d (audio stream %s)\n", gDebug_AudioStreamLog, gDebug_AudioStreamLog?"ON":"OFF");
    }
}

void CConsoleManager::Console_StreamLogOnOffHelp(void)
{
    printf("\n\t  avideolog <Mode>\n");
    printf("\n\t  Mode: 0/1. [0] Video, [1] Audio\n");
    printf("\n\t  example: avideolog 1 \n");
}

void  CConsoleManager::Console_RecordAudioInData(void)
{
    MI_S32 index = 0;
    MI_S32 param = -1;
    char *buf[2];

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);
    if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("buf is null. err\n");
        return;
    }
    index  = atoi(buf[0]);
    if(index > MIXER_AI_MAX_NUMBER)
    {
        MIXER_ERR("index is err. %d\n", index);
        return;
    }

    param = atoi(buf[1]);
    if(param != 1 && param != 0)
    {
        MIXER_ERR("start[1],stop[0]. err:%d\n", param);
        return;
    }

    if(1 == param)
    {
        printf("%s:%d Start Record AudioIn-%d data ...\n", __func__, __LINE__, param);
        gDebug_saveAudioData[index] = 0xFF;
    }
    else if(0 == param)
    {
        printf("%s:%d Stop Record AudioIn-%d data\n", __func__, __LINE__, param);
        gDebug_saveAudioData[index] = 0;
    }
}

void CConsoleManager::Console_RecordAudioInDataHelp(void)
{
    printf("\n\t audiostream <Ch> <p1>\n");
    printf("\n\t Ch: which Channel of Audio stream data\n");
    printf("\n\t p1: 0/1. [0]: stop save. [1]: start save\n");
    printf("\n\t example: audiostream 0 1\n");
}

void CConsoleManager::Console_AudioAEDParam(void)
{
    MI_S32 sensitivity = 0;
    char *buf[2];
    MI_S32 input = 0;
    buf[0] = GetParam(1);
    buf[1] = GetParam(2);
    if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("buf is null. err\n");
        return;
    }

    sensitivity  = atoi(buf[0]);
    if(sensitivity > 2 || sensitivity < 0)
    {
        MIXER_ERR("sensitivity is err. %d\n", sensitivity);
        return;
    }

    MI_S32 point = 0;

    input  = atoi(buf[1]);
    if(input > 5 || input < 0)
    {
        MIXER_ERR("input is err. %d\n", input);
        return;
    }

    mixer_send_cmd(CMD_AUDIO_SET_AED_SENSITIVITY, (MI_S8 *)&sensitivity, sizeof(sensitivity));

    switch(input)
    {
        case 0:
        {
            point = 10;
        }
        break;
        case 1:
        {
            point = 5;
        }
        break;
        case 2:
        {
            point = 0;
        }
        break;
        case 3:
        {
            point = -5;
        }
        break;
        case 4:
        {
            point = -10;
        }
        break;
    }

    MIXER_DBG("CMD_AUDIO_SET_AED_OPERATIONGPOINT %d\n", point);
    mixer_send_cmd(CMD_AUDIO_SET_AED_OPERATIONGPOINT, (MI_S8 *)&point, sizeof(point));

}

void CConsoleManager::Console_AudioAEDParamHelp(void)
{
    printf("\n\t aed <sensitivity> <operatingpoint>\n");
    printf("\n\t     sensitivity: 0~2.  [0]low,[1]middle,[2]high\n");
    printf("\n\t     operatingpoint: 0~4. [0]: 10. [1]: 5. [2]: 0. [3]: -5. [4]: -10\n");
    printf("\n\t example: aed 1 1\n");
}

void CConsoleManager::Console_MdParam(void)
{

    char *buf[5];
    buf[0] = GetParam(1);
    buf[1] = GetParam(2);
    buf[2] = GetParam(3);
    buf[3] = GetParam(4);
    buf[4] = GetParam(5);

    if(NULL == buf[0] || NULL == buf[1] || NULL == buf[2] || NULL == buf[3] || NULL == buf[4])
    {
        MIXER_ERR("buf is null. err\n");
        return;
    }

    if(0 == g_mdParam)
    {
        printf("MD function of mixer is not opened.\n");
    }
    else if(1 == g_mdParam || 2 == g_mdParam)
    {
        if(TRUE != g_MD_InitDone)
        {
            printf("MD function is initializing.\n");
        }
        else
        {
            MI_S8 param[8] = {0};
            param[0] = g_mdParam;
            MI_S32 tmp = 0;

            tmp = atoi(buf[0]);
            if((-1 != tmp) && (0x0 != tmp) && (0x1 != tmp))
            {
                MIXER_ERR("tmp=%d -1 use current value, 0 or 1\n",tmp);
                return;
            }
            if(-1 != tmp)
            {
                if(0 != tmp)
                {
                    param[2] = 1;
                }
                else
                {
                    param[2] = 0;
                }
            }
            else
            {
#if TARGET_CHIP_I5
                param[2] = 1;
#elif TARGET_CHIP_I6
                param[2] = 0xff;
#elif TARGET_CHIP_I6E
                param[2] = 0xff;
#elif TARGET_CHIP_I6B0
                param[2] = 0xff;
#endif
            }
            tmp = atoi(buf[1]);
#if TARGET_CHIP_I5
            if(tmp < -1  || tmp > 255)
            {
                MIXER_ERR("-1 use current value, [0, 255]\n");
                return;
            }
#elif TARGET_CHIP_I6
            if(tmp < -1  || tmp > 99)
            {
                MIXER_ERR("-1 use current value, [0, 99]\n");
                return;
            }
#elif TARGET_CHIP_I6E
            if(tmp < -1  || tmp > 99)
            {
                MIXER_ERR("-1 use current value, [0, 99]\n");
                return;
            }
#elif TARGET_CHIP_I6B0
            if(tmp < -1  || tmp > 99)
            {
                MIXER_ERR("-1 use current value, [0, 99]\n");
                return;
            }
#endif

            if(-1 != tmp)
            {
#if TARGET_CHIP_I5
                if(tmp >= 0 && tmp >= 255)
                {
                    param[3] = tmp - 128;
                }
#elif TARGET_CHIP_I6
                if(tmp >= 0 && tmp <= 99)
                {
                    param[3] = tmp;
                }
#elif TARGET_CHIP_I6E
                if(tmp >= 0 && tmp <= 99)
                {
                    param[3] = tmp;
                }
#elif TARGET_CHIP_I6B0
                if(tmp >= 0 && tmp <= 99)
                {
                    param[3] = tmp;
                }
#endif
            }
            else
            {
#if TARGET_CHIP_I5
                param[3] = 0;
#elif TARGET_CHIP_I6
                param[3] = 0xff;
#elif TARGET_CHIP_I6E
                param[3] = 0xff;
#elif TARGET_CHIP_I6B0
                param[3] = 0xff;
#endif
            }

            tmp = atoi(buf[2]);
#if TARGET_CHIP_I5
            if(tmp < -1  || tmp > 255)
            {
                MIXER_ERR("-1 use current value, [0, 255]\n");
                return;
            }
#elif TARGET_CHIP_I6
            if(tmp < -1  || tmp > 100)
            {
                MIXER_ERR("-1 use current value, [0, 100]\n");
                return;
            }
#elif TARGET_CHIP_I6E
            if(tmp < -1  || tmp > 100)
            {
                MIXER_ERR("-1 use current value, [0, 100]\n");
                return;
            }
#elif TARGET_CHIP_I6B0
            if(tmp < -1  || tmp > 100)
            {
                MIXER_ERR("-1 use current value, [0, 100]\n");
                return;
            }
#endif

            if(-1 != tmp)
            {
#if TARGET_CHIP_I5
                if(tmp >= 0 && tmp >= 255)
                {
                    param[4] = tmp - 128;
                }
#elif TARGET_CHIP_I6
                if(tmp >= 1 && tmp <= 100)
                {
                    param[4] = tmp;
                }
#elif TARGET_CHIP_I6E
                if(tmp >= 1 && tmp <= 100)
                {
                    param[4] = tmp;
                }
#elif TARGET_CHIP_I6B0
                if(tmp >= 1 && tmp <= 100)
                {
                    param[4] = tmp;
                }

#endif
            }
            else
            {
#if TARGET_CHIP_I5
                param[4] = 16;
#elif TARGET_CHIP_I6
                param[4] = 0xff;
#elif TARGET_CHIP_I6E
                param[4] = 0xff;
#elif TARGET_CHIP_I6B0
                param[4] = 0xff;
#endif
            }

            tmp = atoi(buf[3]);
            if(tmp < -1  || tmp > 255)
            {
                MIXER_ERR("-1 use current value, [0, 255]\n");
                return;
            }

            if(-1 != tmp)
            {
                tmp = tmp/10  * 10;
                if(tmp >= 10 && tmp <= 100 && (0 == tmp % 10))
                {
                    param[5] = tmp;
                }
            }
            else
            {
#if TARGET_CHIP_I5
                param[5] = 80;
#elif TARGET_CHIP_I6
                param[5] = 0xff;
#elif TARGET_CHIP_I6E
                param[5] = 0xff;
#elif TARGET_CHIP_I6B0
                param[5] = 0xff;
#endif
            }

            tmp = atoi(buf[4]);
            if(tmp < -1 )
            {
                MIXER_ERR("-1 use current value, [1000, 30000]\n");
                return;
            }
            else if(-1 != tmp && (tmp < 1000 || tmp > 30000))
            {
                MIXER_ERR("-1 use current value, [1000, 30000]\n");
                return;
            }

            if(-1 != tmp)
            {
                if(tmp >= 1000 && tmp <= 30000)
                {
                    tmp = tmp & 0x0000ffff;
                    //printf("%s:%d learn_rate = %d.\n", __func__, __LINE__, tmp);
#if TARGET_CHIP_I5
                    param[6] = ((tmp & 0xff00) >> 8) - 128;
                    param[7] = (tmp & 0xff) - 128;
#elif TARGET_CHIP_I6
                    param[6] = (((tmp & 0xff00) >> 8)&0xff);
                    param[7] = (tmp &  0x00ff);
                    if(128 < (((tmp & 0xff00) >> 8)&0xff))
                    {
                        param[6] = (((tmp & 0xff00) >> 8)&0xff) - 128;
                    }
                    if(128 < (tmp &  0x00ff))
                    {
                        param[7] = (tmp &  0x00ff)-128;
                    }
#elif TARGET_CHIP_I6E
                    param[6] = ((tmp & 0xff00) >> 8)&0xff;
                    param[7] = (tmp & 0x00ff);
#elif TARGET_CHIP_I6B0
                    param[6] = ((tmp & 0xff00) >> 8)&0xff;
                    param[7] = (tmp & 0x00ff);
#else
                    param[6] = ((tmp & 0xff00) >> 8)&0xff;
                    param[7] = (tmp & 0x00ff);
#endif
                    printf("%s:%d param[6] = %0x, param[7] = %0x.\n", __func__, __LINE__, param[6], param[7]);
                }

            }
            else
            {
#if TARGET_CHIP_I5
                param[6] = (2000 & 0x0000ff00) >> 8;
                param[7] = 2000 & 0xff;
#elif TARGET_CHIP_I6
                param[6] = 0xff;
                param[7] = 0xff;
#elif TARGET_CHIP_I6E
                param[6] = 0xff;
                param[7] = 0xff;
#elif TARGET_CHIP_I6B0
                param[6] = 0xff;
                param[7] = 0xff;
#endif
            }

            mixer_send_cmd(CMD_IE_MD_CHANGE, (MI_S8 *) param, sizeof(param));
        }
    }
}

void CConsoleManager::Console_MdParamHelp(void)
{
    printf("\n\t md <state><num_max><md_thr><sensitivity><learn_rate>\n");
    printf("\n\t state: 0/1, 0:disable,1:Enable\n");
    printf("\n\t num_max:(0,...) \n");
    printf("\n\t md_thr:(0,...)\n");
    printf("\n\t sensitivity:[10,20,30...100]\n");
    printf("\n\t learn_rate:[1000,30000]\n");
    printf("\n\t example: md 1 12 30 20 2000\n");
}

void CConsoleManager::Console_VideoFps(void)
{
    MI_S32 param[2];
    char *buf[2];

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);


    if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("param err\n");
        return;
    }

    param[0] = atoi(buf[0]);
    if(param[0] < 0 || param[0] >= (MI_S32)g_videoNumber)
    {
        MIXER_ERR("ch out of range. %d\n", g_videoNumber);
        return;
    }

    param[1] = mixerStrtok2Int(buf[1], "/");
    if(0x0 == param[1])
    {
        MIXER_ERR("param err, fps can not be zero\n");
        return;
    }

#if TARGET_CHIP_I5
    if(param[1] > MIXER_MAX_FPS)
    {
        param[1] = MIXER_DEFAULT_FPS;
    }
#endif

    mixer_send_cmd(CMD_VIDEO_SET_FRAMERATE, (MI_S8 *)param, sizeof(param));
}

void CConsoleManager::Console_VideoFpsHelp(void)
{
    printf("\n\t videofps <channel> <fps>\n");
    printf("\n\t channel: which video stream to change frame rate\n");
    printf("\n\t fps: fmt: xxx or x/y\n");
    printf("\n\t example videofps 0  20 \n");
}

void CConsoleManager::Console_VideoGop(void)
{
    MI_U32 vIndex = 0, vgop = 0;

    char *buf[2];

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);

    if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("param err\n");
        return;
    }

    vIndex = atoi(buf[0]);
    if(vIndex >= g_videoNumber)
    {
        MIXER_ERR("ch out of range. %d\n", g_videoNumber);
        return;
    }

    vgop = atoi(buf[1]);
    if(0x0 == vgop)
    {
        MIXER_ERR("param err, gop can not be zero\n");
        return;
    }

    if(vgop > 0)
    {
        MI_S32 param[2];
        param[0] = vIndex;
        param[1] = vgop;
        mixer_send_cmd(CMD_VIDEO_SET_GOP, (MI_S8 *)param, sizeof(param));
    }
}

void CConsoleManager::Console_VideoGopHelp(void)
{
    printf("\n\t    gop <channel> <gop>\n");
    printf("\n\t     channel[0,%d]: which video stream to change gop\n",g_videoNumber);
    printf("\n\t     gop: (0,)\n");
    printf("\n\t    example: gop 0 80\n");
}

void CConsoleManager::Console_VideoIframeInterval(void)
{
    MI_S32 param[3];

    char *buf[2];

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);

    if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("param err\n");
        return;
    }

    param[0] = atoi(buf[0]);
    if(param[0] < 0 || param[0] >= (MI_S32)g_videoNumber)
    {
        MIXER_ERR("ch out of range. %d\n", g_videoNumber);
        return;
    }

    param[1] = atoi(buf[1]);
    /*if(0x0 == param[1])
    {
    	// param[1] mean disable ltr.
        MIXER_ERR("param err, gop can not be zero\n");
        return;
    }*/

    param[2] = g_videoParam[param[0]].virtualIEnable;
    mixer_send_cmd(CMD_VIDEO_SET_VIRTUAL_IINTERVAL, (MI_S8 *)param, sizeof(param));
}

void CConsoleManager::Console_VideoIframeIntervalHelp(void)
{
    printf("\n\t    ltr <Channel> <virtualIInterval>\n");
    printf("\n\t     Channel[0,%d]: which video stream to change virtualIInterval\n",g_videoNumber-1);
    printf("\n\t     virtualIInterval: \n");
    printf("\n\t    example: ltr 0 20\n");
}

void CConsoleManager::Console_IspParam()
{
    MI_S8 *cbuf[PARAMLIST] = {NULL};
    MI_U8 i = 0x0;

    for(i=0x0; i<GetParamNum();  i++)
    {
        cbuf[i] = (MI_S8 *)GetParam(i+1);
    }
    setIspParam(cbuf, GetParamNum()-1);
}

void CConsoleManager::Console_IspParamHelp()
{
    printf("\n\t isp <function index> .......\n");
    printf("\n\t     function index: the test ISP function index value\n");
    printf("\n\t     index 1: <value> test ColorToGray. value[0/1]: 0 disbale, 1 enable\n");
    printf("\n\t           3: <value>.test mirror/flip. value[0~3]: 00, 01, 10, 11. mirror flip\n");
    printf("\n\t           6: <v1> <v2> <v3> <v4> <v5>.test AWB. \n");
    printf("                v1: 0-auto, 1-manual\n");
    printf("                v2: u16Bgain:[0,0x2000]\n");
    printf("                v3: u16Gbgain:[0,0x2000]\n");
    printf("                v4: u16Grgain:[0,0x2000]\n");
    printf("                v5: u16Rgain:[0,0x2000]\n");
    printf("\n\t           7: <v1>. test AeStrate.  [0,2]\n");
    printf("\n\t           8: <v1>. test IQ_CONTRAST.  [0,100]\n");
    printf("\n\t           9: <v1>. test BRIGHTNESS.  [0,100]\n");
    printf("\n\t          10: <v1> <v2>. test RGBGamma. \n");
    printf("                v1: [1,2]. 1 RGBGamma, 2 YUVGamma\n");
    printf("                v2: [1,3]. 1 Low contrast, 2 Normal, 3 High contrast\n");
    printf("\n\t          11: <v1>. test SATURATION. u8SatAllStr [0,127]\n");
    printf("\n\t          12: <v1>. test LIGHTNESS. LIGHTNESS [0,100]\n");
    printf("\n\t          14: <v1> <v2>. test SHARPNESS.\n");
    printf("                v1: [0,1023]. u16SharpnessUD\n");
    printf("                v2: [0,1023]. u16SharpnessD\n");
    printf("\n\t          15: <v1>. test AE_FLICKER.  [0, 2]\n");
    printf("\n\t          17: <v1>...<v9>. test CCM.  (u16CCM [0,8191] )x 9\n");
    printf("\n\t          18: <v1> <v2>. test FALSECOLOR.  \n");
    printf("                v1: [0,11]. u8StrengthMid\n");
    printf("                v2: [0,2047]. u16ChromaThrdOfStrengthMid\n");
    printf("\n\t          19: <v1>...<v9>. test crosstalk.  \n");
    printf("                when TARGET_CHIP_I5\n");
    printf("                v1: [0,1]. IQ_SetCrossTalk enV2\n");
    printf("                v2: [0,255]. u16ThresholdHigh\n");
    printf("                v3: [0,255]. u16ThresholdLow\n");
    printf("                v4: [0,255]. u16ThresholdV2\n");
    printf("                v5: [0,7]. u8StrengthHigh\n");
    printf("                v6: [0,7]. u8StrengthLow\n");
    printf("                v7: [0,31]. u8StrengthV2\n");
    printf("                when TARGET_CHIP_I6\n");
    printf("                v1: [0,4095]. u16ThresholdOffsetV2\n");
    printf("                v2: [0,255]. u16ThresholdV2\n");
    printf("                v3: [0,31]. u8StrengthV2\n");
    printf("                when TARGET_CHIP_I6E\n");
    printf("                v1: [0,4095]. u16ThresholdOffsetV2\n");
    printf("                v2: [0,255]. u16ThresholdV2\n");
    printf("                v3: [0,31]. u8StrengthV2\n");
    printf("                when TARGET_CHIP_I6B0\n");
    printf("                v1: [0,4095]. u16ThresholdOffsetV2\n");
    printf("                v2: [0,255]. u16ThresholdV2\n");
    printf("                v3: [0,31]. u8StrengthV2\n");

    printf("\n\t          20: <v1>...<v4>. test DP.  \n");
    printf("                v1: [0,1]. bHotPixEn\n");
    printf("                v2: [0,255]. u16HotPixCompSlpoe\n");
    printf("                v3: [0,1]. bDarkPixEn\n");
    printf("                v4: [0,255]. u16DarkPixCompSlpoe\n");

    printf("\n\t          21: <v1>...<v4>. test blackLevel.  \n");
    printf("                v1: [0,255]. u16ValB\n");
    printf("                v2: [0,255]. u16ValGb\n");
    printf("                v3: [0,255]. u16ValGr\n");
    printf("                v4: [0,255]. u16ValR\n");

    printf("\n\t          22: <v1>...<v2>. test blackLevel.  \n");
    printf("                v1: [0,1]. bEnable\n");
    printf("                v2: [0,100]. IQ_SetDefog u8Strength\n");

    printf("\n\t          23: <v1>...<v2>. test ae.  \n");
    printf("                v1: [1,5]. 1 weight table, 2 exposure limit, 3 target luma,5 default\n");
    printf("                when v1 == 1. v2: [1,3]. 1 average table, 2 center table, 3 spot tale\n");
    printf("                when v1 == 3. v2: [1,x]. target_luma\n");
    printf("                else case. v2: mean nothing\n");

    printf("\n\t          25: test AE_EXPO_INFO.  \n");

    printf("\n\t          27: <v1>...<v3>. test 3DNR.  \n");
    printf("                v1: [0,64]. IQ_SetNR3D u8TfStr [0,64]\n");
    printf("                v2: [0,64]. IQ_SetNR3D u8TfStrEx [0,64]\n");
    printf("                v3: [0,64]. IQ_SetNR3D u8TfLut [0,63]\n");

    printf("\n\t          28: <v1>...<v4>. test AWB_attr.  \n");
    printf("                when TARGET_CHIP_I5 \n");
    printf("                v1: [0,16]. AWB_SetAttrEx AreaScale \n");
    printf("                v2: [256, 4095]. AWB_SetAttrEx u16WhiteBgain \n");
    printf("                v3: [256, 4095]. AWB_SetAttrEx u16WhiteRgain \n");
    printf("                v4: [1,32]. AWB_SetAttrEx u8AreaSize \n");
    printf("                when TARGET_CHIP_I6 || TARGET_CHIP_I6E || TARGET_CHIP_I6B0\n");
    printf("                v1: [256, 4095]. AWB_SetAttrEx u16WhiteBgain \n");
    printf("                v2: [256, 4095]. AWB_SetAttrEx u16WhiteRgain \n");
    printf("                v3: [1,32]. AWB_SetAttrEx u8AreaSize \n");

    printf("\n\t          30: test AWB_QUERY_INFO.  \n");

    printf("\n\t          32: test AFInfo.  \n");

    printf("\n\t          33: <v1>...<v4>. test IQ_NRDESPIKE.  \n");
    printf("                v1: [0, 255]. IQ_SetNRDeSpike u16DiffThdCornerCross \n");
    printf("                v2: [0, 15]. IQ_SetNRDeSpike u8BlendRatio \n");
    printf("                v3: [256, 4095]. AWB_SetAttrEx u16WhiteRgain \n");
    printf("                v4: [1,32]. AWB_SetAttrEx u8AreaSize \n");

#if TARGET_CHIP_I5
    printf("\n\t          34: <v1>...<v3>. test IQ_NRLUMA.  \n");
    printf("                v1: [0, 1]. IQ_SetNRLuma bEnLscReference \n");
    printf("                v2: [0, 5]. IQ_SetNRLuma u8FilterLevel \n");
    printf("                v3: [0, 63]. IQ_SetNRLuma u8BlendRatio \n");
#elif TARGET_CHIP_I6
    printf("\n\t          34: <v1>. test IQ_NRLUMA.  \n");
    printf("                v1: [1, 3]. 1 Low strength, 2 Normal, 3 High strength \n");
#endif

    printf("\n\t          35: <v1>..<v2>. test IQ_HSV.  \n");
    printf("                v1: [-64, 64]. IQ_SetHSV s16HueLut \n");
    printf("                v1: [0, 255].  IQ_SetHSV u16SatLut \n");

    printf("\n\t          36: <v1>..<v4>. test RGBIR.  \n");
    printf("                v1: [0, 1]. IQ_SetRGBIR bRemovelEn \n");
    printf("                v2: [0, 65535]. IQ_SetRGBIR u16Ratio_R \n");
    printf("                v3: [0, 65535]. IQ_SetRGBIR u16Ratio_G \n");
    printf("                v4: [0, 65535]. IQ_SetRGBIR u16Ratio_B \n");

    printf("\n\t          37: <v1>..<v3>. test WDR.  \n");
    printf("                v1: [0, 255]. IQ_SetWDR u8BrightLimit  \n");
    printf("                v2: [0, 255]. IQ_SetWDR u8DarkLimit \n");
    printf("                v3: [0, 255]. IQ_SetWDR u8Strength \n");

    printf("\n\t          39: <v1>..<v2>. test load iq binfile.  \n");
    printf("                v1:  bin file path  \n");
    printf("                v2:  bin user_key \n");

    printf("\n\t          42: <v1>..<v3>. test load CaliData.  \n");
    printf("                v1:  channel  \n");
    printf("                v2: [0, 3].  0 : AWB, 1 : OBC, 2 : SDC, 3 : ALSC, 4 : LSC \n");
    printf("                v3:  calidata filepath  \n");

    printf("\n\t          43: <v1>..<v2>. test load iq binfile.  \n");
    printf("                v1: [0, 1]. board type(0: 326D, 1:328Q)  \n");
    printf("                v2: [0, 1]. ircut mode\n");

    printf("\n\t          44: <v1>..<v4>. test cus3a.  \n");
    printf("                v1: [0, 2]. ( 0: closeCUS3A_misc, 1: openCUS3A_misc, 2: testCUS3A_MI(only I6)  \n");
    printf("                when v1 ==1.  v2: [0, 1]. (0: platformAE, 1: cusAE)\n");
    printf("                             v3: [0, 1]. (0: platformAWB, 1: cusAWB)\n");
    printf("                             v4: [0, 1]. (0: platformAF, 1: cusAF)\n");
    printf("                else case , v2~4 means nothing\n");

#if TARGET_CHIP_I6
    printf("\n\t          45: <v1>. test AE state.  \n");
    printf("                v1:[0,1].  0: open, 1: close \n");
#endif
    printf("\n\t       example:isp 3 01\n");
}

void CConsoleManager::Console_VideoRequestIDR(void)
{
    MI_S32 param[1];
    char *buf[1];

    buf[0] = GetParam(1);

    if(NULL == buf[0])
    {
        MIXER_ERR("param err buf is NULL\n");
        return;
    }

    param[0] = atoi(buf[0]);
    if(param[0] < 0 || param[0] >= (MI_S32)g_videoNumber)
    {
        MIXER_ERR("channel out of range. [0,%d]\n", g_videoNumber-1);
        return;
    }

    mixer_send_cmd(CMD_VIDEO_REQUEST_IDR, (MI_S8 *)param, sizeof(param));
}

void CConsoleManager::Console_VideoRequestIDRHelp(void)
{
    printf("\n\t idr <Channel> \n");
    printf("\n\t Channel[0,%d]: which venc channel to request idr \n",g_videoNumber);
    printf("\n\t example: idr 0\n");
}

void CConsoleManager::Console_AudioOnOff(void)
{
    MI_S32 open = 0;
    MI_S32 option = 0;
    char *buf[2];

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);

    if(NULL == buf[0])
    {
        MIXER_ERR("param err\n");
        return;
    }
    if(NULL == buf[1])
    {
        MIXER_ERR("param err\n");
        return;
    }

    option = atoi(buf[0]);
    open = atoi(buf[1]);
    if(0 > option && option < 2)
    {
        MIXER_ERR("audio option param out of range. %d\n", option);
        return;
    }
    if((0 != open)  && (1 != open))
    {
        MIXER_ERR("on/off param out of range. %d\n", open);
        return;
    }

    if(open == g_audioInParam[0].bFlag && 0 == option)
    {
        MIXER_ERR("audioIn already open or close, don`t do this again. current bFlag = %d\n",g_audioInParam[0].bFlag);
        return;
    }
    if(open == g_audioOutParam[0].bFlag && 1 == option)
    {
        MIXER_ERR("audioOut already open or close, don`t do this again. current bFlag = %d\n",g_audioInParam[0].bFlag);
        return;
    }
    if(open == g_audioOutParam[0].bFlag && open == g_audioInParam[0].bFlag && 2 == option)
    {
        MIXER_ERR("audioOut already open or close, don`t do this again. current bFlag = %d\n",g_audioInParam[0].bFlag);
        return;
    }

    mixer_send_cmd(CMD_SYSTEM_LIVE555_CLOSE, NULL, 0);
    if(0 == option)
    {
        if(1 == open)
        {
            if(0 == g_audioInParam[0].u8AudioInNum)
            {
                g_audioInParam[0].stAudioInChnPort.u32ChnId = 0;
                g_audioInParam[0].u8AudioInNum = 1;
                g_audioInParam[0].stAudioAttr.u32ChnCnt = g_audioInParam[0].u8AudioInNum;
            }
            mixer_send_cmd(CMD_AUDIO_OPEN, (MI_S8 *)&g_audioInParam, sizeof(MixerAudioInParam) * g_audioInParam[0].u8AudioInNum);
            for(int i = 0; i < g_audioInParam[0].u8AudioInNum; i++)
            {
                g_audioInParam[i].bFlag = TRUE;
            }
        }
        else
        {
            mixer_send_cmd(CMD_AUDIO_CLOSE, NULL, 0);
            for(int i = 0; i < g_audioInParam[0].u8AudioInNum; i++)
            {
                g_audioInParam[i].bFlag = FALSE;
            }
        }
    }
    else if(1 == option)
    {
        if(1 == open)
        {
            if(0 == g_audioOutParam[0].s32AudioOutNum)
            {
                g_audioOutParam[0].s32AudioOutNum = 1;
                g_audioOutParam[0].stAudioOutChnPort.u32DevId = 0;
                g_audioOutParam[0].stAudioOutChnPort.u32ChnId = 0;
            }
            mixer_send_cmd(CMD_AUDIO_PLAY_MEDIA, (MI_S8 *)&g_audioOutParam, sizeof(MixerAudioOutParam) * g_audioOutParam[0].s32AudioOutNum);
            g_audioOutParam[0].bFlag = TRUE;
        }
        else
        {
            mixer_send_cmd(CMD_AUDIO_STOPPLAY, NULL, 0);
            g_audioOutParam[0].bFlag = FALSE;
        }
    }
    else
    {
        if(1 == open)
        {
            if(TRUE == g_audioInParam[0].bFlag)
            {
                MIXER_ERR("audioIn already open, do not open it again. bFlag = %d\n",g_audioInParam[0].bFlag);
            }
            else
            {
                if(0 == g_audioInParam[0].u8AudioInNum)
                {
                    g_audioInParam[0].stAudioInChnPort.u32ChnId = 0;
                    g_audioInParam[0].u8AudioInNum = 1;
                    g_audioInParam[0].stAudioAttr.u32ChnCnt = g_audioInParam[0].u8AudioInNum;
                }
                mixer_send_cmd(CMD_AUDIO_OPEN, (MI_S8 *)&g_audioInParam, sizeof(MixerAudioInParam) * g_audioInParam[0].u8AudioInNum);
                for(int i = 0; i < g_audioInParam[0].u8AudioInNum; i++)
                {
                    g_audioInParam[i].bFlag = TRUE;
                }
            }

            if(TRUE == g_audioOutParam[0].bFlag)
            {
                MIXER_ERR("audioOut already open, do not open it again. bFlag = %d\n",g_audioOutParam[0].bFlag);
            }
            else
            {
                if(0 == g_audioOutParam[0].s32AudioOutNum)
                {
                    g_audioOutParam[0].s32AudioOutNum = 1;
                    g_audioOutParam[0].stAudioOutChnPort.u32DevId = 0;
                    g_audioOutParam[0].stAudioOutChnPort.u32ChnId = 0;
                }
                mixer_send_cmd(CMD_AUDIO_PLAY_MEDIA, (MI_S8 *)&g_audioOutParam, sizeof(MixerAudioOutParam) * g_audioOutParam[0].s32AudioOutNum);
                g_audioOutParam[0].bFlag = TRUE;
            }
        }
        else
        {
            if(FALSE == g_audioInParam[0].bFlag)
            {
                MIXER_ERR("audioIn already close, do not close it again. bFlag = %d\n",g_audioInParam[0].bFlag);
            }
            else
            {
                mixer_send_cmd(CMD_AUDIO_CLOSE, NULL, 0);
                for(int i = 0; i < g_audioInParam[0].u8AudioInNum; i++)
                {
                    g_audioInParam[i].bFlag = FALSE;
                }
            }

            if(FALSE == g_audioOutParam[0].bFlag)
            {
                MIXER_ERR("audioOut already close, do not close it again. bFlag = %d\n",g_audioOutParam[0].bFlag);
            }
            else
            {
                g_audioOutParam[0].bFlag = FALSE;
                mixer_send_cmd(CMD_AUDIO_STOPPLAY, NULL, 0);
            }
        }
    }
    mixer_send_cmd(CMD_SYSTEM_LIVE555_OPEN, NULL, 0);
}

void CConsoleManager::Console_AudioOnOffHelp(void)
{
    printf("\n\t audio <Ai/Ao/All> <On/Off>.......\n");
    printf("\n\t Ai/Ao/All: 0:Ai,1:AO,2:All\n");
    printf("     On/Off: 0:off,1:open\n");
    printf("\n\t example: audio 2 0\n");
}

void CConsoleManager::Console_VideoCodec(void)
{
    MI_U32 vIndex = 0;
    MI_U32 codecType = 0;
    MI_S32 param[3];
    MI_U32 videoparam[9];
    Size_t size;
    MI_S32 qfactor = 0x0;
    MI_U32 vgop = 0, bindtype = 0x0;
    MI_U32 ltrType = 0;
    MI_U32 vInterval = 0;
    MI_S32 frameparam = 0;
    MI_U32 VideoProfile = 0x0;
    char *buf[10];
    MI_U8 i = 0x0;
    char bChangeCodec = 0;

    for(i=0x0; i<sizeof(buf)/sizeof(buf[0]); i++)
    {
        buf[i] = GetParam(i+1);
    }

    if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("Param err\n");
        return;
    }

    vIndex = atoi(buf[0]);
    if(vIndex >= g_videoNumber)
    {
        MIXER_ERR("ch out of range. %d\n", vIndex);
        return;
    }
    codecType = atoi(buf[1]);
    if(2 < codecType)
    {
        MIXER_ERR("codec out of range. %d\n", codecType);
        return;
    }

    if(NULL == buf[2])
    {
        MIXER_ERR("Param err\n");
        return;
    }

    VideoProfile = atoi(buf[2]);

    if(VE_AVC == codecType)
    {
#if TARGET_CHIP_I6 || TARGET_CHIP_I5
        if(VideoProfile != 0 && VideoProfile != 1)
        {
            printf("The input codec H264 Profile is error,only support 0 and 1\n");
            return;
        }
#else
        if(VideoProfile != 0 && VideoProfile != 1 && VideoProfile != 2)
        {
            printf("The input codec H264 Profile is error,only support 0/1/2\n");
            return;
        }
#endif
    }
    else if(VE_H265 == codecType)
    {
        VideoProfile = 1;
    }
    else
    {
        VideoProfile = 0;
    }

    frameparam = mixerStr2Int(buf[3]);
    if(frameparam < 0 || frameparam > MIXER_MAX_FPS)
    {
        frameparam = MIXER_DEFAULT_FPS;
    }

    if(NULL == buf[4] || NULL == buf[5])
    {
        MIXER_ERR("Param err\n");
        return;
    }
    size.width = atoi(buf[4]);
    size.height = atoi(buf[5]);

#if LOAD_MIXER_CFG
    ModuleVencInfo_s *pVencInfo_t = GetVencInfo();

    if(size.width > pVencInfo_t->StreamWidth[vIndex] || size.height > pVencInfo_t->StreamHeight[vIndex])
    {
        printf("please check you input param!width vIndex=%d must[<= %d] height must[<=%d]\n",\
               vIndex,\
               pVencInfo_t->StreamWidth[vIndex],\
               pVencInfo_t->StreamHeight[vIndex]);
    }
#endif

    if(FALSE == mixerResolutionValid(vIndex,size.width, size.height))
    {
        printf("resolution error\n");
        return;
    }

    if(NULL == buf[6] || NULL == buf[7])
    {
        MIXER_ERR("Param err\n");
        return;
    }

    if(0 == codecType || 1 == codecType)
    {
        vgop = atoi(buf[6]);
        ltrType = atoi(buf[7]);

        if(ltrType == 1 || ltrType == 2)
        {
            if(NULL == buf[8])
            {
                MIXER_ERR("Param err\n");
                return;
            }
            vInterval = atoi(buf[8]);
            if(vInterval < 0 || vInterval >= vgop)
            {
                MIXER_ERR("LTR Interval (0 ~ gop). %d\n", vInterval);
                return;
            }
            if(NULL == buf[9])
            {
                MIXER_ERR("Param err\n");
                return;
            }
            bindtype = atoi(buf[9]);
        }
        else
        {
            if(ltrType < 0 || ltrType > 2)
            {
                MIXER_WARN("err, LTR type must be 0~2, now ltrType = 0\n");
                ltrType = 0;
            }
            if(NULL == buf[8])
            {
                MIXER_ERR("Param err\n");
                return;
            }
            bindtype = atoi(buf[8]);
        }
#if TARGET_CHIP_I6B0
        if(0x0 != bindtype && 2 != bindtype && 3 != bindtype)
        {
            MIXER_WARN("err, H264/5 bind type must be 0/2/3,now bindtype = 0\n");
            bindtype = 0x0;
        }
#else
        if(0x0 != bindtype && 2 != bindtype)
        {
            MIXER_WARN("err, H264/5 bind type must be 0/2,now bindtype = 0\n");
            bindtype = 0x0;
        }
#endif
    }
    else if(2 == codecType)
    {
        qfactor = atoi(buf[6]);
        if(qfactor < 20)
            qfactor = 20;
        else if(qfactor > 90)
            qfactor = 90;

        bindtype = atoi(buf[7]);
        if(0x0 != bindtype && 1 != bindtype)
        {
            MIXER_WARN("err, Mjpeg/JPE bind type must be 0/1\n");
            bindtype = 0x0;
        }
    }

    if(codecType == g_videoEncoderArray[vIndex]->m_encoderType)
    {
        MIXER_WARN("%s: The current codetype:%d is same as the old one, live555 does not need to switch encoding type \n", __FUNCTION__, codecType);
    }
    else
    {
       mixer_send_cmd(CMD_SYSTEM_LIVE555_STOP_SUBMEDIASESSION, (MI_S8 *)&vIndex, sizeof(MI_U32));
       bChangeCodec = 1;
       usleep(100000);
    }

    if(g_displayOsd || gDebug_osdColorInverse)
        //&& ((g_videoParam[vIndex].width != size.width) || (g_videoParam[vIndex].height != size.height))
    {
        mixerEnableIEModule(0);

        param[0] = vIndex;
        param[1] = g_videoParam[vIndex].width;
        param[2] = g_videoParam[vIndex].height;
        mixer_send_cmd(CMD_OSD_RESET_RESOLUTION, (MI_S8 *) param, sizeof(param));
    }


    //send cmd "CMD_VIDEO_CHANGE_LTR_PARAM" must before send cmd "CMD_VIDEO_CHANGE_CODEC"
    param[0] = vIndex;
    param[1] = ltrType;
    param[2] = vInterval;
    mixer_send_cmd(CMD_VIDEO_CHANGE_LTR_PARAM, (MI_S8 *) param, sizeof(param));

    videoparam[0] = vIndex;
    videoparam[1] = codecType;
    videoparam[2] = qfactor;
    videoparam[3] = size.width;
    videoparam[4] = size.height;
    videoparam[5] = frameparam;
    videoparam[6] = vgop;
    videoparam[7] = VideoProfile;
    videoparam[8] = bindtype;
    mixer_send_cmd(CMD_VIDEO_CHANGE_CODEC, (MI_S8 *) videoparam, sizeof(videoparam));

    switch(codecType)
    {
        case 0:
            if(VE_AVC != g_videoParam[vIndex].encoderType)
            {
                g_videoParam[vIndex].encoderType = VE_AVC;
            }
            break;

        case 1:
            if(VE_H265 != g_videoParam[vIndex].encoderType)
            {
                g_videoParam[vIndex].encoderType = VE_H265;
            }
            break;

        case 2:
            if(VE_JPG != g_videoParam[vIndex].encoderType)
            {
                g_videoParam[vIndex].encoderType = VE_JPG;
            }
            break;
        default:
            break;
    }

    if(g_displayOsd || gDebug_osdColorInverse)
        //&& ((g_videoParam[vIndex].width != size.width) || (g_videoParam[vIndex].height != size.height))
    {
        param[0] = vIndex;
        param[1] = size.width;
        param[2] = size.height;
        mixer_send_cmd(CMD_OSD_RESTART_RESOLUTION, (MI_S8 *) param, sizeof(param));
        mixerEnableIEModule(1);
    }

    if(bChangeCodec)
        mixer_send_cmd(CMD_SYSTEM_LIVE555_SET_ENCODETYPE, (MI_S8 *)&vIndex, sizeof(MI_U32));

}

void CConsoleManager::Console_VideoCodecHelp(void)
{
    printf("\n\t videocodec <v1> .... <v10>\n");
    printf("            v1: which venc channel to change codec \n");
    printf("            v2: which venc codec you want to change \n");
    printf("            v3: venc codec's profile, it only work@H264 but you must input a value!\n");
    printf("            v4: venc fps\n");
    printf("            v5: venc width, it needed 32 align\n");
    printf("            v6: venc height, it needed 32 align\n");
    printf("   when codec type is H264/H265:\n");
    printf("            v7: venc gop\n");
    printf("            v8: venc LtrType\n");
    printf("            when LtrType == true {\n");
    printf("            v9: venc vInterval, it must small than gop}\n");
    printf("            v10: venc BindType\n");
    printf("   when codec type is mjpeg:   v9/10  means nothing\n");
    printf("            v7: venc qfactor\n");
    printf("            v8: venc BindType\n");
    printf("   example: videocodec 0 1 1 30 1920 1080 30 1 20 0 \n");
}
void CConsoleManager::Console_VideoRotate(void)
{
    MI_S32 rotation[2]= {-1, -1};
//  MI_S32 ret = -1;
    MI_U16 fifo_data[256];
    char *buf;
#if TARGET_CHIP_I5
    if(0 == (g_rotation & 0xFF000000))
    {
        printf("  VPE not enable TOP&BUTTOM mode which support rotation function!\n");
        return;
    }
#endif

    buf  = GetParam(1);
    if(NULL == buf)
    {
        MIXER_ERR("%s: param err\n", __func__);
        return;
    }

    rotation[0] = atoi(buf);
    if(0 != rotation[0] && \
       90 != rotation[0] && \
       180 != rotation[0] && \
       270 != rotation[0])
    {
        MIXER_ERR("%s: rotation %d not support\n", __func__, rotation[0]);
        return;
    }
    if((0 == rotation[0]) || (90 == rotation[0]) || (180 == rotation[0]) || (270 == rotation[0]))
    {
        g_bInitRotate = 0xFF000000 + rotation[0];
    }
    //if(((rotation[0] / 90) % 2) != (((g_rotation & 0xFFFF) / 90) % 2))
    {
        MixerDisableOSD(CMD_OSD_RESET_RESOLUTION, -1, 0, 0, 1);
        MySystemDelay(10);
    }

    mixer_send_cmd(CMD_VIDEO_SET_ROTATE, (MI_S8 *) rotation, sizeof(rotation));

    if(0x0 == mixer_wait_cmdresult(CMD_VIDEO_SET_ROTATE, fifo_data, sizeof(fifo_data)))
    {
        if(((rotation[0] / 90) % 2) != (MI_S32)(((g_rotation & 0xFFFF) / 90) % 2))
        {
            //swap width and height
            MI_S32 s32Tmp = g_ieWidth;
            g_ieWidth  = g_ieHeight;
            g_ieHeight = s32Tmp;

            g_rotation = (g_rotation & 0xFF000000) + rotation[0];
            MIXER_DBG("g_rot is %d\n", g_rotation);
        }

        if(fifo_data[0] == CMD_OSD_RESTART_RESOLUTION)
        {
            for(MI_S32 veChn = 0; veChn < fifo_data[1]; veChn++)
            {
                printf("%s:%d cmdId=0x%x, veChn=%d, width=%d, height=%d, IeEnable=%d\n", __func__, __LINE__, fifo_data[0],
                       fifo_data[2+veChn*4], fifo_data[3+veChn*4], fifo_data[4+veChn*4], fifo_data[5+veChn*4]);
                MixerEnableOSD((MixerCmdId)fifo_data[0], \
                               fifo_data[2+veChn*4], \
                               fifo_data[3+veChn*4], \
                               fifo_data[4+veChn*4], \
                               fifo_data[5+veChn*4]);
            }
        }
        else
        {
            MIXER_ERR("%s:%d get wrong result data cmd(%d)\n", __func__, __LINE__, fifo_data[0]);
            return ;
        }
        if(g_fdParam || g_mdParam || g_odParam || g_ieLogParam || g_vgParam ||g_hchdParam || g_dlaParam)
        {
            if(g_fdParam)
                mixer_send_cmd(CMD_IE_FD_CLOSE, NULL, 0);

            if(g_mdParam)
                mixer_send_cmd(CMD_IE_MD_CLOSE, NULL, 0);

            if(g_odParam)
                mixer_send_cmd(CMD_IE_OD_CLOSE, NULL, 0);

            if(g_vgParam)
                mixer_send_cmd(CMD_IE_VG_CLOSE, NULL, 0);

            if(g_hchdParam)
                mixer_send_cmd(CMD_IE_HCHD_CLOSE, NULL, 0);

            if(g_dlaParam)
                mixer_send_cmd(CMD_IE_DLA_CLOSE, NULL, 0);

            mixer_send_cmd(CMD_IE_CLOSE, NULL, 0);

            MySystemDelay(100);

            MI_S32 param[3];
            param[0] = g_ieVpePort;
            param[1] = g_ieWidth;
            param[2] = g_ieHeight;

            if(((g_rotation & 0xFFFF) == 90) || ((g_rotation & 0xFFFF) == 270))
            {
                param[1] = g_ieHeight;
                param[2] = g_ieWidth;
            }

            printf("CMD_IE_OPEN\n");
            mixer_send_cmd(CMD_IE_OPEN, (MI_S8 *)param, sizeof(param));

            if(g_fdParam)
            {
                g_modeParam |= (1 << 2);
                mixer_send_cmd(CMD_IE_FD_OPEN, (MI_S8 *)&g_fdParam, sizeof(g_fdParam));
            }
            if(g_mdParam)
            {
                g_modeParam |= (1 << 0);
                mixer_send_cmd(CMD_IE_MD_OPEN, (MI_S8 *)&g_mdParam, sizeof(g_mdParam));
            }
            if(g_odParam)
            {
                g_modeParam |= (1 << 1);
                mixer_send_cmd(CMD_IE_OD_OPEN, (MI_S8 *)&g_odParam, sizeof(g_odParam));
            }
            if(g_vgParam)
            {
                g_modeParam |= (1 << 5);
                mixer_send_cmd(CMD_IE_VG_OPEN, (MI_S8 *)&g_vgParam, sizeof(g_vgParam));
            }
            if(g_hchdParam)
            {
                if(g_hchdParam == 1)
                    g_modeParam |= (1 << 3);
                else
                    g_modeParam |= (1 << 4);
                mixer_send_cmd(CMD_IE_HCHD_OPEN, (MI_S8 *)&g_hchdParam, sizeof(g_hchdParam));
            }
            if(g_dlaParam)
            {
                g_modeParam |= (1 << 6);
                mixer_send_cmd(CMD_IE_DLA_OPEN, (MI_S8 *)&g_dlaParam, sizeof(g_dlaParam));
            }
        }

    }
    else
    {
        MIXER_ERR("wait CMD_VIDEO_SET_ROTATE err.\n");
        return;
    }
}

void CConsoleManager::Console_VideoRotateHelp(void)
{
    printf("\n\t rot <v1>\n");
    printf("            v1: rotation(0, 90, 180, 270) \n");
    printf("    example: rot 90\n");
}

#if MIXER_SUPPORT_DIVP_BIND_CHANGE
void CConsoleManager::Console_DivpBindChange(void)
{
    MI_S32 bindTypeCmd[2]= {-1, -1}, i = 0;
//  MI_S32 ret = -1;
    MI_U16 fifo_data[256];
    char *buf[10];

    for(i=0x0; i< 2; i++)
    {
        buf[i] = GetParam(i+1);
    }

     if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("%s: param err\n", __func__);
        return;
    }

    bindTypeCmd[0] = atoi(buf[0]);
    bindTypeCmd[1] = atoi(buf[1]);

    if(0 != bindTypeCmd[0]  &&
       1 != bindTypeCmd[0])
    {
        MIXER_ERR("%s: divp bind type %d not support\n", __func__, bindTypeCmd[0]);
        return;
    }

     if(g_fdParam || g_mdParam || g_odParam || g_ieLogParam || g_vgParam ||g_hchdParam || g_dlaParam)
     {
         MIXER_ERR("%s: not support when has ie\n", __func__);
         return;
     }

    //if(((rotation[0] / 90) % 2) != (((g_rotation & 0xFFFF) / 90) % 2))
    {
        MixerDisableOSD(CMD_OSD_RESET_RESOLUTION, -1, 0, 0, 1);
        MySystemDelay(10);
    }

    mixer_send_cmd(CMD_VIDEO_SET_DIVP_BIND_TYPE, (MI_S8 *) bindTypeCmd, sizeof(bindTypeCmd));

    if(0x0 == mixer_wait_cmdresult(CMD_VIDEO_SET_DIVP_BIND_TYPE, fifo_data, sizeof(fifo_data)))
    {
        if(fifo_data[0] == CMD_OSD_RESTART_RESOLUTION)
        {
            for(MI_S32 veChn = 0; veChn < fifo_data[1]; veChn++)
            {
                printf("%s:%d cmdId=0x%x, veChn=%d, width=%d, height=%d, IeEnable=%d\n", __func__, __LINE__, fifo_data[0],
                       fifo_data[2+veChn*4], fifo_data[3+veChn*4], fifo_data[4+veChn*4], fifo_data[5+veChn*4]);
                MixerEnableOSD((MixerCmdId)fifo_data[0], \
                               fifo_data[2+veChn*4], \
                               fifo_data[3+veChn*4], \
                               fifo_data[4+veChn*4], \
                               fifo_data[5+veChn*4]);
            }
        }
        else
        {
            MIXER_ERR("%s:%d get wrong result data cmd(%d)\n", __func__, __LINE__, fifo_data[0]);
            return ;
        }
        if(g_fdParam || g_mdParam || g_odParam || g_ieLogParam || g_vgParam ||g_hchdParam || g_dlaParam)
        {
            if(g_fdParam)
                mixer_send_cmd(CMD_IE_FD_CLOSE, NULL, 0);

            if(g_mdParam)
                mixer_send_cmd(CMD_IE_MD_CLOSE, NULL, 0);

            if(g_odParam)
                mixer_send_cmd(CMD_IE_OD_CLOSE, NULL, 0);

            if(g_vgParam)
                mixer_send_cmd(CMD_IE_VG_CLOSE, NULL, 0);

            if(g_hchdParam)
                mixer_send_cmd(CMD_IE_HCHD_CLOSE, NULL, 0);

            if(g_dlaParam)
                mixer_send_cmd(CMD_IE_DLA_CLOSE, NULL, 0);

            mixer_send_cmd(CMD_IE_CLOSE, NULL, 0);

            MySystemDelay(100);

            MI_S32 param[3];
            param[0] = g_ieVpePort;
            param[1] = g_ieWidth;
            param[2] = g_ieHeight;

            if(((g_rotation & 0xFFFF) == 90) || ((g_rotation & 0xFFFF) == 270))
            {
                param[1] = g_ieHeight;
                param[2] = g_ieWidth;
            }

            printf("CMD_IE_OPEN\n");
            mixer_send_cmd(CMD_IE_OPEN, (MI_S8 *)param, sizeof(param));

            if(g_fdParam)
            {
                g_modeParam |= (1 << 2);
                mixer_send_cmd(CMD_IE_FD_OPEN, (MI_S8 *)&g_fdParam, sizeof(g_fdParam));
            }
            if(g_mdParam)
            {
                g_modeParam |= (1 << 0);
                mixer_send_cmd(CMD_IE_MD_OPEN, (MI_S8 *)&g_mdParam, sizeof(g_mdParam));
            }
            if(g_odParam)
            {
                g_modeParam |= (1 << 1);
                mixer_send_cmd(CMD_IE_OD_OPEN, (MI_S8 *)&g_odParam, sizeof(g_odParam));
            }
            if(g_vgParam)
            {
                g_modeParam |= (1 << 5);
                mixer_send_cmd(CMD_IE_VG_OPEN, (MI_S8 *)&g_vgParam, sizeof(g_vgParam));
            }
            if(g_hchdParam)
            {
                if(g_hchdParam == 1)
                    g_modeParam |= (1 << 3);
                else
                    g_modeParam |= (1 << 4);
                mixer_send_cmd(CMD_IE_HCHD_OPEN, (MI_S8 *)&g_hchdParam, sizeof(g_hchdParam));
            }
            if(g_dlaParam)
            {
                g_modeParam |= (1 << 6);
                mixer_send_cmd(CMD_IE_DLA_OPEN, (MI_S8 *)&g_dlaParam, sizeof(g_dlaParam));
            }
        }

    }
    else
    {
        MIXER_ERR("wait CMD_VIDEO_SET_DIVP_BIND_TYPE err.\n");
        return;
    }
}

void CConsoleManager::Console_DivpBindChangeHelp(void)
{
    printf("\n\t divpBind <v1> <v2>\n");
    printf("            v1:  [0] divp frame mode, [1] divp realtime mode \n");
    printf("            v2:  [0] all mode deinit, [1] only divp deinit \n");
    printf("    example: divpBind 0 0\n");
}
#endif

void CConsoleManager::Console_VideoMaskOSD(void)
{
    MI_S32 s32MaskNum;
    MI_S32 s32VideoIdx;

    char *buf[2];

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);

    if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("param err\n");
        return;
    }

    s32VideoIdx = atoi(buf[0]);
    if(s32VideoIdx < 0 || s32VideoIdx >= (MI_S32)g_videoNumber)
    {
        MIXER_ERR("ch out of range. %d\n", g_videoNumber);
        return;
    }

    s32MaskNum = atoi(buf[1]);
    if(0x0 > s32MaskNum  || s32MaskNum > 4)
    {
        MIXER_ERR("OSD MASK Number on Video Chn[0, 4]. %d\n", s32MaskNum);
        return;
    }

    MasktoRect(s32VideoIdx, s32MaskNum, NULL);
}


void CConsoleManager::Console_VideoMaskOSDHelp(void)
{
    printf("\n\t mask <Channel> <v2>\n");
    printf("            Channel: set OSD MASK on Video Chn \n");
    printf("            v2: set OSD MASK Number on Video Chn \n");
    printf("     example:mask 0 1 \n");
}


void CConsoleManager::Console_VideoSlices(void)
{
    MI_S32 param[2] = {0, 0};
    char *buf[2];

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);

    if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("param err\n");
        return;
    }

    param[0] = atoi(buf[0]);
    if(param[0] < 0 || param[0] >= (MI_S32)g_videoNumber)
    {
        MIXER_ERR("ch out of range. %d\n", g_videoNumber);
        return;
    }

    param[1] = atoi(buf[1]);
    if(0x0 > param[1]  || param[1] > 5)
    {
        MIXER_ERR("Slices Number[0, 5]. %d\n", param[1]);
        return;
    }

    mixer_send_cmd(CMD_VENC_SETH264SLICESPLIT, (MI_S8 *)param, sizeof(param));
}

void CConsoleManager::Console_VideoSlicesHelp(void)
{
    printf("\n\t slices <Channel> <v2>\n");
    printf("            Channel: which video to set Slices \n");
    printf("            v2:[0,5]. set Slices Number\n");
    printf("\t example slices 0 2\n");
}

void  CConsoleManager::Console_Video3DNR(void)
{
    MI_S32 u32Value = 0;
    MI_S32 param[2] = {2,0};
    char *buf;
    /*
    Level 0: 0 set;
    Level 1: 1 set,  8-bit;
    Level 2: 1 set, 10-bit;
    Level 3: 1 set, 12-bit;
    Level 4: 2 set,  8-bit,  8-bit;
    Level 5: 2 set,  8-bit, 10-bit;
    Level 6: 2 set,  8-bit, 12-bit;
    Level 7: 2 set, 12-bit, 12-bit;
    */
    buf  = GetParam(1);
    if(NULL == buf)
    {
        MIXER_ERR("%s: param err\n", __func__);
        return;
    }

    u32Value = atoi(buf);
    if((0/*E_MI_VPE_3DNR_LEVEL_OFF*/ <= u32Value) && (u32Value < 8/*E_MI_VPE_3DNR_LEVEL_NUM*/))
    {
        param[1] = (char)u32Value;
    }
    else
    {
        MIXER_ERR("3DNR_level out of range. %d\n", u32Value);
        return;
    }

    printf("3DNR %sable, 3DNR_level=%d\n", param[0] ? "en" : "dis", param[1]);
    mixer_send_cmd(CMD_VIDEO_SET_3DNR, (MI_S8 *) param, sizeof(param));
}

void  CConsoleManager::Console_Video3DNRHelp(void)
{
    printf("\n\t video3dnr <Level> \n");
    printf("                Level 0: 0 set\n");
    printf("                Level 1: 1 set,  8-bit\n");
#if TARGET_CHIP_I6 || TARGET_CHIP_I6B0 || TARGET_CHIP_I6E
    printf("                Level 2: 1 set, 12-bit\n");
#elif TARGET_CHIP_I5
    printf("                Level 2: 1 set, 10-bit\n");
    printf("                Level 3: 1 set, 12-bit\n");
    printf("                Level 4: 2 set,  8-bit,  8-bit\n");
    printf("                Level 5: 2 set,  8-bit, 10-bit\n");
    printf("                Level 6: 2 set,  8-bit, 12-bit\n");
    printf("                Level 7: 2 set, 12-bit, 12-bit(default if enable 3DNR)\n");
#endif
    printf("\t example: video3dnr 0\n");
}

void CConsoleManager::Console_VideoOsdOpen(void)
{
    if(TRUE == g_displayOsd)
    {
        MI_S8 *cbuf[PARAMLIST] = {NULL};
        MI_U8 i = 0x0;

        for(i=0x0; i<GetParamNum();  i++)
        {
            cbuf[i] = (MI_S8 *)GetParam(i+1);
        }
        setOsdParam((const MI_S8 **)cbuf, GetParamNum()-1);
        return;
    }
    else
    {
        MI_S32 param = 0;
        MI_S32 tmp = 0;
        char *buf;

        buf  = GetParam(1);
        if(NULL == buf)
        {
            MIXER_ERR("%s: param err\n", __func__);
            return;
        }

        tmp = mixerStr2Int(buf);

        if((tmp & 0x10) != 0)
        {
            gDebug_osdDrawRect = TRUE;
            printf("\n  gDebug_osdDrawRect open:\n");
        }

        if((tmp & 0x40) != 0)
        {
            gDebug_OsdTest = TRUE;
            printf("\n  gDebug_osdTest open:\n");
        }

        if((tmp & 0x200) != 0)
        {
            g_displayVideoInfo = TRUE;
            printf("\n  g_displayVideoInfo open\n");
        }

        if((tmp & 0x400) != 0)
        {
            g_s32OsdFlicker = TRUE;
            printf("\n  g_s32OsdFlicker open\n");
        }

        if((tmp & 0x4000) != 0)
        {
            gDebug_osdColorInverse = TRUE;
            printf("\n  gDebug_osdColorInverse open, only for I5\n");
        }

        if((tmp & 0x8000) != 0)
        {
            g_s32OsdHandleCnt = 8; //create 8 osd per video
            printf("\n  g_s32OsdHandleCnt = 8\n");
        }

        if((tmp & 0x10000) != 0)
        {
            gDebug_osdPrivateMask = TRUE;
            printf("\n  gDebug_osdPrivateMask open\n");
        }

        if(FALSE == g_displayOsd)
        {
            printf("%s:%d Enable OSD display\n", __func__, __LINE__);
            mixer_send_cmd(CMD_OSD_FULL_OPEN, (MI_S8 *)&param, sizeof(MI_S32));
            mixer_send_cmd(CMD_OSD_OPEN, (MI_S8 *)&g_videoNumber, sizeof(g_videoNumber));
            g_displayOsd = TRUE;
        }

        if(g_displayVideoInfo)
        {
            mixer_send_cmd(CMD_OSD_OPEN_DISPLAY_VIDEO_INFO, NULL, 0);
        }
    }
}

void CConsoleManager::Console_VideoOsdOpenHelp(void)
{
    if(TRUE == g_displayOsd)
    {
        printf("\n\t videoosd <functionIndex><v1><v2><v3><v4><v5>\n");
        printf("\t\n functionIndex 0: create OSD handle\n");
        printf("\t <v1>:channel\n");
        printf("\t <v2>:osd_X\n");
        printf("\t <v3>:osd_Y\n");
        printf("\t <v4>:osd_W\n");
        printf("\t <v5>:osd_H\n");
        printf("\t <v6>:osd_string\n");
        printf("\t <v7>:osd format default I4\n");
        printf("\t example: videoosd 0 0 600 600 300 300 hello or  videoosd 0 0 600 600 300 300 hello 3\n");

        printf("\t\n functionIndex 1: destroy OSD handle\n");
        printf("\t <v1>:channel\n");
        printf("\t <v2>: s32Idx:-1 osd type:RECT,[0,s32Idex] osd type:TEXT\n");
        printf("\t example:videoosd 1 0 2\n");

        printf("\t\n functionIndex 2: move OSD position by  channel\n");
        printf("\t <v1>:move channel\n");
        printf("\t <v2>:osd  handle index\n");
        printf("\t <v3>:move new_X\n");
        printf("\t <v4>:move new_Y\n");
        printf("\t example:videoosd 2 0 2 500 500\n");

        printf("\t\n functionIndex 3: change OSD size by OSD handle\n");
        printf("\t <v1>:handle\n");
        printf("\t <v2>:osd_X\n");
        printf("\t <v3>:osd_Y\n");
        printf("\t <v4>:osd_W\n");
        printf("\t <v5>:osd_H\n");
        printf("\t <v6>:osd_string\n");
        printf("\t example: videoosd 3 0 200 200 300 300 helloww\n"); //john.he

        printf("\t\n functionIndex 4: update OSD Info\n");
        printf("\t <v1>:move channel\n");
        printf("\t <v2>:osd index handle\n");
        printf("\t <v3>:osd_string\n");
        printf("\t example: videoosd 4 0 2 helloww\n");

        printf("\t\n functionIndex 5: adjust OSD Alpha\n");
        printf("\t <v1>:adjust OSD Alpha channel\n");
        printf("\t <v2>:osd index handle\n");
        printf("\t <v3>:adjust OSD Alpha value\n");
        printf("\t example: videoosd 5 0 2 0\n");

        printf("\t\n functionIndex 6:test OSD format\n");
        printf("\t <v1>[0/1],1:enable 0:disable test OSD format\n");
        printf("\t <v2>:osd format [0:bitmap,1:i2,2:i4]\n");
        printf("\t <v3>:osd_X[0,curr_width-osd_width] osd_width=(bmp:514,i2:200,i4:200)\n");
        printf("\t <v4>:osd_Y[0,curr_height-osd_height] osd_height=(bmp:66,i2:200,i4:200)\n");
        printf("\t example: videoosd 6 1 0 700 700\n");//john.he
        printf("\t when disable test osd\n");
        printf("\t <v1> 0:disable test OSD format\n");
        printf("\t <v2>:0->all osd: 1->all bitmap:2->NO.idex time osd\n");
        printf("\t <v3>:when v2 is 2, this is idex number\n");
        printf("\t example: videoosd 6 0 0/1 or videoosd 6 0 2 1\n");

        printf("\t\n functionIndex 7:create Cover handle\n");
        printf("\t <v1>:channel\n");
        printf("\t <v2>:osdCover_X\n");
        printf("\t <v3>:osdCover_Y\n");
        printf("\t <v4>:osdCover_W\n");
        printf("\t <v5>:osdCover_H\n");
        printf("\t <v6>:colorindex[0:black,1:blue,2: red,3:green]\n");
        printf("\t example: videoosd 7 0 500 500 200 200 1\n");

        printf("\t\n functionIndex 8:distory Cover handle\n");
        printf("\t <v1>:channel\n");
        printf("\t <v2>:index\n");
        printf("\t example: videoosd 8 0 0\n");

        printf("\t\n functionIndex 9:move Cover position by Cover handle\n");
        printf("\t <v1>:handle\n");
        printf("\t <v2>:osdCover_X\n");
        printf("\t <v3>:osdCover_Y\n");
        printf("\t example: videoosd 9 10  400 400\n");//john.he

        printf("\t\n functionIndex 10:change Cover size by Cover handle\n");
        printf("\t <v1>:handle\n");
        printf("\t <v2>:osdCover_X\n");
        printf("\t <v3>:osdCover_Y\n");
        printf("\t <v4>:osdCover_W\n");
        printf("\t <v5>:osdCover_H\n");
        printf("\t <v6>:colorindex[0:black,1:blue,2: red,3:green]\n");
        printf("\t example: videoosd 10 35  400 400 300 300 3\n");//john.he
    }
    else
    {
        printf("\n\t videoosd <Type>\n");
        printf("\t PrivateMask : 0x10000\n");
        printf("\t HandleCnt : 0x8000\n");
        printf("\t ColorInverse :0x4000\n");
        printf("\t Flicker: 0x400\n");
        printf("\t VideoInfo:0x200\n");
        printf("\t OsdTest: 0x40\n");
        printf("\t osdDrawRect: 0x10\n");
        printf("            Type: Type & 011100011001010000.    PrivateMask | HandleCnt | ColorInverse |Flicker | VideoInfo | OsdTest |osdDrawRect\n");
        printf("\t example full osd: videoosd 0x1C650\n");
    }
}

void CConsoleManager::Console_VideoOsdPrivateMaskParam(void)
{
    MI_S32 odparam[6];
    char *buf[6];
    MI_U8 i = 0x0;

    for(i=0x0; i<sizeof(buf)/sizeof(buf[0]); i++)
    {
        buf[i] = GetParam(i+1);
        if(NULL == buf[i])
        {
            MIXER_ERR("param err\n");
            return;
        }
    }
    odparam[0] = atoi(buf[0]);
    if(odparam[0] < 0 || odparam[0] >= (MI_S32)g_videoNumber)
    {
        MIXER_ERR("OSD MASK on Video Chn out of range. %d\n",  odparam[0]);
        return;
    }

    odparam[1] = atoi(buf[1]);
    if(odparam[1] < 0 || odparam[1] > 3)
    {
        MIXER_ERR("OSD MASK on Video index out of range. %d\n",  odparam[1]);
        return;
    }

    odparam[2] = atoi(buf[2]);
    odparam[3] = atoi(buf[3]);
    odparam[4] = atoi(buf[4]);
    odparam[5] = atoi(buf[5]);

#if TARGET_CHIP_I5 || TARGET_CHIP_I6B0
    mixer_send_cmd(CMD_OSD_PRIVATEMASK_SET, (MI_S8 *) odparam, sizeof(odparam));
#elif TARGET_CHIP_I6
    printf("This device not support this function\n");
    //mixer_send_cmd(CMD_OSD_CUSTOM_SET, (MI_S8 *) odparam, sizeof(odparam));
#elif TARGET_CHIP_I6E
    printf("This device not support this function\n");
#endif

}

void CConsoleManager::Console_VideoOsdPrivateMaskParamHelp(void)
{
    printf("\n\t privatemask <v1>....<v6>\n");
    printf("            v1: OSD MASK on Video Chn\n");
    printf("            v2: OSD MASK index\n");
    printf("            v3: point.x\n");
    printf("            v4: point.y\n");
    printf("            v5: point.w\n");
    printf("            v6: point.h\n");
    printf("\t privatemask 0 0 200 200 300 300\n");
}

void CConsoleManager::Console_VideoOnOff(void)
{
    MI_S32 videoIndex = 0, stop = 0;
    char *buf[2];
    MI_U8 i = 0x0;

    for(i=0x0; i<sizeof(buf)/sizeof(buf[0]); i++)
    {
        buf[i] = GetParam(i+1);
        if(NULL == buf[i])
        {
            MIXER_ERR("param err\n");
            return;
        }
    }

    videoIndex = atoi(buf[0]);
    if(videoIndex < 0 || videoIndex >= (MI_S32)5)
    {
        printf("video index %d invalid\n", videoIndex);
        return;
    }

    stop = !!atoi(buf[1]);

    if(0 == stop)
    {
        mixer_send_cmd(CMD_VIDEO_START_FRAME_CAPTUR, (MI_S8 *) &videoIndex, sizeof(videoIndex));
    }
    else
    {
        mixer_send_cmd(CMD_VIDEO_STOP_FRAME_CAPTUR, (MI_S8 *) &videoIndex, sizeof(videoIndex));
    }
}

void CConsoleManager::Console_VideoOnOffHelp(void)
{
    printf("\n\t videoon <v1>....<v2>\n");
    printf("            v1: which video index to stop/start\n");
    printf("            v2: [0,1]. [0]start, [1]stop\n");
    printf("\t example: videoon 1 1\n");
}


#if MIXER_PWM_MOTO_ENABLE
void  CConsoleManager::Console_pwm_mode(void)
{
    int set=0,i=0,m=0,n=0;
    int eparm[2];
    int hsparm[3];
    int setparm[18];
    MI_U32 offset = 0x0;
    int gparm = 0;
    char *buf[7] = NULL;

    for(i=0x0; i<2; i++)
    {
        buf[i] = GetParam(i+1);
        if(NULL == buf[i])
        {
            MIXER_ERR("param err\n");
            return;
        }
    }

    set = atoi(buf[0]);
    if(set != 0 && set != 1 && set != 2 && set != 3)
    {
        printf("buf[0] param %d is error!!!\n",set);
        return;
    }

    if(set == 0)
    {
        int modeparma = 0;

        gparm = atoi(buf[1]);
        offset += 1;
        if(gparm == 0 || gparm == 1 ||gparm == 2)
        {
            if(gparm == 0)
                m = 0,n = 4;
            else if (gparm == 1)
                m = 4,n = 8;
            else if (gparm == 2)
                m = 8,n = 11;
            for(i=m; i<n; i++)
            {
                offset += 1;
                buf[ offset] = GetParam(offset+1);
                if(NULL == buf[offset] )
                {
                    modeparma = atoi(buf[offset ]);
                }

                if(modeparma == 0 || modeparma == 1)
                {
                    pwm_groud[i] = modeparma;
                }
                else
                {
                    printf("param is error,please enter again\n");
                    return;
                }
            }
        }
        else
        {
            printf("buf[1] param %d is error!!!\n", gparm);
            return;
        }
        mixer_send_cmd(CMD_SYSTEM_PWM_MOTO_GROUP, (MI_S8 *) &gparm, sizeof(gparm));
    }
    else if (set == 1)
    {
        gparm = atoi(buf[1]);
        offset += 1;
        memset(setparm,0,sizeof(setparm));
        if(gparm == 0 || gparm == 1 ||gparm == 2)
        {
            int current_pwm = 0;
            if(gparm == 0)
                m = 0,n = 4;
            else if (gparm == 1)
                m = 4,n = 8;
            else if (gparm == 2)
                m = 8,n = 11;
            for(i=m; i<n; i++)
            {
                if(pwm_groud[i] == 1)
                {
                    offset += 1;
                    buf[ offset ] = GetParam(offset +1);
                    if(NULL == buf[ offset] )
                    {
                        MIXER_ERR("buf is null. %d!!!\n", i-m + 2);
                        return;
                    }
                    setparm[current_pwm + i-m] = atoi(buf[offset]);
                    if(0x0 == (i-m))
                    {

                    }
                    else if(0x1 == (i-m))
                    {
                        setparm[current_pwm + i-m] = !!setparm[current_pwm + i-m];
                    }
                    else if(0x2 == (i-m) || 0x03 == (i-m))
                    {
                        if(setparm[current_pwm + i-m] < 0)
                            setparm[current_pwm + i-m] = 0x0;
                        else if(setparm[current_pwm + i-m] > 1000)
                            setparm[current_pwm + i-m] = 1000;
                    }

                    current_pwm = current_pwm + 4;
                }
            }
        }
        else
        {
            printf("buf[1] param %d is error!!!\n",gparm);
            return;
        }
        offset += 1;
        buf[ offset ] = GetParam(offset +1);
        if(NULL == buf[ offset] )
        {
            MIXER_ERR("buf is null. %d!!!\n", i-m + 2);
            return;
        }
        setparm[16] = atoi(buf[offset]);

        setparm[17] = gparm;
        printf("ddddddddd %d\n",setparm[17]);
        mixer_send_cmd(CMD_SYSTEM_PWM_MOTO_PARAM, (MI_S8 *) setparm, sizeof(setparm));
    }
    else if (set == 2)
    {
        offset += 1;
        if(NULL == buf[ offset] )
        {
            MIXER_ERR("buf is null. %d!!!\n", i-m + 2);
            return;
        }
        eparm[0]  = atoi(buf[offset]);
        if(0> eparm[0] || eparm[0] > 2)
        {
            MIXER_ERR("eparam err. %d\n", eparm[0]);
            return;
        }
        offset += 1;
        if(NULL == buf[ offset] )
        {
            MIXER_ERR("buf is null. %d!!!\n", i-m + 2);
            return;
        }
        eparm[1]  = !!atoi(buf[offset]);
        mixer_send_cmd(CMD_SYSTEM_PWM_MOTO_ENABLE, (MI_S8 *) eparm, sizeof(eparm));
    }
    else if (set == 3)
    {
        offset += 1;
        buf[ offset ] = GetParam(offset +1);
        if(NULL == buf[ offset] )
        {
            MIXER_ERR("buf is null. %d!!!\n", i-m + 2);
            return;
        }
        hsparm[0]  = atoi(buf[offset]);

        if(0> hsparm[0] || hsparm[0] > 2)
        {
            MIXER_ERR("hsparm err. %d\n", hsparm[0]);
            return;
        }
        offset += 1;
        buf[ offset ] = GetParam(offset +1);
        if(NULL == buf[ offset] )
        {
            MIXER_ERR("buf is null. %d!!!\n", i-m + 2);
            return;
        }
        hsparm[1]  = !!atoi(buf[offset]);
        if(hsparm[1] == 0)
        {
            offset += 1;
            buf[ offset ] = GetParam(offset +1);
            if(NULL == buf[ offset] )
            {
                MIXER_ERR("buf is null. %d!!!\n", i-m + 2);
                return;
            }
            hsparm[2]  = !!atoi(buf[offset]);
        }
        mixer_send_cmd(CMD_SYSTEM_PWM_MOTO_HSTOP, (MI_S8 *) hsparm, sizeof(hsparm));
    }
}

void  CConsoleManager::Console_pwm_modeHelp(void)
{
    printf("\n\t pwm <v1>....<v7>\n");
    printf("            v1: [0,3].  0:IN/OUT Group,1:set period/polority/duty/round parame,2:enable/disable groud,3:stop/hold group\n");
    printf("            v2: [0,2]. 0: group0, 1: group1, 2: group2\n");
    printf("            when v1 == 0: \n");
    printf("                   v3: [0,1]. period\n");
    printf("                   v4: [0,1]. 0:out, 1:in\n");
    printf("                   v5: [0,1]. 0:out, 1:in\n");
    printf("                   v6: [0,1]. 0:out, 1:in\n");
    printf("            when v1 == 1: \n");
    printf("                   v3: [0,1]. period\n");
    printf("                   v4: [0,1]. polarity\n");
    printf("                   v5: [0,1000]. begin\n");
    printf("                   v6: [0,1000]. end\n");
    printf("                   v7: [0,1000]. round\n");
    printf("            when v1 == 2: \n");
    printf("                   v2: [0,2].  group(v2)\n");
    printf("                   v3: [0,1].  group(v2) hold enble/disable:1-enable 0-disable\n");
    printf("            when v1 == 3: \n");
    printf("                   v2: [0,2].  group(v2)\n");
    printf("                   v3: [0,1].  group(v2) hold/stop:1-stop 0-hold\n");
    printf("            when v1 == 3  && v3 == 0: \n");
    printf("                   v4: [0,1].  group(v2)  hold enble/disable:1-enable 0-disable\n");
    printf("\t example: pwm 1 1 1 30 30 30 \n");
}
#endif

void CConsoleManager::Console_VideoChnRoiConfig(void)
{
    MI_U32 u32roiparam[9];

    char *buf[9] = {NULL};
    MI_U32 i = 0x0;

    for(i=0x0; i<9; i++)
    {
        buf[i] = GetParam(i+1);
        if(NULL == buf[i])
        {
            MIXER_ERR("param err\n");
            return;
        }
    }
    u32roiparam[0] = atoi(buf[0] );
    if(u32roiparam[0] >= g_videoNumber)
    {
        MIXER_ERR("venc channel out of range %d!!!\n", u32roiparam[0]);
        return;
    }

    u32roiparam[1] = atoi(buf[1] );
    if(u32roiparam[1] > 7)
    {
        MIXER_ERR("roi index out of range %d!!!\n", u32roiparam[1]);
        return;
    }
    u32roiparam[2] = atoi(buf[2] );
    if((0 != u32roiparam[2]) && (1 != u32roiparam[2]))
    {
        MIXER_ERR("roi enable out of range %d!!!\n", u32roiparam[2]);
        return;
    }
    u32roiparam[3] = !!atoi(buf[3] );
    u32roiparam[4] = atoi(buf[4] );
    u32roiparam[5] = atoi(buf[5] );
    u32roiparam[6] = atoi(buf[6] );
    u32roiparam[7] = atoi(buf[7] );
    u32roiparam[8] = atoi(buf[8] );

    mixer_send_cmd(CMD_VIDEO_SET_ROICFG, (MI_S8 *) u32roiparam, sizeof(u32roiparam));
}

void CConsoleManager::Console_VideoChnRoiConfigHelp(void)
{
    printf("\n\t roi  <v1>....<v9>\n");
    printf("            v1: [0,%d].  which venc channel\n",g_videoNumber-1);
    printf("            v2: [0,7]. roi Index\n");
    printf("            v3: [0,1]. roi enable\n");
    printf("            v4: [0,1]. roi bAbsQp [Absolutely qp:1, Relative qp:0]\n");
	#if TARGET_CHIP_I6E || TARGET_CHIP_I6B0
    printf("            v5: [-32, 31]. roi Qp value\n");
	#elif TARGET_CHIP_I6 || TARGET_CHIP_I5
	printf("            v5: [-7, 7]. roi Qp value\n");
	#endif
    printf("            v6: roi Rect Left [H264 n*16] [H265 n*32]\n");
    printf("            v7: roi Rect top [H264 n*16] [H265 n*32]\n");
    printf("            v8: roi Rect Width [H264 n*16] [H265 n*32]\n");
    printf("            v9: roi Rect Height[H264 n*16][H265 n*32]\n");
    printf("\t example: roi 0 0 1 0 20 10 20 200 200\n");
}

void CConsoleManager::Console_VideoResolution(void)
{
    Size_t size;
    MI_U32 veChannel = 0;
    char *buf[3] = {NULL};
    MI_U32 i = 0x0;
	MI_U16 fifo_data[100];

    for(i=0x0; i<3; i++)
    {
        buf[i] = GetParam(i+1);
        if(NULL == buf[i])
        {
            MIXER_ERR("param err\n");
            return;
        }
    }

    veChannel = atoi(buf[0]);
    if(g_videoNumber <= veChannel)
    {
        MIXER_ERR("venc channel out of range %d!!!\n", veChannel);
        return;
    }

    size.width = atoi(buf[1]);
    size.height = atoi(buf[2]);

    MI_S32 param[3];
#if LOAD_MIXER_CFG
    ModuleVencInfo_s *pVencInfo_t = GetVencInfo();

    if(size.width > pVencInfo_t->StreamWidth[veChannel] || pVencInfo_t->StreamHeight[veChannel] < size.width)
    {
        printf("resolution_size_width[0-%d]:", pVencInfo_t->StreamWidth[veChannel]);
        printf("resolution_size_height[0-%d]:", pVencInfo_t->StreamHeight[veChannel]);
    }
#endif
    if(FALSE == mixerResolutionValid(veChannel,size.width, size.height))
    {
        printf("resolution error\n");
        return;
    }

    param[0] = veChannel;//veChn
    param[1] = size.width;
    param[2] = size.height;

    if(g_displayOsd || gDebug_osdColorInverse)
    {
        if(g_ieLogParam && (0 == param[0]))
        {
            MixerDisableOSD(CMD_OSD_RESET_RESOLUTION, param[0], 0, 0, 1);
        }
        else
        {
            MixerDisableOSD(CMD_OSD_RESET_RESOLUTION, param[0], 0, 0, 0);
        }
    }

    mixer_send_cmd(CMD_VIDEO_SET_RESOLUTION, (MI_S8 *) param, sizeof(param));
    if(0x0 == mixer_wait_cmdresult(CMD_VIDEO_SET_RESOLUTION, fifo_data, sizeof(fifo_data)))
    {
      if((0 == fifo_data[1]) && (fifo_data[0] == CMD_VIDEO_SET_RESOLUTION))
      {
	    if(g_displayOsd || gDebug_osdColorInverse)
	    {
	        if(g_ieLogParam && (0 == param[0]))
	        {
	            MixerEnableOSD(CMD_OSD_RESTART_RESOLUTION, param[0], 0, 0, 1);
	        }
	        else
	        {
	            MixerEnableOSD(CMD_OSD_RESTART_RESOLUTION, param[0], 0, 0, 0);
	        }
	    }
       }
     }
	else
	{
	  MIXER_DBG("mixer_wait_cmdresult CMD_VIDEO_SET_RESOLUTION is time out fifo_data[1]:%d fifo_data[0] %d\n",fifo_data[1],fifo_data[0]);
	}
}

void CConsoleManager::Console_VideoResolutionHelp(void)
{
    printf("\n\t res  <v1>....<v3>\n");
    printf("            v1: [0,%d].  which venc channel\n",g_videoNumber-1);
    printf("            v2: resolution_size_width\n");
    printf("            v3: resolution_size_height\n");
    printf("\t example: res 0 1280 720\n");
}

void CConsoleManager::Console_SnrResolution(void)
{
    MI_S32 param[5];
    MI_S32 ret = -1;
    MI_U16 fifo_data[100];
    static MI_S32 g_SnrResolutionIdx = -1;
    MI_U32 i = 0x0;
    char *buf = NULL;
    MI_S32 tmpSnrResolutionIdx = -1;
    float delta = 0.001;
    buf = GetParam(1);
    if(NULL == buf)
    {
        MIXER_ERR("param err\n");
        return;
    }

    tmpSnrResolutionIdx = atoi(buf);

    mixer_send_cmd(CMD_VIDEO_GET_SENSOR_RESOLUTION, NULL, 0);

    ret = mixer_wait_cmdresult(CMD_VIDEO_GET_SENSOR_RESOLUTION, fifo_data, sizeof(fifo_data));
    if(0x0 != ret)
    {
        MIXER_ERR("wait cmd result timeout. ret=%d\n", ret);
        return;
    }

    if((tmpSnrResolutionIdx >= fifo_data[1])||(1 == fifo_data[1]))
    {
        if(1 == fifo_data[1])
        {
            MIXER_ERR("This sensor res just only one res,not support change res!\n");
            return ;
        }
        else
        {
            MIXER_ERR("sensor res out of range. [0, %d]. %d\n", fifo_data[1]-1, tmpSnrResolutionIdx);
        }
        return ;
    }
    if((g_SnrResolutionIdx == tmpSnrResolutionIdx) && \
       (g_videoParam[0].width == (MI_U32)fifo_data[4 + tmpSnrResolutionIdx * 5]) && \
       (g_videoParam[0].height == (MI_U32)fifo_data[5 + tmpSnrResolutionIdx * 5]) && \
       (fabs(g_videoParam[0].stVifInfo.sensorFrameRate -(float)fifo_data[6 + tmpSnrResolutionIdx * 5])<= delta))
    {
        printf("sensor have work SnrResolutionIdx=%d\n",tmpSnrResolutionIdx);
        return ;
    }

    MixerDisableOSD(CMD_OSD_RESET_RESOLUTION, -1, 0, 0, 1);
    MySystemDelay(100);

    g_SnrResolutionIdx = tmpSnrResolutionIdx;
    g_videoParam[0].width  = fifo_data[4 + tmpSnrResolutionIdx * 5];
    g_videoParam[0].height = fifo_data[5 + tmpSnrResolutionIdx * 5];
    g_videoParam[0].stVifInfo.vifframeRate = fifo_data[6 + tmpSnrResolutionIdx * 5];
    g_videoParam[0].vpeframeRate = fifo_data[6 + tmpSnrResolutionIdx * 5];
    g_videoParam[0].stVifInfo.sensorFrameRate = fifo_data[6 + tmpSnrResolutionIdx * 5];
    g_videoMaxWidth  = g_videoParam[0].width;
    g_videoMaxHeight = g_videoParam[0].height;

    for(i = 1; i < g_videoNumber; i++)
    {
        g_videoParam[i].stVifInfo.sensorFrameRate = g_videoParam[0].stVifInfo.sensorFrameRate;
        g_videoParam[i].stVifInfo.vifframeRate = g_videoParam[0].stVifInfo.vifframeRate;
        g_videoParam[i].vpeframeRate = g_videoParam[0].vpeframeRate;
    }

    param[0] = fifo_data[3 + tmpSnrResolutionIdx * 5];  //sensor resolution index
    param[1] = fifo_data[4 + tmpSnrResolutionIdx * 5];  //select sensor width
    param[2] = fifo_data[5 + tmpSnrResolutionIdx * 5];  //select sensor height
    param[3] = fifo_data[6 + tmpSnrResolutionIdx * 5];  //select sensor FPS

    mixer_send_cmd(CMD_VIDEO_SET_SENSOR_RESOLUTION, (MI_S8 *) param, sizeof(param));


    if(0x0 == mixer_wait_cmdresult(CMD_VIDEO_SET_SENSOR_RESOLUTION, fifo_data, sizeof(fifo_data)))
    {
        if((0 == fifo_data[99]) && (fifo_data[0] == CMD_VIDEO_SET_SENSOR_RESOLUTION))
        {
            for(MI_U16 veChn = 0; veChn < fifo_data[1]; veChn++)
            {
                fifo_data[0] = CMD_OSD_RESTART_RESOLUTION;
                printf("%s:%d cmdId=0x%x, veChn=%d, width=%d, height=%d, IeEnable=%d\n", \
                       __func__, \
                       __LINE__, \
                       CMD_VIDEO_SET_SENSOR_RESOLUTION,\
                       fifo_data[2+veChn*4], fifo_data[3+veChn*4], fifo_data[4+veChn*4], fifo_data[5+veChn*4]);

                MixerEnableOSD((MixerCmdId)fifo_data[0], \
                               fifo_data[2+veChn*4], \
                               fifo_data[3+veChn*4], \
                               fifo_data[4+veChn*4], \
                               fifo_data[5+veChn*4]);
            }
        }
        else
        {
            if(0 != fifo_data[99])
            {
                MIXER_ERR("%s:%d set Sensor Resolution fail(%d)\n", __func__, __LINE__, fifo_data[99]);
            }
            else
            {
                MIXER_ERR("%s:%d get wrong pipe data cmd(%d)\n", __func__, __LINE__, fifo_data[0]);
            }
            return;
        }
    }
    else
    {
        MIXER_ERR("cmd wait result timeout.\n");
        return;
    }
}

void CConsoleManager::Console_SnrResolutionHelp(void)
{
    printf("\n\t snrres  <v1>\n");
    printf("            v1: [0,SnrResCount].  which SnrRes Idx you select\n");
    printf("\t example: snrres 0\n");
}

void CConsoleManager::Console_ShowFrameInterval(void)
{
    if(FALSE == g_ShowFrameInterval)
    {
        g_ShowFrameInterval = TRUE;
        printf("%s:%d Enable record video frame interval log!\n", __func__,__LINE__);
    }
    else
    {
        g_ShowFrameInterval = FALSE;
        printf("%s:%d Disable record video frame interval log!\n", __func__,__LINE__);
    }
}

void CConsoleManager::Console_ShowFrameIntervalHelp(void)
{
    printf("\n\t showframeint \n");
    printf("\t example: showframeint \n");
}

void CConsoleManager::Console_RunTimeCmd(void)
{
#if 1
    char cmd[256] = {0x0};
    MI_U32 num = GetParamNum()-1;
    MI_U32 i = 0x0;
    MI_U32 len = 0x0;
    char *buf = NULL;

    memset(cmd, 0x00, sizeof(cmd));

    for(i=0x0; i<num ; i++)
    {
        buf =  GetParam(i+1);
        if(NULL == buf )
            break;
        if((len+strlen(buf)) >= sizeof(cmd))
            break;
        len += sprintf(cmd + len, "%s ", buf);
    }

    if(0x0 != len)
    {
        printf("%s:%d run cmd:\"%s\"\n", __func__,__LINE__, cmd);
        system(cmd);
    }
    else
    {
        printf("your shell without shell cmd!\n");
    }
#endif
}

void CConsoleManager::Console_RunTimeCmdHelp(void)
{
    printf("\n\t shell \n");
    printf("\t example: shell ls\n");
}

void CConsoleManager::Console_SuperFrameMode(void)
{
    MI_S32 param[2];
    char *buf[2];

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);

    if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("err param\n");
        return;
    }

    param[0] = atoi(buf[0]);
    if(param[0] < 0 || param[0] >= (MI_S32)g_videoNumber)
    {
        MIXER_ERR("channel err. %d\n", param[0]);
        return;
    }


    param[1] = atoi(buf[1]);
    if((E_MI_VENC_SUPERFRM_NONE <= param[1]) && (param[1] < E_MI_VENC_SUPERFRM_MAX))
    {
        mixer_send_cmd(CMD_VIDEO_SET_SUPERFRAME_MODE, (MI_S8 *)param, sizeof(param));
    }
    else
    {
        printf("your choice is %d, is out of range[0, %d)\n", param[1], E_MI_VENC_SUPERFRM_MAX);
    }
}

void CConsoleManager::Console_SuperFrameModeHelp(void)
{
    printf("\n\t superframemode  <v1>... <v2>\n");
    printf("            v1: [0, %d].  select the SuperFrame for which stream to set\n",g_videoNumber-1);
    printf("            v2: [0, 2].  0->NONE, 1->DISCARD, 2->REENCODE\n");
    printf("\t example: superframemode 0 1\n");
}

void CConsoleManager::Console_VideoMirrorFlip(void)
{
    MI_U32 nVpeMirrorFlipparam[4];
    MI_S8 bAllVideo = -1;
    MI_U32 nVpechid = 0;
    MI_U32 nVpePortid = 0;
    MI_BOOL bMirror = 0;
    MI_BOOL bFlip = 0;
    MI_S8  nValue = 0;
    char *buf[4] = {NULL};
    MI_U32 i = 0x0;
    MI_U32 offset = 0x0;

    for(i=0x0; i<3; i++)
    {
        buf[i] = GetParam(i+1);
        if(NULL == buf[i])
        {
            MIXER_ERR("param err buff[%d] is NULL\n",i);
            return;
        }
    }

    if((g_rotation & 0xFF000000))
    {

        bAllVideo = atoi(buf[offset]);
        offset += 1;
        nVpeMirrorFlipparam[0] = bAllVideo;
        if(bAllVideo)
        {
            //rotate mode top is channel0
            nVpechid = 0;
            nVpePortid = 0;
        }
        else
        {
            //rotate mode bottom is channel1
            nVpechid =1;

            nVpePortid = atoi(buf[offset]);
            offset += 1;
        }
    }
    else
    {
        nVpechid =0;

        nVpePortid = atoi(buf[offset]);
        offset += 1;
    }

    nValue = atoi(buf[offset]);
    offset += 1;

    if(nValue == 0)
    {
        bMirror = 0;
        bFlip = 0;
    }
    else if(nValue == 1)
    {
        bMirror = 1;
        bFlip = 0;
    }
    else if(nValue == 2)
    {
        bMirror = 0;
        bFlip = 1;
    }
    else if(nValue == 3)
    {
        bMirror = 1;
        bFlip = 1;
    }

    nVpeMirrorFlipparam[0] = nVpechid;
    nVpeMirrorFlipparam[1] = nVpePortid;
    nVpeMirrorFlipparam[2] = bMirror;
    nVpeMirrorFlipparam[3] = bFlip;

    mixer_send_cmd(CMD_VIDEO_SET_MIRROR_FLIP, (MI_S8 *)nVpeMirrorFlipparam, sizeof(nVpeMirrorFlipparam));
}

void CConsoleManager::Console_VideoMirrorFlipHelp(void)
{
    printf("\n\t sclmirrorflip  <v1>... <v3>\n");
    printf("            v1: [0, 1].  when open Rotate, set all video; yes[1], no[0]\n");
    printf("            v2: [0, 2].  which vpe portindex to set\n");
    printf("            v3: [0, 3].  mirror/flip state\n");
    printf("\t example: sclmirrorflip 1 0 1\n");
}

void CConsoleManager::Console_CUS3AEnable(void)
{
    MI_S32 param[] = {0,0,0};
    char *pStr;
    MI_U32 n;

    //param0, AE
    //param1, AWB
    //param2, AF
    for(n=0; n<sizeof(param)/sizeof(param[0]); ++n)
    {
        pStr = GetParam(n+1);
        if(pStr)
            param[n] = atoi(pStr);
        else
            break;
    }


    if(param[0]||param[1]||param[2])
    {
        printf("Cus3A active \n");
        mixer_send_cmd(CMD_ISP_OPEN_CUS3A, (MI_S8 *)param, sizeof(param));
    }
    else
    {
        printf("Cus3A inactive \n");
        mixer_send_cmd(CMD_ISP_CLOSE_CUS3A, (MI_S8 *)param, sizeof(param));
    }
}

void CConsoleManager::Console_CUS3AHelp(void)
{
    printf("\n\t CUS3A  0~1\n");
    printf("            0: Disable CUS AE/AWB \n");
    printf("            1: Enable CUS AE/AWB\n");
}

void CConsoleManager::Console_Sed(void)
{
#if MIXER_SED_ENABLE
    MI_S32 param[5];
    char *pStr = NULL;

    pStr = GetParam(1);
    if(NULL == pStr)
    {
        MIXER_ERR("param err.\n");
        return;
    }
    if(g_mdParam || g_odParam || g_hchdParam || g_vgParam || g_fdParam)
    {
        MIXER_ERR("ie channel already use divp's vdf channel.  please close ie first.\n");
        return ;
    }
    if(0x0 == memcmp(pStr, "on", 2))
    {
        param[0] = atoi(GetParam(2));
        param[1] = atoi(GetParam(3));
        mixer_send_cmd(CMD_SED_OPEN, (MI_S8 *)param, sizeof(param));
        printf("CMD_SED_OPEN:open sed\n");
    }
    else if(0x0 == memcmp(pStr, "off", 3))
    {
        mixer_send_cmd(CMD_SED_CLOSE, NULL, 0x0);
        printf("CMD_SED_CLOSE:close sed\n");
    }
    else
    {
        MIXER_ERR("err command.\n");
    }
#endif

}

void CConsoleManager::Console_SedHelp(void)
{
#if MIXER_SED_ENABLE
    printf("\n\t sed  <v1>\n");
    printf("            v1: [on, off].  on:open sed; off: close sed.\n");
    printf("\t example: sed on\n");
#endif
}

void CConsoleManager::Console_MAF(void)
{
    char *pStr[2] = {NULL};
    MI_S8 motor[2] ={0x0};

    pStr[0] = GetParam(1);
    pStr[1] = GetParam(2);

    if(NULL == pStr[0] || NULL == pStr[1])
    {
        MIXER_ERR("param err.\n");
        return;
    }

    if(0x0 == strcmp(pStr[0], "zoom"))
    {
    motor[0] = 1;
    }
    else if(0x0 != strcmp(pStr[0], "focus"))
    {
    motor[0] = 2;
    }
    else
    {
        MIXER_ERR("param err.\n");
        return;
    }

    if(0x0 != strcmp(pStr[1], "+"))
    {
        motor[1] = 1;
    }
    else if(0x0 != strcmp(pStr[1], "-"))
    {
        motor[1] = 2;
    }
    else
    {
         MIXER_ERR("param err.\n");
        return;
    }

    mixer_send_cmd(CMD_MOTOR_CONTROL, motor, sizeof(motor));
}

void CConsoleManager::Console_MafHelp(void)
{
    printf("\n\t maf  <v1> <v2>\n");
    printf("            v1: [zoom/focus].  zoom:control zoom ; focus: control focus.\n");
    printf("            v1: [+/-].  +:forward ; -: backward.\n");
    printf("\t example: maf zoom +\n");
}

void CConsoleManager::Console_VideoChnSaveTask(void)
{
    MI_S32 param[2] = { 0, 0};
    char *buf[2];

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);

    if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("err param\n");
        return;
    }

    param[0] = atoi(buf[0]);
    if(param[0] < 0 || param[0] >= (MI_S32)g_videoNumber)
    {
        MIXER_ERR("channel err. %d\n", param[0]);
        return;
    }

    param[1] = atoi(buf[1]);
    if(param[1] != 0 && param[1] != 1)
    {
        MIXER_ERR("flag err. %d\n", param[1]);
        return;
    }

    //MIXER_DBG("ch(%d), param[1](%d)\n", param[0], param[1]  );
    mixer_send_cmd(CMD_RECORD_SETMODE, (MI_S8 *)param, sizeof(param));

}

void CConsoleManager::Console_VideoChnSaveTaskHelp(void)
{
    printf("\n\t rec  <v1>... <v2>\n");
    printf("            v1: [0, %d].  which channel you want to save?\n",g_videoNumber-1);
    printf("            v2: [0, 1]. 0: disbale rec. 1: enable rec\n");
    printf("\t example rec 0 1");
}

void CConsoleManager::Console_Switch_HDR_Linear_Mode(void)
{
    MI_S32 ret = -1;
    MI_S32 u32Value = 0;
    MI_S32 param[5];
    MI_U16 fifo_data[256];
    //int nCus3aispParam[3] = {0,0,0};
    //MI_BOOL bCusAEenable = FALSE;
    char *buf;

    if((0 != (g_rotation & 0xFF000000)) && ((3840 <= g_videoMaxWidth) || (2160 <= g_videoMaxHeight)))
    {
        printf("%s:%d Enable Rotate@8M, not support swich hdr <--> linaer mode!\n", __func__, __LINE__);
        return;
    }

    buf = GetParam(1);

    if(NULL == buf)
    {
        MIXER_ERR("err param\n");
        return;
    }

    u32Value = atoi(buf);
    if(Mixer_HDR_Mode_NUM <= u32Value || Mixer_HDR_TYPE_OFF > u32Value)
    {
        MIXER_ERR("sdk don't support this value\n");
        return;
    }
    if((!mixer_GetHdrValue()) && (u32Value != 0))
    {
        mixer_setHdrValue(TRUE);
        g_videoParam[0].stVifInfo.level3DNR = 7;
#if TARGET_CHIP_I5
        //to do should discuss with I5 owner
        g_videoParam[0].stVifInfo.HdrType = 2;
#else
        g_videoParam[0].stVifInfo.HdrType = u32Value;
#endif
        //bCusAEenable = FALSE;
    }
    else if((mixer_GetHdrValue()) && (u32Value == 0))
    {
        mixer_setHdrValue(FALSE);
        g_videoParam[0].stVifInfo.level3DNR = 3;
        g_videoParam[0].stVifInfo.HdrType = 0;
        //bCusAEenable = TRUE;
#if TARGET_CHIP_I6
        if(E_MI_VPE_3DNR_LEVEL2 < g_videoParam[0].stVifInfo.level3DNR)
        {
            g_videoParam[0].stVifInfo.level3DNR = E_MI_VPE_3DNR_LEVEL2;
            printf("%s:%d for Infinity6 3DNR level only support %d(E_MI_VPE_3DNR_LEVEL2)\n", __func__, __LINE__, E_MI_VPE_3DNR_LEVEL2);
        }
#elif TARGET_CHIP_I6E
        if(E_MI_VPE_3DNR_LEVEL2 < g_videoParam[0].stVifInfo.level3DNR)
        {
            g_videoParam[0].stVifInfo.level3DNR = E_MI_VPE_3DNR_LEVEL2;
            printf("%s:%d for Infinity6E 3DNR level only support %d(E_MI_VPE_3DNR_LEVEL2)\n", __func__, __LINE__, E_MI_VPE_3DNR_LEVEL2);
        }
#elif TARGET_CHIP_I6B0
        if(E_MI_VPE_3DNR_LEVEL2 < g_videoParam[0].stVifInfo.level3DNR)
        {
            g_videoParam[0].stVifInfo.level3DNR = E_MI_VPE_3DNR_LEVEL2;
            printf("%s:%d for Infinity6B0 3DNR level only support %d(E_MI_VPE_3DNR_LEVEL2)\n", __func__, __LINE__, E_MI_VPE_3DNR_LEVEL2);
        }
#endif
    }
    else
    {
        printf("\nDoes not support the input values(Hdr=%d, Set_mode=%d)\n", mixer_GetHdrValue(), u32Value);
        return;
    }

    printf("Open %s mode, 3DNR_level=%d HdrType=%d\n", u32Value?"hdr":"linaer",
           g_videoParam[0].stVifInfo.level3DNR, g_videoParam[0].stVifInfo.HdrType);

    if(g_displayOsd || gDebug_osdColorInverse)
    {
        if(g_ieLogParam)
        {
            MixerDisableOSD(CMD_OSD_RESET_RESOLUTION, -1, 0, 0, 1);
        }
        else
        {
            MixerDisableOSD(CMD_OSD_RESET_RESOLUTION, -1, 0, 0, 0);
        }
    }

    memset(param, 0, sizeof(param));
    param[0] = (MI_S32)1;   //vif dev num
    param[1] = (MI_S32)g_videoParam[0].stVifInfo.level3DNR;
    param[2] = (MI_S32)g_videoParam[0].stVifInfo.HdrType;

    mixer_send_cmd(CMD_VIDEO_SWITCH_HDR_LINAER_MODE,  (MI_S8 *)param, sizeof(param));

    if(0x0 == mixer_wait_cmdresult(CMD_VIDEO_SWITCH_HDR_LINAER_MODE, fifo_data, sizeof(fifo_data)))
    {
        if(0 != fifo_data[1])
        {
            printf("%s:%d read pipe data error(%d) fifo_data[1]=%d\n", __func__, __LINE__, ret, fifo_data[1]);
            if(g_displayOsd || gDebug_osdColorInverse)
            {
                if(g_ieLogParam)
                {
                    MixerEnableOSD(CMD_OSD_RESTART_RESOLUTION, -1, 0, 0, 1);
                }
                else
                {
                    MixerEnableOSD(CMD_OSD_RESTART_RESOLUTION, -1, 0, 0, 0);
                }
            }
            return ;
        }

    }
    else
    {
        MIXER_ERR("cmd wait result timeout.\n");
        if(g_displayOsd || gDebug_osdColorInverse)
        {
            if(g_ieLogParam)
            {
                MixerEnableOSD(CMD_OSD_RESTART_RESOLUTION, -1, 0, 0, 1);
            }
            else
            {
                MixerEnableOSD(CMD_OSD_RESTART_RESOLUTION, -1, 0, 0, 0);
            }
        }
        return;
    }

    //nCus3aispParam[0] = bCusAEenable;
    //nCus3aispParam[1] = 0;
    //nCus3aispParam[2] = 0;
    //mixer_send_cmd(CMD_ISP_OPEN_CUS3A, (MI_S8 *)nCus3aispParam, sizeof(nCus3aispParam));

    if(g_displayOsd || gDebug_osdColorInverse)
    {
        if(g_ieLogParam)
        {
            MixerEnableOSD(CMD_OSD_RESTART_RESOLUTION, -1, 0, 0, 1);
        }
        else
        {
            MixerEnableOSD(CMD_OSD_RESTART_RESOLUTION, -1, 0, 0, 0);
        }
    }
}

void CConsoleManager::Console_Switch_HDR_Linear_ModeHelp(void)
{
    printf("\n\t hdr  <v1>\n");
    printf("            v1: [0, 4]. linaer[0],  hdr VC [1], hdr DOL [2], hdr EMBEDDED[3], hdr LI [4]\n");
    printf("\t example: hdr 0\n");
}

void CConsoleManager::Console_AudioInVolume(void)
{
    MI_S32 param[2];
    char * buf[2];

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);

    if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("err param\n");
        return;
    }
    param[0] = atoi(buf[0]);
    if((param[0] < 0) || (param[0] >= MIXER_AI_MAX_NUMBER) || FALSE == g_audioInParam[(MI_U8)param[0]].bFlag)
    {
        printf("Audio input device without working\n");
        return;
    }
    if((param[0] < 0) || (param[0] > (g_audioInParam[0].u8AudioInNum - 1)))
    {
        printf("%s:%d set AudioIn Chn in [0 %d], you set value=%d is out of range\n", __func__, __LINE__,g_audioInParam[0].u8AudioInNum - 1,param[0]);
        return;
    }
    param[1] = atoi(buf[1]);

    if((0 <= param[1]) && (param[1] <= 21))
    {
        g_audioInParam[param[0]].s32VolumeInDb = param[1];
    }
    else
    {
        printf("%s:%d VolumeInDb in [0 21], you set value=%d is out of range\n", __func__, __LINE__, param[1]);
        return;
    }

    mixer_send_cmd(CMD_AUDIO_SET_AIVOLUME, (MI_S8 *)param, sizeof(param));
}

void CConsoleManager::Console_AudioInVolumeHelp(void)
{
    printf("\n\t audioin  <v1>..<v2>\n");
    printf("            v1: [0, %d]. which AudioIn VolumeInDb to been set\n", g_audioInParam[0].u8AudioInNum - 1);
    printf("            v2: [0, 21]. set VolumeInDb\n");
    printf("\texample: audioin 0 10\n");
}

void CConsoleManager::Console_AudioOutVolume(void)
{
    //char param[64];
    MI_S32 PARAM[2];
    MI_S32 s32Value = 0x0;
    MI_S32 AudioIdx = 0x0;

    char * buf[2];

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);

    if(NULL == buf[0] || NULL == buf[1])
    {
        MIXER_ERR("err param\n");
        return;
    }

    AudioIdx = atoi(buf[0]);

    if(FALSE == g_audioOutParam[AudioIdx].bFlag)
    {
        printf("Audio output device without working\n");
        return;
    }

    if((0 > AudioIdx) || (AudioIdx > MIXER_AO_MAX_NUMBER))
    {
        MIXER_ERR("error param!please input again. %d\n", AudioIdx);
        return;
    }

#if TARGET_CHIP_I5
    printf("%s:%d VolumeOutDb in [-60 10], Please set VolumeOutDb(set val=%d):", __func__, __LINE__, g_audioOutParam[0].s32VolumeOutDb);
#elif TARGET_CHIP_I6
    printf("%s:%d VolumeOutDb in [-60 30], Please set VolumeOutDb(set val=%d):", __func__, __LINE__, g_audioOutParam[0].s32VolumeOutDb);
#elif TARGET_CHIP_I6E
    printf("%s:%d VolumeOutDb in [-60 30], Please set VolumeOutDb(set val=%d):", __func__, __LINE__, g_audioOutParam[0].s32VolumeOutDb);
#elif TARGET_CHIP_I6B0
    printf("%s:%d VolumeOutDb in [-60 30], Please set VolumeOutDb(set val=%d):", __func__, __LINE__, g_audioOutParam[0].s32VolumeOutDb);
#endif

    if('-' == buf[1][0])
    {
        s32Value = mixerStr2Int(&buf[1][1]);
        s32Value = (~s32Value) + 1;
    }
    else
    {
        s32Value = mixerStr2Int(buf[1]);
    }
#if TARGET_CHIP_I5
    if((-60 <= s32Value) && (s32Value <= 10))
    {
        g_audioOutParam[AudioIdx].s32VolumeOutDb = s32Value;
    }
    else
    {
        printf("%s:%d VolumeOutDb in [-60 10], you set value=%d is out of range!\n", __func__, __LINE__, s32Value);
        return;
    }
#elif TARGET_CHIP_I6
    if((-60 <= s32Value) && (s32Value <= 30))
    {
        g_audioOutParam[AudioIdx].s32VolumeOutDb = s32Value;
    }
    else
    {
        printf("%s:%d VolumeOutDb in [-60 30], you set value=%d is out of range!\n", __func__, __LINE__, s32Value);
        return;
    }
#elif TARGET_CHIP_I6E
    if((-60 <= s32Value) && (s32Value <= 30))
    {
        g_audioOutParam[AudioIdx].s32VolumeOutDb = s32Value;
    }
    else
    {
        printf("%s:%d VolumeOutDb in [-60 30], you set value=%d is out of range!\n", __func__, __LINE__, s32Value);
        return;
    }
#elif TARGET_CHIP_I6B0
    if((-60 <= s32Value) && (s32Value <= 30))
    {
        g_audioOutParam[AudioIdx].s32VolumeOutDb = s32Value;
    }
    else
    {
        printf("%s:%d VolumeOutDb in [-60 30], you set value=%d is out of range!\n", __func__, __LINE__, s32Value);
        return;
    }
#endif

    PARAM[0] = AudioIdx;
    PARAM[1] = s32Value;
    mixer_send_cmd(CMD_AUDIO_SET_AOVOLUME, (MI_S8 *)&PARAM, sizeof(PARAM));
}

void CConsoleManager::Console_AudioOutVolumeHelp(void)
{
    printf("\n\t audioout  <v1>..<v2>\n");
    printf("            v1: [0, %d]. which AudioOut to been set VolumeOutDb\n", g_audioOutParam[0].s32AudioOutNum - 1);
    printf("            v2: VolumeOutDb in [-60 30]\n");
    printf("\t example: audioout 0 20\n");
}

void CConsoleManager::Console_IeOnOff(void)
{
    MI_S32 param[3];
    MI_S32 tmp;
    MI_S32 iMode = 0;
    MI_U32 TmpMode = 0x0;
    static MI_S32 iLastMode = MixerGetModeParam();  //get global g_modeParam
    char *buf[10] ;

    param[0] = g_ieVpePort;
    param[1] = g_ieWidth;
    param[2] = g_ieHeight;

    if( g_displayOsd == FALSE)
    {
        printf("g_displayOSD is not open\n");
        return;
    }

    buf[0] = GetParam(1);
    buf[1] = GetParam(2);
    buf[2] = GetParam(3);
    if(NULL == buf[0])
    {
        MIXER_ERR("err param\n");
        return;
    }

    iMode = atoi(buf[0]);
    if(iMode < 0 || iMode > 255)
    {
        MIXER_ERR("md/od mode(0bit-md, 1bit-od, 2bit-fd, 3bit-hc, 4bit-hchd, 5bit-vg, 6bit-dla): %x\n", iMode);
        return;
    }

    if(NULL != buf[1])
    {
        tmp = atoi(buf[1]);
        if((0x0 == tmp) && (iMode & 0x1f))
        {
            MIXER_WARN("Ie width: %d\n", tmp);
        }
        else
        {
            param[1] = g_ieWidth = tmp;
        }
    }

    if(NULL != buf[2])
    {
        tmp = atoi(buf[2]);
        if((0x0 == tmp) && (iMode & 0x1f))
        {
            MIXER_WARN("Ie height: %d\n", tmp);
        }
        else
        {
            param[2] = g_ieHeight = tmp;
        }
    }

    if(iMode != iLastMode)
    {
        if(( 0x0 == iLastMode) && (0x0 != iMode))
        {
            mixer_send_cmd(CMD_IE_OPEN, (MI_S8 *)param, sizeof(param));
            param[0] = 1;
            mixer_send_cmd(CMD_OSD_IE_OPEN, (MI_S8 *)&param[0], sizeof(param[0]));
        }

        if((0x0 == (iMode&0x01))  && (0x01 == (iLastMode & 0x01)))  //close md
        {
            mixer_send_cmd(CMD_IE_MD_CLOSE, (MI_S8 *)&g_mdParam, sizeof(g_mdParam));
            g_mdParam = 0;
        }
        else if((0x01 == (iMode&0x01))  && (0x0 == (iLastMode & 0x01))) //open md
        {
            g_mdParam = 1;
            mixer_send_cmd(CMD_IE_MD_OPEN, (MI_S8 *)&g_mdParam, sizeof(g_mdParam));
        }

        if((0x0 == (iMode&0x02))  && (0x02 == (iLastMode & 0x02)))  //close od
        {
            mixer_send_cmd(CMD_IE_OD_CLOSE, (MI_S8 *)&g_odParam, sizeof(g_odParam));
            g_odParam = 0;
        }
        else if((0x2 == (iMode&0x02))  && (0x0 == (iLastMode & 0x02)))  // open od
        {
            g_odParam = 1;
            mixer_send_cmd(CMD_IE_OD_OPEN, (MI_S8 *)&g_odParam, sizeof(g_odParam));
        }

        if((0x0 == (iMode&0x04))  && (0x04 == (iLastMode & 0x04)))  //close fd
        {
            mixer_send_cmd(CMD_IE_FD_CLOSE, (MI_S8 *)&g_fdParam, sizeof(g_fdParam));
            g_fdParam = 0;
        }
        else if((0x4 == (iMode&0x04))  && (0x0 == (iLastMode & 0x04)))  // open fd
        {
            g_fdParam = 1;
            mixer_send_cmd(CMD_IE_FD_OPEN, (MI_S8 *)&g_fdParam, sizeof(g_fdParam));
        }
        else if((0x0 == (iMode&0x08))  && (0x08 == (iLastMode & 0x08))) //close hc
        {
            mixer_send_cmd(CMD_IE_HCHD_CLOSE, NULL, 0);
            g_hchdParam = 0;
        }
        else if((0x8 == (iMode&0x08))  && (0x00 == (iLastMode & 0x08))) //open hc
        {
            g_hchdParam = 1;    // 1: HC;  2: HD;  3: FD
            TmpMode = 1;
            mixer_send_cmd(CMD_IE_HCHD_OPEN, (MI_S8 *)&TmpMode, sizeof(TmpMode));
        }
        else if((0x0 == (iMode&0x10))  && (0x10 == (iLastMode & 0x10))) //close hchd
        {
            mixer_send_cmd(CMD_IE_HCHD_CLOSE, NULL, 0);
            g_hchdParam = 0;
        }
        else if((0x10 == (iMode&0x10))  && (0x0 == (iLastMode & 0x10))) // open hchd
        {
            g_hchdParam = 1;    // 1: HC;  2: HD;  3: FD
            TmpMode = 2;
            mixer_send_cmd(CMD_IE_HCHD_OPEN, (MI_S8 *)&TmpMode, sizeof(TmpMode));
        }

        if((0x0 == (iMode&0x20))  && (0x20 == (iLastMode & 0x20)))  //close vg
        {
            mixer_send_cmd(CMD_IE_VG_CLOSE, (MI_S8 *)&g_vgParam, sizeof(g_vgParam));
            g_vgParam = 0;
        }
        else if((0x20 == (iMode&0x20))  && (0x0 == (iLastMode & 0x20))) // open vg
        {
            g_vgParam = 1;
            mixer_send_cmd(CMD_IE_VG_OPEN, (MI_S8 *)&g_vgParam, sizeof(g_vgParam));
        }

        if((0x0 == (iMode&0x40))  && (0x40 == (iLastMode & 0x40)))  //close dla
        {
            mixer_send_cmd(CMD_IE_DLA_CLOSE, (MI_S8 *)&g_dlaParam, sizeof(g_dlaParam));
            g_dlaParam = 0;
        }
        else if((0x40 == (iMode&0x40))  && (0x0 == (iLastMode & 0x40))) // open dla
        {
            g_dlaParam = 1;
            mixer_send_cmd(CMD_IE_DLA_OPEN, (MI_S8 *)&g_dlaParam, sizeof(g_dlaParam));
        }

        if(iMode && iMode != iLastMode)
        {
//          param[0] = 1;
//          mixer_send_cmd(CMD_OSD_IE_OPEN, (MI_S8 *)&param[0], sizeof(param[0]));
        }
        else
        {
            if( iMode != iLastMode)
            {
                if(g_displayOsd)
                {
                    mixer_send_cmd(CMD_OSD_IE_CLOSE, NULL, 0);
                }

                mixer_send_cmd(CMD_IE_CLOSE, (MI_S8 *)param, sizeof(param));
            }
        }

        iLastMode = iMode;
    }
    else
    {
        if((0x40 == (iMode&0x40)) && (0x40 == (iLastMode & 0x40))) //set DLA param
        {
            buf[3] = GetParam(4);
            buf[4] = GetParam(5);
            buf[5] = GetParam(6);
            buf[6] = GetParam(7);
            buf[7] = GetParam(8);
            IeParamInfo IeTmp;
            MI_BOOL state = FALSE;

            MIXER_DBG(" %s, %s, %s, %s, %s\n", buf[3],buf[4],buf[5],buf[6],buf[7]);

            memset(&IeTmp, 0x0, sizeof(IeTmp));
            IeTmp.box_id = -1;

            if((NULL != buf[3]  && !memcmp(buf[3], "add", 3)) && \
               (NULL != buf[4]) && ( NULL != buf[5]))
            {
                IeTmp.box_id = atoi(buf[4]);
                memcpy(IeTmp.NewAddName, buf[5], strlen(buf[5]));

                state = TRUE;

                if((NULL != buf[6] && !memcmp(buf[6], "del", 3)) && \
                   (NULL != buf[7]))
                {
                    memcpy(IeTmp.NewDelName, buf[7], strlen(buf[7]));
                }
            }
            else if((NULL != buf[3] && !memcmp(buf[3], "del", 3)) && \
                    (NULL != buf[4]))
            {
                memcpy(IeTmp.NewDelName, buf[4], strlen(buf[4]));

                state = TRUE;

                if((NULL != buf[5]  && !memcmp(buf[5], "add", 3)) && \
                   (NULL != buf[6]) && ( NULL != buf[7]))
                {
                    IeTmp.box_id = atoi(buf[6]);
                    memcpy(IeTmp.NewAddName, buf[7], strlen(buf[7]));
                }
            }

            if(TRUE == state)
            {
                mixer_send_cmd(CMD_IE_DLA_SETPARAM, (MI_S8 *)&IeTmp, sizeof(IeTmp));
            }
        }
    }
}

void CConsoleManager::Console_IeOnOffHelp(void)
{
    printf("\n\t ie  <v1>..|v2| |v3| .... |Vn|\n");
    printf("            v1: md/od mode(0bit-md, 1bit-od, 2bit-fd, 3bit-hc, 4bit-hd, 5bit-vg, 6bit-dla)\n");
    printf("            v2: IE width\n");
    printf("            v3: IE height\n");
    printf("\t example : ie 1 200 200\n");
    printf("\t when open dla. v4 == 'add', v5 = box_id, v6=add_name. v7=='del', v8 = del_name.\n");
    printf("\t when open dla. v4 == 'del', v5 = delname, v6='add'. v7== box_id, v8 = add_name.\n");
    printf("\t example : ie 64 200 200 add 1 xxx del xxxx\n");
    printf("\t example : ie 64 200 200 del xxxx add 1 xxxx\n");
}

void CConsoleManager::Console_IPUInitInfo(void)
{
    IPU_InitInfo_S stIPUInitInfo;
    char *pTmp = NULL;
    MI_S32 tmp = 0;

    memset(&stIPUInitInfo, 0, sizeof(IPU_InitInfo_S));

    pTmp = GetParam(1);
    if (pTmp != NULL)
    {
        tmp = atoi(GetParam(1));
        if (tmp < Model_Type_None || tmp >= Model_Type_Butt)
        {
            return;
        }

        stIPUInitInfo.enModelType   = (IPU_Model_Type_E)(tmp);
    }

    pTmp = GetParam(2);
    if (pTmp != NULL)
        snprintf(stIPUInitInfo.szIPUfirmware, sizeof(stIPUInitInfo.szIPUfirmware) - 1, "%s", pTmp);

    pTmp = GetParam(3);
    if (pTmp != NULL)
        snprintf(stIPUInitInfo.szModelFile, sizeof(stIPUInitInfo.szModelFile) - 1, "%s", pTmp);

    if (stIPUInitInfo.enModelType == Model_Type_Classify ||
        stIPUInitInfo.enModelType == Model_Type_Detect)
    {
        pTmp = GetParam(4);
        if (pTmp != NULL)
            snprintf(stIPUInitInfo.u.ExtendInfo1.szLabelFile,
                     sizeof(stIPUInitInfo.u.ExtendInfo1.szLabelFile) - 1, "%s", pTmp);
    }
    else
    {
        pTmp = GetParam(4);
        if (pTmp != NULL)
            snprintf(stIPUInitInfo.u.ExtendInfo2.szModelFile1,
                     sizeof(stIPUInitInfo.u.ExtendInfo2.szModelFile1) - 1, "%s", pTmp);

        pTmp = GetParam(5);
        if (pTmp != NULL)
            snprintf(stIPUInitInfo.u.ExtendInfo2.szFaceDBFile,
                     sizeof(stIPUInitInfo.u.ExtendInfo2.szFaceDBFile) - 1, "%s", pTmp);

        pTmp = GetParam(6);
        if (pTmp != NULL)
            snprintf(stIPUInitInfo.u.ExtendInfo2.szNameListFile,
                     sizeof(stIPUInitInfo.u.ExtendInfo2.szNameListFile) - 1, "%s", pTmp);
    }

    mixer_send_cmd(CMD_IE_DLA_SETINITINFO, (MI_S8 *)&stIPUInitInfo, sizeof(IPU_InitInfo_S));
}

void CConsoleManager::Console_IPUInitInfoHelp(void)
{
    printf("\n\t ipu  <v1>..|v2| |v3| .... |Vn|\n");
    printf("            v1: model type, 0:object classify, 1:object detect, 2:face recognition\n");
    printf("            v2: ipu firmware path\n");
    printf("            v3: model file path\n");
    printf("            v4: label file path\n");
    printf("\t example : \n");
    printf("\t         ipu 0 ipu_firmware.bin.shrink caffe_resnet50_conv_fixed.tflite_sgsimg.img labels.txt\n");
    printf("\t         ipu 1 ipu_firmware.bin.shrink ssd_mobilenet_v1_fixed.tflite_sgsimg.img mscoco_label.txt\n");
    printf("\t         ipu 2 ipu_firmware.bin.shrink caffe_fda_fixed.tflite_sgsimg.img caffe_fr_fixed.tflite_sgsimg.img feat.bin name.list\n");
}

void CConsoleManager::Console_Exit(void)
{
    mThread = FALSE;
}

void CConsoleManager::Console_ExitHelp(void)
{
    printf("\n\t  q |Q |exit    exit mixer\n");
}



static ConsoleCmdMap ConsoleCmdMapTable[]=
{
    {"sensorfps", &CConsoleManager::Console_SnrFramerate, &CConsoleManager::Console_SnrFramerateHelp},
    {"audiovqe", &CConsoleManager::Console_AudioVqeMode, &CConsoleManager::Console_AudioVqeModeHelp},
    {"videobitrate", &CConsoleManager::Console_VideoBitrate, &CConsoleManager::Console_VideoBitrateHelp},
    {"superframesize", &CConsoleManager::Console_VideoSuperFrameSize, &CConsoleManager::Console_VideoSuperFrameSizeHelp},
    {"ircut", &CConsoleManager::Console_VideoIrcut, &CConsoleManager::Console_VideoIrcutHelp},
    {"afmoto", &CConsoleManager::Console_VideoAfMoto, &CConsoleManager::Console_VideoAfMotoHelp},
    {"avideolog",&CConsoleManager::Console_StreamLogOnOff, &CConsoleManager::Console_StreamLogOnOffHelp},
    {"audiostream",&CConsoleManager::Console_RecordAudioInData, &CConsoleManager::Console_RecordAudioInDataHelp},
    {"aed", &CConsoleManager::Console_AudioAEDParam, &CConsoleManager::Console_AudioAEDParamHelp},
    {"md", &CConsoleManager::Console_MdParam, &CConsoleManager::Console_MdParamHelp},
    {"videofps", &CConsoleManager::Console_VideoFps, &CConsoleManager::Console_VideoFpsHelp},
    {"gop", &CConsoleManager::Console_VideoGop, &CConsoleManager::Console_VideoGopHelp},
    {"ltr", &CConsoleManager::Console_VideoIframeInterval, &CConsoleManager::Console_VideoIframeIntervalHelp},
    {"isp", &CConsoleManager::Console_IspParam, &CConsoleManager::Console_IspParamHelp},
    {"idr", &CConsoleManager::Console_VideoRequestIDR, &CConsoleManager::Console_VideoRequestIDRHelp},
    {"audio", &CConsoleManager::Console_AudioOnOff, &CConsoleManager::Console_AudioOnOffHelp},
    {"videocodec", &CConsoleManager::Console_VideoCodec, &CConsoleManager::Console_VideoCodecHelp},
    {"rot", &CConsoleManager::Console_VideoRotate, &CConsoleManager::Console_VideoRotateHelp},
#if MIXER_SUPPORT_DIVP_BIND_CHANGE
    {"divpbind", &CConsoleManager::Console_DivpBindChange, &CConsoleManager::Console_DivpBindChangeHelp},
#endif
    {"mask", &CConsoleManager::Console_VideoMaskOSD, &CConsoleManager::Console_VideoMaskOSDHelp},
    {"slices", &CConsoleManager::Console_VideoSlices, &CConsoleManager::Console_VideoSlicesHelp},
    {"video3dnr", &CConsoleManager::Console_Video3DNR, &CConsoleManager::Console_Video3DNRHelp},
    {"videoosd", &CConsoleManager::Console_VideoOsdOpen, &CConsoleManager::Console_VideoOsdOpenHelp},
    {"privatemask", &CConsoleManager::Console_VideoOsdPrivateMaskParam, &CConsoleManager::Console_VideoOsdPrivateMaskParamHelp},
    {"videoon", &CConsoleManager::Console_VideoOnOff, &CConsoleManager::Console_VideoOnOffHelp},
#if MIXER_PWM_MOTO_ENABLE
    {"pwm", &CConsoleManager::Console_pwm_mode, &CConsoleManager::Console_pwm_modeHelp},
#endif
    {"roi", &CConsoleManager::Console_VideoChnRoiConfig, &CConsoleManager::Console_VideoChnRoiConfigHelp},
    {"res", &CConsoleManager::Console_VideoResolution, &CConsoleManager::Console_VideoResolutionHelp},
    {"snrres", &CConsoleManager::Console_SnrResolution, &CConsoleManager::Console_SnrResolutionHelp},
    {"showframeint", &CConsoleManager::Console_ShowFrameInterval, &CConsoleManager::Console_ShowFrameIntervalHelp},
    {"shell", &CConsoleManager::Console_RunTimeCmd, &CConsoleManager::Console_RunTimeCmdHelp},
    {"superframemode", &CConsoleManager::Console_SuperFrameMode, &CConsoleManager::Console_SuperFrameModeHelp},
    {"sclmirrorflip", &CConsoleManager::Console_VideoMirrorFlip, &CConsoleManager::Console_VideoMirrorFlipHelp},
    {"rec", &CConsoleManager::Console_VideoChnSaveTask, &CConsoleManager::Console_VideoChnSaveTaskHelp},
    {"hdr", &CConsoleManager::Console_Switch_HDR_Linear_Mode, &CConsoleManager::Console_Switch_HDR_Linear_ModeHelp},
    {"audioin", &CConsoleManager::Console_AudioInVolume, &CConsoleManager::Console_AudioInVolumeHelp},
    {"audioout", &CConsoleManager::Console_AudioOutVolume, &CConsoleManager::Console_AudioOutVolumeHelp},
    {"ie", &CConsoleManager::Console_IeOnOff, &CConsoleManager::Console_IeOnOffHelp},
    {"ipu", &CConsoleManager::Console_IPUInitInfo, &CConsoleManager::Console_IPUInitInfoHelp},
    {"cus3a", &CConsoleManager::Console_CUS3AEnable, &CConsoleManager::Console_CUS3AHelp},
    {"sed", &CConsoleManager::Console_Sed, &CConsoleManager::Console_SedHelp},
    {"maf", &CConsoleManager::Console_MAF, &CConsoleManager::Console_MafHelp},
    {"q", &CConsoleManager::Console_Exit, &CConsoleManager::Console_ExitHelp},
    {"Q", &CConsoleManager::Console_Exit, &CConsoleManager::Console_ExitHelp},
    {"exit", &CConsoleManager::Console_Exit, &CConsoleManager::Console_ExitHelp},
    {"help", &CConsoleManager::Console_Cmd, &CConsoleManager::Console_Cmd},

};

void CConsoleManager::Console_Cmd(void)
{
    MI_U32 i = 0x0;

    for(i=0x0; i<sizeof(ConsoleCmdMapTable)/sizeof(ConsoleCmdMapTable[0]); i++)
    {
        printf("\n\t %s", ConsoleCmdMapTable[i].CmdStr);
    }
    printf("\n");
}

CConsoleManager::CConsoleManager()
{
    MI_U32 i = 0x0;
    mParamNum = 0x00;
    m_iWordPosition = 0x00;
    for(i=0x0; i<PARAMLIST; i++)
    {
        pmbuf[i] = NULL;

    }
    memset(mData, 0x0, sizeof(mData));
    mThread = TRUE;
}

CConsoleManager::~CConsoleManager()
{
    MI_U32 i = 0x0;

    for(i=0x0; i<PARAMLIST; i++)
    {
        pmbuf[i] = NULL;
    }
}

CConsoleManager *CConsoleManager::instance()
{
    static CConsoleManager _instance;

    return &_instance;
}

char * CConsoleManager::GetParam(MI_U8 index)
{
    if( index >= PARAMLIST )
    {
        MIXER_ERR("index out of range\n");
        return NULL;
    }

    return (char *)pmbuf[index];
}

MI_U8 CConsoleManager::GetParamNum(void)
{
    if( mParamNum >= PARAMLIST )
    {
        return PARAMLIST;
    }

    return mParamNum;
}
void CConsoleManager::StrParse(void)
{
    char* argv[PARAMLIST] = {NULL};
    char *outer_ptr=NULL;
    int argc = 0x0;
    int i = 0x0;
    MI_U32 j = 0x0;


    argv[0] = (char *)mData;
    //MIXER_DBG("argv[0] is %s\n", argv[0]);
    while((argc<PARAMLIST)&&(NULL != (argv[argc] = strtok_r(argv[argc], " \n", &outer_ptr)))  )
        argc++;

    mParamNum = argc;

    if(0x0 == argc)
    {
        return;
    }

    memset(pmbuf, 0x0, sizeof(pmbuf));

    for(i=0x0; i<argc; i++)
    {
        pmbuf[i] =  (MI_S8*)&CharBuf[i][0];

        if(NULL != pmbuf[i])
        {
            memset(pmbuf[i], 0x0, PARAMLENGTH);
            memcpy(pmbuf[i], argv[i], strlen(argv[i]) > PARAMLENGTH ? PARAMLENGTH : strlen(argv[i]));
            //MIXER_DBG("pmbuf[%d]: %s\n", i, pmbuf[i]);
        }
    }

    for(j=0x0; j < sizeof(ConsoleCmdMapTable)/sizeof(ConsoleCmdMapTable[0]); j++)
    {
        if(NULL != pmbuf[0] && NULL != ConsoleCmdMapTable[j].CmdStr &&\
           (strlen((const char *)pmbuf[0]) == strlen(ConsoleCmdMapTable[j].CmdStr)) && \
           (0x0 == memcmp(pmbuf[0], ConsoleCmdMapTable[j].CmdStr, strlen((char*)ConsoleCmdMapTable[j].CmdStr))))
        {
            if(NULL != pmbuf[1] && \
               (strlen((const char *)pmbuf[1]) == strlen("help")) &&\
               (0x0 == memcmp(pmbuf[1], "help", strlen((char*)pmbuf[1]))))
            {
                if(ConsoleCmdMapTable[j].prochelp)
                    (this->*ConsoleCmdMapTable[j].prochelp)();
            }
            else
            {
                if(ConsoleCmdMapTable[j].proc)
                {
                    (this->*ConsoleCmdMapTable[j].proc)();
                }
            }
        }
    }

}

void CConsoleManager::OnData(const MI_S8 *pbuf, MI_S32 length /* =1 */)
{
    char  l_dbData[2];
    char *l_pdbData = (char *)pbuf;

    if(NULL == l_pdbData)
        return ;

    while(length--)
    {
        l_dbData[0] = *l_pdbData++;

        switch (l_dbData[0])
        {
            case ASCII_LF:  //收到回车字符
            case ASCII_CR:
            {
                if (m_iWordPosition > 0)
                {
                    mData[m_iWordPosition++] = '\0';
                    //这里做数据解析工作，主要是字符串切割。
                    mParamNum = 0x0;
                    StrParse();
                    m_iWordPosition = 0;
                }
                printf("\r>");
                fflush(stdout);
            }
            break;

            case ASCII_BACK:
                if (m_iWordPosition)
                {
                    printf("\b \b");
                    m_iWordPosition--;
                    if(m_iWordPosition <= 0x0)
                        m_iWordPosition = 0x0;
                }
                break;

            default:
                //printf("%x\n", l_dbData[0]);
                mData[m_iWordPosition++] = l_dbData[0];

                /*l_dbData[1] = 0;
                printf(&l_dbData[0], 1);*/
                if (m_iWordPosition >= PARAMLENGTH)
                {
                    m_iWordPosition = 0;
                }
                break;
        }
    }
    return;

}

MI_BOOL CConsoleManager::RThreadProc()
{

    char buf[PARAMLENGTH]= {0x0}; // ch = 0x0;
    MI_S32 nRead = 0x0;
    MI_S32 i = 0x0;
    char ch = 0x0;

    while(TRUE == mThread)
    {
        if((nRead = read(0, buf, sizeof(buf))) > 0)
        {
            //因为串口的数据是很快上来的，这里主要做数据搬移工作
            for(i=0x0; i < nRead; i++)
            {
                if(0x0A == buf[i] && 0x0D == ch)
                    continue;

                OnData((const MI_S8*)&buf[i]);
                ch = buf[i];
            }
        }
    }
    printf("mixerInputProcessLoop exit\n");

    return 0;
}

