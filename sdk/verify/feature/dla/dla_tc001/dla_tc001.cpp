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
#include "cnrt.h"
#include <pthread.h>
#include <sys/time.h>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <fstream>
#include <vector>
#include <iomanip>
using namespace std;
std::string file_name;

string get_string(string names, int index){
  ifstream files(names.c_str(), std::ios::in);
  string file;
  for(int i = 0; i < index; i++){
    getline(files, file);
  }
  getline(files, file);
  files.close();
  return file;
}

void* softmax_print(int outputSize, float* output_cpu, string names){
  int max_index[5] = {0};
  double max_num[5] = {0};

  for(int i = 0; i < outputSize; i++){
    double tmp = output_cpu[i];
    int tmp_index = i;
    for(int j = 0; j < 5; j++){
      if(tmp > max_num[j]){
        tmp_index += max_index[j];
        max_index[j] = tmp_index - max_index[j];
        tmp_index -= max_index[j];
        tmp += max_num[j];
        max_num[j] = tmp-max_num[j];
        tmp -= max_num[j];
      }
    }
  }
  cout << "------------------------detection result---------------------------" << endl;
  for(int i = 0; i < 5; i++){
    cout << fixed << setprecision(4) << max_num[i] << "  -  " << get_string(names,max_index[i]) << endl;
  }
  //golden print
  if (file_name.find("resnet18_abgr.dat")!=std::string::npos) {
        bool isPass = false;
        cout<<"-----------------------goloden compare result---------------------"<<endl;
        if ((max_index[0]==701 && max_num[0] > 0.8) &&
             (max_index[1]==557 && max_num[1]>0.08)) {
            isPass = true;
        }
        if (isPass) {
            cout<<"Compare result is PASS!!"<<endl;
        } else {
            cout<<"Compare result is FAIL!!"<<endl;
        }
  }
  return NULL;
}

