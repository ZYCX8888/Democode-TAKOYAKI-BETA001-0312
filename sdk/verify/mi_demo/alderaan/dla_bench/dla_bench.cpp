/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdbool.h>
#include <error.h>
#include <errno.h>
#include <pthread.h>

#include <string.h>
#include <fstream>
#include <iostream>
#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/time.h>
#include <unistd.h>
using namespace std;
using std::cout;
using std::endl;
using std::ostringstream;
using std::vector;
using std::string;

#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_ipu.h"
#include "mi_sys.h"
#include <arm_neon.h>

#define  LABEL_IMAGE_FUNC_INFO(fmt, args...)           do {printf("[Info ] [%-4d] [%10s] ", __LINE__, __func__); printf(fmt, ##args);} while(0)

struct PreProcessedData {
    char *pImagePath;
    int intResizeH;
    int intResizeW;
    int intResizeC;
    bool bNorm;
    float fmeanB;
    float fmeanG;
    float fmeanR;
    float std;
    bool bRGB;
    unsigned char * pdata;

} ;

#define VARIABLE_BUF_SIZE (20*1000*1000)
#define INOUT_HEAP_SIZE   (4*1000*1000)

MI_S32  IPUCreateDevice(char *pFirmwarePath,MI_U32 u32VarBufSize)
{
    MI_S32 s32Ret = MI_SUCCESS;
    MI_IPU_DevAttr_t stDevAttr;
    stDevAttr.u32MaxVariableBufSize = u32VarBufSize;
    stDevAttr.u32YUV420_W_Pitch_Alignment = 16;
    stDevAttr.u32YUV420_H_Pitch_Alignment = 2;
    stDevAttr.u32XRGB_W_Pitch_Alignment = 16;
    s32Ret = MI_IPU_CreateDevice(&stDevAttr, NULL, pFirmwarePath, 0);
    return s32Ret;
}



MI_S32 IPUCreateChannel(MI_U32 *s32Channel, char *pModelImage,MI_U32 u32HeapSize)
{
    MI_IPUChnAttr_t stChnAttr;

    //create channel
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth = 2;
    stChnAttr.u32OutputBufDepth = 2;
    return MI_IPU_CreateCHN(s32Channel, &stChnAttr, NULL, pModelImage);
}

MI_S32 IPUDestroyChannel(MI_U32 s32Channel, MI_U32 u32HeapSize)
{
    return MI_IPU_DestroyCHN(s32Channel);
}


#include <sys/mman.h>
#include <unistd.h>
#include <getopt.h>

const char *_gpOptstring  = "i:m:g:w:n:s:";
typedef struct {
    struct option stOpt;
    const char *  pstrDesc;
} ST_Option_t;

ST_Option_t _astLongOpts[] = {
    {
        .stOpt    = {"image", required_argument, NULL, 'i'},
        .pstrDesc = "Indicate image file path",
    },
    {
        .stOpt    = {"model", required_argument, NULL, 'm'},
        .pstrDesc = "Indicate model file path",
    },
    {
        .stOpt    = {"golden", required_argument, NULL, 'g'},
        .pstrDesc = "Indicate output golden file name",
    },
    {
        .stOpt    = {"firmware", required_argument, NULL, 'w'},
        .pstrDesc = "Indicate firmware path",
    },
    {
        .stOpt    = {"invoke count", required_argument, NULL, 'n'},
        .pstrDesc = "Indicate invoke count",
    },
    {
        .stOpt    = {"output scalar", required_argument, NULL, 's'},
        .pstrDesc = "Indicate net output tensor golden scalar",
    },

};

