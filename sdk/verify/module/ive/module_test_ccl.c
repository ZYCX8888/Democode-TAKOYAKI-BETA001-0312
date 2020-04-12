#include "module_test_main.h"

#define RAW_WIDTH    640
#define RAW_HEIGHT   480
#define INPUT_NAME   "Img640x480.raw"
#define OUTPUT_NAME  "Output_CCL.raw"

MI_S32 ModuleTest_CCL()
{
    MI_S32 ret;
    MI_IVE_HANDLE handle = 0;
    MI_IVE_SrcImage_t src;
    MI_IVE_DstMemInfo_t blob;
    MI_IVE_CclCtrl_t ctrl  =
    {
        .u16InitAreaThr = 4,
        .u16Step        = 2
    };

    memset(&src, 0, sizeof(src));
    memset(&blob, 0, sizeof(blob));

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
        goto RETURN_3;
    }

    // Allocate output buffer
    ret = ModuleTest_AllocateBuffer(&blob, sizeof(MI_IVE_CcBlob_t));
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate output buffer\n");
        goto RETURN_2;
    }

    // Init input buffer
    ret = ModuleTest_InitInputImage(&src, INPUT_NAME);
    if (ret != MI_SUCCESS)
    {
        printf("Can't init input buffer\n");
        goto RETURN_1;
    }

    // Run MI_IVE_Ccl()
    ret = MI_IVE_Ccl(handle, &src, &blob, &ctrl, 0);
    if (ret != MI_SUCCESS)
    {
        printf("MI_IVE_Ccl() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Save ouput data
    ret = ModuleTest_SaveOutputBuffer(&blob, OUTPUT_NAME, sizeof(MI_IVE_CcBlob_t));
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
    ModuleTest_FreeImage(&src);
RETURN_2:
    ModuleTest_FreeBuffer(&blob);
RETURN_3:
    MI_IVE_Destroy(handle);

    return ret;
}
