#include "module_test_main.h"

#define RAW_WIDTH    1280
#define RAW_HEIGHT   720
#define INPUT_NAME   "Img1280x720_16bit.raw"
#define OUTPUT_NAME  "Output_Thresh_S16.raw"

MI_S32 ModuleTest_Thresh_S16()
{
    MI_S32 ret;
    MI_IVE_HANDLE handle = 0;
	MI_IVE_SrcImage_t src;
	MI_IVE_DstImage_t dst;
    MI_IVE_ThreshS16Ctrl_t ctrl  =
    {
        .eMode      = E_MI_IVE_THRESH_S16_MODE_S16_TO_S8_MIN_MID_MAX,
        .s16LowThr  = -10922,
        .s16HighThr = 10922,
        .un8MinVal  = 0,
        .un8MidVal  = 127,
        .un8MaxVal  = 255
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
    ret = ModuleTest_AllocateImage(&src, E_MI_IVE_IMAGE_TYPE_S16C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate input buffer\n");
        goto RETURN_3;
    }

    // Allocate output buffer
    ret = ModuleTest_AllocateImage(&dst, E_MI_IVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
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

    // Run MI_IVE_ThreshS16()
	ret = MI_IVE_ThreshS16(handle, &src, &dst, &ctrl, 0);
    if (ret != MI_SUCCESS)
    {
        printf("MI_IVE_ThreshS16() return ERROR 0x%X\n", ret);
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
