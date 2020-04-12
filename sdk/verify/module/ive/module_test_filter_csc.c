#include "module_test_main.h"

#define RAW_WIDTH    1280
#define RAW_HEIGHT   720
#define INPUT_NAME   "Img1280x720_Yuv420.raw"
#define OUTPUT_NAME  "Output_FilterAndCSC.raw"

MI_S32 ModuleTest_FilterAndCSC()
{
    MI_S32 ret;
    MI_IVE_HANDLE handle = 0;
	MI_IVE_SrcImage_t src;
	MI_IVE_DstImage_t dst;
    MI_IVE_FilterAndCscCtrl_t ctrl =
    {
        .eMode = E_MI_IVE_CSC_MODE_PIC_BT601_YUV2RGB,
        .as8Mask =
        {
            //Gaussian filter
             0, 0, 0, 0, 0,
             0, 1, 2, 1, 0,
             0, 2, 4, 2, 0,
             0, 1, 2, 1, 0,
             0, 0, 0, 0, 0
        },
        .u8Norm = 4
    };

    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));

    // Init IVE
    ret = MI_IVE_Create(handle);
	if (ret != MI_SUCCESS)
	{
		printf("Could not create IVE handle\n");
		return ret;
	}

    // Allocate input buffer
    ret = ModuleTest_AllocateImage(&src, E_MI_IVE_IMAGE_TYPE_YUV420SP, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate input buffer\n");
        goto RETURN_3;
    }

    // Allocate output buffer
    ret = ModuleTest_AllocateImage(&dst, E_MI_IVE_IMAGE_TYPE_U8C3_PACKAGE, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate output buffer\n");
        goto RETURN_2;
    }

    // Init input buffer
    ret = ModuleTest_InitInputImage(&src, INPUT_NAME);
    if (ret != MI_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME);
        goto RETURN_1;
    }

    // Run MI_IVE_FilterAndCsc()
	ret = MI_IVE_FilterAndCsc(handle, &src, &dst, &ctrl, 0);
    if (ret != MI_SUCCESS)
    {
        printf("MI_IVE_FilterAndCsc() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Save ouput data
    ret = ModuleTest_SaveOutputImage(&dst, OUTPUT_NAME);
    if (ret != MI_SUCCESS)
    {
        printf("Can't save data to output file %s\n", OUTPUT_NAME);
        goto RETURN_1;
    }

    // Compare result
    if (system("cmp " OUTPUT_NAME " " RESULT_PATH OUTPUT_NAME) != 0)
    {
        printf("Data comparison is incorrect\n");
        ret = MI_IVE_ERR_FAILED;
        goto RETURN_1;
    }

RETURN_1:
    ModuleTest_FreeImage(&dst);
RETURN_2:
    ModuleTest_FreeImage(&src);
RETURN_3:
    MI_IVE_Destroy(handle);

    return ret;
}
