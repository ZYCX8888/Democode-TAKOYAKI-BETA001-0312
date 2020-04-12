
#ifndef _FACE_FEATURE_H_
#define _FACE_FEATURE_H_
#pragma once
#include <malloc.h>
#include <string.h>
#include <stdio.h>

class FaceFeature
{
public:
    int length;
    float* pData;

    FaceFeature()
    {
        length = 0;
        pData = 0;
    }
    FaceFeature(const FaceFeature& other)
    {
        length = other.length;

        if (length > 0)
        {
            pData = (float*)malloc(sizeof(float)*length);
            memcpy(pData, other.pData, sizeof(float)*length);
        }
        else
            pData = 0;

    }

    ~FaceFeature()
    {
        if (pData)
             delete[]pData;
        pData = 0;
        length = 0;
    }


    void CopyData(const FaceFeature& other)
    {
        if (length != other.length)
        {
            length = other.length;
            if (pData != 0)
                delete[]pData;
            pData = (float*)malloc(sizeof(float)*length);
        }
        if (length > 0)
            memcpy(pData, other.pData, sizeof(float)*length);
        else
            pData = 0;
    }

    FaceFeature& operator=(const FaceFeature& other)
    {
        CopyData(other);
        return *this;
    }

    void ChangeSize(int dst_len)
    {
        if (length != dst_len)
        {
            if (pData)
            {
                free(pData);
                pData = 0;
            }
            if (dst_len > 0)
            {
                pData = (float*)malloc(sizeof(float)*dst_len);
                length = dst_len;
            }
            else
            {
                pData = 0;
                length = 0;
            }
        }
    }
};

#endif
