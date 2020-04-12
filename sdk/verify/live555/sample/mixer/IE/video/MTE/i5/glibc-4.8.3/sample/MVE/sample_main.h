/*
* sample_main.h- Sigmastar
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
#ifndef _SAMPLE_MAIN_H_
#define _SAMPLE_MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "mi_mve.h"

typedef struct
{
    int sysfd;
    int memfd;
} BUFFER_MANAGER_HANDLE;

MI_RET Sample_InitInputImage(MVE_SRC_IMAGE_S *image, const char *file_name);
MI_RET Sample_InitInputImageEx(MVE_SRC_IMAGE_S *image, int file_handle);
MI_RET Sample_SaveOutputImage(MVE_DST_IMAGE_S *image, const char *file_name);
MI_RET Sample_SaveOutputImageEx(MVE_DST_IMAGE_S *image, int file_handle);

MI_RET Sample_InitInputBuffer(MVE_MEM_INFO_S *buffer, const char *file_name, int size);
MI_RET Sample_InitInputBufferEx(MVE_MEM_INFO_S *buffer, int file_handle, int size);
MI_RET Sample_SaveOutputBuffer(MVE_MEM_INFO_S *buffer, const char *file_name, int size);
MI_RET Sample_SaveOutputBufferEx(MVE_MEM_INFO_S *buffer, int file_handle, int size);
MI_RET Sample_SaveOutputBufferDirect(void *buffer, const char *file_name, int size);

MI_RET Sample_Filter(void);
MI_RET Sample_CSC(void);
MI_RET Sample_FilterAndCsc(void);
MI_RET Sample_Sobel(void);
MI_RET Sample_MagAndAng(void);
MI_RET Sample_Dilate(void);
MI_RET Sample_Erode(void);
MI_RET Sample_Thresh(void);
MI_RET Sample_And(void);
MI_RET Sample_Sub(void);
MI_RET Sample_Or(void);
MI_RET Sample_Xor(void);
MI_RET Sample_Add(void);
MI_RET Sample_Thresh_U16(void);
MI_RET Sample_Thresh_S16(void);
MI_RET Sample_16BitTo8Bit(void);
MI_RET Sample_OrdStatFilter(void);
MI_RET Sample_Bernsen(void);
MI_RET Sample_Map(void);
MI_RET Sample_NCC(void);
MI_RET Sample_Integ(void);
MI_RET Sample_CCL(void);
MI_RET Sample_Hist(void);
MI_RET Sample_EqualizeHist(void);
MI_RET Sample_SAD(void);
MI_RET Sample_NormGrad(void);
MI_RET Sample_LBP(void);
MI_RET Sample_GMM(void);
MI_RET Sample_Canny(void);
MI_RET Sample_LineFilterHor(void);
MI_RET Sample_NoiseRemoveHor(void);
MI_RET Sample_Adpthresh(void);
MI_RET Sample_Lk_Optical_Flow(void);

#endif // _SAMPLE_MAIN_H_
