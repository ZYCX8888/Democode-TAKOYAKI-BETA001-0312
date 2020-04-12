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
#include <iomanip>
#include <map>
//#include <vector>

#include <string>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sys/time.h>
#include <unistd.h>

using namespace std;

#if 0
using std::cout;
using std::endl;
using std::ostringstream;
using std::vector;
using std::string;
using std::ios;
#endif

#include "mi_common_datatype.h"
#include "mi_sys_datatype.h"
#include "mi_ipu.h"
#include "mi_sys.h"



#define  DETECT_IMAGE_FUNC_INFO(fmt, args...)           do {printf("[Info ] [%-4d] [%10s] ", __LINE__, __func__); printf(fmt, ##args);} while(0)



#define VARIABLE_BUF_SIZE (20*1000*1000)
#define INOUT_HEAP_SIZE   (6*1000*1000)
#define LABEL_CLASS_COUNT (100)
#define LABEL_NAME_MAX_SIZE (60)

struct PreProcessedData {
    char *pImagePath;
    int iResizeH;
    int iResizeW;
    int iResizeC;
    bool bRGB;
    unsigned char * pdata;

} ;


struct DetectionBBoxInfo {
    float xmin;
    float ymin;
    float xmax;
    float ymax;
    float score;
    int   classID;

};



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


    MI_S32 s32Ret ;
    MI_SYS_GlobalPrivPoolConfig_t stGlobalPrivPoolConf;
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

void GetImage(   PreProcessedData *pstPreProcessedData)
{
    string filename=(string)(pstPreProcessedData->pImagePath);
    cv::Mat sample;
    cv::Mat img = cv::imread(filename, -1);
    if (img.empty()) {
      std::cout << " error!  image don't exist!" << std::endl;
      exit(1);
    }


    int num_channels_  = pstPreProcessedData->iResizeC;
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



    cv::Mat sample_norm =sample_float ;
    if (pstPreProcessedData->bRGB)
    {
        cv::cvtColor(sample_float, sample_norm, cv::COLOR_BGR2RGB);
    }

    cv::Mat sample_resized;
    cv::Size inputSize = cv::Size(pstPreProcessedData->iResizeH, pstPreProcessedData->iResizeW);
    if (sample.size() != inputSize)
    {
		cout << "input size should be :" << pstPreProcessedData->iResizeC << " " << pstPreProcessedData->iResizeH << " " << pstPreProcessedData->iResizeW << endl;
		cout << "now input size is :" << img.channels() << " " << img.rows<<" " << img.cols << endl;
		cout << "img is going to resize!" << endl;
		cv::resize(sample_norm, sample_resized, inputSize);
	}
    else
	{
      sample_resized = sample_norm;
    }

    float *pfSrc = (float *)sample_resized.data;
    int imageSize = pstPreProcessedData->iResizeC*pstPreProcessedData->iResizeW*pstPreProcessedData->iResizeH;

    for(int i=0;i<imageSize;i++)
    {
        *(pstPreProcessedData->pdata+i) = (unsigned char)(round(*(pfSrc + i)));
    }


}


#define INNER_MOST_ALIGNMENT (8)
#define ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))