static void showUsageLabelImage(const char *name)
{
    unsigned int i = 0;
    printf("%s Usage:\n", name);
    for (i = 0; i < sizeof(_astLongOpts)/sizeof(_astLongOpts[0]); i++)
    {
        if (_astLongOpts[i].stOpt.val >= 0 && _astLongOpts[i].stOpt.val <= 9) {
            printf("\t  \t--%-10s\t%s\n", _astLongOpts[i].stOpt.name, _astLongOpts[i].pstrDesc);
        }
        else {
            printf("\t-%c\t--%-10s\t\t%s\n", _astLongOpts[i].stOpt.val, _astLongOpts[i].stOpt.name, _astLongOpts[i].pstrDesc);
        }
    }
}

#define INNER_MOST_ALIGNMENT (8)
#define ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

static MI_U32 _getTensorBufferAlignSize(MI_IPU_TensorDesc_t *pstShape, MI_U32 u32TypeSize)
{
    MI_U32 cnt = 1;
    for (int k = 0; k < (pstShape->u32TensorDim -1); k++)
    {
        cnt *= pstShape->u32TensorShape[k];
    }
    cnt *= ALIGN_UP(pstShape->u32TensorShape[pstShape->u32TensorDim -1], 8);

    return cnt;
}

static MI_S32 _getTensorShapeArray(MI_IPU_TensorDesc_t *pstShape, MI_U32 au32Shape[], MI_S32 s32Size)
{
    MI_S32 cnt = 1;
    assert(pstShape->u32TensorDim <= s32Size);
    for (int k = 0; k < s32Size; k++)
    {
        if (k < ((s32Size) - pstShape->u32TensorDim))
        {
            au32Shape[k] = 1;
        }
        else
        {
            au32Shape[k] = pstShape->u32TensorShape[k - ((s32Size) - pstShape->u32TensorDim)];
        }

        //cout << "au32Shape[ " << k << " ] = " << au32Shape[k] << endl;
        cnt *= au32Shape[k];
    }

    return cnt;

}

float getMse(float *pData, float *pData2, int amax, MI_IPU_TensorDesc_t *pstShape)
{
    MI_U32 au32Shape[4], u32Internal = 0;
    MI_S32 cnt = 1;

    cnt = _getTensorShapeArray(pstShape, au32Shape, sizeof(au32Shape)/sizeof(au32Shape[0]));
    u32Internal = ALIGN_UP(au32Shape[3], 8);
    assert(cnt <= amax);

	float diff = 0.0;
	float diffSum = 0.0;
    unsigned int index = 0;

    for (unsigned int n = 0; n < au32Shape[0]; n++)
    {
        for (unsigned int h = 0; h < au32Shape[1]; h++)
        {
            for (unsigned int w = 0; w < au32Shape[2]; w++)
            {
                for (unsigned int c = 0; c < au32Shape[3]; c++)
                {
                    index = n * au32Shape[1]*au32Shape[2]*u32Internal + h*au32Shape[2]*u32Internal + w*u32Internal + c;
            		diff = pData[index] - pData2[index];
            		diff = fabs(diff);
            		//printf("array[%d] = %f --> %f diff: %f\n", index, pData[index], pData2[index], diff);
                    diffSum += diff;
                }
            }
        }
    }
	diffSum = diffSum/cnt;
	return diffSum;
}

float getRMSE(float *pData, float *pData2, int amax, MI_IPU_TensorDesc_t *pstShape)
{
	float diff = 0.0;
	float diffSum = 0.0;
	float rdiffSum = 0.0;
	float rSum = 0.0;
    unsigned int index = 0;

    MI_U32 au32Shape[4], u32Internal = 0;
    MI_S32 cnt = 1;
    cnt = _getTensorShapeArray(pstShape, au32Shape, sizeof(au32Shape)/sizeof(au32Shape[0]));
    u32Internal = ALIGN_UP(au32Shape[3], 8);
    assert(cnt <= amax);

    for (unsigned int n = 0; n < au32Shape[0]; n++)
    {
        for (unsigned int h = 0; h < au32Shape[1]; h++)
        {
            for (unsigned int w = 0; w < au32Shape[2]; w++)
            {
                for (unsigned int c = 0; c < au32Shape[3]; c++)
                {
                    index = n * au32Shape[1]*au32Shape[2]*u32Internal + h*au32Shape[2]*u32Internal + w*u32Internal + c;
            		diff = pData[index] - pData2[index];
            		diff =fabs(diff);
            		rdiffSum += diff;
            		diff = diff*diff;
            		diffSum += diff;
            		//printf("array[%d] = %f --> %f diff: %f\n", index, pData[index], pData2[index], diff);
            		rSum += fabs(pData2[index]);
                }
            }
        }
    }

	diffSum  = diffSum/cnt;
	rdiffSum   = rdiffSum/rSum;
	//printf("MSE:\t%f\tRMSE:\t%f rSum: %f\n", diffSum, rdiffSum, rSum);

	return rdiffSum;
}

