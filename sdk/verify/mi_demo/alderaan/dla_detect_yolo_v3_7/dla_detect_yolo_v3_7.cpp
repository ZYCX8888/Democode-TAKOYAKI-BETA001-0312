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



#define VARIABLE_BUF_SIZE (17*1024*1024)
#define INOUT_HEAP_SIZE   (18*1024*1024)
#define LABEL_CLASS_COUNT (100)
#define LABEL_NAME_MAX_SIZE (60)
#define CPU_POST_PROCESS (1)
#if 1==CPU_POST_PROCESS
#define CPU_POST_PROCESS_GOLDEN (0)
#include "math.h"
#include "yolov3_filter_golden_float19x19.h"
#include "yolov3_filter_golden_float38x38.h"
#include "yolov3_filter_golden_float76x76.h"

#define MAX_FILTER_COUNT (1024)
#define BBOX_TOTAL_INFO_COUNT (85)
#define BBOX_SCORES_COUNT (80)
#define FILTER_INFO_COUNT (6)
#define BOX_INFO_COUNT (4)
#define CONFRENCE_THRESHOLD (0.05)
#define IOU_THRESHOLD (0.45)
#define INPUT_WIDTH (608)
#define INPUT_HEIGHT (608)
#define IMG_WIDTH (640)
#define IMG_HEIGHT (426)

static inline float logistic(float x){return (1.f / (1.f + exp(-x)));}



#endif
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
    stChnAttr.u32InputBufDepth = 1;
    stChnAttr.u32OutputBufDepth = 1;
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


#define INNER_MOST_ALIGNMENT (4)
#define ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))


