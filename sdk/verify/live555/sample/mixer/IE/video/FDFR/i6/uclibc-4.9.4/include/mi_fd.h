/*
* mi_fd.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: XXXX <XXXX@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#ifndef __MI_FD_H__
#define __MI_FD_H__

#include "mid_common.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum _MI_RET_E
{
    /*MI_FDFR*/
    MI_FDFR_RET_SUCCESS                     = 0x00000000, /* FDFR API execution success */
    MI_FDFR_RET_INIT_ERROR                  = 0x10000601,   /*FDFR init error*/
    MI_FDFR_RET_IC_CHECK_ERROR              = 0x10000602,   /*Incorrect platform check for FDFR*/
    MI_FDFR_RET_INVALID_HANDLE              = 0x10000603,   /*Invalid FDFR handle*/
    MI_FDFR_RET_INVALID_PARAMETER           = 0x10000604,   /*Invalid FDFR parameter*/
    MI_FDFR_RET_FD_ENABLE_ERROR             = 0x10000605,   /*FD enable error*/
    MI_FDFR_RET_FR_ENABLE_ERROR             = 0x10000606,   /*FR enable error*/
    MI_FDFR_RET_FR_GET_FEATURE_DATA_ERROR   = 0x10000607,   /*FR get feature data error*/
    MI_FDFR_RET_FR_SET_FEATURE_DATA_ERROR   = 0x10000608,   /*FR set feature data error*/
    MI_FDFR_RET_IN_IMAGE_ERROR              = 0x10000609   /*Input image error*/
} MI_RET;


typedef enum
{
    FD_OPTION_DETECT_MODE   ,
    FD_OPTION_FACE_WIDTH    ,
    FD_OPTION_PARTIAL_WIDTH ,
    FD_OPTION_FACE_DIRECTION,
    FD_OPTION_NUM
} FDOption_e;

typedef     enum
{
    FULL_MODE = 0,
    TRACK_MODE,
    PARTIAL_MODE,
    NUM_MODE
} FDDetectMode_e;

typedef struct
{
    MI_S32 store_idx;
    MI_U32 percentage;
    MI_S8 name[32];
} FRFaceInfo_t;

typedef struct
{
    MI_S16    posx;
    MI_S16    posy;
    MI_S16    posw;
    MI_S16    posh;
	MI_S16    rotate;
	MI_S16    conf;
} FacePos_t;

typedef struct _MI_FDFR_RECT
{
    MI_S16 sx;
    MI_S16 sy;
    MI_S16 ex;
    MI_S16 ey;
} MI_FDFR_RECT;

typedef void* FD_HANDLE;
typedef void* FR_HANDLE;
#define FD_NUM_OF_FACE_UNIT (10)
#define FR_UNKNOWN_NAME "UNKNOWN NAME"
/***********************************FD API define  *****************************************/
/**
 * 初始化 FD 库
 * inImgW:  输入源宽
 * inImhH:  输入源高
 * return:  FD 的 handle句柄
 */
FD_HANDLE MI_FD_Init(MI_S32 inImgW, MI_S32 inImhH);

/**
 * 退出库函数，释放内存。
 * fdHandle:    FD 的 handle 句柄
 * return:  无
 */
void MI_FD_Uninit(FD_HANDLE fdHandle);

/**
 * 使能脸部检测
 * fdHandle:    FD 的 handle 句柄
 * bEnable:     使能标志
 * return:      MI_FDFR_RET_SUCCESS成功，MI_FDFR_RET_FAIL失败
 */
MI_RET MI_FD_EnableFD(FD_HANDLE fdHandle, MI_S32 bEnable);

/**
 * 设置脸部识别参数
 * fdHandle:    FD 的 handle 句柄
 * opt:         参数选项
 * val:         参数值
 * return:      MI_FDFR_RET_SUCCESS成功，MI_FDFR_RET_FAIL失败
 */
MI_RET MI_FD_SetOption(FD_HANDLE fdHandle, FDOption_e opt, MI_S32 val);

/**
 * 开始处理脸部识别图像帧
 * fdHandle:    FD 的 handle 句柄
 * yFrame:      图像 YUV 中的 y分量数据
 * return:      MI_FDFR_RET_SUCCESS成功，MI_FDFR_RET_FAIL失败
 */
MI_S32 MI_FD_Run(FD_HANDLE fdHandle, const MI_U8 *yFrame);

/**
 * 获取脸部检测结果
 * fdHandle:    FD 的 handle 句柄
 * out_facepos: 脸部位置结构体
 * return:      MI_FDFR_RET_SUCCESS成功，MI_FDFR_RET_FAIL失败
 */
