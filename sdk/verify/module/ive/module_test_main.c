#include "module_test_main.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

MI_S32 ModuleTest_AllocateImage(MI_IVE_Image_t      *pstImage,
                            MI_IVE_ImageType_e  eImageType,
                            MI_U16              u16Stride,
                            MI_U16              u16Width,
                            MI_U16              u16Height)
{
    MI_S32 ret = MI_SUCCESS;
    int size, i;

    memset(pstImage, 0, sizeof(MI_IVE_Image_t));

    pstImage->eType = eImageType;
    pstImage->u16Width  = u16Width;
    pstImage->u16Height = u16Height;

    switch(eImageType) {
        // 64bit Gray
        case E_MI_IVE_IMAGE_TYPE_S64C1:
        case E_MI_IVE_IMAGE_TYPE_U64C1:
            size = u16Stride * u16Height * sizeof(MI_U64);
            pstImage->aphyPhyAddr[0] = (uintptr_t)(pstImage->apu8VirAddr[0] = malloc(size));
            pstImage->azu16Stride[0] = u16Stride;
            if (pstImage->apu8VirAddr[0] == NULL)
            {
                goto ERROR;
            }
            break;

        // 32bit Gray
        case E_MI_IVE_IMAGE_TYPE_S32C1:
        case E_MI_IVE_IMAGE_TYPE_U32C1:
            size = u16Stride * u16Height * sizeof(MI_U32);
            pstImage->aphyPhyAddr[0] = (uintptr_t)(pstImage->apu8VirAddr[0] = malloc(size));
            pstImage->azu16Stride[0] = u16Stride;
            if (pstImage->apu8VirAddr[0] == NULL)
            {
                goto ERROR;
            }
            break;

        // 16bit Gray
        case E_MI_IVE_IMAGE_TYPE_S16C1:
        case E_MI_IVE_IMAGE_TYPE_U16C1:
            size = u16Stride * u16Height * sizeof(MI_U16);
            pstImage->aphyPhyAddr[0] = (uintptr_t)(pstImage->apu8VirAddr[0] = malloc(size));
            pstImage->azu16Stride[0] = u16Stride;
            if (pstImage->apu8VirAddr[0] == NULL)
            {
                goto ERROR;
            }
            break;

        // 8bit Gray
        case E_MI_IVE_IMAGE_TYPE_S8C1:
        case E_MI_IVE_IMAGE_TYPE_U8C1:
            size = u16Stride * u16Height * sizeof(MI_U8);
            pstImage->aphyPhyAddr[0] = (uintptr_t)(pstImage->apu8VirAddr[0] = malloc(size));
            pstImage->azu16Stride[0] = u16Stride;
            if (pstImage->apu8VirAddr[0] == NULL)
            {
                goto ERROR;
            }
            break;

        // YUV 420 semi plane
        case E_MI_IVE_IMAGE_TYPE_YUV420SP:
            size = u16Stride * u16Height * sizeof(MI_U8);
            pstImage->aphyPhyAddr[0] = (uintptr_t)(pstImage->apu8VirAddr[0] = malloc(size));
            pstImage->aphyPhyAddr[1] = (uintptr_t)(pstImage->apu8VirAddr[1] = malloc(size/2));
            pstImage->azu16Stride[0] = u16Stride;
            pstImage->azu16Stride[1] = u16Stride;
            if (pstImage->apu8VirAddr[0] == NULL || pstImage->apu8VirAddr[1] == NULL)
            {
                goto ERROR;
            }
            break;

        // YUV 422 semi plane
        case E_MI_IVE_IMAGE_TYPE_YUV422SP:
            size = u16Stride * u16Height * sizeof(MI_U8);
            pstImage->aphyPhyAddr[0] = (uintptr_t)(pstImage->apu8VirAddr[0] = malloc(size));
            pstImage->aphyPhyAddr[1] = (uintptr_t)(pstImage->apu8VirAddr[1] = malloc(size));
            pstImage->azu16Stride[0] = u16Stride;
            pstImage->azu16Stride[1] = u16Stride;
            if (pstImage->apu8VirAddr[0] == NULL || pstImage->apu8VirAddr[1] == NULL)
            {
                goto ERROR;
            }
            break;

        // RGB packed
        case E_MI_IVE_IMAGE_TYPE_U8C3_PACKAGE:
            size = u16Stride * u16Height * sizeof(MI_U8) * 3;
            pstImage->aphyPhyAddr[0] = (uintptr_t)(pstImage->apu8VirAddr[0] = malloc(size));
            pstImage->azu16Stride[0] = u16Stride;
            if (pstImage->apu8VirAddr[0] == NULL)
            {
                goto ERROR;
            }
            break;

        // RGB plane
        case E_MI_IVE_IMAGE_TYPE_U8C3_PLANAR:
            size = u16Stride * u16Height * sizeof(MI_U8);
            pstImage->aphyPhyAddr[0] = (uintptr_t)(pstImage->apu8VirAddr[0] = malloc(size));
            pstImage->aphyPhyAddr[1] = (uintptr_t)(pstImage->apu8VirAddr[1] = malloc(size));
            pstImage->aphyPhyAddr[2] = (uintptr_t)(pstImage->apu8VirAddr[2] = malloc(size));
            pstImage->azu16Stride[0] = u16Stride;
            pstImage->azu16Stride[1] = u16Stride;
            pstImage->azu16Stride[2] = u16Stride;
            if (pstImage->apu8VirAddr[0] == NULL || pstImage->apu8VirAddr[1] == NULL || pstImage->apu8VirAddr[2] == NULL)
            {
                goto ERROR;
            }
            break;

        case E_MI_IVE_IMAGE_TYPE_S8C2_PACKAGE:
            size = u16Stride * u16Height * sizeof(MI_U8) * 2;
            pstImage->aphyPhyAddr[0] = (uintptr_t)(pstImage->apu8VirAddr[0] = malloc(size));
            pstImage->azu16Stride[0] = u16Stride;
            if (pstImage->apu8VirAddr[0] == NULL)
            {
                goto ERROR;
            }
            break;

        case E_MI_IVE_IMAGE_TYPE_S8C2_PLANAR:
            size = u16Stride * u16Height * sizeof(MI_U8);
            pstImage->aphyPhyAddr[0] = (uintptr_t)(pstImage->apu8VirAddr[0] = malloc(size));
            pstImage->aphyPhyAddr[1] = (uintptr_t)(pstImage->apu8VirAddr[1] = malloc(size));
            pstImage->azu16Stride[0] = u16Stride;
            pstImage->azu16Stride[1] = u16Stride;
            if (pstImage->apu8VirAddr[0] == NULL || pstImage->apu8VirAddr[1] == NULL)
            {
                goto ERROR;
            }
            break;

        default:
            printf("Format is not support!!\n");
            return MI_IVE_ERR_ILLEGAL_PARAM;
    }

    return ret;

ERROR:
    for (i=0; i<3; i++)
    {
        if(pstImage->apu8VirAddr[i] != NULL)
        {
            free(pstImage->apu8VirAddr[i]);
        }
    }

    memset(pstImage, 0, sizeof(MI_IVE_Image_t));

    return MI_IVE_ERR_NOMEM;
}