void ShowFloatOutPutTensor(float *pfBBox,float *pfClass, float *pfscore, float *pfDetect)
{
    // show bbox
    int s32DetectCount = round(*pfDetect);
    cout<<"BBox:"<<std::endl;
    cout.flags(ios::left);
    for(int i=0;i<s32DetectCount;i++)
    {
       for(int j=0;j<4;j++)
       {
            cout<<setw(15)<<*(pfBBox+(i*ALIGN_UP(4,INNER_MOST_ALIGNMENT))+j);
       }
       if (i!=0)
       {
            cout<<std::endl;
       }
    }

    //show class
    cout<<"Class:"<<std::endl;
    for(int  i=0;i<s32DetectCount;i++)
    {
       cout<<setw(15)<<round(*(pfClass+i));
    }
    cout<<std::endl;

    //show score
    cout<<"score:"<<std::endl;
    for(int  i=0;i<s32DetectCount;i++)
    {
       cout<<setw(15)<<*(pfscore+i);
    }
    cout<<std::endl;

    //show deteccout
    cout<<"DetectCount"<<std::endl;
    cout<<s32DetectCount<<std::endl;


}

 void WriteVisualizeBBox(string strImageName,
                   const vector<DetectionBBoxInfo > detections,
                   const float threshold, const vector<cv::Scalar>& colors,
                   const map<int, string>& classToDisName)
  {
  // Retrieve detections.
  cv::Mat image = cv::imread(strImageName, -1);
  map< int, vector<DetectionBBoxInfo> > detectionsInImage;

 for (unsigned int j = 0; j < detections.size(); j++) {
   DetectionBBoxInfo bbox;
   const int label = detections[j].classID;
   const float score = detections[j].score;
   if (score < threshold) {
     continue;
   }


   bbox.xmin =  detections[j].xmin*image.cols;
   bbox.xmin = bbox.xmin < 0 ? 0:bbox.xmin ;

   bbox.ymin =  detections[j].ymin*image.rows;
   bbox.ymin = bbox.ymin < 0 ? 0:bbox.ymin ;

   bbox.xmax =  detections[j].xmax*image.cols;
   bbox.xmax = bbox.xmax > image.cols?image.cols:bbox.xmax;

   bbox.ymax =  detections[j].ymax*image.rows;
   bbox.ymax = bbox.ymax > image.rows ? image.rows:bbox.ymax ;

   bbox.score = score;
   bbox.classID = label;
   detectionsInImage[label].push_back(bbox);
 }


  int fontface = cv::FONT_HERSHEY_SIMPLEX;
  double scale = 0.5;
  int thickness = 1;
  int baseline = 0;
  char buffer[50];

    // Show FPS.
//    snprintf(buffer, sizeof(buffer), "FPS: %.2f", fps);
//    cv::Size text = cv::getTextSize(buffer, fontface, scale, thickness,
//                                    &baseline);
//    cv::rectangle(image, cv::Point(0, 0),
//                  cv::Point(text.width, text.height + baseline),
//                  CV_RGB(255, 255, 255), CV_FILLED);
//    cv::putText(image, buffer, cv::Point(0, text.height + baseline / 2.),
//                fontface, scale, CV_RGB(0, 0, 0), thickness, 8);
    // Draw bboxes.
    std::string name = strImageName;

    unsigned int pos = strImageName.rfind("/");
    if (pos > 0 && pos < strImageName.size()) {
      name = name.substr(pos + 1);
    }

    std::string strOutImageName = name;
    strOutImageName = "out_"+strOutImageName;

    pos = name.rfind(".");
    if (pos > 0 && pos < name.size()) {
      name = name.substr(0, pos);
    }

    name = name + ".txt";
    std::ofstream file(name);
    for (map<int, vector<DetectionBBoxInfo> >::iterator it =
         detectionsInImage.begin(); it != detectionsInImage.end(); ++it) {
      int label = it->first;
      string label_name = "Unknown";
      if (classToDisName.find(label) != classToDisName.end()) {
        label_name = classToDisName.find(label)->second;
      }
      const cv::Scalar& color = colors[label];
      const vector<DetectionBBoxInfo>& bboxes = it->second;
      for (unsigned int j = 0; j < bboxes.size(); ++j) {
        cv::Point top_left_pt(bboxes[j].xmin, bboxes[j].ymin);
        cv::Point bottom_right_pt(bboxes[j].xmax, bboxes[j].ymax);
        cv::rectangle(image, top_left_pt, bottom_right_pt, color, 1);
        cv::Point bottom_left_pt(bboxes[j].xmin, bboxes[j].ymax);
        snprintf(buffer, sizeof(buffer), "%s: %.2f", label_name.c_str(),
                 bboxes[j].score);
        cv::Size text = cv::getTextSize(buffer, fontface, scale, thickness,
                                        &baseline);
        cv::rectangle(
            image, top_left_pt + cv::Point(0, 0),
            top_left_pt + cv::Point(text.width, -text.height - baseline),
            color, 1);
        cv::putText(image, buffer, top_left_pt - cv::Point(0, baseline),
                    fontface, scale, CV_RGB(0,255, 0), thickness, 8);
        file << label_name << " " << bboxes[j].score << " "
            << bboxes[j].xmin / image.cols << " "
            << bboxes[j].ymin / image.rows << " "
            << bboxes[j].xmax / image.cols
            << " " << bboxes[j].ymax / image.rows << std::endl;
      }
    }
    file.close();
    cv::imwrite(strOutImageName.c_str(), image);

}

