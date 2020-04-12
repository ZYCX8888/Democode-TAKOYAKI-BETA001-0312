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
#include <sys/mman.h>
#if 0
#include <openssl/aes.h>

#include <openssl/evp.h>

#include <openssl/rsa.h>
#endif
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



#define  LABEL_IMAGE_FUNC_INFO(fmt, args...)           do {printf("[Info ] [%-4d] [%10s] ", __LINE__, __func__); printf(fmt, ##args);} while(0)

#define alignment_up(a,b)  (((a)+(b-1))&(~(b-1)))



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
#define LABEL_CLASS_COUNT (1200)
#define LABEL_NAME_MAX_SIZE (60)
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


static int H2SerializedReadFunc_1(void *dst_buf,int offset, int size, char *ctx)
{
// read data from buf

}

static int H2SerializedReadFunc_2(void *dst_buf,int offset, int size, char *ctx)
{
// read data from buf
    std::cout<<"read from call back function"<<std::endl;
    memcpy(dst_buf,ctx+offset,size);
    return 0;

}

MI_S32 IPUCreateChannel(MI_U32 *s32Channel, char *pModelImage,MI_U32 u32HeapSize)
{


    MI_S32 s32Ret ;
    MI_SYS_GlobalPrivPoolConfig_t stGlobalPrivPoolConf;
    MI_IPUChnAttr_t stChnAttr;

    //create channel
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth = 2;
    stChnAttr.u32OutputBufDepth = 2;
    return MI_IPU_CreateCHN(s32Channel, &stChnAttr, NULL, pModelImage);
}




MI_S32 IPUCreateChannel_FromMemory(MI_U32 *s32Channel, char *pModelImage,MI_U32 u32HeapSize)
{

    MI_S32 s32Ret ;
    MI_SYS_GlobalPrivPoolConfig_t stGlobalPrivPoolConf;
    MI_IPUChnAttr_t stChnAttr;

    //create channel
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth = 2;
    stChnAttr.u32OutputBufDepth = 2;

    return MI_IPU_CreateCHN(s32Channel, &stChnAttr, H2SerializedReadFunc_2, pModelImage);
}



MI_S32 IPUCreateChannel_FromEncryptFile(MI_U32 *s32Channel, char *pModelImage,MI_U32 u32HeapSize)
{

    MI_S32 s32Ret ;
    MI_SYS_GlobalPrivPoolConfig_t stGlobalPrivPoolConf;
    MI_IPUChnAttr_t stChnAttr;

    //create channel
    memset(&stChnAttr, 0, sizeof(stChnAttr));
    stChnAttr.u32InputBufDepth = 2;
    stChnAttr.u32OutputBufDepth = 2;

    return MI_IPU_CreateCHN(s32Channel, &stChnAttr, H2SerializedReadFunc_2, pModelImage);
}



MI_S32 IPUDestroyChannel(MI_U32 s32Channel, MI_U32 u32HeapSize)
{
    MI_S32 s32Ret = MI_SUCCESS;

    s32Ret = MI_IPU_DestroyCHN(s32Channel);
    return s32Ret;

}