__attribute__((target("fpu=neon")))
static void neon_accelator_vector_1_bias(short *input, float *output, float *bias)
{
/* c code logics
      float temp = (float)(*(input));
      *(output) = temp*(*bias);
*/
    int16x4_t input_s16vec0 ;
    memset(&input_s16vec0, 0, sizeof(input_s16vec0));
    vld1_lane_s16(input,input_s16vec0,0);
    int32x4_t input_s32vec0 =  vmovl_s16(input_s16vec0);
    float32x4_t bias_f32vec =  vld1q_f32 (bias);
    float32x4_t input_f32vec0 = vcvtq_f32_s32(input_s32vec0);
    float32x4_t output_f32vec0= vmulq_f32 (input_f32vec0, bias_f32vec);
    vst1q_lane_f32(output,output_f32vec0,0);
}

__attribute__((target("fpu=neon")))
static void neon_accelator_vector_2_bias(short *input, float *output, float *bias)
{
/*
   c code logic
    for(int i=0;i<2;i++)
    {
        float temp = (float)(*(input+i));
        *(output+i) = temp*(*bias);
    }
 */
    int16x4_t input_s16vec0 ;
    memset(&input_s16vec0, 0, sizeof(input_s16vec0));

    vld1_lane_s16(input,input_s16vec0,0);
    input++;
    vld1_lane_s16(input,input_s16vec0,1);
    int32x4_t input_s32vec0 =  vmovl_s16(input_s16vec0);
    float32x4_t bias_f32vec =  vld1q_f32 (bias);
    float32x4_t input_f32vec0 = vcvtq_f32_s32(input_s32vec0);
    float32x4_t output_f32vec0= vmulq_f32 (input_f32vec0, bias_f32vec);
    vst1q_lane_f32(output,output_f32vec0,0);
    output++;
    vst1q_lane_f32(output,output_f32vec0,1);
}

__attribute__((target("fpu=neon")))
static void neon_accelator_vector_4_bias(short *input, float *output, float *bias)
{
/*  c code logic
    for(int i=0;i<4;i++)
    {
        float temp = (float)(*(input+i));
        *(output+i) = temp*(*bias);
    }
*/
     int16x4_t input_s16vec0 =  vld1_s16(input);
     int32x4_t input_s32vec0 =  vmovl_s16(input_s16vec0);
     float32x4_t bias_f32vec =  vld1q_f32 (bias);
     float32x4_t input_f32vec0 = vcvtq_f32_s32(input_s32vec0);
     float32x4_t output_f32vec0= vmulq_f32 (input_f32vec0, bias_f32vec);
     vst1q_f32 (output, output_f32vec0);
}

