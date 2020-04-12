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

#ifndef _ST_RGN_H_
#define _ST_RGN_H_

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

#include "mi_rgn.h"
#include "st_common.h"

#define MAX_OSD_NUM_PER_CHN     4
#define ST_OSD_HANDLE_INVALID   0xFFFF

#define RGB2PIXEL1555(r,g,b)	\
	((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3) | 0x8000)


#define I4_RED      (1)
#define I4_GREEN    (2)
#define I4_BLUE     (3)
#define I4_BLACK    (12)

typedef enum
{
	ST_OSD_ATTACH_DIVP,
	ST_OSD_ATTACH_VPE,

	ST_OSD_ATTACH_BUTT,
} ST_OSD_ATTACH_TYPE_E;

typedef enum
{
    DMF_Font_Size_16x16 = 0,    // ascii 8x16
    DMF_Font_Size_32x32,        // ascii 16x32
    DMF_Font_Size_48x48,        // ascii 24x48
    DMF_Font_Size_64x64,        // ascii 32x64

    DMF_Font_Size_BUTT,
} DMF_Font_Size_E;

typedef struct
{
    MI_S32 divpChn;
} ST_OSD_ATTACH_DIVP_S;

typedef struct
{
    MI_S32 vpeChn;
    MI_S32 vpePort;
} ST_OSD_ATTACH_VPE_S;

typedef union
{
    ST_OSD_ATTACH_DIVP_S stDivp;
    ST_OSD_ATTACH_VPE_S stVpe;
} ST_OSD_ATTACH_INFO_U;

typedef struct
{
    MI_RGN_HANDLE hHandle;
    ST_OSD_ATTACH_TYPE_E enAttachType;
    ST_OSD_ATTACH_INFO_U uAttachInfo;
} ST_OSD_INFO_S;

typedef struct
{
    ST_Rect_T stRect;
    MI_RGN_PixelFormat_e ePixelFmt;
    MI_RGN_ChnPort_t stRgnChnPort;
    MI_U32 u32Layer;
    char szBitmapFile[64];
    char szAsciiBitmapFile[64];
} ST_OSD_Attr_T;

MI_S32 ST_OSD_Init(void);

MI_S32 ST_OSD_Deinit(void);

MI_S32 ST_OSD_DrawPoint(MI_RGN_HANDLE hHandle, ST_Point_T stPoint, MI_U32 u32Color);

MI_S32 ST_OSD_DrawLine(MI_RGN_HANDLE hHandle, ST_Point_T stPoint0, ST_Point_T stPoint1, MI_U8 u8BorderWidth, MI_U32 u32Color);

MI_S32 ST_OSD_DrawRect(MI_RGN_HANDLE hHandle, ST_Rect_T stRect, MI_U8 u8BorderWidth, MI_U32 u32Color);

MI_S32 ST_OSD_DrawRectFast(MI_RGN_HANDLE hHandle, ST_Rect_T stRect, MI_U8 u8BorderWidth, MI_U32 u32Color);

MI_S32 ST_OSD_ClearRectFast(MI_RGN_HANDLE hHandle, ST_Rect_T stRect);

MI_S32 ST_OSD_FillRect(MI_RGN_HANDLE hHandle, ST_Rect_T stRect, MI_U32 u32Color);

MI_S32 ST_OSD_DrawCircle(MI_RGN_HANDLE hHandle, ST_Point_T stPoint, int radii, int from, int end, MI_U8 u8BorderWidth, MI_U32 u32Color);

MI_S32 ST_OSD_FillCircle(MI_RGN_HANDLE hHandle, ST_Point_T stPoint, int radii, int from, int end, MI_U32 u32Color);

MI_S32 ST_OSD_DrawText(MI_RGN_HANDLE hHandle, ST_Point_T stPoint, const char *szString, MI_U32 u32Color, DMF_Font_Size_E enSize);

MI_S32 ST_OSD_Clear(MI_RGN_HANDLE hHandle, ST_Rect_T *pstRect);

MI_S32 ST_OSD_GetCanvasInfo(MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t** ppstCanvasInfo);

MI_S32 ST_OSD_Update(MI_RGN_HANDLE hHandle);

MI_S32 ST_OSD_Create(MI_RGN_HANDLE hHandle, MI_RGN_Attr_t *pstRegion);

MI_S32 ST_OSD_Destroy(MI_RGN_HANDLE hHandle);

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif //_ST_RGN_H_