void GetImage(   PreProcessedData *pstPreProcessedData)
{
    string filename=(string)(pstPreProcessedData->pImagePath);
    cv::Mat sample;
    cv::Mat img = cv::imread(filename, -1);
    if (img.empty()) {
      std::cout << " error!  image don't exist!" << std::endl;
      exit(1);
    }


    int num_channels_  = pstPreProcessedData->intResizeC;
    if (img.channels() == 3 && num_channels_ == 1)
    {
        cv::cvtColor(img, sample, cv::COLOR_BGR2GRAY);
    }
    else if (img.channels() == 4 && num_channels_ == 1)
    {
        cv::cvtColor(img, sample, cv::COLOR_BGRA2GRAY);
    }
    else if (img.channels() == 4 && num_channels_ == 3)
    {
        cv::cvtColor(img, sample, cv::COLOR_BGRA2BGR);
    }
    else if (img.channels() == 1 && num_channels_ == 3)
    {
        cv::cvtColor(img, sample, cv::COLOR_GRAY2BGR);
    }
    else
    {
        sample = img;
    }



    cv::Mat sample_float;
    if (num_channels_ == 3)
      sample.convertTo(sample_float, CV_32FC3);
    else
      sample.convertTo(sample_float, CV_32FC1);

    cv::Mat sample_norm = sample_float;
    if (pstPreProcessedData->bRGB)
    {
        cv::cvtColor(sample_float, sample_norm, cv::COLOR_BGR2RGB);
    }


    cv::Mat sample_resized;
    cv::Size inputSize = cv::Size(pstPreProcessedData->intResizeH, pstPreProcessedData->intResizeW);
    if (sample.size() != inputSize)
    {
		cout << "input size should be :" << pstPreProcessedData->intResizeC << " " << pstPreProcessedData->intResizeH << " " << pstPreProcessedData->intResizeW << endl;
		cout << "now input size is :" << img.channels() << " " << img.rows<<" " << img.cols << endl;
		cout << "img is going to resize!" << endl;
		cv::resize(sample_norm, sample_resized, inputSize);
	}
    else
	{
      sample_resized = sample_norm;
    }

    float *pfSrc = (float *)sample_resized.data;
    int imageSize = pstPreProcessedData->intResizeC*pstPreProcessedData->intResizeW*pstPreProcessedData->intResizeH;

    for(int i=0;i<imageSize;i++)
    {
        *(pstPreProcessedData->pdata+i) = (unsigned char)(round(*(pfSrc + i)));
    }


}


static MI_BOOL GetTopN(float aData[], int dataSize, int aResult[], int TopN)
{
    int i, j, k;
    float data = 0;
    MI_BOOL bSkip = FALSE;

    for (i=0; i < TopN; i++)
    {
        data = -0.1f;
        for (j = 0; j < dataSize; j++)
        {
            if (aData[j] > data)
            {
                bSkip = FALSE;
                for (k = 0; k < i; k++)
                {
                    if (aResult[k] == j)
                    {
                        bSkip = TRUE;
                    }
                }

                if (bSkip == FALSE)
                {
                    aResult[i] = j;
                    data = aData[j];
                }
            }
        }
    }

    return TRUE;
}