__attribute__((target("fpu=neon")))
static void neon_accelator_vector_8_bias(short *input, float *output, float *bias)
{
    /* c code logics
    for(int i=0;i<8;i++)
    {
        float temp = (float)(*(input+i));
        *(output+i) = temp*(*bias);
    }

    */

     int16x4_t input_s16vec0 =  vld1_s16(input);
     input += 4;
     int16x4_t input_s16vec1 =  vld1_s16(input);
     int32x4_t input_s32vec0 =  vmovl_s16(input_s16vec0);
     int32x4_t input_s32vec1 =  vmovl_s16(input_s16vec1);
     float32x4_t bias_f32vec =  vld1q_f32 (bias);
     float32x4_t input_f32vec0 = vcvtq_f32_s32(input_s32vec0);
     float32x4_t input_f32vec1 = vcvtq_f32_s32(input_s32vec1);
     float32x4_t output_f32vec0= vmulq_f32 (input_f32vec0, bias_f32vec);
     float32x4_t output_f32vec1= vmulq_f32 (input_f32vec1, bias_f32vec);
     vst1q_f32 (output, output_f32vec0);
     output += 4;
     vst1q_f32 (output, output_f32vec1);
}

typedef void (*neon_vector)(short*, float*, float*);
typedef struct vector_struct_s {
    int vector_size;
    int vector_cnt;
    neon_vector neon_func;
} vector_struct_t;
static void int16_to_float_with_neon(void *input, unsigned int input_size, void *output, float bias)
{
    float bias_q[4];
    int cnt, base = 8, data_cnt;
    unsigned int loop;
    short *s_in;
    float *f_out;
    vector_struct_t vs[4];  //vector_8, vector_4, vector_2, vector_1

    memset(vs, 0, sizeof(vs));
    bias_q[0] = bias;
    bias_q[1] = bias;
    bias_q[2] = bias;
    bias_q[3] = bias;
    data_cnt = input_size / sizeof(short);
    for (loop = 0; loop < sizeof(vs)/sizeof(vs[0]); loop++) {
        vs[loop].vector_size = base;
        vs[loop].vector_cnt= data_cnt / base;
        data_cnt = data_cnt % base;
        base /= 2;
        if (!data_cnt)
            break;
    }
    //ipu_info("vector_8_cnt, vector_4_cnt, vector_2_cnt, vector_1_cnt: "
    //                "%d %d %d %d\n", vs[0].vector_cnt, vs[1].vector_cnt, vs[2].vector_cnt, vs[3].vector_cnt);

    s_in = (short *)input;
    f_out = (float *)output;
    vs[0].neon_func= neon_accelator_vector_8_bias;
    vs[1].neon_func = neon_accelator_vector_4_bias;
    vs[2].neon_func = neon_accelator_vector_2_bias;
    vs[3].neon_func = neon_accelator_vector_1_bias;
    for (loop = 0; loop < sizeof(vs)/sizeof(vs[0]); loop++) {
        if (vs[loop].vector_cnt) {
            for (cnt = 0; cnt < vs[loop].vector_cnt; cnt++) {
                vs[loop].neon_func(s_in, f_out, bias_q);
                s_in += vs[loop].vector_size;
                f_out += vs[loop].vector_size;
            }
        }
    }

#if 0
    cout << "Scalar: " << bias << endl;
    for (unsigned int i = 0; i < input_size / sizeof(short); i++)
    {
        printf("[%d] 0x%x -> %1.12f\n", i, *(((MI_S16 *)input)+ i), *(((float *) output) + i));
    }
#endif
}

static MI_U32 _getScalarFromString(const char *pStr, char cSplite, float afScalar[], MI_U32 u32ArraySize)
{
    char buf[64];
    const char *p = pStr, *q = NULL;
    MI_U32 u32Size = 0, cnt = 0;
    do {
        q = strchr(p, cSplite);
        if (q != NULL)
        {
           u32Size = q - p;
           strncpy(buf, p, u32Size);
           buf[u32Size] = 0;
           p = q + 1;
        }
        else
        {
            strcpy(buf, p);
        }
        afScalar[cnt] = atof(buf);
        cnt++;
    } while ((q != NULL) && (cnt < u32ArraySize));
    return cnt;
}

