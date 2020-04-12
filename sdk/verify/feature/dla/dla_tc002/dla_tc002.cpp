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
#include <sys/time.h>
#include <iostream>
#include <algorithm>
#include <map>
#include <sstream>
#include <set>
#include <fstream>
#include <iosfwd>
#include <iomanip>
#include <string>
#include <utility>
#include <vector>
#include "cnrt.h"
#include "boundingBox.h"
using namespace std;

using std::string;
using std::vector;
using std::map;

int h_c=0, w_c=0;

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


std::vector<std::vector<float> > detection_out(float* net_output,
                                  int out_n, int out_c, int out_h, int out_w) {
  vector< vector<float> > result;
  float* swap_data = reinterpret_cast<float*>
                   (malloc(sizeof(float) * out_n * out_c * out_h * out_w));
  int index = 0;
  for (int b = 0; b < out_n; ++b)
    for (int h = 0; h < out_h; ++h)
      for (int w = 0; w < out_w; ++w)
        for (int c = 0; c < out_c; ++c) {
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
  for (int b = 0; b < out_n; ++b) {
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
    std::cout<<"detection_out num_kept="<<num_kept<<std::endl;
    for (int i = 0; i < num_kept; i++) {
      std::vector<float> temp;
      temp.push_back(b);  // Image_Id
      temp.push_back(predicts[idxes[i]].classType);  // label
      temp.push_back(predicts[idxes[i]].confidence);  // confidence
      temp.push_back(predicts[idxes[i]].x);
      temp.push_back(predicts[idxes[i]].y);
      temp.push_back(predicts[idxes[i]].w);
      temp.push_back(predicts[idxes[i]].h);
      result.push_back(temp);
    }
  }

  free(swap_data);
  return result;
}

typedef struct Pointi
{
    int x;
    int y;
}Point;

void get_point_position(const vector<float> pos,
                        Point* p1, Point* p2, int h, int w) {
  int left = (pos[3] - pos[5] / 2) * w;
  int right = (pos[3] + pos[5] / 2) * w;
  int top = (pos[4] - pos[6] / 2) * h;
  int bottom = (pos[4] + pos[6] / 2) * h;
  if (left < 0) left = 5;
  if (top < 0) top = 5;
  if (right > w) right = w-5;
  if (bottom > h) bottom = h-5;
  p1->x = left;
  p1->y = top;
  p2->x = right;
  p2->y = bottom;
  return;
}

//  ./yolo_offline yolo.cambricon raw_image_data label.txt
int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cout << "usage:\n"
                  << argv[0] << " yolo.cambricon raw_image_data label.txt\n";
        exit(0);
    }

    // 1. init runtime_lib and device
    int elapse = 0;
    float totaltime = 0.0f;
    struct timeval ts,te;
    gettimeofday(&ts, NULL);

    cnrtInit(0);
    unsigned dev_num;
    cnrtGetDeviceCount(&dev_num);
    if (dev_num == 0) {
        std::cout << "no device found" << std::endl;
        exit(-1);
    }
    cnrtDev_t dev;
    cnrtGetDeviceHandle(&dev, 0);
    cnrtSetCurrentDevice(dev);
    // 2. load model and set function
    std::string fname = static_cast<string>(argv[1]);
    printf("load file: %s\n", fname.c_str());
    cnrtModel_t model;
    cnrtLoadModel(&model, fname.c_str());
    cnrtFunction_t function;
    string name = "subnet0";
    cnrtCreateFunction(&function);
    cnrtExtractFunction(&function, model, name.c_str());
    // 3. get function's I/O DataDesc
    int inputNum, outputNum;
    cnrtDataDescArray_t inputDescS, outputDescS;
    cnrtGetInputDataDesc(&inputDescS, &inputNum, function);
    cnrtGetOutputDataDesc(&outputDescS, &outputNum, function);

    void** inputCpuPtrS =
        reinterpret_cast<void**>(malloc(sizeof(void*) * inputNum));
    void** outputCpuPtrS =
         reinterpret_cast<void**>(malloc(sizeof(void*) * outputNum));

    int in_n, in_c, in_h, in_w;
    int out_n, out_c, out_h, out_w;
    // 4. allocate I/O data space on CPU memory and prepare Input data
    int in_count;
    cnrtDataDesc_t inputDesc = inputDescS[0];
    cnrtSetHostDataLayout(inputDesc, CNRT_UINT8, CNRT_NHWC);
    cnrtGetHostDataCount(inputDesc, &in_count);
    cnrtGetDataShape(inputDesc, reinterpret_cast<unsigned int*>(&in_n),
                              reinterpret_cast<unsigned int*>(&in_c),
                              reinterpret_cast<unsigned int*>(&in_h),
                              reinterpret_cast<unsigned int*>(&in_w));
    std::cout<<"Input info w="<<in_w<<" h="<<in_h<<"channel="
                <<in_c<<"in_count="<<in_count<<std::endl;
    uint8_t* dataBuffer =
        reinterpret_cast<uint8_t*>(malloc(sizeof(uint8_t) * in_count));
    inputCpuPtrS[0] = reinterpret_cast<void*>(dataBuffer);

    int out_count;
    cnrtDataDesc_t outputDesc = outputDescS[0];
    cnrtSetHostDataLayout(outputDesc, CNRT_FLOAT32, CNRT_NCHW);
    cnrtGetHostDataCount(outputDesc, &out_count);
    cnrtGetDataShape(outputDesc, reinterpret_cast<unsigned int*>(&out_n),
                               reinterpret_cast<unsigned int*>(&out_c),
                               reinterpret_cast<unsigned int*>(&out_h),
                               reinterpret_cast<unsigned int*>(&out_w));
    float* net_output =
         reinterpret_cast<float*>(malloc (sizeof(float) * out_count));
    outputCpuPtrS[0] = reinterpret_cast<void*>(net_output);

    // 5. allocate I/O data space on MLU memory and copy Input data
    void **inputMluPtrS;
    void **outputMluPtrS;
    cnrtMallocByDescArray(&inputMluPtrS, inputDescS, inputNum);
    cnrtMallocByDescArray(&outputMluPtrS, outputDescS, outputNum);
    // 6. create stream and run function
    void *param[2] = {inputMluPtrS[0], outputMluPtrS[0]};
    cnrtDim3_t dim = {1, 1, 1};
    cnrtStream_t stream;
    cnrtCreateStream(&stream);
    //7.initialize function memory
  cnrtInitFunctionMemory(function, CNRT_FUNC_TYPE_BLOCK);
  gettimeofday(&te, NULL);
  elapse = 1000000 * (te.tv_sec - ts.tv_sec) + te.tv_usec - ts.tv_usec;
  cout<<"out_n="<<out_n<<" out_c="<<out_c<<"  out_h="<<out_h
    <<" out_w="<<out_w<<" total out_count="<<out_count<<std::endl;
  cout<<"cnrt init time: " << elapse<<"us" << std::endl;

    float time_use;
    struct timeval tstart, tend;

    float scale = 1.0f;
    std::vector<string> labels_;
    {
        std::ifstream labels(argv[3]);
        string line;
        while (std::getline(labels, line)) {
          labels_.push_back(string(line));
        }
        labels.close();
    }
    gettimeofday(&tstart, NULL);
    std::ifstream fin(argv[2], std::fstream::in | std::fstream::binary);
    unsigned int temp;
    int idx = 0;
    unsigned int * buffer = (unsigned int *)dataBuffer;
    std::cout<<"in_h="<<in_h<<" in_w="<<in_w
        <<" in_count="<<in_count<<std::endl;
    while (fin.read((char*)&temp,sizeof(unsigned int))) {
        if(idx >= in_h * in_w )
        {
            std::cout<<"out of size"<<std::endl;
            break;
        }
        buffer[idx++] = temp;
    }

    gettimeofday(&tend, NULL);
    time_use = 1000000 * (tend.tv_sec - tstart.tv_sec) +
               tend.tv_usec - tstart.tv_usec;
    std::cout << " read raw data time: "  << time_use << " us" << std::endl;
    // 7. copy back Output data and write it to file

    std::cout << __LINE__ << "----------ipuMemcpy-----------" << std::endl;
    gettimeofday(&tstart, NULL);
    cnrtMemcpyByDescArray(inputMluPtrS, inputCpuPtrS, inputDescS,
                          inputNum, CNRT_MEM_TRANS_DIR_HOST2DEV);

    gettimeofday(&tend, NULL);
    time_use = 1000000 * (tend.tv_sec - tstart.tv_sec) +
               tend.tv_usec - tstart.tv_usec;
    std::cout << " img memcpy  time: "  << time_use << " us" << std::endl;

    gettimeofday(&tstart, NULL);
    cnrtInvokeFunction(function, dim, param,
                       CNRT_FUNC_TYPE_BLOCK, stream, NULL);
    cnrtSyncStream(stream);
    gettimeofday(&tend, NULL);
    time_use = 1000000 * (tend.tv_sec - tstart.tv_sec) +
               tend.tv_usec - tstart.tv_usec;
    std::cout << " invoke time: "  << time_use << " us" << std::endl;


    std::cout << __LINE__ << "----------ipuMemcpy-----------" << std::endl;
    gettimeofday(&tstart, NULL);

    cnrtMemcpyByDescArray(outputCpuPtrS, outputMluPtrS,
                          outputDescS, outputNum, CNRT_MEM_TRANS_DIR_DEV2HOST);
    gettimeofday(&tend, NULL);
    time_use = 1000000 * (tend.tv_sec - tstart.tv_sec) +
               tend.tv_usec - tstart.tv_usec;
    std::cout << " memcpy out  time: "  << time_use << " us" << std::endl;


    gettimeofday(&tstart, NULL);
    std::vector<std::vector<float> > boxes =
        detection_out(net_output, out_n, out_c, out_h, out_w);
    std::cout<<"detection result boxes size="<<boxes.size()
        <<"out_n="<<out_n
        <<" out_c="<<out_c
        <<" out_h="<<out_h
        <<" out_w="<<out_w<<std::endl;
    //8.output detection result
    for (int j = 0; j < boxes.size(); ++j) {
      Point p1, p2;
      get_point_position(boxes[j], &p1, &p2,
                         in_h, in_w);
      p1.x = p1.x / scale;
      p1.y = p1.y / scale;
      p2.x = p2.x / scale;
      p2.y = p2.y / scale;
      std::cout << ">>> label: " << labels_[boxes[j][1]] << " score: "
                << boxes[j][2] << " , position : ";
      for (int idx = 0; idx < 4; ++idx) {
        std::cout << boxes[j][3 + idx] << " ";
      }
      std::cout << std::endl;
  }
  gettimeofday(&tend, NULL);
    time_use = 1000000 * (tend.tv_sec - tstart.tv_sec) +
               tend.tv_usec - tstart.tv_usec;
   std::cout<<"post process time="<<time_use<<std::endl;
  free(dataBuffer);
  free(net_output);

  free(inputCpuPtrS);
  free(outputCpuPtrS);
  cnrtFreeArray(inputMluPtrS, inputNum);
  cnrtFreeArray(outputMluPtrS, outputNum);
  cnrtDestroyStream(stream);
  cnrtDestroyFunction(function);
  cnrtUnloadModel(model);
  cnrtDestroy();
  exit(0);
}