MI_S32 ModuleTest_FreeImage(MI_IVE_Image_t *pstImage)
{
    MI_S32 ret = MI_SUCCESS;

    switch(pstImage->eType) {
        case E_MI_IVE_IMAGE_TYPE_S64C1:
        case E_MI_IVE_IMAGE_TYPE_U64C1:
        case E_MI_IVE_IMAGE_TYPE_S32C1:
        case E_MI_IVE_IMAGE_TYPE_U32C1:
        case E_MI_IVE_IMAGE_TYPE_S16C1:
        case E_MI_IVE_IMAGE_TYPE_U16C1:
        case E_MI_IVE_IMAGE_TYPE_S8C1:
        case E_MI_IVE_IMAGE_TYPE_U8C1:
        case E_MI_IVE_IMAGE_TYPE_U8C3_PACKAGE:
        case E_MI_IVE_IMAGE_TYPE_S8C2_PACKAGE:
            free(pstImage->apu8VirAddr[0]);
            break;

        case E_MI_IVE_IMAGE_TYPE_YUV420SP:
        case E_MI_IVE_IMAGE_TYPE_YUV422SP:
        case E_MI_IVE_IMAGE_TYPE_S8C2_PLANAR:
            free(pstImage->apu8VirAddr[0]);
            free(pstImage->apu8VirAddr[1]);
            break;

        case E_MI_IVE_IMAGE_TYPE_U8C3_PLANAR:
            free(pstImage->apu8VirAddr[0]);
            free(pstImage->apu8VirAddr[1]);
            free(pstImage->apu8VirAddr[2]);
            break;

        default:
            printf("Format is not support!!\n");
            ret = MI_IVE_ERR_ILLEGAL_PARAM;
    }

    memset(pstImage, 0, sizeof(MI_IVE_Image_t));

    return ret;
}