int main(int argc, char* argv[]) {
  if(argc != 4) {
    printf("wrong args\n");
    std::cerr << "Usage: " << argv[0]
      << " offline.cambricon raw_image_data"
      << " labels.txt"  << std::endl;
    exit(0);
  }
  std::cout<<"in"<<std::endl;
  std::string names = (string)argv[3];
  int elapse = 0;
  float totaltime = 0.0f;
  struct timeval ts,te;
  gettimeofday(&ts, NULL);
  cnrtInit(0);
  file_name = (string)(argv[2]);

  // 1. init runtime_lib and device
  unsigned dev_num;
  cnrtGetDeviceCount(&dev_num);
  if (dev_num == 0){
    std::cout<<"no device found"<<std::endl;
    exit(-1);
  }
  cnrtDev_t dev;
  cnrtGetDeviceHandle(&dev, 0);
  cnrtSetCurrentDevice(dev);
  // 2. load model and get function
  cnrtModel_t model;
  string fname = (string)argv[1];
  printf("load file: %s\n", fname.c_str());
  cnrtLoadModel(&model, fname.c_str());
  cnrtFunction_t function;
  string name="subnet0";
  cnrtCreateFunction(&function);
  cnrtExtractFunction(&function, model, name.c_str());
  // 3. get function's I/O DataDesc
  int inputNum, outputNum;
  cnrtDataDescArray_t inputDescS, outputDescS;
  cnrtGetInputDataDesc (&inputDescS , &inputNum , function);
  cnrtGetOutputDataDesc(&outputDescS, &outputNum, function);
  if(inputNum != 1){
    std::cout<<"error, not classfiy model!"<<std::endl;
    exit(-1);
  }
  if(outputNum != 1){
    std::cout<<"error, not classfiy model!"<<std::endl;
    exit(-1);
  }
  // 4. allocate I/O data space on CPU memory and prepare Input data
  void** inputCpuPtrS  = (void**) malloc (sizeof(void*) * inputNum);
  void** outputCpuPtrS = (void**) malloc (sizeof(void*) * outputNum);
  int in_count;
  unsigned int in_n, in_c, in_h, in_w;
  cnrtDataDesc_t inputDesc = inputDescS[0];
  cnrtSetHostDataLayout(inputDesc, CNRT_UINT8, CNRT_NHWC);
  cnrtGetHostDataCount(inputDesc, &in_count);
  cnrtGetDataShape(inputDesc, &in_n, &in_c, &in_h, &in_w);
  uint8_t* dataBuffer = (uint8_t*) malloc(sizeof(uint8_t) * in_count);
  inputCpuPtrS[0] = (void*)dataBuffer;
  std::ifstream fin(argv[2], std::fstream::in | std::fstream::binary);
  unsigned char * buffer = (unsigned char *)dataBuffer;
  unsigned char temp;
  int i = 0;
  while(fin.read((char *)&temp, sizeof(char))) {
    if(i >= in_h * in_w *4 ) {
        std::cout<<"out of size"<<std::endl;
        break;
    }
    buffer[i++] = temp;
  }
  std::cout<<"input size "<< i <<std::endl;
  fin.close();

  int out_count;
  unsigned int out_n, out_c, out_h, out_w;
  cnrtDataDesc_t outputDesc = outputDescS[0];
  cnrtSetHostDataLayout(outputDesc, CNRT_FLOAT32, CNRT_NCHW);
  cnrtGetHostDataCount(outputDesc, &out_count);
  cnrtGetDataShape(outputDesc, &out_n, &out_c, &out_h, &out_w);
  float* output_cpu = (float*) malloc (sizeof(float) * out_count);

  outputCpuPtrS[0] = (void*)output_cpu;
  // 5. allocate I/O data space on MLU memory and copy Input data
  void **inputMluPtrS;
  void **outputMluPtrS;
  cnrtMallocByDescArray(&inputMluPtrS ,  inputDescS,  inputNum);
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
  cout<<"cnrt init time: " << elapse<<"us" << endl;


  int count = 1;
  float time_use;

  struct timeval tpstart, tpend;
  gettimeofday(&tpstart, NULL);
  //8. memcpy input data to mlu and invoked function
  cnrtMemcpyByDescArray(inputMluPtrS, inputCpuPtrS, inputDescS, inputNum, CNRT_MEM_TRANS_DIR_HOST2DEV);
  gettimeofday(&tpend, NULL);
  time_use = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
  cout<<"Input memcpy time="<<time_use<<"us"<<std::endl;

  gettimeofday(&tpstart, NULL);
  cnrtInvokeFunction(function, dim, param, CNRT_FUNC_TYPE_BLOCK, stream, NULL);
  cnrtSyncStream(stream);
  cout<<"after cnrtSyncStream"<<endl;
  gettimeofday(&tpend, NULL);
  time_use = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
  cout<<"Invoke function time="<<time_use<<"us"<<std::endl;
  //9.copy output data form mlu to cpu
  gettimeofday(&tpstart, NULL);
  cnrtMemcpyByDescArray(outputCpuPtrS, outputMluPtrS,
        outputDescS, outputNum, CNRT_MEM_TRANS_DIR_DEV2HOST);
  gettimeofday(&tpend, NULL);
  time_use = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
  cout<<"Output memcpy time="<<time_use<<"us"<<std::endl;
    //10. post execution time
  gettimeofday(&tpstart, NULL);
  softmax_print(out_count, output_cpu, (string)argv[3]);
  gettimeofday(&tpend, NULL);
  time_use = 1000000 * (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec;
  totaltime =   1000000 * (tpend.tv_sec - ts.tv_sec) + tpend.tv_usec - ts.tv_usec;
  if(count >= 1 )
    cout<<" iter " << count - 1 << "post execution time: "<<time_use<<"us"<<endl;

  std::cout<<"Total time (input memcpy+mlu execute+output memcpy+post process="<<totaltime<<"us"<<std::endl;
  free(dataBuffer);
  free(output_cpu);
  // 11. free memory space
  free(inputCpuPtrS);
  free(outputCpuPtrS);
  cnrtFreeArray(inputMluPtrS, inputNum);
  cnrtFreeArray(outputMluPtrS, outputNum);
  cnrtDestroyStream(stream);
  cnrtDestroyFunction(function);
  cnrtUnloadModel(model);
  cnrtDestroy();

  return 0;
}
