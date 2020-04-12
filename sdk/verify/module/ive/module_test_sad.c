#include "module_test_main.h"

#define RAW_WIDTH       1280
#define RAW_HEIGHT      720
#define INPUT_NAME_0    "Img1280x720_0.raw"
#define INPUT_NAME_1    "Img1280x720_1.raw"
#define OUTPUT_NAME_0   "Output_SAD_0.raw"
#define OUTPUT_NAME_1   "Output_SAD_1.raw"

MI_S32 ModuleTest_SAD()
{
    MI_S32 ret;
    MI_IVE_HANDLE handle = 0;
    MI_IVE_SrcImage_t src0, src1;
    MI_IVE_DstImage_t dst0, dst1;
    MI_IVE_SadCtrl_t ctrl =
    {
        .eMode     = E_MI_IVE_SAD_MODE_MB_8X8,
        .eOutCtrl  = E_MI_IVE_SAD_OUT_CTRL_16BIT_BOTH,
        .u16Thr    = 8160,
        .u8MinVal  = 0,
        .u8MaxVal  = 255
    };

    memset(&src0, 0, sizeof(src0));
    memset(&src1, 0, sizeof(src1));
    memset(&dst0, 0, sizeof(dst0));
    memset(&dst1, 0, sizeof(dst0));

    // Init IVE
    ret = MI_IVE_Create(handle);
    if (ret != MI_SUCCESS)
    {
        printf("Could not create IVE handle\n");
		return ret;
    }

    // Allocate input buffer 0
    ret = ModuleTest_AllocateImage(&src0, E_MI_IVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate input buffer 0\n");
        goto RETURN_5;
    }

    // Allocate input buffer 1
    ret = ModuleTest_AllocateImage(&src1, E_MI_IVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate input buffer 1\n");
        goto RETURN_4;
    }

    // Allocate output buffer 0
    ret = ModuleTest_AllocateImage(&dst0, E_MI_IVE_IMAGE_TYPE_U16C1, RAW_WIDTH/8, RAW_WIDTH/8, RAW_HEIGHT/8);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate output buffer 0\n");
        goto RETURN_3;
    }

    // Allocate output buffer 1
    ret = ModuleTest_AllocateImage(&dst1, E_MI_IVE_IMAGE_TYPE_U8C1, RAW_WIDTH/8, RAW_WIDTH/8, RAW_HEIGHT/8);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate output buffer 1\n");
        goto RETURN_2;
    }

    // Init input buffer 0
    ret = ModuleTest_InitInputImage(&src0, INPUT_NAME_0);
    if (ret != MI_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME_0);
        goto RETURN_1;
    }

    // Init input buffer 0
    ret = ModuleTest_InitInputImage(&src1, INPUT_NAME_1);
    if (ret != MI_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME_1);
        goto RETURN_1;
    }

    // Run MI_IVE_Sad()
    ret = MI_IVE_Sad(handle, &src0, &src1, &dst0, &dst1, &ctrl, 0);
    if (ret != MI_SUCCESS)
    {
        printf("MI_IVE_Sad() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Save ouput data
    ret = ModuleTest_SaveOutputImage(&dst0, OUTPUT_NAME_0);
    if (ret != MI_SUCCESS)
    {
        printf("Can't save output 0 to output file %s\n", OUTPUT_NAME_0);
        goto RETURN_1;
    }

    ret = ModuleTest_SaveOutputImage(&dst1, OUTPUT_NAME_1);
    if (ret != MI_SUCCESS)
    {
        printf("Can't save output 0 to output file %s\n", OUTPUT_NAME_1);
        goto RETURN_1;
    }

    // Compare result
    if (system("cmp " OUTPUT_NAME_0 " " RESULT_PATH OUTPUT_NAME_0) != 0 ||
        system("cmp " OUTPUT_NAME_1 " " RESULT_PATH OUTPUT_NAME_1) != 0)
    {
        printf("Data comparison is incorrect\n");
        ret = MI_IVE_ERR_FAILED;
        goto RETURN_1;
    }

RETURN_1:
    ModuleTest_FreeImage(&dst1);
RETURN_2:
    ModuleTest_FreeImage(&dst0);
RETURN_3:
    ModuleTest_FreeImage(&src1);
RETURN_4:
    ModuleTest_FreeImage(&src0);
RETURN_5:
    MI_IVE_Destroy(handle);

    return ret;
}
