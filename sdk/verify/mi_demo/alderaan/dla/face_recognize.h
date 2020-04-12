#ifndef _FACE_RECOGNIZE_H
#define _FACE_RECOGNIZE_H
#include <iostream>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <string.h>
#include <stdio.h>
#include <map>
#include <dirent.h>

#include "iout.h"
#include "FaceDatabase.h"


#define FEATURE_SIZE (128)
#define FEATURE_COUNT_MAX (3)
#define FACE_NUM_MAX (10)
#define ALIGN_UP(val, alignment) ((( (val)+(alignment)-1)/(alignment))*(alignment))



class FaceRecognizeUtils {
public:
    template<class T>
    static float CalculSimilarity(const T* feature1, const T* feature2, int dim) {
        float inner_product = 0.0f;


        for (int i = 0; i < dim; ++i) {
            inner_product += feature1[i] * feature2[i];
        }
        return inner_product;
    }





    template<class BaseType>
    static bool CropImage_112x112(const cv::Mat& img, const BaseType* facial5point, cv::Mat& crop) {
        cv::Size designed_size(112, 112);
        static BaseType coord5point[10] =
        {
            30.2946 + 8, 51.6963,
            65.5318 + 8, 51.5014,
            48.0252 + 8, 71.7366,
            33.5493 + 8, 92.3655,
            62.7299 + 8, 92.2041
        };

        cv::Mat transform;
        _findSimilarity(5, facial5point, coord5point, transform);
        cv::warpAffine(img, crop, transform, designed_size);
        return true;
    }


    template<class BaseType>
    static bool CropImage_112x112_YUV420_NV12(const cv::Mat& img, const BaseType* facial5point, cv::Mat& crop) {
        int crop_width = 112;
        int crop_height = 112;
        cv::Size designed_Y_size(crop_width, crop_height);
        cv::Size designed_UV_size(crop_width/2, crop_height/2);
        static BaseType coord5point[10] =
        {
            30.2946 + 8, 51.6963,
            65.5318 + 8, 51.5014,
            48.0252 + 8, 71.7366,
            33.5493 + 8, 92.3655,
            62.7299 + 8, 92.2041
        };

        cv::Mat transform;
        _findSimilarity(5, facial5point, coord5point, transform);



        //1. get Y transform and uv transform
        cv::Mat transform_y = transform;
        cv::Mat transform_uv = transform.clone();
        transform_uv.at<BaseType>(0,2) = transform_uv.at<BaseType>(0,2)/2;
        transform_uv.at<BaseType>(1,2) =  transform_uv.at<BaseType>(1,2)/2;


        //2.split the source img to y channel and uv channel
        int rows = img.rows;
        int cols = img.cols;
        cv::Mat img_Y(rows/3*2,cols,CV_8UC1, img.data,ALIGN_UP(cols, 16));
        unsigned char *pImage_UV =img.data+img_Y.rows*ALIGN_UP(cols, 16)*img_Y.channels();
        cv::Mat img_UV(rows/3,cols/2,CV_8UC2,pImage_UV,ALIGN_UP(cols, 16));

        cv::Mat crop_Y(crop_height,crop_width,CV_8UC1,crop.data,ALIGN_UP(crop_width, 16));
        unsigned char *pCrop_UV = crop.data+crop_Y.rows*ALIGN_UP(crop_width, 16)*crop_Y.channels();
        cv::Mat crop_UV(crop_height/2,crop_width/2,CV_8UC2,pCrop_UV,ALIGN_UP(crop_width, 16));

        //3.do the warpAffine for Y and UV separately
        cv::warpAffine(img_Y, crop_Y, transform_y, designed_Y_size);
        cv::warpAffine(img_UV, crop_UV, transform_uv, designed_UV_size);
        return true;
    }



