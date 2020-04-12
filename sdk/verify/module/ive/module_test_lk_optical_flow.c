#include "module_test_main.h"

#define RAW_WIDTH       (160)
#define RAW_HEIGHT      (120)
#define INPUT_NAME_0     "Img160x120_0.raw"
#define INPUT_NAME_1     "Img160x120_1.raw"
#define OUTPUT_NAME      "Output_Lk_Optical_Flow.raw"

MI_S32 ModuleTest_Lk_Optical_Flow()
{
    MI_S32 ret;
    MI_IVE_HANDLE handle = 0;
    MI_IVE_SrcImage_t src_pre, src_cur;
    MI_IVE_SrcMemInfo_t point;
    MI_IVE_MemInfo_t move;
    int x, y, count;
    MI_IVE_LkOpticalFlowCtrl_t ctrl =
    {
        .u16CornerNum  = 49,
        .u0q8MinEigThr = 255,
        .u8IterCount   = 10,
        .u0q8Epsilon   = 26
    };

    memset(&src_pre, 0, sizeof(src_pre));
    memset(&src_cur, 0, sizeof(src_cur));

    // Init IVE
    ret = MI_IVE_Create(handle);
    if (ret != MI_SUCCESS)
    {
        printf("Could not create IVE handle\n");
        return ret;
    }

    // Allocate input buffer 0
    ret = ModuleTest_AllocateImage(&src_pre, E_MI_IVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate input buffer 0\n");
        goto RETURN_5;
    }

    // Allocate input buffer 1
    ret = ModuleTest_AllocateImage(&src_cur, E_MI_IVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate input buffer 1\n");
        goto RETURN_4;
    }

    // Init point buffer
    ret = ModuleTest_AllocateBuffer(&point, sizeof(MI_IVE_PointS25Q7_t) * ctrl.u16CornerNum);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate Optical Flow point buffer\n");
        goto RETURN_3;
    }

    // Init move buffer
    ret = ModuleTest_AllocateBuffer(&move, sizeof(MI_IVE_MvS9Q7_t) * ctrl.u16CornerNum);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate Optical Flow move buffer\n");
        goto RETURN_2;
    }

    // Init input buffer 0
    ret = ModuleTest_InitInputImage(&src_pre, INPUT_NAME_0);
    if (ret != MI_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME_0);
        goto RETURN_1;
    }

    // Init input buffer 1
    ret = ModuleTest_InitInputImage(&src_cur, INPUT_NAME_1);
    if (ret != MI_SUCCESS)
    {
        printf("Can't init input buffer from %s\n", INPUT_NAME_1);
        goto RETURN_1;
    }

    // Init point
    count = 0;
    for (x=0; x<7; x++)
    {
        for (y=0; y<7; y++)
        {
            ((MI_IVE_PointS25Q7_t*)point.pu8VirAddr)[count].s25q7X = ((src_pre.u16Width  / 13)*(x+3)) << 7;
            ((MI_IVE_PointS25Q7_t*)point.pu8VirAddr)[count].s25q7Y = ((src_pre.u16Height / 13)*(y+3)) << 7;
            count++;
        }
    }

    // Run MI_IVE_LkOpticalFlow
    ret = MI_IVE_LkOpticalFlow(handle, &src_pre, &src_cur, &point, &move, &ctrl, 0);
    if (ret != MI_SUCCESS)
    {
        printf("MI_IVE_LkOpticalFlow() return ERROR 0x%X\n", ret);
        goto RETURN_1;
    }

    // Save ouput data
    ret = ModuleTest_SaveOutputBuffer(&move, OUTPUT_NAME, sizeof(MI_IVE_MvS9Q7_t) * ctrl.u16CornerNum);
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
    ModuleTest_FreeBuffer(&move);
RETURN_2:
    ModuleTest_FreeBuffer(&point);
RETURN_3:
    ModuleTest_FreeImage(&src_cur);
RETURN_4:
    ModuleTest_FreeImage(&src_pre);
RETURN_5:
    MI_IVE_Destroy(handle);

    return ret;
}
