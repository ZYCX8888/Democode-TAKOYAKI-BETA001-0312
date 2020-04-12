#include <iostream>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <string.h>
#include <stdio.h>

struct DetBBox {
    float x1, y1;
    float x2, y2;
    float score;
    float lm1_x, lm1_y;
    float lm2_x, lm2_y;
    float lm3_x, lm3_y;
    float lm4_x, lm4_y;
    float lm5_x, lm5_y;
};

class FaceRecognizeUtils {
public:
    template<class T>
    static float CalculSimilarity(const T* feature1, const T* feature2, int dim) {
        float inner_product = 0.0f;
        float feature_norm1 = 0.0f;
        float feature_norm2 = 0.0f;

        for (int i = 0; i < dim; ++i) {
            inner_product += feature1[i] * feature2[i];
        }
        return inner_product;
    }

    template<class BaseType>
    static bool CropImage_112x112(const cv::Mat &img, const BaseType* facial5point, cv::Mat &crop) {
        cv::Size designed_size(112, 112);
        BaseType coord5point[10] =
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