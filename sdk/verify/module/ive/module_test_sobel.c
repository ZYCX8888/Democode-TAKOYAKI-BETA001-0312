#include "module_test_main.h"

#define RAW_WIDTH               1280
#define RAW_HEIGHT              720
#define INPUT_NAME              "Img1280x720_0.raw"
#define OUTPUT_NAME_HORIZONTAL  "Output_Sobel_Horizontal.raw"
#define OUTPUT_NAME_VIRTICAL    "Output_Sobel_Vertical.raw"

MI_S32 ModuleTest_Sobel()
{
    MI_S32 ret;
    MI_IVE_HANDLE handle = 0;
	MI_IVE_SrcImage_t src;
	MI_IVE_DstImage_t dst_h, dst_v;

	MI_IVE_SobelCtrl_t ctrl =
    {
        .eOutCtrl = E_MI_IVE_SOBEL_OUT_CTRL_BOTH,
        .as8Mask =
        {
             0,  0,  0,  0, 0,
             0, -1, -2, -1, 0,
             0,  0,  0,  0, 0,
             0,  1,  2,  1, 0,
             0,  0,  0,  0, 0
        }
    };

    memset(&src, 0, sizeof(src));
    memset(&dst_h, 0, sizeof(dst_h));
    memset(&dst_v, 0, sizeof(dst_v));

    // Init IVE
	ret = MI_IVE_Create(handle);
    if (ret != MI_SUCCESS)
	{
		printf("Could not create IVE handle\n");
		return ret;
	}

    // Allocate input buffer
    ret = ModuleTest_AllocateImage(&src, E_MI_IVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate input buffer\n");
        goto RETURN_4;
    }

    // Allocate horizontal output buffer
    ret = ModuleTest_AllocateImage(&dst_h, E_MI_IVE_IMAGE_TYPE_S16C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate horizontal output buffer\n");
        goto RETURN_3;
    }

    // Allocate virtual output buffer
    ret = ModuleTest_AllocateImage(&dst_v, E_MI_IVE_IMAGE_TYPE_S16C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate virtual output buffer\n");
        goto RETURN_2;
    }

    // Init input buffer
    ret = ModuleTest_InitInputImage(&src, INPUT_NAME);
    if (ret != MI_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME);
        goto RETURN_1;
    }

    // Run MI_IVE_Sobel()
	ret = MI_IVE_Sobel(handle, &src, &dst_h, &dst_v, &ctrl, 0);
    if (ret != MI_SUCCESS)
    {
        printf("MI_IVE_Sobel() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Save horizontal ouput image
    ret = ModuleTest_SaveOutputImage(&dst_h, OUTPUT_NAME_HORIZONTAL);
    if (ret != MI_SUCCESS)
    {
        printf("Can't save horizontal image to output file %s\n", OUTPUT_NAME_HORIZONTAL);
        goto RETURN_1;
    }

    // Save vertical ouput image
    ret = ModuleTest_SaveOutputImage(&dst_v, OUTPUT_NAME_VIRTICAL);
    if (ret != MI_SUCCESS)
    {
        printf("Can't save vertical image to output file %s\n", OUTPUT_NAME_VIRTICAL);
        goto RETURN_1;
    }

    // Compare result
    if (system("cmp " OUTPUT_NAME_HORIZONTAL " " RESULT_PATH OUTPUT_NAME_HORIZONTAL) != 0 ||
        system("cmp " OUTPUT_NAME_VIRTICAL   " " RESULT_PATH OUTPUT_NAME_VIRTICAL) != 0)
    {
        printf("Data comparison is incorrect\n");
        ret = MI_IVE_ERR_FAILED;
        goto RETURN_1;
    }

RETURN_1:
    ModuleTest_FreeImage(&dst_v);
RETURN_2:
    ModuleTest_FreeImage(&dst_h);
RETURN_3:
    ModuleTest_FreeImage(&src);
RETURN_4:
    MI_IVE_Destroy(handle);

    return ret;
}
