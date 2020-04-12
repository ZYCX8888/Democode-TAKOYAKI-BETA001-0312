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
#include "yolo.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

inline short max(short a, short b)
{
    if(b < a)
        return a;
    return b;
}

inline short min(short a, short b)
{
    if(a < b)
        return a;
    return b;
}

inline float logistic(float x)
{
    return 1/(1+exp(-x));
}

float IoU(BBox b1, BBox b2)
{
    short intersect_x, intersect_y;
    int area1, area2;
    float iou = 0, intersect_area;

    area1 = (b1.x_max-b1.x_min)*(b1.y_max-b1.y_min);
    area2 = (b2.x_max-b2.x_min)*(b2.y_max-b2.y_min);

    intersect_x = min(b1.x_max, b2.x_max) - max(b1.x_min, b2.x_min);
    intersect_y = min(b1.y_max, b2.y_max) - max(b1.y_min, b2.y_min);
    intersect_area = intersect_x*intersect_y;
    if(intersect_area < 0)
        intersect_area = 0;

    iou = intersect_area/(area1+area2-intersect_area);

    return iou;
}

int NMS(BBox *boxes, int n, float nms_thresh)
{
    int i, j, count=0;
    float iou;
    char ismax;
    BBox *local_max;

    local_max = (BBox*) malloc(sizeof(BBox)*n);

    for(i=0; i<n; i++)
    {
        ismax = 1;
        for(j=0; j<n; j++)
        {
            if(i==j || boxes[i].class_idx!=boxes[j].class_idx)
                continue;
            iou = IoU(boxes[i], boxes[j]);
            if(iou >= nms_thresh && boxes[i].prob < boxes[j].prob)
            {
                ismax = 0;
                break;
            }
        }
        if(ismax)
        {
            local_max[count] = boxes[i];
            count++;
        }
    }
    memcpy(boxes, local_max, count*sizeof(BBox));

    free(local_max);

    return count;
}

void softmax(float *x, int n)
{
    int i;
    float sum=0;
    for(i=0; i<n; i++)
    {
        x[i] = exp(x[i]);
        sum += x[i];
    }
    for(i=0; i<n; i++)
        x[i] /= sum;
}

float arg_max(float* x, int n, int *idx)
{
    float x_max = x[0];
    int i;
    *idx = 0;
    for(i=1; i<n; i++)
    {
        if(x[i]>x_max)
        {
            x_max = x[i];
            *idx = i;
        }
    }
    return x_max;
}

int GetBBox(CNN_DATATYPE *prediction, YoloOutputParam param, BBox *boxes, int max_num)
{
    BBox *all_boxes;
    int i, j, k, cls, conf_idx, coord_idx, class_idx, max_idx, count;
    int box_stride, stride;
    float conf, max_class;
    float *class_prob;
    float bx, by, bw, bh;
    float height_ratio, width_ratio;

    // == Some values that will be repeatedly used in the following == //
    stride = param.height*param.width;
    box_stride = param.n_class+5;
    height_ratio = ((float)param.img_height)/param.height;
    width_ratio = ((float)param.img_width)/param.width;

    // == Gather all the boxes with prob higher than threshold == //
    all_boxes = (BBox*)malloc(sizeof(BBox)*param.n_box*(param.n_class+5)*param.height*param.width);
    class_prob = (float*)malloc(sizeof(float)*param.n_class);

    count = 0;
    for(i=0; i<param.height; i++)
    {
        for(j=0; j<param.width; j++)
        {
            for(k=0; k<param.n_box; k++)
            {
                conf_idx = (k*box_stride + 4)*stride + i*param.width + j;
                conf = logistic(TO_FLOAT(prediction[conf_idx]));
                if(conf >= param.prob_thresh)
                {
                    class_idx = (k*box_stride + 5)*stride + i*param.width + j;
                    for(cls=0; cls<param.n_class; cls++)
                        class_prob[cls] = TO_FLOAT(prediction[class_idx + stride*cls]);
                    softmax(class_prob, param.n_class);
                    max_class = arg_max(class_prob, param.n_class, &max_idx);

                    if(max_class*conf >= param.prob_thresh)
                    {
                        coord_idx = k*box_stride*stride + i*param.width + j;
                        bx = logistic(TO_FLOAT(prediction[coord_idx])) + j;
                        by = logistic(TO_FLOAT(prediction[coord_idx+stride])) + i;
                        bw = exp(TO_FLOAT(prediction[coord_idx+stride*2]))*param.anchors[k*2];
                        bh = exp(TO_FLOAT(prediction[coord_idx+stride*3]))*param.anchors[k*2+1];

                        all_boxes[count].class_idx = max_idx;
                        all_boxes[count].prob = max_class*conf;
                        all_boxes[count].x_min = (bx - 0.5*bw)*width_ratio + 0.5;
                        all_boxes[count].x_max = (bx + 0.5*bw)*width_ratio + 0.5;
                        all_boxes[count].y_min = (by - 0.5*bh)*height_ratio + 0.5;
                        all_boxes[count].y_max = (by + 0.5*bh)*height_ratio + 0.5;

                        count++;
                    }
                }
            }
        }
    }

    // == Non-Maximal Suppression (NMS) == //
    count = NMS(all_boxes, count, param.nms_thresh);
    count = min(count, max_num);
    memcpy(boxes, all_boxes, sizeof(BBox)*count);

    // == Restrict the boxes to be inside the image == //
    for(i=0; i<count; i++)
    {
        if(boxes[i].x_min < 0)
            boxes[i].x_min = 0;
        if(boxes[i].y_min < 0)
            boxes[i].y_min = 0;
        if(boxes[i].x_max >= param.img_width)
            boxes[i].x_max = param.img_width - 1;
        if(boxes[i].y_max >= param.img_height)
            boxes[i].y_max = param.img_height - 1;
    }

    // == Free memory == //
    free(all_boxes);
    free(class_prob);

    return count;
}
