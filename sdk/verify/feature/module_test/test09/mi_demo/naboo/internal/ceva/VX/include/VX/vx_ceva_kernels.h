/*
 * Copyright (c) 2012-2015 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#ifndef _VX_CEVA_KERNELS_H_
#define _VX_CEVA_KERNELS_H_

#include <VX/vx.h>
#include <VX/vx_vendors.h>
#include <VX/vx_types.h>

/*!
 * \file
 * \brief The list of supported kernels in the OpenVX standard.
 */

#ifdef  __cplusplus
extern "C" {
#endif

/*! \brief The standard list of available libraries */
enum vx_ibrary_cevacv_e {
    /*! \brief The base set of kernels as defined by Khronos. */
    VX_LIBRARY_CEVACV_BASE = (0x1)
};

/*!
 * \brief The standard list of available vision kernels.
 *
 * Each kernel listed here can be used with the <tt>\ref vxGetKernelByEnum</tt> call.
 * When programming the parameters, use
 * \arg <tt>\ref VX_INPUT</tt> for [in]
 * \arg <tt>\ref VX_OUTPUT</tt> for [out]
 * \arg <tt>\ref VX_BIDIRECTIONAL</tt> for [in,out]
 *
 * When programming the parameters, use
 * \arg <tt>\ref VX_TYPE_IMAGE</tt> for a <tt>\ref vx_image</tt> in the size field of <tt>\ref vxGetParameterByIndex</tt> or <tt>\ref vxSetParameterByIndex</tt>  * \arg <tt>\ref VX_TYPE_ARRAY</tt> for a <tt>\ref vx_array</tt> in the size field of <tt>\ref vxGetParameterByIndex</tt> or <tt>\ref vxSetParameterByIndex</tt>  * \arg or other appropriate types in \ref vx_type_e.
 * \ingroup group_kernel
 */
enum vx_kernel_cevacv_e {

    /*! \brief The SumX SumX2 Kernel.
    * \note Use "com.ceva-dsp.cevacv.sumx_sumx2" to \ref vxGetKernelByName.
    * \param [in] vx_image The VX_DF_IMAGE_U8 input image.
    * \param [out] vx_scalar The VX_TYPE_INT32 sum of pixels.
    * \param [out] vx_scalar The VX_TYPE_INT64 sum of squares of pixels.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_SUMX_SUMX2 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x1,

    /*! \brief The Sum Kernel.
    * \note Use "com.ceva-dsp.cevacv.sum" to \ref vxGetKernelByName.
    * \param [in]  vx_image  The VX_DF_IMAGE_U8 input image.
    * \param [out] vx_scalar The VX_TYPE_INT32 sum of pixels.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_SUM = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x2,

    /*! \brief The Abs Kernel.
    * \note Use "com.ceva-dsp.cevacv.abs" to \ref vxGetKernelByName.
    * \param [in]  vx_image The VX_DF_IMAGE_S16 input image.
    * \param [iyt] vx_image The VX_DF_IMAGE_U16 abs image.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_ABS = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x3,

    /*! \brief The Scale-Add Kernel.
    * \note Use "com.ceva-dsp.cevacv.scaleadd" to \ref vxGetKernelByName.
    * \param [in]  vx_image The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 first input image.
    * \param [in]  vx_image The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 second input image.
    * \param [in]  vx_scalar The VX_TYPE_INT8 scaling factor scalar.
    * \param [out] vx_image The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 output image.
    * \ingroup group_vision_function_cevacv
    */

    VX_KERNEL_CEVACV_SCALE_ADD = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x4,

    /*! \brief The Add Weighted Kernel.
    * \note Use "com.ceva-dsp.cevacv.add_weighted" to \ref vxGetKernelByName.
    * \param [in]    vx_image The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 input image.
    * \param [in]    vx_image The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 input image.
    * \param [out]  vx_image The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 output image.
    * \param [in]    vx_image The VX_TYPE_INT8 alpha scalar factor.
    * \param [in]    vx_image The VX_TYPE_INT8 beta  scalar factor.
    * \param [in]    vx_image The VX_TYPE_INT8 gamma scalar factor.
    * \ingroup group_vision_function_cevacv
    */

    VX_KERNEL_CEVACV_ADD_WEIGHTED = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x5,

    /*! \brief The Correlation Kernel.
    * \note Use "com.ceva-dsp.cevacv.corr" to \ref vxGetKernelByName.
    * \param [in]    vx_image The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 input image.
    * \param [in]    vx_matrix The VX_TYPE_INT32 coefficients matrix.
    * \param [in]  vx_scalar The VX_TYPE_INT32 filter size (3,5,7,11,13,15,17) scalar.
    * \param [out]    vx_image The VX_TYPE_S16 output image.
    * \param [in]  vx_scalar The VX_TYPE_INT32 post-shift factor.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_CORR = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x6,

    /*! \brief Minimum in a 3x3 neighborhood (erode) Kernel.
    * \note Use "com.ceva-dsp.cevacv.min3x3" to \ref vxGetKernelByName.
    * \param [in]    vx_image The VX_DF_IMAGE_U8 input image.
    * \param [out]    vx_image The VX_DF_IMAGE_U8 output image .
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_MIN3X3 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x7,


    /*! \brief Maximum in a 3x3 neighborhood (dilate) Kernel.
    * \note Use "com.ceva-dsp.cevacv.max3x3" to \ref vxGetKernelByName.
    * \param [in]    vx_image    The VX_DF_IMAGE_U8 input image.
    * \param [in]    vx_scalar    The VX_TYPE_UINT8 filter threshold.
    * \param [out]    vx_image    The VX_DF_IMAGE_U8 output image.
    * \ingroup group_vision_function_cevacv
    */

    VX_KERNEL_CEVACV_MAX3X3 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x8,



    /*! \brief The laplacian filter kernel.
    * \note Use "com.ceva-dsp.cevacv.laplacian3x3" to \ref vxGetKernelByName.
    * \param [in] vx_image The VX_DF_IMAGE_U8 input image.
    * \param [out] vx_image The VX_DF_IMAGE_U8 output image.
    * \see group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_LAPLACIAN3X3 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x9,


    /*! \brief Minimum and Maximum in a 3x3 neighborhood (erode, dilate) Kernel.
    * \note Use "com.ceva-dsp.cevacv.minMax3x3" to \ref vxGetKernelByName.
    * \param [in]    vx_image    The VX_DF_IMAGE_U8 input image.
    * \param [in]    vx_image    The VX_DF_IMAGE_U8 min output image.
    * \param [out]    vx_image    The VX_DF_IMAGE_U8 max output image.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_MINMAX3X3 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0xA,

    /*! \brief The Element-Wise Minimum Kernel.
    * \note Use "com.ceva-dsp.cevacv.min" to \ref vxGetKernelByName.
    * \param [in]    vx_image    The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 first  input image.
    * \param [in]    vx_image    The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 second input image.
    * \param [out]    vx_image    The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 output image.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_MIN = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0xB,

    /*! \brief The Element-Wise Maximum Kernel.
    * \note Use "com.ceva-dsp.cevacv.max" to \ref vxGetKernelByName.
    * \param [in]    vx_image    The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 first  input image.
    * \param [in]    vx_image    The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 second input image.
    * \param [out]    vx_image    The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 output image.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_MAX = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0xC,

    /*! \brief The Count Non-Zero Elements Kernel.
    * \note Use "com.ceva-dsp.cevacv.countNonZero" to \ref vxGetKernelByName.
    * \param [in]    vx_image    The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 first input image.
    * \param [out]  vx_scalar    The VX_TYPE_UINT32 output scalar.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_COUNT_NON_ZERO = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0xD,


    /*! \brief The Non-Maximum Suppression Threshold 3x3 Kernel.
    * \note Use "com.ceva-dsp.cevacv.nonMaxSuppressThreshold3x3" to \ref vxGetKernelByName.
    * \param [in] vx_image The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 input image.
    * \param [in] vx_scalar The VX_TYPE_U8 or VX_TYPE_S16 threshold scalar.
    * \param [out] vx_image The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 output image.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_NON_MAX_SUPPRESS_THRESHOLD_3X3 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0xE,


    /*! \brief The Gaussian Kernel.
    * \note Use "com.ceva-dsp.cevacv.Gaussian" to \ref vxGetKernelByName.
    * \param [in]  vx_image        The VX_DF_IMAGE_U8 input image.
    * \param [out]  vx_image    The VX_DF_IMAGE_U8 output image.
    * \param [in]  vx_scalar    The VX_DF_IMAGE_INT32 window size (3,5,7) scalar.
    * \ingroup group_vision_function_cevacv
    */

    VX_KERNEL_CEVACV_GAUSSIAN = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0xF,

    /*! \brief The Sort Ascending Kernel.
    * \note Use "com.ceva-dsp.cevacv.sort_ascending" to \ref vxGetKernelByName.
    * \param [in]    vx_array    The VX_TYPE_UINT32 input array.
    * \param [out]    vx_array    The VX_TYPE_UINT32 output array.
    * \ingroup group_vision_function_cevacv
    */

    VX_KERNEL_CEVACV_SORT_ASCENDING = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x10,

    /*! \brief The Sort Descending Kernel.
    * \note Use "com.ceva-dsp.cevacv.sort_descending" to \ref vxGetKernelByName.
    * \param [in]    vx_array    The VX_TYPE_UINT32 input array.
    * \param [out]    vx_array    The VX_TYPE_UINT32 output array.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_SORT_DESCENDING = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x11,

    /*! \brief The Box Filter 5x5 Kernel.
    *    \note Use "com.ceva-dsp.cevacv.box_5x5" to \ref vxGetKernelByName.
    *    \param[in]     vx_image    The VX_DF_IMAGE_U8 input image.
    *    \param[out]  vx_image    The VX_DF_IMAGE_U8 output image.
    *    \param[out]  vx_scalar    The VX_DF_IMAGE_INT32 window size (5) scalar.
    *    \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_BOX_FILTER5X5 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x12,

    /*! \brief The Bit Count Kernel.
    *    \note Use "com.ceva-dsp.cevacv.bitCount" to \ref vxGetKernelByName.
    *    \param[in]     vx_image    The VX_DF_IMAGE_U8 input image.
    *    \param[out]  vx_scalar    Number of bits in the image (VX_TYPE_INT32).
    *    \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_BIT_COUNT = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x13,

    /*! \brief The Sum9x9 Kernel.
    * \note Use "com.ceva-dsp.cevacv.sum9x9" to \ref vxGetKernelByName.
    * \param [in]    vx_image    The VX_DF_IMAGE_U8 input array.
    * \param [out]    vx_image    The VX_DF_IMAGE_U16 output array.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_SUM9x9 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x14,

    /*! \brief The Sum9x9 Kernel.
    * \note Use "com.ceva-dsp.cevacv.sum9x9" to \ref vxGetKernelByName.
    * \param [in]    vx_image    The VX_DF_IMAGE_U8 input array.
    * \param [out]    vx_image    The VX_DF_IMAGE_U16 output array.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_SUM_SQUARE9x9 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x15,

    /*! \brief The Sum9x9 Kernel.
    * \note Use "com.ceva-dsp.cevacv.sum9x9" to \ref vxGetKernelByName.
    * \param [in]    vx_image    The VX_DF_IMAGE_U8 input array.
    * \param [out]    vx_image    The VX_DF_IMAGE_U16 output array.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_MEDIAN_SEP5x5 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x16,


    /*! \brief The fast9 Kernel.
    * \note Use "com.ceva-dsp.cevacv.fast9" to \ref vxGetKernelByName.
    * \param [in]    vx_image    The VX_DF_IMAGE_U8 input image.
    * \param [in]    vx_scalar    The VX_TYPE_UINT8 input threshold.
    * \param [out]  vx_image    The VX_DF_IMAGE_U8 output image.
    * \ingroup group_vision_function_cevacv
    */

    VX_KERNEL_CEVACV_FAST9 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x17,

    /*! \brief The Non-Maximum Suppression 3x3 Kernel.
    * \note Use "com.ceva-dsp.cevacv.nonMaxSuppress3x3" to \ref vxGetKernelByName.
    * \param [in] vx_image The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 input image.
    * \param [out] vx_image The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 output image.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_NON_MAX_SUPPRESS_3X3 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x18,

    /*! \brief The Non-Maximum Suppression 3x3 Kernel.
    * \note Use "com.ceva-dsp.cevacv.nonMaxSuppress3x3" to \ref vxGetKernelByName.
    * \param [in] vx_image The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 input image.
    * \param [out] vx_image The VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 output image.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_KEYPOINTS_COLLECT = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x19,

    /*! \brief The Extract Patches Kernel.
    * \note Use "com.ceva-dsp.cevacv.extractPatches" to \ref vxGetKernelByName.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [in] in_keypoints The input keypoints.
    * \param [in] patch_width The patches width.
    * \param [in] patch_height The patches height.
    * \param [out] out_patches The output array of patches.
    * \ingroup group_vision_function_cevacv
    */
    VX_KERNEL_CEVACV_EXTRACT_PATCHES = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x1A,

    VX_KERNEL_CEVACV_HISTOGRAM_EQUALIZATION = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x1B,

    /*! \brief The Patch Gaussian Kernel.
    * \note Use "com.ceva-dsp.cevacv.PatchGaussian" to \ref vxGetKernelByName.
    * \param [in]   vx_array    The VX_TYPE_PATCH input array.
    * \param [out]  vx_array    The VX_TYPE_PATCH output array.
    * \param [in]   vx_scalar    The VX_DF_IMAGE_UINT8 window size (3,5) scalar.
    * \ingroup group_vision_function_cevacv
    */

    VX_KERNEL_CEVACV_PATCH_GAUSSIAN = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x1C,

    VX_KERNEL_CEVACV_PATCH_MOMENTS = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x1D,

    VX_KERNEL_CEVACV_PATCH_OBRIEF_DESCRIPTOR = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x1E,

    //VX_KERNEL_CEVACV_PYRAMID_AND_DETECT = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x1F,

    VX_KERNEL_CEVACV_HARRIS_SCORE = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x20,

    VX_KERNEL_CEVACV_ORB_DESCRIPTOR = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x21,

    VX_KERNEL_CEVACV_BRIEF_DESCRIPTOR_MATCHER = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x22,

    VX_KERNEL_CEVACV_SCALE_UP_GAUSSIAN_BY2 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x23,

    VX_KERNEL_CEVACV_SCALE_DOWN_GAUSSIAN_BY2 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x24,

    VX_KERNEL_CEVACV_OPTICAL_FLOW_PYR_LK = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x25,

    VX_KERNEL_CEVACV_OPTICAL_FLOW_IMG_LK = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x26,

    VX_KERNEL_CEVACV_BILATERAL = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x27,

  
    /*! \brief The Calculation of 3x3 corner min eigenvalue value kernel.
    * \note Use "com.ceva-dsp.cevacv.calc_3x3_corner_min_eigen_val" to \ref vxGetKernelByName.
    * \param[in]  p_s8Dx     Pointer to derivative in the x direction
    * \param[in]  p_s8Dy     Pointer to derivative in the y direction
    * \param[out] p_u8Dst     Pointer to the destination
    *
    * \ingroup group_vision_function_cevacv
    */

    VX_KERNEL_CEVACV_CALC_3X3_CORNER_MIN_EIGEN_VAL = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x28,

    VX_KERNEL_CEVACV_CALC_SYM_2X2_CORNER_MIN_EIGEN_VAL = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x29,

    VX_KERNEL_CEVACV_EXP_MINUS_X = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x2A,

    VX_KERNEL_CEVACV_GRADIENT_2D = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x2B,

    /*! \brief [Graph] Creates a Hog node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */

    //VX_KERNEL_CEVACV_HOG = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x2C,

    VX_KERNEL_CEVACV_LBP = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x2D,

    VX_KERNEL_CEVACV_CORR_SEP = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x2E,

    VX_KERNEL_CEVACV_SIGMA = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x2F,

    VX_KERNEL_CEVACV_THRESHOLD_HYSTERESIS = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x30,

    VX_KERNEL_CEVACV_VERTICAL_MIRROR = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x31,

    VX_KERNEL_CEVACV_SCALE_UP_BY2 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x32,

    VX_KERNEL_CEVACV_SCALE_DOWN_BY2 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x33,

    VX_KERNEL_CEVACV_SCALE_DOWN_BY4 = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x34,

    VX_KERNEL_CEVACV_LAPLACIAN_PYRAMID = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x35,

    VX_KERNEL_CEVACV_COMBINE_PYRAMID_LEVELS = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x36,

    VX_KERNEL_CEVACV_EXTRACT_PYRAMID_LEVEL = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x37,

    VX_KERNEL_CEVACV_IDENTITY = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x38,

    /*! \brief Implements rectangle fill kernel
    * \This function fills a rectangle with a specified value.
    * \param[in]  vx_scalar - fill value
    * \param[out] vx_image - filled image
    *  \ingroup group_vision_function_cevacv
    */
    //VX_KERNEL_CEVACV_FILL = VX_KERNEL_BASE(VX_ID_CEVA, VX_LIBRARY_CEVACV_BASE) + 0x39,


        /* insert new kernels here */
    VX_KERNEL_CEVACV_MAX_1_0 /*!< \internal Used for bounds checking in the conformance test. */
};

//vx_kernel vxCevaCVPyramidAndDetectKernel(vx_context context);
vx_kernel vxCevaCVOpticalFlowPyrLKKernel(vx_context context);
vx_kernel vxCevaCVOpticalFlowImgLKKernel(vx_context context);
vx_kernel vxCevaCVFast9Kernel(vx_context context);
vx_kernel vxCevaCVOrbDescriptorKernel(vx_context context);
vx_kernel vxCevaCVBriefDescrptorMatcherKernel(vx_context context);
vx_kernel vxCevaCVHarrisScoreKernel(vx_context context);
vx_kernel vxCevaCVLaplacianPyramidKernel(vx_context context);
vx_kernel vxCevaCVKeypointsCollectKernel(vx_context context);
vx_kernel vxCevaCVHistogramEqualizationKernel(vx_context context);
vx_kernel vxCevaCVBitCountKernel(vx_context context);
vx_kernel vxCevaCVCountNonZeroKernel(vx_context context);
vx_kernel vxCevaCVAbsKernel(vx_context context);
vx_kernel vxCevaCVAddWeightedKernel(vx_context context);
vx_kernel vxCevaCVScaleAddKernel(vx_context context);
vx_kernel vxCevaCVMinKernel(vx_context context);
vx_kernel vxCevaCVMin3x3Kernel(vx_context context);
vx_kernel vxCevaCVMaxKernel(vx_context context);
vx_kernel vxCevaCVMax3x3Kernel(vx_context context);
vx_kernel vxCevaCVMinMax3x3Kernel(vx_context context);
vx_kernel vxCevaCVMedianSep5x5Kernel(vx_context context);
vx_kernel vxCevaCVSumKernel(vx_context context);
vx_kernel vxCevaCVSum9x9Kernel(vx_context context);
vx_kernel vxCevaCVSumSquare9x9Kernel(vx_context context);
vx_kernel vxCevaCVSumXSumX2Kernel(vx_context context);
vx_kernel vxCevaCVBoxFilter5x5Kernel(vx_context context);
vx_kernel vxCevaCVGaussianKernel(vx_context context);
vx_kernel vxCevaCVLaplacian3x3Kernel(vx_context context);
vx_kernel vxCevaCVCorrelationKernel(vx_context context);
vx_kernel vxCevaCVExtractPatchesKernel(vx_context context);
vx_kernel vxCevaCVPatchMomentsKernel(vx_context context);
vx_kernel vxCevaCVPatchGaussianKernel(vx_context context);
vx_kernel vxCevaCVPatchOrientedBRIEFDescriptorKernel(vx_context context);
vx_kernel vxCevaCVNonMaxSuppressThreshold3x3Kernel(vx_context context);
vx_kernel vxCevaCVNonMaxSuppress3x3Kernel(vx_context context);
vx_kernel vxCevaCVSortAscendingKernel(vx_context context);
vx_kernel vxCevaCVSortDescendingKernel(vx_context context);
vx_kernel vxCevaCVScaleUpGaussianBy2Kernel(vx_context context);
vx_kernel vxCevaCVScaleDownGaussianBy2Kernel(vx_context context);
vx_kernel vxCevaCVBilateralKernel(vx_context context);
vx_kernel vxCevaCVCalcSym2x2CornerMinEigenValKernel(vx_context context);
vx_kernel vxCevaCVCorrSepKernel(vx_context context);
vx_kernel vxCevaCVExpMinusXKernel(vx_context context);
vx_kernel vxCevaCVGradient2DKernel(vx_context context);
vx_kernel vxCevaCVLbpKernel(vx_context context);
vx_kernel vxCevaCVSigmaKernel(vx_context context);
vx_kernel vxCevaCVThresholdHysteresisKernel(vx_context context);
vx_kernel vxCevaCVVerticalMirrorKernel(vx_context context);
vx_kernel vxCevaCVScaleUpBy2Kernel(vx_context context);
vx_kernel vxCevaCVScaleDownBy2Kernel(vx_context context);
vx_kernel vxCevaCVScaleDownBy4Kernel(vx_context context);
vx_kernel vxCevaCVCombinePyramidLevelsKernel(vx_context context);
vx_kernel vxCevaCVExtractPyramidLevelKernel(vx_context context);
vx_kernel vxCevaCVIdentityKernel(vx_context context);
vx_kernel vxCevaCVCalc3x3CornerMinEigenValKernel(vx_context context);
//vx_kernel vxCevaCVFillKernel(vx_context context);
//vx_kernel vxCevaCVHogKernel(vx_context context);

#ifdef  __cplusplus
}
#endif

#endif  /* _VX_CEVA_KERNELS_H_ */
