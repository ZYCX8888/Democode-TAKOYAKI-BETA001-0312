#ifndef _ST_VDEC_H_
#define _ST_VDEC_H_

#include "st_common.h"
#define DISP_WIDTH_ALIGN 2
#define DISP_HEIGHT_ALIGN 2

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

MI_S32 ST_CreateVdecChannel(MI_S32 s32VdecChn, MI_S32 s32CodecType,
    MI_U32 u32Width, MI_U32 u32Height, MI_U32 u32OutWidth, MI_U32 u32OutHeight);
MI_S32 ST_DestroyVdecChannel(MI_S32 s32VdecChn);
MI_S32 ST_SendVdecFrame(MI_S32 s32VdecChn, MI_U8 *pu8Buffer, MI_S32 s32Len);

MI_S32 ST_CreateDivpChannel(MI_S32 s32DivpChn, MI_U32 u32Width, MI_U32 u32Height, MI_S32 s32ColorFmt, ST_Rect_T stCropRect);
MI_S32 ST_DestroyDivpChannel(MI_S32 s32DivpChn);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif //_ST_VDEC_H_