MI_S32 ModuleTest_AllocateBuffer(MI_IVE_MemInfo_t *pstBuffer, MI_U32 u32Size)
{
    MI_S32 ret = MI_SUCCESS;

    memset(pstBuffer, 0, sizeof(MI_IVE_MemInfo_t));

    pstBuffer->u32Size = u32Size;
    pstBuffer->phyPhyAddr = (uintptr_t)(pstBuffer->pu8VirAddr = malloc(u32Size));
    if (pstBuffer->pu8VirAddr == NULL)
    {
        ret = MI_IVE_ERR_NOMEM;
    }

    return ret;
}

MI_S32 ModuleTest_FreeBuffer(MI_IVE_MemInfo_t *pstBuffer)
{
    MI_S32 ret = MI_SUCCESS;

    free(pstBuffer->pu8VirAddr);

    memset(pstBuffer, 0, sizeof(MI_IVE_MemInfo_t));

    return ret;
}




MI_S32 ModuleTest_InitInputImageEx(MI_IVE_SrcImage_t *image, int file_handle)
{
    int size;

    switch(image->eType) {
        // Gray
        case E_MI_IVE_IMAGE_TYPE_U8C1:
            size   = image->azu16Stride[0] * image->u16Height * sizeof(MI_U8);
            read(file_handle, (char*)(image->apu8VirAddr[0]), size);
            break;

        case E_MI_IVE_IMAGE_TYPE_U16C1:
        case E_MI_IVE_IMAGE_TYPE_S16C1:
            size   = image->azu16Stride[0] * image->u16Height * sizeof(MI_U16);
            read(file_handle, (char*)(image->apu8VirAddr[0]), size);
            break;

        // YUV 420 semi plane
        case E_MI_IVE_IMAGE_TYPE_YUV420SP:
            size   = image->azu16Stride[0] * image->u16Height * sizeof(MI_U8);
            read(file_handle, (char*)(image->apu8VirAddr[0]), size);
            read(file_handle, (char*)(image->apu8VirAddr[1]), size/2);
            break;

        // YUV 422 semi plane
        case E_MI_IVE_IMAGE_TYPE_YUV422SP:
            size   = image->azu16Stride[0] * image->u16Height * sizeof(MI_U8);
            read(file_handle, (char*)(image->apu8VirAddr[0]), size);
            read(file_handle, (char*)(image->apu8VirAddr[1]), size);
            break;

        // RGB packed
        case E_MI_IVE_IMAGE_TYPE_U8C3_PACKAGE:
            size   = image->azu16Stride[0] * image->u16Height * sizeof(MI_U8) * 3;
            read(file_handle, (char*)(image->apu8VirAddr[0]), size);
            break;

        // RGB plane
        case E_MI_IVE_IMAGE_TYPE_U8C3_PLANAR:
            size   = image->azu16Stride[0] * image->u16Height * sizeof(MI_U8);
            read(file_handle, (char*)(image->apu8VirAddr[0]), size);
            read(file_handle, (char*)(image->apu8VirAddr[1]), size);
            read(file_handle, (char*)(image->apu8VirAddr[2]), size);
            break;

        default:
            printf("Unimplemented format %X\n", image->eType);
            return MI_IVE_ERR_ILLEGAL_PARAM;
    }

    return MI_SUCCESS;
}

MI_S32 ModuleTest_InitInputImage(MI_IVE_SrcImage_t *image, const char *file_name)
{
    int size, file_handle;
    MI_S32 ret = MI_SUCCESS;

    file_handle = open(file_name, O_RDONLY);
    if (file_handle <= 0)
    {
        printf("Can't open file_handle %s (%d: %s)\n", file_name, errno, strerror(errno));
        return MI_IVE_ERR_ILLEGAL_PARAM;
    }

    ret = ModuleTest_InitInputImageEx(image, file_handle);

    close(file_handle);

    return ret;
}

