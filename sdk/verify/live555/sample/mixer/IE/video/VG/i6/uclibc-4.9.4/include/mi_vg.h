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
/*
 *      mi_vg.h
 *      Author: chiao.su
 */

#ifndef MI_VG_H_
#define MI_VG_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <mid_common.h>
#include <stdint.h>

typedef struct _VG_Point_t
{
	S32 x;
	S32 y;
} MI_VG_Point_t;

/**
 * VG_line
 */
typedef struct _VG_Line_t
{
	MI_VG_Point_t px;
	MI_VG_Point_t py;
	MI_VG_Point_t pdx;
	MI_VG_Point_t pdy;
} MI_VgLine_t;

typedef struct _MI_VgSet_t
{
    //Common Information
	U16 object_size_thd;
    U16 line_number;
    U8 indoor;

    //First Line
    U16 fx;
    U16 fy;
    U16 sx;
    U16 sy;
    U16 fdx;
    U16 fdy;
    U16 sdx;
    U16 sdy;

    //Second Line
    U16 sfx;
    U16 sfy;
    U16 ssx;
    U16 ssy;
    U16 sfdx;
    U16 sfdy;
    U16 ssdx;
    U16 ssdy;
} MI_VgSet_t;

typedef struct _MI_VgResult_t
{
	U16 alarm1;
	U16 alarm2;
} MI_VgResult_t;

typedef struct _MI_VgDebug_t
{
    //Common Information
	U16 background_state;
	U32 version;  //Modify date
	U32 debug_object_size;
	U32 debug_state;

    //First Line
    U16 debug_fx;
    U16 debug_fy;
    U16 debug_sx;
    U16 debug_sy;
    U16 debug_fdx;
    U16 debug_fdy;
    U16 debug_sdx;
    U16 debug_sdy;

    //Second Line
    U16 debug_sfx;
    U16 debug_sfy;
    U16 debug_ssx;
    U16 debug_ssy;
    U16 debug_sfdx;
    U16 debug_sfdy;
    U16 debug_ssdx;
    U16 debug_ssdy;

    U16 debug_fsp_x;
    U16 debug_fsp_y;
    U16 debug_fep_x;
    U16 debug_fep_y;
    U16 debug_ssp_x;
    U16 debug_ssp_y;
    U16 debug_sep_x;
    U16 debug_sep_y;
} MI_VgDebug_t;

typedef  void*  MI_VG_HANDLE;

// Don't change the value of VG_MAX_LINE_NUM
#define VG_MAX_LINE_NUM     (2)      /* Max numbers of Lines which can be set */
#define VG_MAX_SCENE_NUM    (2)      /* Max numbers of Scene which can be set */
#define VG_MAX_OBJ_SIZE_THD (100)    /* Max percent of object size which can be set */

MI_VG_HANDLE MI_VG_Init(MI_VgSet_t* vg_user_info, U16 width, U16 height);

void MI_VG_Uninit(MI_VG_HANDLE vg_handle);

MI_RET MI_VG_SetScene(MI_VgSet_t* vg_user_info, S8 scene);

MI_RET MI_VG_GetScene(MI_VgSet_t* vg_user_info, S8* scene);

MI_RET MI_VG_SetLineNumber(MI_VgSet_t* vg_user_info, U16 lineno);

MI_RET MI_VG_GetLineNumber(MI_VgSet_t* vg_user_info, U16* lineno);

MI_RET MI_VG_SetLineAndDir(MI_VgSet_t* vg_user_info, MI_VgLine_t* line_coordinate, U16 lineno);

MI_RET MI_VG_GetLineAndDir(MI_VgSet_t* vg_user_info, MI_VgLine_t* line_coordinate, U16 lineno);

MI_RET MI_VG_SetObjSizeThd(MI_VgSet_t* vg_user_info, U16 size_thd);

MI_RET MI_VG_GetObjSizeThd(MI_VgSet_t* vg_user_info, U16* size_thd);

MI_RET MI_VG_Run(MI_VG_HANDLE vg_handle, U8* _ucMask);

MI_RET MI_VG_GetResult(MI_VG_HANDLE vg_handle, MI_VgResult_t *cross_alarm);

MI_RET MI_VG_GetDebugInfo(MI_VG_HANDLE vg_handle, MI_VgDebug_t *debug_info);


#ifdef __cplusplus
}
#endif

#endif /* MI_VG_H_ */