std::vector<DetectionBBoxInfo >  GetDetections(float *pfBBox,float *pfClass, float *pfScore, float *pfDetect)
{
    // show bbox
    int s32DetectCount = round(*pfDetect);
    std::vector<DetectionBBoxInfo > detections(s32DetectCount);
    for(int i=0;i<s32DetectCount;i++)
    {
        DetectionBBoxInfo  detection;
        memset(&detection,0,sizeof(DetectionBBoxInfo));
        //box coordinate
        detection.ymin =  *(pfBBox+(i*ALIGN_UP(4,INNER_MOST_ALIGNMENT))+0);
        detection.xmin =  *(pfBBox+(i*ALIGN_UP(4,INNER_MOST_ALIGNMENT))+1);
        detection.ymax =  *(pfBBox+(i*ALIGN_UP(4,INNER_MOST_ALIGNMENT))+2);
        detection.xmax =  *(pfBBox+(i*ALIGN_UP(4,INNER_MOST_ALIGNMENT))+3);


        //box class
        detection.classID = round(*(pfClass+i));


        //score
        detection.score = *(pfScore+i);
        detections.push_back(detection);

    }

    return detections;

}

int  GetLabels(char *pLabelPath, char label[][LABEL_NAME_MAX_SIZE])
{
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
        }
    }

    LabelFile.close();
    return n;

}
cv::Scalar HSV2RGB(const float h, const float s, const float v) {
  const int h_i = static_cast<int>(h * 6);
  const float f = h * 6 - h_i;
  const float p = v * (1 - s);
  const float q = v * (1 - f*s);
  const float t = v * (1 - (1 - f) * s);
  float r, g, b;
  switch (h_i) {
    case 0:
      r = v; g = t; b = p;
      break;
    case 1:
      r = q; g = v; b = p;
      break;
    case 2:
      r = p; g = v; b = t;
      break;
    case 3:
      r = p; g = q; b = v;
      break;
    case 4:
      r = t; g = p; b = v;
      break;
    case 5:
      r = v; g = p; b = q;
      break;
    default:
      r = 1; g = 1; b = 1;
      break;
  }
  return cv::Scalar(r * 255, g * 255, b * 255);
}
vector<cv::Scalar> GetColors(const int n)
{
  vector<cv::Scalar> colors;
  cv::RNG rng(12345);
  const float golden_ratio_conjugate = 0.618033988749895;
  const float s = 0.3;
  const float v = 0.99;
  for (int i = 0; i < n; ++i) {
    const float h = std::fmod(rng.uniform(0.f, 1.f) + golden_ratio_conjugate,
                              1.f);
    colors.push_back(HSV2RGB(h, s, v));
  }
  return colors;
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
    char * pLabelPath = argv[4];
    MI_U32 u32ChannelID = 0;
    char * pRGB = argv[5];
    MI_BOOL bRGB = FALSE;
    MI_IPU_SubNet_InputOutputDesc_t desc;
    MI_IPU_TensorVector_t InputTensorVector;
    MI_IPU_TensorVector_t OutputTensorVector;
    static char labels[LABEL_CLASS_COUNT][LABEL_NAME_MAX_SIZE];
    int labelCount = GetLabels(pLabelPath, labels);


    //1.create device
    if(MI_SUCCESS !=IPUCreateDevice(pFirmwarePath,VARIABLE_BUF_SIZE))
    {
        cout<<"create ipu device failed!"<<std::endl;
        return -1;
    }



    //2.create channel
    if(MI_SUCCESS!=IPUCreateChannel(&u32ChannelID,pModelImgPath,INOUT_HEAP_SIZE))
    {
         cout<<"create ipu channel failed!"<<std::endl;
         MI_IPU_DestroyDevice();
         return -1;
    }


    //3.get input/output tensor
    MI_IPU_GetInOutTensorDesc(u32ChannelID, &desc);

    int iResizeH = desc.astMI_InputTensorDescs[0].u32TensorShape[1];
    int iResizeW = desc.astMI_InputTensorDescs[0].u32TensorShape[2];
    int iResizeC = desc.astMI_InputTensorDescs[0].u32TensorShape[3];
    unsigned char *pu8ImageData = new unsigned char[iResizeH*iResizeW*iResizeC];

    PreProcessedData stProcessedData;
    stProcessedData.iResizeC = iResizeC;
    stProcessedData.iResizeH = iResizeH;
    stProcessedData.iResizeW = iResizeW;
    stProcessedData.pdata = pu8ImageData;
    stProcessedData.pImagePath = pImagePath;
    if(strncmp(pRGB,"RGB",sizeof("RGB"))==0)
    {
        bRGB = TRUE;
    }
    stProcessedData.bRGB = bRGB;
    GetImage(&stProcessedData);


    MI_IPU_GetInputTensors( u32ChannelID, &InputTensorVector);

    memcpy(InputTensorVector.astArrayTensors[0].ptTensorData[0],pu8ImageData,iResizeH*iResizeW*iResizeC);
    MI_SYS_FlushInvCache(InputTensorVector.astArrayTensors[0].ptTensorData[0], iResizeH*iResizeW*iResizeC);

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
        }
    }