MI_S32 ModuleTest_SaveOutputImageEx(MI_IVE_DstImage_t *image, int file_handle)
{
    int size;

    // write to file_handle
    switch(image->eType)
    {
        case E_MI_IVE_IMAGE_TYPE_S64C1:
        case E_MI_IVE_IMAGE_TYPE_U64C1:
            size = image->azu16Stride[0]*image->u16Height*sizeof(MI_U64);
            write(file_handle, (char*)(image->apu8VirAddr[0]), size);
            break;

        case E_MI_IVE_IMAGE_TYPE_S32C1:
        case E_MI_IVE_IMAGE_TYPE_U32C1:
            size = image->azu16Stride[0]*image->u16Height*sizeof(MI_U32);
            write(file_handle, (char*)(image->apu8VirAddr[0]), size);
            break;

        case E_MI_IVE_IMAGE_TYPE_S16C1:
        case E_MI_IVE_IMAGE_TYPE_U16C1:
            size = image->azu16Stride[0]*image->u16Height*sizeof(MI_U16);
            write(file_handle, (char*)(image->apu8VirAddr[0]), size);
            break;

        case E_MI_IVE_IMAGE_TYPE_S8C1:
        case E_MI_IVE_IMAGE_TYPE_U8C1:
            size = image->azu16Stride[0]*image->u16Height*sizeof(MI_U8);
            write(file_handle, (char*)(image->apu8VirAddr[0]), size);
            break;

        case E_MI_IVE_IMAGE_TYPE_YUV420SP:
            size = image->azu16Stride[0]*image->u16Height*sizeof(MI_U8);
            write(file_handle, (char*)(image->apu8VirAddr[0]), size);
            write(file_handle, (char*)(image->apu8VirAddr[1]), size/2);
            break;

        case E_MI_IVE_IMAGE_TYPE_YUV422SP:
            size = image->azu16Stride[0]*image->u16Height*sizeof(MI_U8);
            write(file_handle, (char*)(image->apu8VirAddr[0]), size);
            write(file_handle, (char*)(image->apu8VirAddr[1]), size);
            break;

        case E_MI_IVE_IMAGE_TYPE_U8C3_PACKAGE:
            size = image->azu16Stride[0]*image->u16Height*sizeof(MI_U8) * 3;
            write(file_handle, (char*)(image->apu8VirAddr[0]), size);
            break;

        case E_MI_IVE_IMAGE_TYPE_U8C3_PLANAR:
            size = image->azu16Stride[0]*image->u16Height*sizeof(MI_U8);
            write(file_handle, (char*)(image->apu8VirAddr[0]), size);
            write(file_handle, (char*)(image->apu8VirAddr[1]), size);
            write(file_handle, (char*)(image->apu8VirAddr[2]), size);
            break;

        case E_MI_IVE_IMAGE_TYPE_S8C2_PACKAGE:
            size = image->azu16Stride[0]*image->u16Height*sizeof(MI_U8) * 2;
            write(file_handle, (char*)(image->apu8VirAddr[0]), size);
            break;

        case E_MI_IVE_IMAGE_TYPE_S8C2_PLANAR:
            size = image->azu16Stride[0]*image->u16Height*sizeof(MI_U8);
            write(file_handle, (char*)(image->apu8VirAddr[0]), size);
            write(file_handle, (char*)(image->apu8VirAddr[1]), size);
            break;

        default:
            printf("Format is not support!!\n");
            return MI_IVE_ERR_ILLEGAL_PARAM;
    }

    return MI_SUCCESS;
}

MI_S32 ModuleTest_SaveOutputImage(MI_IVE_DstImage_t *image, const char *file_name)
{
    MI_S32 ret;
    int file_handle;

    file_handle = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (file_handle <= 0)
    {
        printf("Can't open file_handle %s (%d: %s)\n", file_name, errno, strerror(errno));
        return MI_IVE_ERR_ILLEGAL_PARAM;
    }

    ret = ModuleTest_SaveOutputImageEx(image, file_handle);

    close(file_handle);

    return ret;
}

MI_S32 ModuleTest_InitInputBufferEx(MI_IVE_MemInfo_t *buffer, int file_handle, int size)
{
    read(file_handle, (char*)(buffer->pu8VirAddr), size);

    return MI_SUCCESS;
}

MI_S32 ModuleTest_InitInputBuffer(MI_IVE_MemInfo_t *buffer, const char *file_name, int size)
{
    int file_handle;
    MI_S32 ret = MI_SUCCESS;

    file_handle = open(file_name, O_RDONLY);
    if (file_handle <= 0)
    {
        printf("Can't open file_handle %s (%d: %s)\n", file_name, errno, strerror(errno));
        return MI_IVE_ERR_ILLEGAL_PARAM;
    }

    ret = read(file_handle, (char*)(buffer->pu8VirAddr), size);

    close(file_handle);

    return ret;
}

MI_S32 ModuleTest_SaveOutputBufferEx(MI_IVE_MemInfo_t *buffer, int file_handle, int size)
{
    write(file_handle, buffer->pu8VirAddr, size);

    return MI_SUCCESS;
}

MI_S32 ModuleTest_SaveOutputBuffer(MI_IVE_MemInfo_t *buffer, const char *file_name, int size)
{
    MI_S32 ret;
    int file_handle;

    file_handle = open(file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);

    if (file_handle <= 0)
    {
        printf("Can't open file_handle %s (%d: %s)\n", file_name, errno, strerror(errno));
        return MI_IVE_ERR_ILLEGAL_PARAM;
    }

    ret = ModuleTest_SaveOutputBufferEx(buffer, file_handle, size);

    close(file_handle);

    return ret;
}

