/*
* yolov2_offline.h- Sigmastar
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
#ifndef _YOLOV2_OFFLINE_H_
#define _YOLOV2_OFFLINE_H_

#include <stdlib.h>
#include <stdio.h>
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


typedef struct Pointi
{
    int x;
    int y;
}Point;

typedef struct Rectanglei
{
    Point leftTop;
    Point rightBottom;

    char szObjName[64];
}Rectangle;

class yolov2_detect
{
    public:
        explicit yolov2_detect(const std::string& modelfileName, const std::string& labelfileName);
        virtual ~yolov2_detect();
        void  setInputPhyAddr(void* phyAddr);
        void  setInputVirAddr(void* virAddr);
        void  do_detect();
        void outputDetectResult();
        int getInputWidth() {return in_w;}
        int getInputHeight() {return in_h;}
        void getOutputShape(unsigned int* n,  unsigned int* c, unsigned int* h, unsigned int* w)
        {
            *n =  out_n;
            *c = out_c;
            *h = out_h;
            *w = out_w;
        }
        int getBoxSize();
        void getDetectRectangle(std::vector<Rectangle>& rects);
        void get_point_position(const std::vector<float> pos, Point* p1, Point* p2);
        int  getDlaMiu();
        void* getDetectData()
        {
            return outputCpuPtrS[0];
        }
        void copyDetectResult2Host();
        void syncDetectFinish();
    private:
        void init(const std::string& modelfileName, const std::string& labelfileName);
        void deInit();
        void detection_out();
        //input blob shape
        unsigned int in_n = 0;
        unsigned int in_c = 0;
        unsigned int in_h = 0;
        unsigned int in_w = 0;
        //output blob shape
        unsigned int out_n = 0;
        unsigned int out_c = 0;
        unsigned int out_h = 0;
        unsigned int out_w = 0;
        //bytes count of input data
        int in_count = 0;
        //bytes count of output data
        int out_count = 0;
        //allocate I/O date space on MLU memory for input data
        void** inputMluPtrS = nullptr;
        //allocate I/O date space on MLU memory for output data
        void** outputMluPtrS = nullptr;
        //cnrt stream
        cnrtStream_t stream = nullptr;
        //cpu data space for input data
        void** inputCpuPtrS = nullptr;
        //cpu data space for output data
        void** outputCpuPtrS = nullptr;
        //cnrt model
        cnrtModel_t model = nullptr;
        //cnrt function
        cnrtFunction_t function = nullptr;
        //The input info for I/O data space on MLU memory
        cnrtDataDescArray_t inputDescS = nullptr;
        int inputNum = 0;
        //The output info for I/O data space on MLU memory
        cnrtDataDescArray_t outputDescS = nullptr;
        int outputNum = 0;
        //used to store detection result include veterx of bounding box and score
        typedef std::vector<std::vector<float> > Boxes;
        Boxes detect_boxes;
        std::vector<std::string> labels;
};

class  yolov2_postProcess
{
public:
   explicit  yolov2_postProcess(const std::string& labelfileName);
   virtual ~yolov2_postProcess();
   void setInputSize(int inputWidth, int inputHeight) {
        in_width = inputWidth;
        in_height = inputHeight;
   }
   int getInputWidth() {return in_width;}
   int getInputHeight() {return in_height;}
   void setProcessData(void* mluOutputCpuPtr, int n, int c, int h, int w);
   void do_postProcess();
   void outputProcessResult();
    int get_process_BoxSize();
    void get_process_Rectangle(std::vector<Rectangle>& rects, int inputWidth, int inputHeight);
    void get_proc_point_position(const std::vector<float> pos, Point* p1, Point* p2,
            int inputWidth, int inputHeight);
private:
    float* detect_data = nullptr;
    typedef std::vector<std::vector<float> > ResultBoxes;
    ResultBoxes result_boxes;
    std::vector<std::string> labels;
    //output shapes
    int detect_out_n = 0;
    int detect_out_c = 0;
    int detect_out_h = 0;
    int detect_out_w = 0;
    //input size
    int in_width = 416;
    int in_height = 416;
};



#endif

