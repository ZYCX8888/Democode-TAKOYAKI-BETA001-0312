/*
* yolov2_offline.cpp- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: XXXX <XXXX@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#include <math.h>
#include "yolov2_offline.h"

using std::vector;
using std::map;

const std::string funcName = "subnet0";
const std::string dla_miu = "dla_miu";
const std::string dla_base_address = "dla_base_address";
const std::string dla_buf_size = "dla_buf_size";
const std::string file_dla_info = "/proc/dla/address_info";
int dalMiu = -1;
unsigned long dlaBaseAddr = 0x0;
unsigned long dlaBufSize = 0x0;

void initDlaConf()
{
    std::ifstream dlaconf (file_dla_info.c_str());
    std::string::size_type pos = std::string::npos;
    std::string conf_val;
    std::string line;
    while(std::getline(dlaconf,line)) {
        std::cout<<line<<std::endl;
        if ((pos = line.find(dla_miu))
            != std::string::npos) {
            conf_val = line.substr(pos + dla_miu.size());
            sscanf(conf_val.c_str(), "%d", &dalMiu);
            std::cout<<"dalMiu="<<dalMiu<<std::endl;
        } else if((pos = line.find(dla_base_address))
            != std::string::npos){
            conf_val = line.substr(pos +  dla_base_address.size());
            sscanf(conf_val.c_str(), "%lx", &dlaBaseAddr);
            std::cout<<"dlaBaseAddr="<<std::hex<<dlaBaseAddr<<std::endl;
        } else if((pos=line.find(dla_buf_size))
            !=std::string::npos) {
            conf_val = line.substr(pos + dla_buf_size.size());
            sscanf(conf_val.c_str(), "%lx", &dlaBufSize);
            std::cout<<"dla_buf_size="<<std::hex<<dlaBufSize<<std::endl;
        }
    }
}

void IntersectBBox(const NormalizedBBox& bbox1, const NormalizedBBox& bbox2,
                   NormalizedBBox* intersect_bbox) {
  if (bbox2.xmin() > bbox1.xmax() || bbox2.xmax() < bbox1.xmin() ||
      bbox2.ymin() > bbox1.ymax() || bbox2.ymax() < bbox1.ymin()) {
    // Return [0, 0, 0, 0] if there is no intersection.
    intersect_bbox->set_xmin(0);
    intersect_bbox->set_ymin(0);
    intersect_bbox->set_xmax(0);
    intersect_bbox->set_ymax(0);
  } else {
    intersect_bbox->set_xmin(std::max(bbox1.xmin(), bbox2.xmin()));
    intersect_bbox->set_ymin(std::max(bbox1.ymin(), bbox2.ymin()));
    intersect_bbox->set_xmax(std::min(bbox1.xmax(), bbox2.xmax()));
    intersect_bbox->set_ymax(std::min(bbox1.ymax(), bbox2.ymax()));
  }
}

float BBoxSize(const NormalizedBBox& bbox, const bool normalized = true) {
  if (bbox.xmax() < bbox.xmin() || bbox.ymax() < bbox.ymin()) {
    // If bbox is invalid (e.g. xmax < xmin or ymax < ymin), return 0.
    return 0;
  } else {
    if (bbox.has_size()) {
      return bbox.size();
    } else {
      float width = bbox.xmax() - bbox.xmin();
      float height = bbox.ymax() - bbox.ymin();
      if (normalized) {
        return width * height;
      } else {
        // If bbox is not within range [0, 1].
        return (width + 1) * (height + 1);
      }
    }
  }
}

float JaccardOverlap(const NormalizedBBox& bbox1, const NormalizedBBox& bbox2,
                     const bool normalized) {
  NormalizedBBox intersect_bbox;
  IntersectBBox(bbox1, bbox2, &intersect_bbox);
  float intersect_width, intersect_height;
  if (normalized) {
    intersect_width = intersect_bbox.xmax() - intersect_bbox.xmin();
    intersect_height = intersect_bbox.ymax() - intersect_bbox.ymin();
  } else {
    intersect_width = intersect_bbox.xmax() - intersect_bbox.xmin() + 1;
    intersect_height = intersect_bbox.ymax() - intersect_bbox.ymin() + 1;
  }
  if (intersect_width > 0 && intersect_height > 0) {
    float intersect_size = intersect_width * intersect_height;
    float bbox1_size = BBoxSize(bbox1);
    float bbox2_size = BBoxSize(bbox2);
    return intersect_size / (bbox1_size + bbox2_size - intersect_size);
  } else {
    return 0.;
  }
}

template <typename Dtype>
void setNormalizedBBox(NormalizedBBox* bbox,
                       Dtype x, Dtype y, Dtype w, Dtype h) {
  Dtype xmin = x - w / 2.0;
  Dtype xmax = x + w / 2.0;
  Dtype ymin = y - h / 2.0;
  Dtype ymax = y + h / 2.0;

  if (xmin < 0.0) {
    xmin = 0.0;
  }
  if (xmax > 1.0) {
    xmax = 1.0;
  }
  if (ymin < 0.0) {
    ymin = 0.0;
  }
  if (ymax > 1.0) {
    ymax = 1.0;
  }
  bbox->set_xmin(xmin);
  bbox->set_ymin(ymin);
  bbox->set_xmax(xmax);
  bbox->set_ymax(ymax);
  float bbox_size = BBoxSize(*bbox, true);
  bbox->set_size(bbox_size);
}

template <typename Dtype>
class PredictionResult{
  public:
    Dtype x;
    Dtype y;
    Dtype w;
    Dtype h;
    Dtype objScore;
    Dtype classScore;
    Dtype confidence;
    int classType;
};
template <typename Dtype>
void class_index_and_score(Dtype* input, int classes,
                           PredictionResult<Dtype>* predict) {
  Dtype sum = 0;
  Dtype large = input[0];
  int classIndex = 0;
  for (int i = 0; i < classes; ++i) {
    if (input[i] > large)
      large = input[i];
  }
  for (int i = 0; i < classes; ++i) {
    Dtype e = exp(input[i] - large);
    sum += e;
    input[i] = e;
  }

  for (int i = 0; i < classes; ++i) {
    input[i] = input[i] / sum;
  }
  large = input[0];
  classIndex = 0;

  for (int i = 0; i < classes; ++i) {
    if (input[i] > large) {
      large = input[i];
      classIndex = i;
    }
  }
  predict->classType = classIndex;
  predict->classScore = large;
}

template <typename Dtype>
inline Dtype sigmoid(Dtype x) {
  return 1. / (1. + exp(-x));
}

template <typename Dtype>
void get_region_box(Dtype* x, PredictionResult<Dtype>* predict,
                    vector<Dtype> biases, int n, int index,
                    int i, int j, int w, int h) {
  predict->x = (i + sigmoid(x[index + 0])) / w;
  predict->y = (j + sigmoid(x[index + 1])) / h;
  predict->w = exp(x[index + 2]) * biases[2 * n] / w;
  predict->h = exp(x[index + 3]) * biases[2 * n + 1] / h;
}

template <typename Dtype>
void ApplyNms(vector< PredictionResult<Dtype> >* boxes,
              vector<int>* idxes, Dtype threshold) {
  map<int, int> idx_map;
  for (int i = 0; i < (*boxes).size() - 1; ++i) {
    if (idx_map.find(i) != idx_map.end()) {
      continue;
    }
    for (int j = i + 1; j < (*boxes).size(); ++j) {
      if (idx_map.find(j) != idx_map.end()) {
        continue;
      }
      NormalizedBBox Bbox1, Bbox2;
      setNormalizedBBox(&Bbox1, (*boxes)[i].x, (*boxes)[i].y,
                        (*boxes)[i].w, (*boxes)[i].h);
      setNormalizedBBox(&Bbox2, (*boxes)[j].x, (*boxes)[j].y,
                        (*boxes)[j].w, (*boxes)[j].h);

      float overlap = JaccardOverlap(Bbox1, Bbox2, true);

      if (overlap >= threshold) {
        idx_map[j] = 1;
      }
    }
  }
  for (int i = 0; i < (*boxes).size(); ++i) {
    if (idx_map.find(i) == idx_map.end()) {
      (*idxes).push_back(i);
    }
  }
}

#if 0
void get_point_position(const vector<float> pos,
                        Point* p1, Point* p2, int h, int w) {
#if 0
  int left = (pos[3] - pos[5] / 2) * w;
  int right = (pos[3] + pos[5] / 2) * w;
  int top = (pos[4] - pos[6] / 2) * h;
  int bottom = (pos[4] + pos[6] / 2) * h;
  if (left < 0) left = 5;
  if (top < 0) top = 5;
  if (right > w) right = w-5;
  if (bottom > h) bottom = h-5;
#if 0
  p1->x = left;
  p1->y = top;
  p2->x = right;
  p2->y = bottom;
#endif

  p1->x = (left * w) / 416;
  p1->y = (top * h) / 416;
  p2->x = (right * w) / 416;
  p2->y = (bottom * h) / 416;
#endif

  int width = 416, height = 416;

  long left = (pos[3] - pos[5] / 2) * width;
  long right = (pos[3] + pos[5] / 2) * width;
  long top = (pos[4] - pos[6] / 2) * height;
  long bottom = (pos[4] + pos[6] / 2) * height;
  if (left < 0) left = 5;
  if (top < 0) top = 5;
  if (right > width) right = width-5;
  if (bottom > height) bottom = height-5;

#if 0
  p1->x = left;
  p1->y = top;
  p2->x = right;
  p2->y = bottom;
#else

  p1->x = (left * w) / 416;
  p1->y = (top * h) / 416;
  p2->x = (right * w) / 416;
  p2->y = (bottom * h) / 416;
  // printf("(%d,%d)->(%d,%d),  (%d,%d)->(%d,%d)\n", left, top, right, bottom,
  //     p1->x, p1->y, p2->x, p2->y);
#endif

  return;
}
#endif

void yolov2_detect::get_point_position(const std::vector<float> pos, Point* p1, Point* p2)
{
  int width = getInputWidth();
  int height = getInputHeight();

  long left = (pos[3] - pos[5] / 2) * width;
  long right = (pos[3] + pos[5] / 2) * width;
  long top = (pos[4] - pos[6] / 2) * height;
  long bottom = (pos[4] + pos[6] / 2) * height;
  if (left < 0) left = 5;
  if (top < 0) top = 5;
  if (right > width) right = width-5;
  if (bottom > height) bottom = height-5;

  p1->x = left;
  p1->y = top;
  p2->x = right;
  p2->y = bottom;

  //printf("(%d,%d)->(%d,%d),  (%d,%d)->(%d,%d)\n", p1->x, p1->y, p2->x, p2->y);

  return;
}

yolov2_detect::yolov2_detect(const  std::string& modelfileName,
    const std::string & labelfileName)
{
    init(modelfileName, labelfileName);
}

void yolov2_detect::init(const  std::string& modelfileName,
    const std::string & labelfileName)
{
    initDlaConf();
    //init runtime_lib and device
    cnrtInit(0);
    unsigned dev_num;
    cnrtGetDeviceCount(&dev_num);
    if (dev_num==0) {
        std::cout<<"no device found"<<std::endl;
        return;
    }
    std::cout<<"yolov2_detect::init dev_num="<<dev_num<<std::endl;
    cnrtDev_t dev;
    cnrtGetDeviceHandle(&dev, 0);
    cnrtSetCurrentDevice(dev);
    //load model and set function
    std::cout<<"load model"<<modelfileName<<std::endl;
    cnrtLoadModel(&model, modelfileName.c_str());
    cnrtCreateFunction(&function);
    cnrtExtractFunction(&function, model, funcName.c_str());
    //get function's I/O datadesc
    cnrtGetInputDataDesc(&inputDescS, &inputNum, function);
    cnrtGetOutputDataDesc(&outputDescS, &outputNum, function);

    inputCpuPtrS =
        static_cast<void**>(malloc(sizeof(void*) * inputNum));
    outputCpuPtrS =
        static_cast<void**>(malloc(sizeof(void*) * outputNum));
    //MLU input info
    cnrtDataDesc_t inputDesc = inputDescS[0];
    cnrtSetHostDataLayout(inputDesc, CNRT_UINT8, CNRT_NHWC);
    cnrtGetHostDataCount(inputDesc, &in_count);
    cnrtGetDataShape(inputDesc, &in_n, &in_c, &in_h, &in_w);
    std::cout<<"Input info w="<<in_w<<" h="<<in_h<<" channel="
                <<in_c<<" in_count="<<in_count<<std::endl;
    //MLU output info
    cnrtDataDesc_t outputDesc = outputDescS[0];
    cnrtSetHostDataLayout(outputDesc, CNRT_FLOAT32, CNRT_NCHW);
    cnrtGetHostDataCount(outputDesc, &out_count);
    cnrtGetDataShape(outputDesc, &out_n, &out_c, &out_h, &out_w);
    std::cout<<"Output Info w="<<out_w<<" h="<<out_h<<" channel="
                <<out_c<<" out_count="<<out_count<<std::endl;
    float* net_output =
         reinterpret_cast<float*>(malloc (sizeof(float) * out_count));
    outputCpuPtrS[0] = static_cast<void*>(net_output);
    //allocate I/O data space on MLU memory
    cnrtMallocByDescArray(&inputMluPtrS, inputDescS, inputNum);
    cnrtMallocByDescArray(&outputMluPtrS, outputDescS, outputNum);
    cnrtCreateStream(&stream);
    cnrtInitFunctionMemory(function, CNRT_FUNC_TYPE_BLOCK);
    //get lable list
    std::ifstream label_in(labelfileName);
    std::string line;
    while(std::getline(label_in, line))
    {
        labels.push_back(line);
    }
    label_in.close();
}
yolov2_detect::~yolov2_detect()
{
    deInit();
}
void yolov2_detect::deInit()
{
     float* net_output = static_cast<float*>(outputCpuPtrS[0]);
     if (net_output)
        free(net_output);
     if (inputCpuPtrS) {
        free(inputCpuPtrS);
        inputCpuPtrS = nullptr;
     }
     if (outputCpuPtrS) {
        free(outputCpuPtrS);
        outputCpuPtrS = nullptr;
     }
    cnrtFreeArray(inputMluPtrS, inputNum);
    cnrtFreeArray(outputMluPtrS, outputNum);
    cnrtDestroyStream(stream);
    cnrtDestroyFunction(function);
    cnrtUnloadModel(model);
    cnrtDestroy();
}

void  yolov2_detect::setInputPhyAddr(void* phyAddr)
{
    inputMluPtrS[0] = (void*)((unsigned long)phyAddr-dlaBaseAddr);
}

void  yolov2_detect::setInputVirAddr(void* virAddr)
{
    inputCpuPtrS[0] = virAddr;
    cnrtMemcpyByDescArray(inputMluPtrS, inputCpuPtrS,inputDescS,
        inputNum,CNRT_MEM_TRANS_DIR_HOST2DEV);
}

void yolov2_detect::do_detect()
{
     cnrtDim3_t dim = {1, 1, 1};
     void *param[2] = {inputMluPtrS[0], outputMluPtrS[0]};
     //struct timeval tpstart, tpend;
     //float time_use = 0.0f;
     //gettimeofday(&tpstart, NULL);
     cnrtInvokeFunction(function, dim, param,
                       CNRT_FUNC_TYPE_BLOCK, stream, NULL);
     //cnrtSyncStream(stream);

     //gettimeofday(&tpend, NULL);

     //time_use = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
     //std::cout<<"DLA process time use"<<time_use<<"us"<<std::endl;


     //gettimeofday(&tpstart, NULL);
     //cnrtMemcpyByDescArray(outputCpuPtrS, outputMluPtrS,
     //                       outputDescS, outputNum, CNRT_MEM_TRANS_DIR_DEV2HOST);
     //gettimeofday(&tpend, NULL);
     //time_use = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
     //std::cout<<"DLA out memcpy time use"<<time_use<<"us"<<std::endl;
#if 0
      std::cout<<"yolov2_detect::do_detect outputNum="<<outputNum<<std::endl;
      gettimeofday(&tpstart, NULL);
      detection_out();
      gettimeofday(&tpend, NULL);
      time_use = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
      std::cout<<"PostProcess time use"<<time_use<<"us"<<std::endl;
#endif
}

void yolov2_detect::syncDetectFinish()
{
    cnrtSyncStream(stream);
}
void yolov2_detect::copyDetectResult2Host()
{
    cnrtMemcpyByDescArray(outputCpuPtrS, outputMluPtrS,
                            outputDescS, outputNum, CNRT_MEM_TRANS_DIR_DEV2HOST);
}


void yolov2_detect::detection_out()
{
    float* swap_data = static_cast<float*>
        (malloc(sizeof(float) * out_n * out_c * out_h * out_w));
    float* net_output = static_cast<float*>(outputCpuPtrS[0]);
    int index = 0;
    for(unsigned int b=0; b < out_n; b++)
        for (unsigned int h = 0; h < out_h; ++h)
            for (unsigned int w = 0; w < out_w; ++w)
                for (unsigned int c = 0; c < out_c; ++c) {
                    swap_data[index++] =
                    net_output[ b*out_c*out_h*out_w + c*out_h*out_w + h*out_w +w];
                }
    vector< PredictionResult<float> > predicts;
    PredictionResult<float> predict;
    predicts.clear();
    vector<float> biases_({1.3221, 1.73145, 3.19275, 4.00944, 5.05587, 8.09892, 9.47112, 4.84053, 11.2364, 10.0071});
    int side_ = 13;
    int num_box_ = 5;
    int num_classes_ = 20;
    float nms_threshold_ = 0.4;
    float confidence_threshold_  = 0.45;
    for (unsigned int b = 0; b < out_n; ++b) {
        for (int j = 0; j < side_; ++j)
            for (int i = 0; i < side_; ++i)
                for (int n = 0; n < num_box_; ++n) {
                    int index = b * out_c * out_h * out_w + (j * side_ + i) * out_c +
                        n * out_c / num_box_;
                    get_region_box(swap_data, &predict, biases_, n,
                         index, i, j, side_, side_);
                    predict.objScore = sigmoid(swap_data[index + 4]);
                    class_index_and_score(swap_data+index + 5, num_classes_, &predict);
                    predict.confidence = predict.objScore * predict.classScore;
                    if (predict.confidence >= confidence_threshold_) {
                        predicts.push_back(predict);
                    }
               }
               vector<int> idxes;
               int num_kept = 0;
               if (predicts.size() > 0) {
                    ApplyNms(&predicts, &idxes, nms_threshold_);
                    num_kept = idxes.size();
               }
                // std::cout<<"detection_out num_kept="<<num_kept<<std::endl;
            detect_boxes.clear();
                for (int i = 0; i < num_kept; i++) {
                    std::vector<float> temp;
                    temp.push_back(b);  // Image_Id
                    temp.push_back(predicts[idxes[i]].classType);  // label
                    temp.push_back(predicts[idxes[i]].confidence);  // confidence
                    temp.push_back(predicts[idxes[i]].x);
                    temp.push_back(predicts[idxes[i]].y);
                    temp.push_back(predicts[idxes[i]].w);
                    temp.push_back(predicts[idxes[i]].h);
                    detect_boxes.push_back(temp);
            }
  }
  // std::cout<<"detection result boxes size="<<detect_boxes.size()<<std::endl;
  free(swap_data);
}
//output detection result
void yolov2_detect::outputDetectResult()
{
    if (detect_boxes.size()==0) {
        // std::cout<<"Detect nothing in current frame!"<<std::endl;
        return;
    }
    for(unsigned int j=0; j<detect_boxes.size();j++) {
        std::cout << ">>> label: " << labels[detect_boxes[j][1]] << " score: "
                << detect_boxes[j][2] << " , position : ";
        for (int idx = 0; idx < 4; ++idx) {
            std::cout << detect_boxes[j][3 + idx] << " ";
        }
        std::cout << std::endl;
    }
}

int yolov2_detect::getBoxSize()
{
    return detect_boxes.size();
}

#if 0
void yolov2_detect::getDetectResult(Boxes &detectResult)
{
    detectResult = detect_boxes;
}
#endif

//getRectangles
void yolov2_detect::getDetectRectangle(std::vector<Rectangle>& rects)
{
    for (unsigned int j=0; j < detect_boxes.size(); j++) {
        Rectangle rect;
        Point p1, p2;
        get_point_position(detect_boxes[j], &p1, &p2);
        rect.leftTop = p1;
        rect.rightBottom = p2;
        rects.push_back(rect);
    }
}

int yolov2_detect::getDlaMiu()
{
    return dalMiu;
}

//post process for yolov2 detect
yolov2_postProcess::yolov2_postProcess(const std::string& labelfileName)
{
    //get lable list
    std::ifstream label_in(labelfileName);
    std::string line;
    while(std::getline(label_in, line))
    {
        labels.push_back(line);
    }
    label_in.close();
}

yolov2_postProcess::~yolov2_postProcess()
{
    if (detect_data) {
        free(detect_data);
    }
    detect_data = nullptr;
}

void yolov2_postProcess::setProcessData(void* mluOutputCpuPtr, int n, int c, int h, int w)
{
    detect_out_n = n;
    detect_out_c = c;
    detect_out_h = h;
    detect_out_w = w;
    if (detect_data==nullptr)
         detect_data = static_cast<float*>(malloc(sizeof(float) * n * c * h * w));

    //struct timeval tpstart, tpend;
    //float time_use = 0.0f;
    //gettimeofday(&tpstart, NULL);
    //memcpy data and reshape from NCHW to NHWC
    float* net_output = static_cast<float*>(mluOutputCpuPtr);
    int index = 0;
    for(int b=0; b < detect_out_n; b++)
        for (int h = 0; h < detect_out_h; ++h)
            for (int w = 0; w < detect_out_w; ++w)
                for (int c = 0; c < detect_out_c; ++c) {
                    detect_data[index++] =
                        net_output[ b*detect_out_c*detect_out_h*detect_out_w + c*detect_out_h*detect_out_w + h*detect_out_w +w];
                }
   //gettimeofday(&tpend, NULL);
   //time_use = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
   //std::cout<<"yolov2_postProcess::setProcessData use time="<<time_use<<"us"<<std::endl;
}

void yolov2_postProcess::do_postProcess()
{
    if (detect_data==nullptr)
        return;
    vector< PredictionResult<float> > predicts;
    PredictionResult<float> predict;
    predicts.clear();
    vector<float> biases_({1.3221, 1.73145, 3.19275, 4.00944, 5.05587, 8.09892, 9.47112, 4.84053, 11.2364, 10.0071});
    int side_ = 13;
    int num_box_ = 5;
    int num_classes_ = 20;
    float nms_threshold_ = 0.4;
    float confidence_threshold_  = 0.45;
    for (int b = 0; b < detect_out_n; ++b) {
        for (int j = 0; j < side_; ++j)
            for (int i = 0; i < side_; ++i)
                for (int n = 0; n < num_box_; ++n) {
                    int index = b * detect_out_c * detect_out_h * detect_out_w + (j * side_ + i) * detect_out_c +
                        n * detect_out_c / num_box_;
                    get_region_box(detect_data, &predict, biases_, n,
                         index, i, j, side_, side_);
                    predict.objScore = sigmoid(detect_data[index + 4]);
                    class_index_and_score(detect_data+index + 5, num_classes_, &predict);
                    predict.confidence = predict.objScore * predict.classScore;
                    if (predict.confidence >= confidence_threshold_) {
                        predicts.push_back(predict);
                    }
               }
               vector<int> idxes;
               int num_kept = 0;
               if (predicts.size() > 0) {
                    ApplyNms(&predicts, &idxes, nms_threshold_);
                    num_kept = idxes.size();
               }
                // std::cout<<"detection_out num_kept="<<num_kept<<std::endl;
            result_boxes.clear();
                for (int i = 0; i < num_kept; i++) {
                    std::vector<float> temp;
                    temp.push_back(b);  // Image_Id
                    temp.push_back(predicts[idxes[i]].classType);  // label
                    temp.push_back(predicts[idxes[i]].confidence);  // confidence
                    temp.push_back(predicts[idxes[i]].x);
                    temp.push_back(predicts[idxes[i]].y);
                    temp.push_back(predicts[idxes[i]].w);
                    temp.push_back(predicts[idxes[i]].h);
                    result_boxes.push_back(temp);
            }
  }
}

void yolov2_postProcess::outputProcessResult()
{
    if (result_boxes.size()==0) {
        return;
    }
    for(unsigned int j=0; j<result_boxes.size();j++) {
        std::cout << ">>> label: " << labels[result_boxes[j][1]] << " score: "
                << result_boxes[j][2] << " , position : ";
        for (int idx = 0; idx < 4; ++idx) {
            std::cout << result_boxes[j][3 + idx] << " ";
        }
        std::cout << std::endl;
    }
}
int yolov2_postProcess::get_process_BoxSize()
{
    return result_boxes.size();
}
void yolov2_postProcess::get_proc_point_position(const  vector < float > pos, Point * p1, Point * p2,
    int inputWidth, int inputHeight)
{
    int width = inputWidth;
    int height = inputHeight;

    long left = (pos[3] - pos[5] / 2) * width;
    long right = (pos[3] + pos[5] / 2) * width;
    long top = (pos[4] - pos[6] / 2) * height;
    long bottom = (pos[4] + pos[6] / 2) * height;
    if (left < 0) left = 5;
    if (top < 0) top = 5;
    if (right > width) right = width-5;
    if (bottom > height) bottom = height-5;

    p1->x = left;
    p1->y = top;
    p2->x = right;
    p2->y = bottom;
}

void yolov2_postProcess::get_process_Rectangle( vector < Rectangle > & rects,
    int inputWidth, int inputHeight)
{
     for (unsigned int j=0; j < result_boxes.size(); j++) {
        Rectangle rect;
        Point p1, p2;
        get_proc_point_position(result_boxes[j], &p1, &p2, inputWidth, inputHeight);

        snprintf(rect.szObjName, sizeof(rect.szObjName) - 1, "%s", labels[result_boxes[j][1]].c_str());
        rect.leftTop = p1;
        rect.rightBottom = p2;
        rects.push_back(rect);
    }
}
