#include "module_test_main.h"

#define RAW_WIDTH    640
#define RAW_HEIGHT   480
#define INPUT_NAME   "Img640x480.raw"
#define OUTPUT_NAME  "Output_Canny.raw"

MI_S32 ModuleTest_Canny()
{
    int i;
    MI_S32 ret;
    MI_IVE_HANDLE handle = 0;
    MI_IVE_SrcImage_t src;
    MI_IVE_DstImage_t dst;
    MI_IVE_DstMemInfo_t stack;
    MI_IVE_CannyHysEdgeCtrl_t ctrl =
    {
        .u16LowThr  = 7*2,
        .u16HighThr = 14*2,
        .as8Mask =
        {
            0,  0, 0, 0, 0,
            0, -1, 0, 1, 0,
            0, -2, 0, 2, 0,
            0, -1, 0, 1, 0,
            0,  0, 0, 0, 0
        }
    };

    memset(&src, 0, sizeof(src));
    memset(&dst, 0, sizeof(dst));
    memset(&stack, 0, sizeof(stack));

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

    // Allocate output buffer
    ret = ModuleTest_AllocateImage(&dst, E_MI_IVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate output buffer\n");
        goto RETURN_3;
    }

    // Init Mv buffer
    ret = ModuleTest_AllocateBuffer(&stack, RAW_WIDTH*RAW_HEIGHT*sizeof(MI_IVE_PointU16_t) + sizeof(MI_IVE_CannyStackSize_t));
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate canny stack buffer\n");
        goto RETURN_2;
    }

    // Init input buffer
    ret = ModuleTest_InitInputImage(&src, INPUT_NAME);
    if (ret != MI_SUCCESS)
    {
        printf("Can't init input buffer\n");
        goto RETURN_1;
    }

    // Run MI_IVE_CannyHysEdge()
    ret = MI_IVE_CannyHysEdge(handle, &src, &dst, &stack, &ctrl, 0);
    if (ret != MI_SUCCESS)
    {
        printf("MI_IVE_CannyHysEdge() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Run MI_IVE_CannyEdge()
    ret = MI_IVE_CannyEdge(handle, &dst, &stack, 0);
    if (ret != MI_SUCCESS)
    {
        printf("MI_IVE_CannyEdge() return ERROR 0x%X\n", ret);
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
    ModuleTest_FreeBuffer(&stack);
RETURN_2:
    ModuleTest_FreeImage(&dst);
RETURN_3:
    ModuleTest_FreeImage(&src);
RETURN_4:
    MI_IVE_Destroy(handle);

    return ret;
}