#if 0
    gettimeofday(&tv_end,NULL);

    int elasped_time = (tv_end.tv_sec-tv_start.tv_sec)*1000+(tv_end.tv_usec-tv_start.tv_usec)/1000;
    cout<<"fps:"<<1000.0/(float(elasped_time)/times)<<std::endl;
#endif

    // show result of detect

    float *pfBBox = (float *)OutputTensorVector.astArrayTensors[0].ptTensorData[0];
    float *pfClass = (float *)OutputTensorVector.astArrayTensors[1].ptTensorData[0];
    float *pfScore = (float *)OutputTensorVector.astArrayTensors[2].ptTensorData[0];
    float *pfDetect = (float *)OutputTensorVector.astArrayTensors[3].ptTensorData[0];

    ShowFloatOutPutTensor( pfBBox, pfClass, pfScore, pfDetect);

    std::vector<DetectionBBoxInfo >  detections  = GetDetections(pfBBox,pfClass,  pfScore,  pfDetect);
    map<int, string> labelToDisName ;
    for (int i=0; i<labelCount;i++)
    {
        labelToDisName[i] = string(&labels[i][0]);
    }
    vector<cv::Scalar> colors = GetColors(labelToDisName.size());
    WriteVisualizeBBox(pImagePath,  detections, 0.5,colors, labelToDisName);

    //5. put intput tensor

    MI_IPU_PutInputTensors(u32ChannelID,&InputTensorVector);
    MI_IPU_PutOutputTensors(u32ChannelID,&OutputTensorVector);


    //6.destroy channel/device

    delete pu8ImageData;
    IPUDestroyChannel(u32ChannelID, INOUT_HEAP_SIZE);
    MI_IPU_DestroyDevice();

    return 0;

}
