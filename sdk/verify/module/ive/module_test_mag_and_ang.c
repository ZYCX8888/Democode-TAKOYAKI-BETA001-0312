#include "module_test_main.h"

#define RAW_WIDTH               1280
#define RAW_HEIGHT              720
#define INPUT_NAME              "Img1280x720_0.raw"
#define OUTPUT_NAME_MAG         "Output_MagAndAng_Mag.raw"
#define OUTPUT_NAME_ANG         "Output_MagAndAng_Ang.raw"

MI_S32 ModuleTest_MagAndAng()
{
    MI_S32 ret;
    MI_IVE_HANDLE handle = 0;
	MI_IVE_SrcImage_t src;
	MI_IVE_DstImage_t dst_mag, dst_ang;

	MI_IVE_MagAndAngCtrl_t ctrl =
    {
        .eOutCtrl = E_MI_IVE_MAG_AND_ANG_OUT_CTRL_MAG_AND_ANG,
        .u16Thr   = 0,
        .as8Mask  =
        {
             0,  0,  0,  0, 0,
             0, -1, -2, -1, 0,
             0,  0,  0,  0, 0,
             0,  1,  2,  1, 0,
             0,  0,  0,  0, 0
        }
    };

    memset(&src, 0, sizeof(src));
    memset(&dst_mag, 0, sizeof(dst_mag));
    memset(&dst_ang, 0, sizeof(dst_ang));

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

    // Allocate Mag output buffer
    ret = ModuleTest_AllocateImage(&dst_mag, E_MI_IVE_IMAGE_TYPE_U16C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate Mag output buffer\n");
        goto RETURN_3;
    }

    // Allocate Ang output buffer
    ret = ModuleTest_AllocateImage(&dst_ang, E_MI_IVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate Ang output buffer\n");
        goto RETURN_2;
    }

    // Init input buffer
    ret = ModuleTest_InitInputImage(&src, INPUT_NAME);
    if (ret != MI_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME);
        goto RETURN_1;
    }

    // Run MI_IVE_MagAndAng()
	ret = MI_IVE_MagAndAng(handle, &src, &dst_mag, &dst_ang, &ctrl, 0);
    if (ret != MI_SUCCESS)
    {
        printf("MI_IVE_MagAndAng() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Save Mag ouput image
    ret = ModuleTest_SaveOutputImage(&dst_mag, OUTPUT_NAME_MAG);
    if (ret != MI_SUCCESS)
    {
        printf("Can't save Mag image to output file %s\n", OUTPUT_NAME_MAG);
        goto RETURN_1;
    }

    // Save Ang ouput image
    ret = ModuleTest_SaveOutputImage(&dst_ang, OUTPUT_NAME_ANG);
    if (ret != MI_SUCCESS)
    {
        printf("Can't save Ang image to output file %s\n", OUTPUT_NAME_ANG);
        goto RETURN_1;
    }

    // Compare result
    if (system("cmp " OUTPUT_NAME_MAG " " RESULT_PATH OUTPUT_NAME_MAG) != 0 ||
        system("cmp " OUTPUT_NAME_ANG " " RESULT_PATH OUTPUT_NAME_ANG) != 0)
    {
        printf("Data comparison is incorrect\n");
        ret = MI_IVE_ERR_FAILED;
        goto RETURN_1;
    }

RETURN_1:
    ModuleTest_FreeImage(&dst_ang);
RETURN_2:
    ModuleTest_FreeImage(&dst_mag);
RETURN_3:
    ModuleTest_FreeImage(&src);
RETURN_4:
    MI_IVE_Destroy(handle);

    return ret;
}