int main(int argc,char *argv[])
{


    if ( argc < 6 )
    {
        std::cout << "USAGE: " << argv[0] <<": <ipu_firmware> <xxxsgsimg.img>" \
        << "<picture> " << "<labels> "<< "<model intput_format:RGB or BGR>"<<std::endl;
        exit(0);
    } else {
         std::cout<<"ipu_firmware:"<<argv[1]<<std::endl;
         std::cout<<"model_img:"<<argv[2]<<std::endl;
         std::cout<<"picture:"<<argv[3]<<std::endl;
         std::cout<<"labels:"<<argv[4]<<std::endl;
         std::cout<<"model input_format:"<<argv[5]<<std::endl;
    }


    char * pFirmwarePath = argv[1];
    char * pModelImgPath = argv[2];
    char * pImagePath= argv[3];
    char * pLabelPath =argv[4];
    char * pRGB = argv[5];
    MI_BOOL bRGB = FALSE;
    static char label[LABEL_CLASS_COUNT][LABEL_NAME_MAX_SIZE];
    MI_U32 u32ChannelID = 0;
    MI_S32 s32Ret;
    MI_IPU_SubNet_InputOutputDesc_t desc;
    MI_IPU_TensorVector_t InputTensorVector;
    MI_IPU_TensorVector_t OutputTensorVector;
    ifstream LabelFile;
    LabelFile.open(pLabelPath);
    int n=0;
    while(1)
    {
        LabelFile.getline(&label[n][0],60);
        if(LabelFile.eof())
            break;
        n++;
        if(n>=LABEL_CLASS_COUNT)
        {
            cout<<"the labels have line:"<<n<<" ,it supass the available label array"<<std::endl;
            break;
        }
    }

    LabelFile.close();



    //1.create device
    if(MI_SUCCESS !=IPUCreateDevice(pFirmwarePath,VARIABLE_BUF_SIZE))
    {
        cout<<"create ipu device failed!"<<std::endl;
        return -1;

    }



    //2.create channel
    /*case 0 create module from path*/
   #if 0
        if(MI_SUCCESS !=IPUCreateChannel(u32ChannelID,pModelImgPath,INOUT_HEAP_SIZE))
    {
         cout<<"create ipu channel failed!"<<std::endl;
         MI_IPU_DestroyDevice();
         return -1;
    }
   #endif

   #if 1
   /*case1 create channel from memory*/
    cout<<"create channel from memory__"<<std::endl;
     char *pmem = NULL;
    int fd = 0;
    struct stat sb;
    fd = open(pModelImgPath, O_RDWR);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }
    memset(&sb, 0, sizeof(sb));
    if (fstat(fd, &sb) < 0)
    {
        perror("fstat");
        return -1;
    }
    pmem = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (pmem == NULL)
    {
        perror("mmap");
        return -1;
    }
    if(MI_SUCCESS !=IPUCreateChannel_FromMemory(&u32ChannelID,pmem,INOUT_HEAP_SIZE))
    {
         cout<<"create ipu channel failed!"<<std::endl;
         MI_IPU_DestroyDevice();
         return -1;
    }
   #endif

   #if 0
   /*
     case 3 encrypt and decrypt the img file , is not ready
   */


    cout<<"decrypt sgs image"<<std::endl;
    char *pmem = NULL;
    int fd = 0;
    struct stat sb;
    fd = open(pModelImgPath, O_RDWR);
    if (fd < 0)
    {
        perror("open");
        return -1;
    }
    memset(&sb, 0, sizeof(sb));
    if (fstat(fd, &sb) < 0)
    {
        perror("fstat");
        return -1;
    }
    cout<<"the img is 16 byte alignment ?"<<(sb.st_size&15)<<std::endl;
    pmem = (char *)mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (pmem == NULL)
    {
        perror("mmap");
        return -1;
    }
    int alignment_size = alignment_up(sb.st_size,16);
    unsigned char * encrypt_buffer = new unsigned char[alignment_size];
    unsigned char * decrypt_buffer = new unsigned char[alignment_size];
    memset(decrypt_buffer,0,alignment_size);
    memset(encrypt_buffer,0,alignment_size);
    static unsigned char dahua_keyval[16] ;
    strncpy((char *)dahua_keyval,"1234567890987654",16);
    AES_KEY  aesEncryptKey;
    AES_KEY  aesDecryptKey;
    AES_set_encrypt_key(dahua_keyval, 128, &aesEncryptKey);
    AES_set_encrypt_key(dahua_keyval, 128, &aesDecryptKey);
     cout<<"line:"<<__LINE__<<std::endl;
     cout<<"alignment_size:"<<alignment_size<<std::endl;
    //AES_BLOCK_SIZE
    for(int i=0;i<1;i++)
    {
        AES_ecb_encrypt((unsigned char *)pmem + AES_BLOCK_SIZE * i, encrypt_buffer + AES_BLOCK_SIZE * i, &aesEncryptKey, AES_ENCRYPT);
    }


    for(int i=0;i<1;i++)
    {
         AES_ecb_encrypt(encrypt_buffer  + AES_BLOCK_SIZE * i, decrypt_buffer + AES_BLOCK_SIZE * i, &aesDecryptKey, AES_DECRYPT);
    }


  cout<<"AES_BLOCK_SIZE"<<AES_BLOCK_SIZE<<std::endl;
  cout<<"pmem:"<<*(int *)pmem<<std::endl;
  cout<<"encrypt:"<<*(int *)encrypt_buffer<<std::endl;
  cout<<"decrypt_buffer:"<<*(int *)decrypt_buffer<<std::endl;


    if(MI_SUCCESS !=IPUCreateChannel_FromMemory(u32ChannelID,(char *)decrypt_buffer,INOUT_HEAP_SIZE))
    {
         cout<<"create ipu channel failed!"<<std::endl;
         MI_IPU_DestroyDevice();
         return -1;
    }
    cout<<"line:"<<__LINE__<<std::endl;
    //delete encrypt_buffer;
    //delete decrypt_buffer;
   #endif



    //3.get input/output tensor
    s32Ret = MI_IPU_GetInOutTensorDesc(u32ChannelID, &desc);
    if (s32Ret == MI_SUCCESS) {
        for (int i = 0; i < desc.u32InputTensorCount; i++) {
            cout<<"input tensor["<<i<<"] name :"<<desc.astMI_InputTensorDescs[i].name<<endl;
        }
        for (int i = 0; i < desc.u32OutputTensorCount; i++) {
            cout<<"output tensor["<<i<<"] name :"<<desc.astMI_OutputTensorDescs[i].name<<endl;
        }
    }


    int intResizeH = desc.astMI_InputTensorDescs[0].u32TensorShape[1];
    int intResizeW = desc.astMI_InputTensorDescs[0].u32TensorShape[2];
    int intResizeC = desc.astMI_InputTensorDescs[0].u32TensorShape[3];
    unsigned char *pu8ImageData = new unsigned char[intResizeH*intResizeW*intResizeC];

    PreProcessedData stProcessedData;
    stProcessedData.intResizeC = intResizeC;
    stProcessedData.intResizeH = intResizeH;
    stProcessedData.intResizeW = intResizeW;
    stProcessedData.pdata = pu8ImageData;
    stProcessedData.pImagePath = pImagePath;
    if(strncmp(pRGB,"RGB",sizeof("RGB"))==0)
    {
        bRGB = TRUE;
    }
    stProcessedData.bRGB = bRGB;
    GetImage(&stProcessedData);


    MI_IPU_GetInputTensors( u32ChannelID, &InputTensorVector);

    memcpy(InputTensorVector.astArrayTensors[0].ptTensorData[0],pu8ImageData,intResizeH*intResizeW*intResizeC);
    MI_SYS_FlushInvCache(InputTensorVector.astArrayTensors[0].ptTensorData[0], intResizeH*intResizeW*intResizeC);

    MI_IPU_GetOutputTensors( u32ChannelID, &OutputTensorVector);


    //4.invoke
 #if 0
    struct  timeval    tv_start;
    struct  timeval    tv_end;

    gettimeofday(&tv_start,NULL);
