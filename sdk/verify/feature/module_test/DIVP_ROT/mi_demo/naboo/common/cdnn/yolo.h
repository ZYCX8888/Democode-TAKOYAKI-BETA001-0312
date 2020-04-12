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
#ifndef _YOLO_H
#define _YOLO_H

#define CNN_OUT_FLOAT   1

#if(CNN_OUT_FLOAT)
#define CNN_DATATYPE    float
#define TO_FLOAT(a)     (a)
#else
#define CNN_DATATYPE    short
#define FLOAT_FACTOR    (1.0/256)
#define TO_FLOAT(a)     ((float)((a)*FLOAT_FACTOR))
#endif

typedef struct
{
    short int x_min, y_min;         // top-left corner
    short int x_max, y_max;         // bottom-right corner
    short int class_idx;
    float prob;
}BBox;

typedef struct
{
    short img_width, img_height;    // original image resolution (not CNN input resolution)
    short width, height;            // the spatial size of CNN output
    short n_box;                    // the number of proposed boxes at each location
    short n_class;
    float prob_thresh;
    float nms_thresh;
    float *anchors;                 // anchors are used to compute the predicted object size
}YoloOutputParam;

int GetBBox(CNN_DATATYPE *prediction, YoloOutputParam param, BBox *boxes, int max_num);
/*
    [in]  CNN_DATATYPE *prediction : the output of CNN with size of width*height*(5+n_class)*n_box
    [in]  YoloOutputParam param    : some parameters of the CNN output
    [out] BBox *boxes              : pointer of a BBox array given by caller
    [in]  int max_num              : the size of boxes array
    [return]                       : the number of the detected objects
*/

#endif