MI_S32 MI_FD_GetFaceInfo(FD_HANDLE fdHandle, FacePos_t **out_facepos);


/***********************************FR API define  *****************************************/
/**
 * 初始化 FR 库
 * inImgW:  输入源宽
 * inImhH:  输入源高
 * return:  FD 的 handle句柄
 */
FR_HANDLE MI_FR_Init(MI_S32 inImgW, MI_S32 inImgH);

/**
 * 退出库函数，释放内存。
 * frHandle:    FR 的 handle 句柄
 * return:  无
 */
void MI_FR_Uninit(FR_HANDLE frHandle);

/**
 * 使能脸部识别
 * fdHandle:    FR 的 handle 句柄
 * bEnable:     使能标志
 * return:      MI_FDFR_RET_SUCCESS成功，MI_FDFR_RET_FAIL失败
 */
MI_RET MI_FR_EnableFR(FR_HANDLE frHandle, MI_S32 bEnable);

/**
 * 获取人脸识别的结果信息
 * fdHandle:    FR 的 handle 句柄
 * fd_num:      最大可获取的人脸信息数，不能超过FD_NUM_OF_FACE_UNIT
 * faceInfo:    人脸信息数组指针
 * return:      MI_FDFR_RET_SUCCESS成功，MI_FDFR_RET_FAIL失败
 */
MI_S32 MI_FR_GetFRInfo(FR_HANDLE frHandle, int FD_NUM, FRFaceInfo_t *faceInfo);

/**
 * 设置人脸特征信息到数据库
 * idx:    人脸数据在数据库的索引号
 * feat_data:     人脸特征数据
 * feat_name:    人脸特征的名字
 * return:      MI_FDFR_RET_SUCCESS成功，MI_FDFR_RET_FAIL失败
 */
MI_RET MI_FR_SetFeatureData(MI_S16 idx, MI_S8* feat_data, MI_S8* feat_name);

/**
 * 获取数据库中的人脸特征
 * idx:    人脸特征在数据库的索引号
 * feat_data:     人脸特征数据
 * feat_name:    人脸特征的名字
 * return:      MI_FDFR_RET_SUCCESS成功，MI_FDFR_RET_FAIL失败
 */
MI_RET MI_FR_GetFeatureData(MI_S16 idx, MI_S8* feat_data, MI_S8* feat_name);

/**
 * 获取人脸特征数据的大小
 * return:      每个人脸特征数据的大小
 */
MI_S32 MI_FR_GetFeatureSizes();

/**
 * 从pgm格式的图片生成人脸特征
 * filename:    图片文件名
 * store_idx:   人脸特征在数据库的索引号
 * return:      MI_FDFR_RET_SUCCESS成功，MI_FDFR_RET_FAIL失败
 */
MI_RET MI_FR_CalFeatureFromImg(MI_S8* filename, MI_S32 store_idx);

/**
 * 从图像YUV中的y分量数据生成人脸特征
 * imgPtr:  y分量数据buffer的指针
 * width:    图像的宽度
 * height:   图像的高度
 * store_idx:   人脸特征在数据库的索引号
 * return:      MI_FDFR_RET_SUCCESS成功，MI_FDFR_RET_FAIL失败
 */
MI_RET MI_FR_CalFeatureFromRawY(MI_U8* imgPtr, MI_S32 width, MI_S32 height, MI_S32 store_idx);

/**
 * 从图像YUV中的y分量计算图像分数
 * imgPtr:  y分量数据buffer的指针
 * width:    图像的宽度
 * height:   图像的高度
 * prect_img_center:   计算图片中的位置
 * return:   图像分数
 */
MI_S32 MI_FR_CalcImageScore(MI_U8* imgPtr, MI_S32 width, MI_S32 height, MI_FDFR_RECT* prect_img_center);

/**
 * Set FR mode.
 * mode
     0 : Low accuracy but speed is fast.
     1 : Middle accuracy but speed is middle.
     2 : High accuracy but speed is slow. (default)
 * return:    MI_FDFR_RET_SUCCESS成功，MI_FDFR_RET_FAIL失败
 */
MI_S32 MI_FR_SetFrMode(MI_S32 mode);

/**
 * To set a threshold to decide whether do FR or not.
 * face_width
  Setting a threshold, If the Face width of Face is bigger than this threshold,
  the process will do FR. Otherwise, the process will still do FD.
 * return:    MI_FDFR_RET_SUCCESS成功，MI_FDFR_RET_FAIL失败
 */
MI_S32 MI_FR_SetFrFaceWidth(MI_S32 face_width);

#ifdef __cplusplus
}
#endif

#endif //__MI_FD_H__