#endif
    int times = 1;
    for (int i=0;i<times;i++ )
    {
        if(MI_SUCCESS!=MI_IPU_Invoke(u32ChannelID, &InputTensorVector, &OutputTensorVector))
        {
            cout<<"IPU invoke failed!!"<<endl;
            delete pu8ImageData;
            IPUDestroyChannel(u32ChannelID, INOUT_HEAP_SIZE);
            MI_IPU_DestroyDevice();
            return -1;
        }
    }
#if 0
    gettimeofday(&tv_end,NULL);

    int elasped_time = (tv_end.tv_sec-tv_start.tv_sec)*1000+(tv_end.tv_usec-tv_start.tv_usec)/1000;
    cout<<"fps:"<<1000.0/(float(elasped_time)/times)<<std::endl;
#endif

    // show result of classify

    int s32TopN[5];
    memset(s32TopN,0,sizeof(s32TopN));
    int iDimCount = desc.astMI_OutputTensorDescs[0].u32TensorDim;
    int s32ClassCount  = 1;
    for(int i=0;i<iDimCount;i++ )
    {
      s32ClassCount *= desc.astMI_OutputTensorDescs[0].u32TensorShape[i];
    }
    float *pfData = (float *)OutputTensorVector.astArrayTensors[0].ptTensorData[0];

    cout<<"the class Count :"<<s32ClassCount<<std::endl;
    cout<<std::endl;
    cout<<std::endl;
    GetTopN(pfData, s32ClassCount, s32TopN, 5);


    for(int i=0;i<5;i++)
    {
      cout<<"order "<<i<<": "<<pfData[s32TopN[i]]<<" "<<s32TopN[i]<<" "<<label[s32TopN[i]]<<endl;

    }


    //5. put intput tensor

    MI_IPU_PutInputTensors(u32ChannelID,&InputTensorVector);
    MI_IPU_PutOutputTensors(u32ChannelID,&OutputTensorVector);


    //6.destroy channel/device

   delete pu8ImageData;
   IPUDestroyChannel(u32ChannelID, INOUT_HEAP_SIZE);
   MI_IPU_DestroyDevice();

    return 0;

}
