#include "module_test_main.h"

#define RAW_WIDTH       1280
#define RAW_HEIGHT      720
#define INPUT_NAME_0    "Img1280x720_0.raw"
#define INPUT_NAME_1    "Img1280x720_1.raw"
#define OUTPUT_NAME     "Output_NCC.raw"

MI_S32 ModuleTest_NCC()
{
    MI_S32 ret;
    MI_IVE_HANDLE handle = 0;
    MI_IVE_SrcImage_t src0, src1;
    MI_IVE_DstMemInfo_t dst;

    memset(&src0, 0, sizeof(src0));
    memset(&src1, 0, sizeof(src1));
    memset(&dst, 0, sizeof(dst));

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
        goto RETURN_4;
    }

    // Allocate input buffer 1
    ret = ModuleTest_AllocateImage(&src1, E_MI_IVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate input buffer 1\n");
        goto RETURN_3;
    }

    // Allocate destination buffer
    ret = ModuleTest_AllocateBuffer(&dst, sizeof(MI_IVE_NccDstMem_t));
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate input buffer 1\n");
        goto RETURN_2;
    }

    // Init input buffer 0
    ret = ModuleTest_InitInputImage(&src0, INPUT_NAME_0);
    if (ret != MI_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME_0);
        goto RETURN_1;
    }

    // Init input buffer 1
    ret = ModuleTest_InitInputImage(&src1, INPUT_NAME_1);
    if (ret != MI_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME_1);
        goto RETURN_1;
    }

    // Run MI_IVE_Ncc
    ret = MI_IVE_Ncc(handle, &src0, &src1, &dst, 0);
    if (ret != MI_SUCCESS)
    {
        printf("MI_IVE_Ncc() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Save ouput data
    ret = ModuleTest_SaveOutputBuffer(&dst, OUTPUT_NAME, sizeof(MI_IVE_NccDstMem_t));
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
    ModuleTest_FreeBuffer(&dst);
RETURN_2:
    ModuleTest_FreeImage(&src1);
RETURN_3:
    ModuleTest_FreeImage(&src0);
RETURN_4:
    MI_IVE_Destroy(handle);

    return ret;
}