#define ST_SHOW(p) do {printf(#p " = %p\n", (p));} while(0)
#define MAX_OUT_TENSOR_SIZE (8)
int main(int argc,char *argv[])
{
    char * pFirmwarePath = NULL;
    char * pModelImgPath = NULL;
    char * pImagePath    = NULL;
    const char *pGodenFile = NULL;
    MI_U32 u32ChannelID = 0;
    MI_IPU_SubNet_InputOutputDesc_t desc;
    MI_IPU_TensorVector_t InputTensorVector;
    MI_IPU_TensorVector_t OutputTensorVector;
    int times = 1;
    float afScalar[MAX_OUT_TENSOR_SIZE];
    MI_U32 u32ScalarSize = 0;
    int optIndex = 0;
    int opt;
    int ret = -1;
    struct option *pstOpt = NULL;
    pstOpt = (struct option *)malloc(sizeof(*pstOpt)*(sizeof(_astLongOpts)/sizeof(_astLongOpts[0]) + 1));
    if (pstOpt == NULL)
    {
        return ret;
    }

    for (unsigned i = 0 ; i < sizeof(_astLongOpts)/sizeof(_astLongOpts[0]); i++)
    {
        memcpy(pstOpt + i, &_astLongOpts[i].stOpt, sizeof(*pstOpt));
    }

    while ((opt = getopt_long(argc, (char * const *)&argv[0], _gpOptstring, pstOpt, &optIndex)) != -1)
    {

//       printf("optarg = %s\n", optarg);
//        printf("optind = %d\n", optind);
//        printf("argv[optind - 1] = %s\n\n",  argv[optind - 1]);
        switch (opt)
        {
            case 'i':
                pImagePath = optarg;
                break;
            case 'm':
                pModelImgPath = optarg;
                break;
            case 'g':
                pGodenFile = optarg;
                break;
            case 'w':
                pFirmwarePath = optarg;
                break;
            case 'n':
                times = atoi(optarg);
                break;
            case 's':
                afScalar[0] = atof(optarg);
                u32ScalarSize = _getScalarFromString(optarg, ';', afScalar, sizeof(afScalar)/sizeof(afScalar[0]));
                break;
            default:
                break;
        }
    }

    if ((pFirmwarePath == NULL)
        || (pModelImgPath == NULL)
        || (pImagePath == NULL)
        || (u32ScalarSize == 0)
        )
    {
        ST_SHOW(pFirmwarePath);
        ST_SHOW(pGodenFile);
        ST_SHOW(pModelImgPath);
        ST_SHOW(pImagePath);
        showUsageLabelImage(argv[0]);
        return ret;
    }

     std::cout<<"ipu_firmware      : "<<pFirmwarePath<<std::endl;
     std::cout<<"model_img         : "<<pModelImgPath<<std::endl;
     std::cout<<"picture           : "<<pImagePath<<std::endl;
     std::cout<<"Times             : "<<times<<std::endl;
     std::cout<<"Golden file       : "<<pGodenFile<<std::endl;
     for (MI_U32 i = 0; i < u32ScalarSize; i++)
    {
        std::cout<<"outScalar[ "<< i <<" ]    : "<<afScalar[i]<<std::endl;
    }
    //1.create device
    if(MI_SUCCESS !=IPUCreateDevice(pFirmwarePath,VARIABLE_BUF_SIZE))
    {
        cout<<"create ipu device failed!"<<std::endl;
        return -1;

    }

    //2.create channel
    if(MI_SUCCESS !=IPUCreateChannel(&u32ChannelID,pModelImgPath,INOUT_HEAP_SIZE))
    {
         cout<<"create ipu channel failed!"<<std::endl;
         MI_IPU_DestroyDevice();
         return -1;
    }

    //3.get input/output tensor
    MI_IPU_GetInOutTensorDesc(u32ChannelID, &desc);

    int intResizeH = desc.astMI_InputTensorDescs[0].u32TensorShape[1];
    int intResizeW = desc.astMI_InputTensorDescs[0].u32TensorShape[2];
    int intResizeC = desc.astMI_InputTensorDescs[0].u32TensorShape[3];

    MI_IPU_GetInputTensors( u32ChannelID, &InputTensorVector);
    MI_BOOL bRet = TRUE;
    int fd = 0;
    struct stat sb;
    char *pmem = NULL;
    fd = open(pImagePath, O_RDWR);
    if (fd > 0)
    {
        memset(&sb, 0, sizeof(sb));
        if (fstat(fd, &sb) < 0)
        {
            perror("fstat");
        }
        else
        {
            assert(sb.st_size >= intResizeH*intResizeW*intResizeC);
            pmem = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
            memcpy(InputTensorVector.astArrayTensors[0].ptTensorData[0], pmem, intResizeH*intResizeW*intResizeC);
            MI_SYS_FlushInvCache(InputTensorVector.astArrayTensors[0].ptTensorData[0], intResizeH*intResizeW*intResizeC);
        }
    }
    else
    {
        cout << "Fail to open image file" << std::endl;
        assert(0);
    }
    MI_IPU_GetOutputTensors(u32ChannelID, &OutputTensorVector);
    //float *pfData = (float *)OutputTensorVector.stArrayTensors[0].pstTensorData[0];
    float fMse = 0.0;

    // Map output golden
    fd = open(pGodenFile, O_RDWR);
    if (fd > 0)
    {
        memset(&sb, 0, sizeof(sb));
        if (fstat(fd, &sb) < 0)
        {
            perror("fstat");
        }
        else
        {
            pmem = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
        }
    }
    static float *paGoldenFloat[MAX_OUT_TENSOR_SIZE];

    //4.invoke
    for (int i=0;i<times;i++ )
    {
        if(MI_SUCCESS!=MI_IPU_Invoke(u32ChannelID, &InputTensorVector, &OutputTensorVector))
        {
            cout<<"IPU invoke failed!!"<<endl;
            IPUDestroyChannel(u32ChannelID, INOUT_HEAP_SIZE);
            MI_IPU_DestroyDevice();
            return -1;
        }

        MI_U32 u32Offset = 0;
        for (unsigned int j = 0; j < desc.u32OutputTensorCount; j++)
        {
            MI_U32 u32ElementAlignSize = _getTensorBufferAlignSize(&desc.astMI_OutputTensorDescs[j], 1);
            if (paGoldenFloat[j] == NULL)
            {
                paGoldenFloat[j] = new float[u32ElementAlignSize];
                int16_to_float_with_neon(pmem + u32Offset, u32ElementAlignSize * sizeof(MI_S16), paGoldenFloat[j], afScalar[j]);
            }
            u32Offset = u32ElementAlignSize * sizeof(MI_S16) + u32Offset;

            fMse = getMse((float *)OutputTensorVector.astArrayTensors[j].ptTensorData[0], paGoldenFloat[j], u32ElementAlignSize, &desc.astMI_OutputTensorDescs[j]);
            if (fMse > 2 * afScalar[j])
            {
                bRet = FALSE;
            }
        }
    }

    munmap(pmem, sb.st_size);
    close(fd);
    if (bRet == TRUE)
    {
        printf("SIT PASS !!!\n");
    }
    else
    {
        printf("SIT FAIL !!!\n");
    }

    for (unsigned int j = 0; j < desc.u32OutputTensorCount; j++)
    {
        if (paGoldenFloat[j] != NULL)
        {
            delete paGoldenFloat[j];
        }
    }

    //5. put intput tensor
    MI_IPU_PutInputTensors(u32ChannelID,&InputTensorVector);
    MI_IPU_PutOutputTensors(u32ChannelID,&OutputTensorVector);

    //6.destroy channel/device
    IPUDestroyChannel(u32ChannelID, INOUT_HEAP_SIZE);
    MI_IPU_DestroyDevice();

    return 0;
}