void ShowFloatOutPutTensor(float *pfBBox,float *pfClass, float *pfscore, float *pfDetect)
{
    // show bbox
    int s32DetectCount = round(*pfDetect);
    cout<<"BBox:"<<s32DetectCount<<std::endl;
    cout.flags(ios::left);
    for(int i=0;i<s32DetectCount;i++)
    {
       for(int j=0;j<4;j++)
       {
            cout<<setw(15)<<*(pfBBox+(i*ALIGN_UP(4,INNER_MOST_ALIGNMENT))+j);
       }
       cout<<std::endl;
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
  double scale = 1;
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
        cv::rectangle(image, top_left_pt, bottom_right_pt, color, 4);
        cv::Point bottom_left_pt(bboxes[j].xmin, bboxes[j].ymax);
        snprintf(buffer, sizeof(buffer), "%s: %.2f", label_name.c_str(),
                 bboxes[j].score);
        cv::Size text = cv::getTextSize(buffer, fontface, scale, thickness,
                                        &baseline);
        cv::rectangle(
            image, bottom_left_pt + cv::Point(0, 0),
            bottom_left_pt + cv::Point(text.width, -text.height - baseline),
            color, cv::FILLED);
        cv::putText(image, buffer, bottom_left_pt - cv::Point(0, baseline),
                    fontface, scale, CV_RGB(0, 0, 0), thickness, 8);
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

#if CPU_POST_PROCESS
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#define min(a,b)    (((a) < (b)) ? (a) : (b))

bool sgs_Refine_ValidateBoxes(DetectionBBoxInfo *box) {
    if(box->ymin < 0.0) box->ymin = 0.0;
    if(box->xmin < 0.0) box->xmin = 0.0;
    if(box->ymax > 1.0*IMG_HEIGHT) box->ymax = 1.0*IMG_HEIGHT;
    if(box->xmax > 1.0*IMG_WIDTH) box->xmax = 1.0*IMG_WIDTH;
    return TRUE;
}

float sgs_ComputeIoU(const float* FilterPP, const int i, const int j) {
    float ret = 0.0;
    DetectionBBoxInfo box_i;
    DetectionBBoxInfo box_j;
    memcpy(&box_i, FilterPP+i*FILTER_INFO_COUNT, BOX_INFO_COUNT*sizeof(float));
    memcpy(&box_j, FilterPP+j*FILTER_INFO_COUNT, BOX_INFO_COUNT*sizeof(float));
    sgs_Refine_ValidateBoxes(&box_i);
    sgs_Refine_ValidateBoxes(&box_j);
    const float area_i = (box_i.ymax - box_i.ymin) * (box_i.xmax - box_i.xmin);
    const float area_j = (box_j.ymax - box_j.ymin) * (box_j.xmax - box_j.xmin);
    if (area_i <= 0 || area_j <= 0) return 0.0;
    const float intersection_ymin = max(box_i.ymin, box_j.ymin);
    const float intersection_xmin = max(box_i.xmin, box_j.xmin);
    const float intersection_ymax = min(box_i.ymax, box_j.ymax);
    const float intersection_xmax = min(box_i.xmax, box_j.xmax);
    const float intersection_area =
        max(intersection_ymax - intersection_ymin, 0.0) *
        max(intersection_xmax - intersection_xmin, 0.0);
    ret = intersection_area / (area_i + area_j - intersection_area);
    return ret;
}


bool sgs_max(float* FilterPP, MI_U32 filter_len, float *pf, MI_U32 j, MI_U32 index, float conference){
    float temp[BBOX_SCORES_COUNT] = {0};
    int offset = j*BBOX_TOTAL_INFO_COUNT + j/3;
    memcpy(temp, (const void*)(pf+offset+BOX_INFO_COUNT+1), BBOX_SCORES_COUNT*sizeof(float));
    float max=logistic(temp[0])*conference;
    int max_index=0;
    for(int i=0; i<BBOX_SCORES_COUNT; i++){
        float log_temp = logistic(temp[i])*conference;
        if(log_temp>max) {
            max_index=i;
            max=log_temp;
        }
    }
    int anchors[9][2] = {10,13, 16,30, 33,23, 30,61, 62,45, 59,119, 116,90, 156,198, 373,326};
    int mask[3][3] = {6,7,8, 3,4,5, 0,1,2};
    MI_U32 w;
    MI_U32 h;
    MI_U32 k;
    if(0==index) {w=19; h=19; k=j%3;}
    if(1==index) {w=38; h=38; k=j%3;}
    if(2==index) {w=76; h=76; k=j%3;}

    float bx = (logistic(*(float*)(pf+offset+0))+(j/3)%w)/w;
    float by = (logistic(*(float*)(pf+offset+1))+(j/3)/h)/h;
    int temp_mask = mask[index][k];
    float bw = anchors[temp_mask][0]*exp(*(float*)(pf+offset+2))/INPUT_WIDTH;
    float bh = anchors[temp_mask][1]*exp(*(float*)(pf+offset+3))/INPUT_HEIGHT;

    float x1 = (bx-bw/2)*IMG_WIDTH;
    float y1 = (by-bh/2)*IMG_HEIGHT;
    float x2 = (bx+bw/2)*IMG_WIDTH;
    float y2 = (by+bh/2)*IMG_HEIGHT;

    *(float*)(FilterPP+filter_len*FILTER_INFO_COUNT+0) = y1;
    *(float*)(FilterPP+filter_len*FILTER_INFO_COUNT+1) = x1;
    *(float*)(FilterPP+filter_len*FILTER_INFO_COUNT+2) = y2;
    *(float*)(FilterPP+filter_len*FILTER_INFO_COUNT+3) = x2;
    *(float*)(FilterPP+filter_len*FILTER_INFO_COUNT+4) = max;
    *(float*)(FilterPP+filter_len*FILTER_INFO_COUNT+5) = max_index;
    return TRUE;
}

MI_U32 CountActiveBox(bool* active_box_candidate, MI_U32 filter_len) {
    MI_U32 ret = 0;
    for(MI_U32 i=0; i<filter_len; i++){
        if(FALSE == active_box_candidate[i]) ret++;
    }
    return ret;
}

void PostProcess(MI_IPU_TensorVector_t* OutputTensorVector, MI_IPU_TensorVector_t* OutputPPTensorVector)
{
    struct  timeval    tv_start;
    struct  timeval    tv_end;
    int elasped_time = 0;
    int times = 1;
    gettimeofday(&tv_start,NULL);

    float *pfBBox = (float *)OutputPPTensorVector->astArrayTensors[0].ptTensorData[0];
    float *pfClass = (float *)OutputPPTensorVector->astArrayTensors[1].ptTensorData[0];
    float *pfScore = (float *)OutputPPTensorVector->astArrayTensors[2].ptTensorData[0];
    float *pfDetect = (float *)OutputPPTensorVector->astArrayTensors[3].ptTensorData[0];
    //mobileV1_YoloV3 as example
    MI_IPU_SubNet_InputOutputDesc_t desc;
    MI_IPU_GetInOutTensorDesc(0, &desc);

    float *pf=NULL;
    #if CPU_POST_PROCESS_GOLDEN
    extern float _fGolden19x19[];
    extern float _fGolden38x38[];
    extern float _fGolden76x76[];

    gettimeofday(&tv_end,NULL);
    elasped_time = (tv_end.tv_sec-tv_start.tv_sec)*1000+(tv_end.tv_usec-tv_start.tv_usec)/1000;
    cout<<"--> pp 1 fps:"<<elasped_time<<", "<<1000.0/(float(elasped_time)/times)<<std::endl;
    gettimeofday(&tv_start,NULL);
    #endif
    //filter
    float* FilterPP = (float*)malloc(FILTER_INFO_COUNT*MAX_FILTER_COUNT*sizeof(float));
    MI_U32 filter_len = 0;
    for(MI_U32 i=0; i<desc.u32OutputTensorCount;i++){
        if(filter_len >= MAX_FILTER_COUNT){
            cout<<"Attention: filter_len= \n"<<filter_len<<std::endl;
            break;
        }
        MI_U32 len=desc.astMI_OutputTensorDescs[i].u32TensorShape[0]
            *desc.astMI_OutputTensorDescs[i].u32TensorShape[1]
            *desc.astMI_OutputTensorDescs[i].u32TensorShape[2]
            *(desc.astMI_OutputTensorDescs[i].u32TensorShape[3]/BBOX_TOTAL_INFO_COUNT);
        #if CPU_POST_PROCESS_GOLDEN
        if(0==i) pf = (float *)_fGolden19x19;
        if(1==i) pf = (float *)_fGolden38x38;
        if(2==i) pf = (float *)_fGolden76x76;
        {
            struct  timeval    tv_start1;
            struct  timeval    tv_end1;
            int elasped_time1= 0;
            int times1= 1;
            gettimeofday(&tv_start1,NULL);
            float conference = logistic(pf[BOX_INFO_COUNT]);
            gettimeofday(&tv_end1, NULL);
            elasped_time1 = (tv_end1.tv_sec-tv_start1.tv_sec)*1000+(tv_end1.tv_usec-tv_start1.tv_usec)/1000;
            cout<<"--> pp one logistic fps:"<<elasped_time1<<", "<<1000.0/(float(elasped_time1)/times1)<<std::endl;
        }
        #else
        cout<<"["<<__LINE__<<"] "<<i<<" len="<<len<<",filter_len="<<filter_len<<std::endl;
        pf = (float *)OutputTensorVector->astArrayTensors[i].ptTensorData[0];
        #endif
        for(MI_U32 j=0; j<len ;j++){
            int offset = 0;
            offset = j*BBOX_TOTAL_INFO_COUNT + j/3 + BOX_INFO_COUNT;
            float conference = logistic(pf[offset]);
            if(conference > CONFRENCE_THRESHOLD){
                if(filter_len >= MAX_FILTER_COUNT){
                    cout<<"Attention2: filter_len= \n"<<filter_len<<std::endl;
                    break;
                }
                sgs_max(FilterPP, filter_len, pf, j, i, conference);
                filter_len++;
            }
        }
    }
    gettimeofday(&tv_end,NULL);
    elasped_time = (tv_end.tv_sec-tv_start.tv_sec)*1000+(tv_end.tv_usec-tv_start.tv_usec)/1000;
    cout<<"--> pp 2.filter fps:"<<elasped_time<<", "<<1000.0/(float(elasped_time)/times)<<std::endl;
    gettimeofday(&tv_start,NULL);

    // Perform non-maximal suppression on max scores
    MI_U32 num_saved = 0;
    MI_U32 num_active_candidate = filter_len;
    bool* active_box_candidate = (bool*)malloc(filter_len*sizeof(bool));
    memset(active_box_candidate, FALSE, filter_len*sizeof(bool));
    float max=0;
    MI_U32 max_index=0;

    for(MI_U32 i=0; i<filter_len; i++){
        num_active_candidate = CountActiveBox(active_box_candidate, filter_len);
        if (0==num_active_candidate || num_saved>=LABEL_CLASS_COUNT) break;
        //Max
        max = 0;
        max_index = 0;
        for(MI_U32 j=0; j<filter_len; j++){
            if((FilterPP[j*FILTER_INFO_COUNT+BOX_INFO_COUNT]>max)  && (FALSE==active_box_candidate[j])) {
                max = FilterPP[j*FILTER_INFO_COUNT+BOX_INFO_COUNT];
                max_index = j;
            }
        }
        *(pfScore+num_saved)=max;
        *(pfClass+num_saved)=FilterPP[max_index*FILTER_INFO_COUNT+BOX_INFO_COUNT+1];
        *(float*)(pfBBox+num_saved*BOX_INFO_COUNT+0) = FilterPP[max_index*FILTER_INFO_COUNT+0];
        *(float*)(pfBBox+num_saved*BOX_INFO_COUNT+1) = FilterPP[max_index*FILTER_INFO_COUNT+1];
        *(float*)(pfBBox+num_saved*BOX_INFO_COUNT+2) = FilterPP[max_index*FILTER_INFO_COUNT+2];
        *(float*)(pfBBox+num_saved*BOX_INFO_COUNT+3) = FilterPP[max_index*FILTER_INFO_COUNT+3];
        num_saved++;
        active_box_candidate[max_index] = TRUE;
        //IoU
        for(MI_U32 j=0; j<filter_len; j++) {
            if (FALSE==active_box_candidate[j]) {
                float intersection_over_union = sgs_ComputeIoU(FilterPP, max_index, j);
                if (intersection_over_union > IOU_THRESHOLD) {
                    active_box_candidate[j] = TRUE;
                    num_active_candidate--;
                }
            }
        }
    }
    *(float*)pfDetect=num_saved;
    cout<<"["<<__LINE__<<"] num_saved="<<num_saved<<std::endl;

    gettimeofday(&tv_end,NULL);
    elasped_time = (tv_end.tv_sec-tv_start.tv_sec)*1000+(tv_end.tv_usec-tv_start.tv_usec)/1000;
    cout<<"--> pp 3.NMS fps:"<<elasped_time<<", "<<1000.0/(float(elasped_time)/times)<<std::endl;

    free(active_box_candidate);
    free(FilterPP);
}
#endif

int main(int argc,char *argv[])
{

    if ( argc < 5 )
    {
        std::cout << "USAGE: " << argv[0] <<": <ipu_firmware> <xxsgsimg.img>" \
        << "<xxxx.jpg> "<<"<label.txt>" << std::endl;
        exit(0);
    }


    char * pFirmwarePath = argv[1];
    char * pModelImgPath = argv[2];
    char * pImagePath= argv[3];
    char * pLabelPath = argv[4];
    MI_U32 u32ChannelID = 0;
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
    stProcessedData.bRGB = FALSE;
    GetImage(&stProcessedData);


    MI_IPU_GetInputTensors( u32ChannelID, &InputTensorVector);

    memcpy(InputTensorVector.astArrayTensors[0].ptTensorData[0],pu8ImageData,iResizeH*iResizeW*iResizeC);
    MI_SYS_FlushInvCache(InputTensorVector.astArrayTensors[0].ptTensorData[0], iResizeH*iResizeW*iResizeC);

    MI_IPU_GetOutputTensors( u32ChannelID, &OutputTensorVector);


    //4.invoke
 #if 1
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
#if 1
    gettimeofday(&tv_end,NULL);

    int elasped_time = (tv_end.tv_sec-tv_start.tv_sec)*1000+(tv_end.tv_usec-tv_start.tv_usec)/1000;
    cout<<"-->  invoke fps:"<<elasped_time<<", "<<1000.0/(float(elasped_time)/times)<<std::endl;
#endif
#if 1==CPU_POST_PROCESS
    MI_IPU_TensorVector_t OutputPPTensorVector;
    OutputPPTensorVector.u32TensorCount = 4;
    OutputPPTensorVector.astArrayTensors[0].ptTensorData[0]=malloc(LABEL_CLASS_COUNT*BOX_INFO_COUNT*sizeof(float));
    OutputPPTensorVector.astArrayTensors[1].ptTensorData[0]=malloc(LABEL_CLASS_COUNT*sizeof(float));
    OutputPPTensorVector.astArrayTensors[2].ptTensorData[0]=malloc(LABEL_CLASS_COUNT*sizeof(float));
    OutputPPTensorVector.astArrayTensors[3].ptTensorData[0]=malloc(1*sizeof(float));
    gettimeofday(&tv_start,NULL);
    PostProcess(&OutputTensorVector, &OutputPPTensorVector);
    gettimeofday(&tv_end,NULL);
    elasped_time = (tv_end.tv_sec-tv_start.tv_sec)*1000+(tv_end.tv_usec-tv_start.tv_usec)/1000;
    cout<<"--> pp total fps:"<<elasped_time<<", "<<1000.0/(float(elasped_time)/times)<<std::endl;

    // show result of detect
    float *pfBBox = (float *)OutputPPTensorVector.astArrayTensors[0].ptTensorData[0];
    float *pfClass = (float *)OutputPPTensorVector.astArrayTensors[1].ptTensorData[0];
    float *pfScore = (float *)OutputPPTensorVector.astArrayTensors[2].ptTensorData[0];
    float *pfDetect = (float *)OutputPPTensorVector.astArrayTensors[3].ptTensorData[0];
#else
    // show result of detect
    float *pfBBox = (float *)OutputTensorVector.astArrayTensors[0].ptTensorData[0];
    float *pfClass = (float *)OutputTensorVector.astArrayTensors[1].ptTensorData[0];
    float *pfScore = (float *)OutputTensorVector.astArrayTensors[2].ptTensorData[0];
    float *pfDetect = (float *)OutputTensorVector.astArrayTensors[3].ptTensorData[0];
#endif

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
#if 1==CPU_POST_PROCESS
    free(OutputPPTensorVector.astArrayTensors[0].ptTensorData[0]);
    free(OutputPPTensorVector.astArrayTensors[1].ptTensorData[0]);
    free(OutputPPTensorVector.astArrayTensors[2].ptTensorData[0]);
    free(OutputPPTensorVector.astArrayTensors[3].ptTensorData[0]);
#endif
    delete pu8ImageData;
    IPUDestroyChannel(u32ChannelID, INOUT_HEAP_SIZE);
    MI_IPU_DestroyDevice();

    return 0;

}