     std::string find_name(FaceDatabase &face_db, float* feat1) {
        int num = face_db.persons.size();

        std::string name;
        float score_max=0.0;
        int person_index= -1;
        //printf("====================================================\n");
        float norm = Normalize<float>(FEATURE_SIZE, feat1);
        if (norm > 2.0f) {
            for (int i_p = 0; i_p < num; i_p++) {
            //printf("%s \n", face_db.persons[i_p].name.c_str());
                int feat_num = face_db.persons[i_p].features.size();
                for (int j = 0; j < feat_num; j++) {
                    float score = CalculSimilarity(feat1, face_db.persons[i_p].features[j].pData, FEATURE_SIZE);
                    if (score > score_max)
                    {
                      score_max = score;
                      person_index = i_p;
                    }
                    //printf("======score: %f ======= \n", score);
                    //printf("~~~~~~~~~~~~~name:%s\n",face_db.persons[i_p].name.c_str());
                   // printf("~~~~~~~score: %f ~~~~~~~ \n", score);
                /*
                    if (score > 0.55) {
                       // c
                        //printf("~~~~~~~score: %f ~~~~~~~ \n", score);
                        return face_db.persons[i_p].name;
                    }
                */
                }
            }
        }

        if (score_max>0.55)
        {
            //printf("max~~~~~~~ %s ~~~~~~~~\n", face_db.persons[person_index].name.c_str());
            //printf("max~~~~~~~score: %f ~~~~~~~ \n", score_max);
            return face_db.persons[person_index].name;

        }
        else
        {
            return name;
        }
    }


private:
    template<class BaseType>
    static void _findNonreflectiveSimilarity(int nPts, const BaseType* uv, const BaseType* xy, cv::Mat& transform) {
        int type = CV_32FC1;
        if (strcmp(typeid(BaseType).name(), "double") == 0)
            type = CV_64FC1;
        using TmpType = BaseType;
        cv::Mat X(nPts * 2, 4, type);
        cv::Mat U(nPts * 2, 1, type);
        for (int i = 0; i < nPts; i++)
        {
            X.ptr<TmpType>(i)[0] = xy[i * 2 + 0];
            X.ptr<TmpType>(i)[1] = xy[i * 2 + 1];
            X.ptr<TmpType>(i)[2] = 1;
            X.ptr<TmpType>(i)[3] = 0;
            X.ptr<TmpType>(i + nPts)[0] = xy[i * 2 + 1];
            X.ptr<TmpType>(i + nPts)[1] = -xy[i * 2 + 0];
            X.ptr<TmpType>(i + nPts)[2] = 0;
            X.ptr<TmpType>(i + nPts)[3] = 1;
            U.ptr<TmpType>(i)[0] = uv[i * 2 + 0];
            U.ptr<TmpType>(i + nPts)[0] = uv[i * 2 + 1];
        }
        cv::Mat r(4, 1, type);
        if (!cv::solve(X, U, r, cv::DECOMP_SVD))
        {
            //std::cout << "failed to solve\n";
            return;
        }

        TmpType sc = r.ptr<TmpType>(0)[0];
        TmpType ss = r.ptr<TmpType>(1)[0];
        TmpType tx = r.ptr<TmpType>(2)[0];
        TmpType ty = r.ptr<TmpType>(3)[0];

        TmpType Tinv[9] =
        {
            sc, -ss, 0,
            ss, sc, 0,
            tx, ty, 1
        };

        cv::Mat Tinv_mat(3, 3, type), T_mat(3, 3, type);
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
                Tinv_mat.ptr<TmpType>(i)[j] = Tinv[i * 3 + j];
        }
        transform = Tinv_mat;
    }

    template<class BaseType>
    static void _findSimilarity(int nPts, const BaseType* uv, const BaseType* xy, cv::Mat& transform) {
        int type = CV_32FC1;
        if (strcmp(typeid(BaseType).name(), "double") == 0)
            type = CV_64FC1;

        using TmpType = BaseType;
        cv::Mat transform1, transform2R, transform2;
        _findNonreflectiveSimilarity(nPts, uv, xy, transform1);
        BaseType* xyR = new BaseType[nPts * 2];
        for (int i = 0; i < nPts; i++)
        {
            xyR[i * 2 + 0] = -xy[i * 2 + 0];
            xyR[i * 2 + 1] = xy[i * 2 + 1];
        }
        _findNonreflectiveSimilarity(nPts, uv, xyR, transform2R);
        delete[]xyR;
        const TmpType TreflectY[9] =
        {
            -1, 0,  0,
            0,  1,  0,
            0,  0,  1
        };
        cv::Mat TreflectY_mat(3, 3, type);
        for (int i = 0; i < 3; i++)
        {
            for (int j = 0; j < 3; j++)
                TreflectY_mat.ptr<TmpType>(i)[j] = TreflectY[i * 3 + j];
        }
        transform2 = transform2R * TreflectY_mat;
        TmpType norm1 = 0, norm2 = 0;
        for (int p = 0; p < nPts; p++)
        {
            TmpType uv1_x = transform1.ptr<TmpType>(0)[0] * xy[p * 2 + 0] + transform1.ptr<TmpType>(1)[0] * xy[p * 2 + 1] + transform1.ptr<TmpType>(2)[0];
            TmpType uv1_y = transform1.ptr<TmpType>(0)[1] * xy[p * 2 + 0] + transform1.ptr<TmpType>(1)[1] * xy[p * 2 + 1] + transform1.ptr<TmpType>(2)[1];
            TmpType uv2_x = transform2.ptr<TmpType>(0)[0] * xy[p * 2 + 0] + transform2.ptr<TmpType>(1)[0] * xy[p * 2 + 1] + transform2.ptr<TmpType>(2)[0];
            TmpType uv2_y = transform2.ptr<TmpType>(0)[1] * xy[p * 2 + 0] + transform2.ptr<TmpType>(1)[1] * xy[p * 2 + 1] + transform2.ptr<TmpType>(2)[1];

            norm1 += (uv[p * 2 + 0] - uv1_x)*(uv[p * 2 + 0] - uv1_x) + (uv[p * 2 + 1] - uv1_y)*(uv[p * 2 + 1] - uv1_y);
            norm2 += (uv[p * 2 + 0] - uv2_x)*(uv[p * 2 + 0] - uv2_x) + (uv[p * 2 + 1] - uv2_y)*(uv[p * 2 + 1] - uv2_y);
        }

        cv::Mat tmp;
        if (norm1 < norm2) {
            cv::invert(transform1, tmp, cv::DECOMP_SVD);
            }
        else
            cv::invert(transform2, tmp, cv::DECOMP_SVD);

        cv::Mat trans(2, 3, type);
        for (int i = 0; i < 2; i++)
        {
            for (int j = 0; j < 3; j++)
            {
                trans.ptr<TmpType>(i)[j] = tmp.ptr<TmpType>(j)[i];
            }
        }

        transform = trans;
    }
};

#endif