int main(int argc, char **argv)
{
    printf("Run ModuleTest_Filter()\n");
    if (ModuleTest_Filter() != MI_SUCCESS)
    {
        printf("IVE filter process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_CSC()\n");
    if (ModuleTest_CSC() != MI_SUCCESS)
    {
        printf("IVE CSC process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_FilterAndCSC()\n");
    if (ModuleTest_FilterAndCSC() != MI_SUCCESS)
    {
        printf("IVE filter CSC process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_Sobel()\n");
    if (ModuleTest_Sobel() != MI_SUCCESS)
    {
        printf("IVE Sobel process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_MagAndAng()\n");
    if (ModuleTest_MagAndAng() != MI_SUCCESS)
    {
        printf("IVE Mag and Ang process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_Dilate()\n");
    if (ModuleTest_Dilate() != MI_SUCCESS)
    {
        printf("IVE Dilate process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_Erode()\n");
    if (ModuleTest_Erode() != MI_SUCCESS)
    {
        printf("IVE Erode process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_Thresh()\n");
    if (ModuleTest_Thresh() != MI_SUCCESS)
    {
        printf("IVE Thresh process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_And()\n");
    if (ModuleTest_And() != MI_SUCCESS)
    {
        printf("IVE And process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_Sub()\n");
    if (ModuleTest_Sub() != MI_SUCCESS)
    {
        printf("IVE Sub process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_Or()\n");
    if (ModuleTest_Or() != MI_SUCCESS)
    {
        printf("IVE Or process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_Xor()\n");
    if (ModuleTest_Xor() != MI_SUCCESS)
    {
        printf("IVE Xor process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_Add()\n");
    if (ModuleTest_Add() != MI_SUCCESS)
    {
        printf("IVE Add process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_Thresh_U16()\n");
    if (ModuleTest_Thresh_U16() != MI_SUCCESS)
    {
        printf("IVE Thresh_U16 process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_Thresh_S16()\n");
    if (ModuleTest_Thresh_S16() != MI_SUCCESS)
    {
        printf("IVE Thresh_S16 process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_16BitTo8Bit()\n");
    if (ModuleTest_16BitTo8Bit() != MI_SUCCESS)
    {
        printf("IVE 16bit to 8bit process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_OrdStatFilter()\n");
    if (ModuleTest_OrdStatFilter() != MI_SUCCESS)
    {
        printf("IVE Order Statistic Filter process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_Map()\n");
    if (ModuleTest_Map() != MI_SUCCESS)
    {
        printf("IVE Map process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_NCC()\n");
    if (ModuleTest_NCC() != MI_SUCCESS)
    {
        printf("IVE NCC process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_Integ()\n");
    if (ModuleTest_Integ() != MI_SUCCESS)
    {
        printf("IVE NoiseRemoveHor process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_CCL()\n");
    if (ModuleTest_CCL() != MI_SUCCESS)
	{
        printf("IVE CCL process is failed\n");
		goto RETURN;
	}

    printf("Run ModuleTest_Hist()\n");
    if (ModuleTest_Hist() != MI_SUCCESS)
    {
        printf("IVE Histogram process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_EqualizeHist()\n");
    if (ModuleTest_EqualizeHist() != MI_SUCCESS)
    {
        printf("IVE EqualizeHist process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_SAD()\n");
    if (ModuleTest_SAD() != MI_SUCCESS)
    {
        printf("IVE SAD process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_NormGrad()\n");
    if (ModuleTest_NormGrad() != MI_SUCCESS)
    {
        printf("IVE Normalized Gradient process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_LBP()\n");
    if (ModuleTest_LBP() != MI_SUCCESS)
    {
        printf("IVE LBP process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_GMM()\n");
    if (ModuleTest_GMM() != MI_SUCCESS)
    {
        printf("IVE GMM process is failed\n");
        goto RETURN;
    }

    printf("Run ModuleTest_Canny()\n");
    if (ModuleTest_Canny() != MI_SUCCESS)
    {
        printf("IVE Canny process is failed\n");
		goto RETURN;
    }

    printf("Run ModuleTest_Lk_Optical_Flow()\n");
    if (ModuleTest_Lk_Optical_Flow() != MI_SUCCESS)
    {
        printf("IVE LK Optical Flow process is failed\n");
        goto RETURN;
    }

    return 0;

RETURN:
    return -1;
}
