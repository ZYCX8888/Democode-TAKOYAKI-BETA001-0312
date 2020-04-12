#ifndef _SAMPLE_MAIN_H_
#define _SAMPLE_MAIN_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "mi_ive.h"

#define RESULT_PATH "./result/"

MI_S32 ModuleTest_AllocateImage(MI_IVE_Image_t *pstImage, MI_IVE_ImageType_e enImageType, MI_U16 u16Stride, MI_U16 u16Width, MI_U16 u16Height);
MI_S32 ModuleTest_FreeImage(MI_IVE_Image_t *pstImage);
MI_S32 ModuleTest_AllocateBuffer(MI_IVE_MemInfo_t *pstBuffer, MI_U32 u32Size);
MI_S32 ModuleTest_FreeBuffer(MI_IVE_MemInfo_t *pstBuffer);

MI_S32 ModuleTest_InitInputImage(MI_IVE_SrcImage_t *image, const char *file_name);
MI_S32 ModuleTest_InitInputImageEx(MI_IVE_SrcImage_t *image, int file_handle);
MI_S32 ModuleTest_SaveOutputImage(MI_IVE_DstImage_t *image, const char *file_name);
MI_S32 ModuleTest_SaveOutputImageEx(MI_IVE_DstImage_t *image, int file_handle);

MI_S32 ModuleTest_InitInputBuffer(MI_IVE_MemInfo_t *buffer, const char *file_name, int size);
MI_S32 ModuleTest_InitInputBufferEx(MI_IVE_MemInfo_t *buffer, int file_handle, int size);
MI_S32 ModuleTest_SaveOutputBuffer(MI_IVE_MemInfo_t *buffer, const char *file_name, int size);
MI_S32 ModuleTest_SaveOutputBufferEx(MI_IVE_MemInfo_t *buffer, int file_handle, int size);

MI_S32 ModuleTest_Filter(void);
MI_S32 ModuleTest_CSC(void);
MI_S32 ModuleTest_FilterAndCsc(void);
MI_S32 ModuleTest_Sobel(void);
MI_S32 ModuleTest_MagAndAng(void);
MI_S32 ModuleTest_Dilate(void);
MI_S32 ModuleTest_Erode(void);
MI_S32 ModuleTest_Thresh(void);
MI_S32 ModuleTest_And(void);
MI_S32 ModuleTest_Sub(void);
MI_S32 ModuleTest_Or(void);
MI_S32 ModuleTest_Xor(void);
MI_S32 ModuleTest_Add(void);
MI_S32 ModuleTest_Thresh_U16(void);
MI_S32 ModuleTest_Thresh_S16(void);
MI_S32 ModuleTest_16BitTo8Bit(void);
MI_S32 ModuleTest_OrdStatFilter(void);
MI_S32 ModuleTest_Map(void);
MI_S32 ModuleTest_NCC(void);
MI_S32 ModuleTest_Integ(void);
MI_S32 ModuleTest_CCL(void);
MI_S32 ModuleTest_Hist(void);
MI_S32 ModuleTest_EqualizeHist(void);
MI_S32 ModuleTest_SAD(void);
MI_S32 ModuleTest_NormGrad(void);
MI_S32 ModuleTest_LBP(void);
MI_S32 ModuleTest_GMM(void);
MI_S32 ModuleTest_Canny(void);
MI_S32 ModuleTest_Lk_Optical_Flow(void);

#endif // _SAMPLE_MAIN_H_
