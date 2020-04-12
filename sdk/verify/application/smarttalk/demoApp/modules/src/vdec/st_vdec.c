#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>

#include "st_vdec.h"
#include "st_common.h"

#include "mi_sys.h"
#include "mi_vdec.h"
#include "mi_vdec_datatype.h"

#include "mi_divp.h"
#include "mi_divp_datatype.h"

MI_S32 ST_CreateVdecChannel(MI_S32 s32VdecChn, MI_S32 s32CodecType,
    MI_U32 u32Width, MI_U32 u32Height, MI_U32 u32OutWidth, MI_U32 u32OutHeight)
{
    MI_VDEC_ChnAttr_t stVdecChnAttr;
    MI_VDEC_OutputPortAttr_t stOutputPortAttr;

    memset(&stVdecChnAttr, 0, sizeof(MI_VDEC_ChnAttr_t));
    stVdecChnAttr.stVdecVideoAttr.u32RefFrameNum = 2;
    stVdecChnAttr.eVideoMode    = E_MI_VDEC_VIDEO_MODE_FRAME;
    stVdecChnAttr.u32BufSize    = 1 * 1024 * 1024;
    stVdecChnAttr.u32PicWidth   = u32Width;
    stVdecChnAttr.u32PicHeight  = u32Height;
    stVdecChnAttr.u32Priority   = 0;
    stVdecChnAttr.eCodecType    = s32CodecType;
    stVdecChnAttr.eDpbBufMode = 0;

    STCHECKRESULT(MI_VDEC_CreateChn(s32VdecChn, &stVdecChnAttr));
    STCHECKRESULT(MI_VDEC_StartChn(s32VdecChn));
    if (u32OutWidth > u32Width)
    {
        u32OutWidth = u32Width;
    }
    if (u32OutHeight > u32Height)
    {
        u32OutHeight = u32Height;
    }
    stOutputPortAttr.u16Width = u32OutWidth;
    stOutputPortAttr.u16Height = u32OutHeight;
    MI_VDEC_SetOutputPortAttr(s32VdecChn, &stOutputPortAttr);

    return MI_SUCCESS;
}

MI_S32 ST_DestroyVdecChannel(MI_S32 s32VdecChn)
{
    MI_S32 s32Ret = MI_SUCCESS;

    s32Ret = MI_VDEC_StopChn(s32VdecChn);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StopRecvPic %d error, %X\n", __func__, __LINE__, s32VdecChn, s32Ret);
    }
    s32Ret |= MI_VDEC_DestroyChn(s32VdecChn);
    if (MI_SUCCESS != s32Ret)
    {
        printf("%s %d, MI_VENC_StopRecvPic %d error, %X\n", __func__, __LINE__, s32VdecChn, s32Ret);
    }

    return s32Ret;
}

MI_S32 ST_SendVdecFrame(MI_S32 s32VdecChn, MI_U8 *pu8Buffer, MI_S32 s32Len)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_VDEC_VideoStream_t stVdecStream;
    stVdecStream.pu8Addr = pu8Buffer;
    stVdecStream.u32Len = s32Len;
    MI_SYS_GetCurPts(&stVdecStream.u64PTS);

    stVdecStream.bEndOfFrame = 1;
    stVdecStream.bEndOfStream = 0;
    s32Ret = MI_VDEC_SendStream(s32VdecChn, &stVdecStream, 20);

    return s32Ret;
}

MI_S32 ST_CreateDivpChannel(MI_S32 s32DivpChn, MI_U32 u32Width, MI_U32 u32Height, MI_S32 s32ColorFmt,
    ST_Rect_T stCropRect)
{
    MI_DIVP_ChnAttr_t stDivpChnAttr;
    MI_DIVP_OutputPortAttr_t stOutputPortAttr;

    memset(&stDivpChnAttr, 0, sizeof(MI_DIVP_ChnAttr_t));
    stDivpChnAttr.bHorMirror            = FALSE;
    stDivpChnAttr.bVerMirror            = FALSE;
    stDivpChnAttr.eDiType               = E_MI_DIVP_DI_TYPE_OFF;
    stDivpChnAttr.eRotateType           = E_MI_SYS_ROTATE_NONE;
    stDivpChnAttr.eTnrLevel             = E_MI_DIVP_TNR_LEVEL_OFF;
    stDivpChnAttr.stCropRect.u16X       = stCropRect.s32X;
    stDivpChnAttr.stCropRect.u16Y       = stCropRect.s32Y;
    stDivpChnAttr.stCropRect.u16Width   = stCropRect.s16PicW;
    stDivpChnAttr.stCropRect.u16Height  = stCropRect.s16PicH;
    stDivpChnAttr.u32MaxWidth           = 1920;
    stDivpChnAttr.u32MaxHeight          = 1080;

    STCHECKRESULT(MI_DIVP_CreateChn(s32DivpChn, &stDivpChnAttr));
    STCHECKRESULT(MI_DIVP_StartChn(s32DivpChn));

    memset(&stOutputPortAttr, 0, sizeof(stOutputPortAttr));
    stOutputPortAttr.eCompMode          = E_MI_SYS_COMPRESS_MODE_NONE;
    stOutputPortAttr.ePixelFormat       = s32ColorFmt;
    stOutputPortAttr.u32Width           = ALIGN_BACK(u32Width, DISP_WIDTH_ALIGN);
    stOutputPortAttr.u32Height          = ALIGN_BACK(u32Height, DISP_HEIGHT_ALIGN);

    STCHECKRESULT(MI_DIVP_SetOutputPortAttr(s32DivpChn, &stOutputPortAttr));

    return MI_SUCCESS;
}

MI_S32 ST_DestroyDivpChannel(MI_S32 s32DivpChn)
{
    MI_S32 s32Ret = MI_SUCCESS;

    s32Ret = MI_DIVP_StopChn(s32DivpChn);
    if (MI_SUCCESS != s32Ret)
    {
        ST_ERR("%s %d, MI_DIVP_StopChn %d error, %X\n", __func__, __LINE__, s32DivpChn, s32Ret);
    }
    s32Ret |= MI_DIVP_DestroyChn(s32DivpChn);
    if (MI_SUCCESS != s32Ret)
    {
        ST_ERR("%s %d, MI_DIVP_DestroyChn %d error, %X\n", __func__, __LINE__, s32DivpChn, s32Ret);
    }

    return s32Ret;
}

