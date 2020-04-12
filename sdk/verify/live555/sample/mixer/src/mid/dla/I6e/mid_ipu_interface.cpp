#include "mid_ipu_interface.h"

#include "mi_ipu.h"

MI_S32 IPUCreateDevice(char *pFirmwarePath,MI_U32 u32VarBufSize)
{
    MI_IPU_DevAttr_t stDevAttr;

    memset(&stDevAttr, 0, sizeof(MI_IPU_DevAttr_t));

    stDevAttr.u32MaxVariableBufSize = u32VarBufSize;
    stDevAttr.u32YUV420_W_Pitch_Alignment = 16;
    stDevAttr.u32YUV420_H_Pitch_Alignment = 2;
    stDevAttr.u32XRGB_W_Pitch_Alignment = 16;

    return MI_IPU_CreateDevice(&stDevAttr, NULL, pFirmwarePath, 0);
}

MI_S32 IPUDestroyDevice(void)
{
    return MI_IPU_DestroyDevice();
}

MI_S32 IPUCreateChannel(MI_U32 *s32Channel, char *pModelImage, MI_U32 u32InBufDepth, MI_U32 u32OutBufDepth)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_SYS_GlobalPrivPoolConfig_t stGlobalPrivPoolConf;
    MI_IPUChnAttr_t stChnAttr;
    //create channel
    memset(&stGlobalPrivPoolConf, 0 ,sizeof(stGlobalPrivPoolConf));
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth         = u32InBufDepth;
    stChnAttr.u32OutputBufDepth     = u32OutBufDepth;
    s32Ret = MI_IPU_CreateCHN((MI_IPU_CHN *)s32Channel, &stChnAttr, NULL, pModelImage);
    if(MI_SUCCESS != s32Ret)
    {
        MIXER_ERR("create IPU chn %d error[0x%x]\n", *s32Channel, s32Ret);
      //  stGlobalPrivPoolConf.bCreate = 0;
      //  MI_SYS_ConfigPrivateMMAPool(&stGlobalPrivPoolConf);
    }

    return s32Ret;
}

MI_S32 IPUDestroyChannel(MI_U32 s32Channel, MI_U32 u32InBufDepth, MI_U32 u32OutBufDepth)
{
    return MI_IPU_DestroyCHN(s32Channel);
}

