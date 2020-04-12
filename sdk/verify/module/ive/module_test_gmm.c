#include "module_test_main.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define RAW_WIDTH       1280
#define RAW_HEIGHT      720
#define INPUT_NAME      "Img1280x720_Video.raw"
#define OUTPUT_NAME     "Output_GMM.raw"

static int ModuleTest_GetGmmBufferSize(MI_U16 width, MI_U16 height, MI_U32 nmixtures, MI_IVE_ImageType_e color)
{
    MI_U32 buf_size = 0;

    if (color == E_MI_IVE_IMAGE_TYPE_U8C1)
    {
        buf_size += (sizeof(double) * 3 * width * height * nmixtures);
    }
    else
    {
        buf_size += (sizeof(double) * 5 * width * height * nmixtures);
    }

    return (buf_size);
}


MI_S32 ModuleTest_GMM()
{
    MI_S32 ret;
    int input_file, output_file;

    MI_IVE_HANDLE handle = 0;
    MI_IVE_SrcImage_t src;
    MI_IVE_DstImage_t dst_foreground, dst_background;
    MI_IVE_MemInfo_t model;
    MI_U32 i, frame_count;
    MI_IVE_GmmCtrl_t ctrl =
    {
        .u22q10NoiseVar  = 230400,
        .u22q10MaxVar    = 2048000,
        .u22q10MinVar    = 204800,
        .u0q16LearnRate  = 327,
        .u0q16BgRatio    = 52428,
        .u8q8VarThr      = 1600,
        .u0q16InitWeight = 3276,
        .u8ModelNum      = 3
    };

    memset(&src, 0, sizeof(src));
    memset(&dst_foreground, 0, sizeof(dst_foreground));
    memset(&dst_background, 0, sizeof(dst_background));

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
        printf("Can't allocate input buffer \n");

        goto RETURN_6;
    }

    // Allocate foreground output buffer
    ret = ModuleTest_AllocateImage(&dst_foreground, E_MI_IVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate foreground output buffer\n");
        goto RETURN_5;
    }

    // Allocate background output buffer
    ret = ModuleTest_AllocateImage(&dst_background, E_MI_IVE_IMAGE_TYPE_U8C1, RAW_WIDTH, RAW_WIDTH, RAW_HEIGHT);
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate background output buffer\n");
        goto RETURN_4;
    }

    // Init model buffer
    ret = ModuleTest_AllocateBuffer(&model, ModuleTest_GetGmmBufferSize(src.u16Width, src.u16Height, ctrl.u8ModelNum, src.eType));
    if (ret != MI_SUCCESS)
    {
        printf("Can't allocate GMM model buffer\n");
        goto RETURN_3;
    }

    // Open input file
    input_file = open(INPUT_NAME, O_RDONLY);
    if (input_file <= 0)
    {
        printf("Can't open input file %s (%d: %s)\n", INPUT_NAME, errno, strerror(errno));
        goto RETURN_2;
    }

    // Open output file
    output_file = open(OUTPUT_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
    if (output_file <= 0)
    {
        printf("Can't open output file %s (%d: %s)\n", OUTPUT_NAME, errno, strerror(errno));
        goto RETURN_1;
    }

    // Process loop
    for (i=0; i<3; i++)
    {
        // Read frame raw
        ret = ModuleTest_InitInputImageEx(&src, input_file);
        if (ret != MI_SUCCESS)
        {
            printf("Can't read frame data\n");
            goto RETURN_1;
        }

        // Run MI_IVE_Gmm()
        ret = MI_IVE_Gmm(handle, &src, &dst_foreground, &dst_background, &model, &ctrl, 0);
        if (ret != MI_SUCCESS)
        {
            printf("MI_IVE_Gmm() return ERROR 0x%X\n", ret);
            goto RETURN_1;
        }

        // Save ouput data
        ret = ModuleTest_SaveOutputImageEx(&dst_foreground, output_file);
        if (ret != MI_SUCCESS)
        {
            printf("Can't save foreground image to output file %s\n", OUTPUT_NAME);
            goto RETURN_1;
        }

        ret = ModuleTest_SaveOutputImageEx(&dst_background, output_file);
        if (ret != MI_SUCCESS)
        {
            printf("Can't save background image to output file %s\n", OUTPUT_NAME);
            goto RETURN_1;
        }
    }

    // Compare result
    if (system("cmp " OUTPUT_NAME " " RESULT_PATH OUTPUT_NAME) != 0)
    {
        printf("Data comparison is incorrect\n");
        ret = MI_IVE_ERR_FAILED;
        goto RETURN_1;
    }

RETURN_1:
    close(output_file);
    close(input_file);
RETURN_2:
    ModuleTest_FreeBuffer(&model);
RETURN_3:
    ModuleTest_FreeImage(&dst_background);
RETURN_4:
    ModuleTest_FreeImage(&dst_foreground);
RETURN_5:
    ModuleTest_FreeImage(&src);
RETURN_6:
    MI_IVE_Destroy(handle);

    return ret;
}
