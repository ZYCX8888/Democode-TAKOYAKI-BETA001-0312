/*******************************************************************************
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
******************************************************************************/

#ifndef _VX_CEVA_NODES_H_
#define _VX_CEVA_NODES_H_

#include <VX/vx.h>
#include <VX/vx_vendors.h>
#include <VX/vx_types.h>

/*!
 * \file vx_khronos_kernels.h
 * \brief The "Simple" API interface for OpenVX. These APIs are just
 * wrappers around the more verbose functions defined in <tt>\ref vx_api.h</tt>.
 */

#ifdef __cplusplus
extern "C" {
#endif


    /* \brief [Graph] Creates a Sum and Sum-Of-Squares node.
    * \param [in] graph The handle to the graph.
    * \param [in] input The input image in VX_DF_IMAGE_U8 format.
    * \param [out] output The output sum in VX_TYPE_INT_32 format.
    * \param [out] output The output sum of squares in VX_TYPE_INT_64 format.
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVSumXSumX2Node(vx_graph graph, vx_image input, vx_scalar sumx, vx_scalar sumx2);

    /* \brief [Graph] Creates a Sum node.
    * \param [in] graph The handle to the graph.
    * \param [in] input The input image in VX_DF_IMAGE_U8 format.
    * \param [out] output The output sum in VX_TYPE_INT_32 format.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVSumNode(vx_graph graph, vx_image input, vx_scalar sum);
    /* \brief [Graph] Creates an Absolute Value node.
    * \param [in] graph The handle to the graph.
    * \param [in] input The input image in VX_DF_IMAGE_U8 or VX_DF_IMAGE_S16 format.
    * \param [out] output The abs image in  VX_DF_IMAGE_U8 or VX_DF_IMAGE_U16 format.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVAbsNode(vx_graph graph, vx_image in1, vx_image out);

    /*! \brief [Graph] Creates a Scale-and-Add node
    * \param [in] graph The reference to the graph.
    * \param [in] in1 An input image.
    * \param [in] in2 An input image.
    * \param [in] scale Scalar factor to apply to in1.
    * \param[out] out The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVScaleAddNode(vx_graph graph, vx_image in1, vx_image in2, vx_scalar scale, vx_image out);

    /*! \brief [Graph] Creates a AddWeighted node.
    * \param [in] graph The reference to the graph.
    * \param [in] input0 First input image.
    * \param [in] input1 Second input image.
    * \param [out] output The output image.
    * \param [in] alpha Alpha input scalar.
    * \param [in] beta Beta input scalar.
    * \param [in] gamma Gamma input scalar.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVAddWeightedNode(vx_graph graph, vx_image input0, vx_image input1, vx_image output, vx_scalar alpha, vx_scalar beta, vx_scalar gamma);

    /*! \brief [Graph] Creates a Correlation node.
    * \param [in] graph The reference to the graph.
    * \param [in] input First input image.
    * \param [in] corr_mat Matrix for correlation.
    * \param [in] right_shift Right shift.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVCorrelationNode(vx_graph graph, vx_image input, vx_matrix corr_mat, vx_uint8 right_shift, vx_image output);

    /*! \brief [Graph] Creates a Min3x3 node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVMin3x3Node(vx_graph graph, vx_image input, vx_image output);

    /*! \brief [Graph] Creates a Max3x3 node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVMax3x3Node(vx_graph graph, vx_image input, vx_image output);

    /*! \brief [Graph] Creates a Laplacian3x3 Node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVLaplacian3x3Node(vx_graph graph, vx_image input, vx_image output);

    /*! \brief [Graph] Creates a Min Max 3x3 node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [out] outputMin The output minimum image.
    * \param [out] outputMax The output maximum image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVMinMax3x3Node(vx_graph graph, vx_image input, vx_image outputMin, vx_image outputMax);



    /*! \brief [Graph] Creates an Ascending Sort node.
    * \param [in] graph The reference to the graph.
    * \param [in] input Input array of unsigned integers.
    * \param [out] output Output array.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVSortAscendingNode(vx_graph graph, vx_array input, vx_array output);


    /*! \brief [Graph] Creates a Descending Sort node.
    * \param [in] graph The reference to the graph.
    * \param [in] input Input array of unsigned integers.
    * \param [out] output Output array.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVSortDescendingNode(vx_graph graph, vx_array input, vx_array output);

    /*! \brief [Graph] Creates an element-wise Minimum node.
    * \param [in] graph The reference to the graph.
    * \param [in] input0 The first input image.
    * \param [in] input1 The second input image.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVMinNode(vx_graph graph, vx_image input0, vx_image input1, vx_image output);

    /*! \brief [Graph] Creates an element-wise Maximum node.
    * \param [in] graph The reference to the graph.
    * \param [in] input0 The first input image.
    * \param [in] input1 The second input image.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVMaxNode(vx_graph graph, vx_image input0, vx_image input1, vx_image output);

    /*! \brief [Graph] Creates a BitCount node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [out] output The result scalar.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVBitCountNode(vx_graph graph, vx_image input, vx_scalar output);

    /*! \brief [Graph] Creates a CountNonZero node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [out] output The result scalar.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVCountNonZeroNode(vx_graph graph, vx_image input, vx_scalar output);

    /*! \brief [Graph] Creates an Exp(-x) node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVExpMinusXNode(vx_graph graph, vx_image input, vx_image output);

    /*! \brief [Graph] Creates a Non-Maxima Suppression 3x3 node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [in] threshold Input threshold.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVNonMaxSuppressThreshold3x3Node(vx_graph graph, vx_image input, vx_scalar threshold, vx_image output);

    /*! \brief [Graph] Creates a Gaussian Filter node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [in] input The kernel size.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVGaussianNode(vx_graph graph, vx_image input, vx_image output, vx_int32 kernel_size);

    /*! \brief [Graph] Scale Down Gaussian node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [in] input The kernel size.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVScaleDownGaussianBy2Node(vx_graph graph, vx_image input, vx_image output, vx_int32 kernel_size);

    /*! \brief [Graph] Scale Up Gaussian node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [in] input The kernel size.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVScaleUpGaussianBy2Node(vx_graph graph, vx_image input, vx_image output, vx_int32 kernel_size);

    /*! \brief [Graph] Creates a Box Filter 5x5 node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVBoxFilter5x5Node(vx_graph graph, vx_image input, vx_image output);

    /*! \brief [Graph] Creates a Sum 9x9 filter node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVSum9x9Node(vx_graph graph, vx_image input, vx_image output);

    /*! \brief [Graph] Creates a Sum Square 9x9 filter node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVSumSquare9x9Node(vx_graph graph, vx_image input, vx_image output);

    /*! \brief [Graph] Creates a Median Separable 5x5 filter node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVMedianSep5x5Node(vx_graph graph, vx_image input, vx_image output);

      /*! \brief [Graph] Creates a Hog node.
      * \param [in] graph The reference to the graph.
      * \param [in] input The input image.
     * \param [out] output The output image.
      * \return <tt>\ref vx_node</tt>.
      * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
      * \ingroup group_vision_function_cevacv
      */
    //VX_API_ENTRY vx_node VX_API_CALL vxCevaCVHogNode(vx_graph graph, vx_image input, vx_scalar threshold, vx_array desc_output);

    /*! \brief [Graph] Creates a Fast9 node.
    * \param [in] Graph.
    * \param [in] Input image.
    * \param [in] Threshold.
    * \param [out] Output image.
    * \param [in] Compute score - specifies whether to compute the FAST-9 score of each pixel. If not, corners will be set to the value of threshold, other pixels will be zeroed.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVFast9Node(vx_graph graph, vx_image input, vx_scalar strength_thresh, vx_image output, vx_bool compute_score);

    /*! \brief [Graph] Creates a Non-Maxima Suppression 3x3 node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [out] output The output image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVNonMaxSuppress3x3Node(vx_graph graph, vx_image input, vx_image output);

    /*! \brief [Graph] Creates a Keypoints Collect node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [in] threshold.
    * \param [out] output The output array of keypoints.
    * \param [out] output number of keypoints (optional).
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVKeypointsCollectNode(vx_graph graph, vx_image input, vx_scalar threshold, vx_array output, vx_scalar num_corners);

    /*! \brief [Graph] Creates an Extract Patches node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [in] in_keypoints The input keypoints.
    * \param [in] scale The scale to apply to keypoints' coordinates
    * \param [out] out_patches The output array of patches.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVExtractPatchesNode(vx_graph graph, vx_image input, vx_array in_keypoints, vx_scalar scale, vx_scalar x_offset, vx_scalar y_offset, vx_array out_patches);

    /*! \brief [Graph] Creates a Histogram Equalization node.
    * \param [in] graph The reference to the graph.
    * \param [in] distribution - input histogram
    * \param [in] input - original image width
    * \param [in] input - original image height
    * \param [out] LUT - output lut of equalization
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVHistogramEqualizationNode(vx_graph graph, vx_distribution distribution, vx_lut lut, vx_scalar width, vx_scalar height);

    /*! \brief [Graph] Creates a Harris score node.
    * \param [in] graph The reference to the graph.
    * \param [in] vx_image - sobel Dx output (VX_IMAGE_DF_S16)
    * \param [in] vx_image - sobel Dy output (VX_IMAGE_DF_S16)
    * \param [in] vx_scalar - detector block size (VX_TYPE_INT32)
    * \param [in] vx_scalar - detector sensitivity (VX_TYPE_FLOAT32)
    * \param [out] vx_image - Harris scores output map (VX_IMAGE_DF_S16)
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVHarrisScoreNode(vx_graph graph, vx_image Dx, vx_image Dy, vx_image score_map, vx_scalar sensitiviy, vx_scalar block_size);

    /*! \brief [Graph] Creates a pyramid and detect node.
    * \param [in] graph The reference to the graph.
    * \param [in] vx_image - input image (VX_IMAGE_DF_U8)
    * \param [in] vx_scalar - number of features to detect (VX_TYPE_SIZE)
    * \param [in] vx_scalar - detector threshold (VX_TYPE_INT32)
    * \param [in] vx_scalar - detector sensitivity (VX_TYPE_FLOAT32)
    * \param [in] vx_scalar - detect level (VX_TYPE_SIZE)
    * \param [in] vx_rectangle_t - ROI
    * \param [in] vx_scalar - grid size (optional)
    * \param [out] vx_pyramid output Gaussian pyramid
    * \param [out] vx_array detected features list (VX_TYPE_KEYPOINT)
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    //VX_API_ENTRY vx_node VX_API_CALL vxCevaCVPyramidAndDetectNode(vx_graph graph, vx_image input, vx_pyramid output,
    //    vx_scalar num_features, vx_scalar detector_thresh,
    //    vx_scalar detector_sensitivity, vx_scalar detect_level, vx_rectangle_t roi,
    //    vx_array output_features_list, vx_scalar grid_size);

    /*! \brief [Graph] Creates a Patch Gaussian node.
    * \param [in] graph The reference to the graph.
    * \param [in] input - input array of patches.
    * \param [out] output - output array of patches.
    * \param [in] kernel_size - kernel size (supported values: 3, 5).
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVPatchGaussianNode(vx_graph graph, vx_array input, vx_array output, vx_uint8 kernel_size);

    /*! \brief [Graph] Creates a Patch Moments 31x31 node.
    * \param [in] graph The reference to the graph.
    * \param [in] input - input array of patches.
    * \param [out] output_mx - output array of moments in X axis.
    * \param [out] output_my - output array of moments in Y axis.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVPatchMomentsNode(vx_graph graph, vx_array input, vx_array output_mx, vx_array output_my);

    /*! \brief [Graph] Creates a Patch Oriented BRIEF Descriptor node.
    * \param [in] graph The reference to the graph.
    * \param [in] input - input array of patches.
    * \param [in] input_mx - input array of moments in X axis.
    * \param [in] input_my - input array of moments in Y axis.
    * \param [out] output  - output array of 256 bit descriptors.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVPatchOrientedBRIEFDescriptorNode(vx_graph graph, vx_array input, vx_array input_mx, vx_array input_my, vx_array output);

    /*! \brief [Graph] Creates a ORB Descriptor node.
    * \param [in] graph The reference to the graph.
    * \param [in] input - input image.
    * \param [in] input_keypoints - input array key-points.
    * \param [in] gaussian_kernel_size - input Gaussian kernel size (0 - without Gaussian, 3, 5).
    * \param [out] descriptors  - output array of 256 bit descriptors.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVOrbDescriptorNode(vx_graph graph, vx_image input, vx_array input_keypoints, vx_uint8 gaussian_kernel_size, vx_array descriptors);

    /*! \brief [Graph] Creates a BRIEF Descriptor Matcher node.
    * \param [in] graph The reference to the graph.
    * \param [in] query_descriptors - input query descriptors.
    * \param [in] train_descriptors - input train descriptors.
    * \param [in] radius    - Maximal distance to closest matching descriptor.
    * \param [in] ratio_q8    - Minimal ratio between the distance to the best match and the distance to the second best match.
    * \param [out] matches  - output array matches.
    * \param [out] num_matches - output scalar number of matches.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVBriefDescriptorMatcherNode(vx_graph graph, vx_array query_descriptors, vx_array train_descriptors, vx_uint16 radius, vx_uint8 ratio_q8, vx_array matches, vx_scalar num_matches);

    /*! \brief [Graph] Creates a conversion from relative coordinates to absolute coordinates node.
    * \param [in] graph The reference to the graph.
    * \param [in] prev_pyramid - input previous pyramid
    * \param [in] curr_pyramid - input current pyramid
    * \param [in] prev_features - input previous features
    * \param [out] curr_features - output tracked features
    * \param [in] num_iterations - number of iterations in each level
    * \param [in] min_determinate - minimum determinate for computational stability
    * \param [in] max_residue - maximum residue
    * \param [in] tracker_dimensions - tracker window (height == width)
    * \param [in] numof_features - number of features to track
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVOpticalFlowPyrLKNode(
        vx_graph graph, vx_pyramid prev_pyramid, vx_pyramid curr_pyramid,
        vx_array prev_features, vx_array curr_features, vx_scalar num_iterations, vx_scalar min_determinate,
        vx_scalar max_residue, vx_scalar tracker_dimensions, vx_size numof_features);

    /*! \brief [Graph] Creates a conversion from relative coordinates to absolute coordinates node.
    * \param [in] graph The reference to the graph.
    * \param [in] prev_image - input previous image
    * \param [in] curr_image - input current image
    * \param [in] prev_features - input previous features
    * \param [out] curr_features - output tracked features
    * \param [in] num_iterations - number of iterations in each level
    * \param [in] min_determinate - minimum determinate for computational stability
    * \param [in] max_residue - maximum residue
    * \param [in] tracker_dimensions - tracker window (height == width)
    * \param [in] numof_features - number of features to track
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVOpticalFlowImgLKNode(vx_graph graph, vx_image prev_image, vx_image curr_image,
        vx_array prev_features, vx_array curr_features, vx_scalar num_iterations, vx_scalar min_determinate,
        vx_scalar max_residue, vx_scalar tracker_dimensions, vx_size numof_features);

    /*! \brief [Graph] Creates dx & dy gradients node.
    * \param [in] graph The reference to the graph.
    * \param [in] input - input image.
    * \param [out] output_dx - dx image.
    * \param [out] output_dy - dy image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVGradient2DNode(vx_graph graph, vx_image input, vx_image output_dx, vx_image output_dy);

    /*! \brief [Graph] Creates bilateral filter node.
    * \param [in] graph The reference to the graph.
    * \param [in] input - input image.
    * \param [out] output - input image.
    * \param [in] input - kernel size.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVBilateralNode(vx_graph graph, vx_image input, vx_image output, vx_scalar ksize);

    /*! \brief [Graph] Creates vertical mirror node.
    * \param [in] graph The reference to the graph.
    * \param [in] input - input image.
    * \param [out] output - input image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVVerticalMirrorNode(vx_graph graph, vx_image input, vx_image output);

    /*! \brief [Graph] Creates 2x2 corner minimum eigenvalue node.
    * \param [in] graph The reference to the graph.
    * \param [in] input_a - input a image.
    * \param [in] input_b - input b image.
    * \param [in] input_c - input c image.
    * \param [out] output - input image.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVCalcSym2x2CornerMinEigenValNode(vx_graph graph, vx_image input_a, vx_image input_b, vx_image input_c, vx_image output);

    /**
    *
    *! \brief [Graph] Creates 3x3 corner minimum eigenvalue node.
    * \param[in]  p_s8Dx     Pointer to derivative in the x direction
    * \param[in]  p_s8Dy     Pointer to derivative in the y direction
    * \param[out] p_u8Dst     Pointer to the destination
    *\return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    *
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVCalc3x3CornerMinEigenValNode(vx_graph graph, vx_image input_x, vx_image input_y, vx_image output);
    /*! \brief [Graph] Creates threshold hysteresis node.
    * \param [in] graph The reference to the graph.
    * \param [in] input - input a image.
    * \param [in] threshold_l - low threshold.
    * \param [in] threshold_h - hight threshold.
    * \param [out] output_map - output map.
    * \param [out] output_list - output coordinates list (int - 16b x coor, 16b y coor).
    * \param [out] output_num_points - output num of points.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVThresholdHysteresisNode(vx_graph graph, vx_image input, vx_scalar threshold_l, vx_scalar threshold_h, vx_image output_map, vx_array output_list, vx_scalar output_num_points);

    /*! \brief [Graph] Creates lbp node.
    * \param [in] graph The reference to the graph.
    * \param [in] input- input image.
    * \param [in] output - output image
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVLbpNode(vx_graph graph, vx_image input, vx_image output);

    /*! \brief [Graph] Creates a Separable Correlation Filter node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [out] output The output image.
    * \param [in] kernelHor filter's horizontal component.
    * \param [in] kernelVer filter's vertical component.
    * \param [in] ksize filter size.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVCorrSepNode(vx_graph graph, vx_image input, vx_image output, vx_array kernelHor, vx_array kernelVer, vx_scalar ksize);

    /*! \brief [Graph] Creates a Sigma Filter node.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image.
    * \param [in] threshold Threshold value for sigma operation.
    * \param [out] output The output image.
    * \param [in] ksize Sigma filter size.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    * \ingroup group_vision_function_cevacv
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVSigmaNode(vx_graph graph, vx_image input, vx_scalar threshold, vx_image output, vx_scalar ksize);

    /*! \brief [Graph] Creates a Scale Up By 2 Node.
    * \param [in] graph The reference to the graph.
    * \param [in] src The source image of type <tt>\ref VX_DF_IMAGE_U8</tt>.
    * \param [out] dst The destination image of type <tt>\ref VX_DF_IMAGE_U8</tt>.
    * \param [in] type The interpolation type to use. \see vx_interpolation_type_e.
    * \ingroup group_vision_function_scale_image
    * \note The destination image must have a defined size and format. Only
    *  <tt>\ref VX_NODE_ATTRIBUTE_BORDER_MODE</tt> value <tt>\ref VX_BORDER_MODE_UNDEFINED</tt>,
    *  <tt>\ref VX_BORDER_MODE_REPLICATE</tt> or <tt>\ref VX_BORDER_MODE_CONSTANT</tt> is supported.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVScaleUpBy2Node(vx_graph graph, vx_image src, vx_image dst, vx_enum type);

    /*! \brief [Graph] Creates a Scale Down By 2 Node.
    * \param [in] graph The reference to the graph.
    * \param [in] src The source image of type <tt>\ref VX_DF_IMAGE_U8</tt>.
    * \param [out] dst The destination image of type <tt>\ref VX_DF_IMAGE_U8</tt>.
    * \param [in] type The interpolation type to use. \see vx_interpolation_type_e.
    * \ingroup group_vision_function_scale_image
    * \note The destination image must have a defined size and format. Only
    *  <tt>\ref VX_NODE_ATTRIBUTE_BORDER_MODE</tt> value <tt>\ref VX_BORDER_MODE_UNDEFINED</tt>,
    *  <tt>\ref VX_BORDER_MODE_REPLICATE</tt> or <tt>\ref VX_BORDER_MODE_CONSTANT</tt> is supported.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVScaleDownBy2Node(vx_graph graph, vx_image src, vx_image dst, vx_enum type);


    /*! \brief [Graph] Creates a Scale Down By 2 Node.
    * \param [in] graph The reference to the graph.
    * \param [in] src The source image of type <tt>\ref VX_DF_IMAGE_U8</tt>.
    * \param [out] dst The destination image of type <tt>\ref VX_DF_IMAGE_U8</tt>.
    * \param [in] type The interpolation type to use. \see vx_interpolation_type_e.
    * \ingroup group_vision_function_scale_image
    * \note The destination image must have a defined size and format. Only
    *  <tt>\ref VX_NODE_ATTRIBUTE_BORDER_MODE</tt> value <tt>\ref VX_BORDER_MODE_UNDEFINED</tt>,
    *  <tt>\ref VX_BORDER_MODE_REPLICATE</tt> or <tt>\ref VX_BORDER_MODE_CONSTANT</tt> is supported.
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVScaleDownBy4Node(vx_graph graph, vx_image src, vx_image dst, vx_enum type);

    /*! \brief [Graph] Creates a node for a Laplacian Pyramid.
    * \param [in] graph The reference to the graph.
    * \param [in] input The input image in <tt>\ref VX_DF_IMAGE_U8</tt> format.
    * \param [out] Laplacian pyramid
    * \param [out] lowest resolution to construct <tt>\ref VX_DF_IMAGE_U8</tt>.
    * \ingroup group_vision_function_laplacian_pyramid
    * \see group_pyramid
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVLaplacianPyramidNode(vx_graph graph, vx_image input, vx_pyramid laplacian, vx_image output);

    /*! \brief [Graph] Creates a node for Combining images into a pyramid.
    * \param [in] graph The reference to the graph.
    * \param [in] level0 Input image <tt>\ref VX_DF_IMAGE_U8</tt> format.
    * \param [in] level1 Input image <tt>\ref VX_DF_IMAGE_U8</tt> format.
    * \param [in] level2 Input image (optional) <tt>\ref VX_DF_IMAGE_U8</tt> format.
    * \param [in] level3 Input image (optional) <tt>\ref VX_DF_IMAGE_U8</tt> format.
    * \param [in] level4 Input image (optional) <tt>\ref VX_DF_IMAGE_U8</tt> format.
    * \param [in] level5 Input image (optional) <tt>\ref VX_DF_IMAGE_U8</tt> format.
    * \param [in] level6 Input image (optional) <tt>\ref VX_DF_IMAGE_U8</tt> format.
    * \param [in] level7 Input image (optional) <tt>\ref VX_DF_IMAGE_U8</tt> format.
    * \param [out] output Output pyramid <tt>\ref VX_DF_IMAGE_U8</tt>.
    * \ingroup group_vision_function_laplacian_pyramid
    * \see group_pyramid
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVCombinePyramidLevelsNode(vx_graph graph, vx_image level0, vx_image level1, vx_image level2, vx_image level3, vx_image level4, vx_image level5, vx_image level6, vx_image level7, vx_pyramid output);

    /*! \brief [Graph] Creates a node for Extracting an image from a pyramid.
    * \param [in] graph The reference to the graph.
    * \param [in] input Input pyramid <tt>\ref VX_DF_IMAGE_U8</tt> format.
    * \param [in] level Level index <tt>\ref VX_TYPE_SIZE</tt> format.
    * \param [out] output Output image <tt>\ref VX_DF_IMAGE_U8</tt>.
    * \ingroup group_vision_function_laplacian_pyramid
    * \see group_pyramid
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVExtractPyramidLevelNode(vx_graph graph, vx_pyramid input, vx_size level, vx_image output);

    /*! \brief [Graph] Creates a node for identity function
    * \param [in] src Input image <tt>\ref VX_DF_IMAGE_U8 / VX_DF_IMAGE_S16 </tt> format.
    * \param [out] dst Output image <tt>\ref VX_DF_IMAGE_U8 / VX_DF_IMAGE_S16 </tt> format.
    * \ingroup group_vision_function_cevacv
    * \return <tt>\ref vx_node</tt>.
    * \retval vx_node A node reference. Any possible errors preventing a successful creation should be checked using <tt>\ref vxGetStatus</tt>
    */
    VX_API_ENTRY vx_node VX_API_CALL vxCevaCVIdentityNode(vx_graph graph, vx_image src, vx_image dst);

    /*! \brief Implements rectangle fill kernel
    * \This function fills a rectangle with a specified value. TODO
    * \param [in] fill_value
    * \param [out] output Output image
    */
    //VX_API_ENTRY vx_node VX_API_CALL vxCevaCVFillNode(vx_graph graph, vx_scalar fill_value, vx_image output);

#ifdef __cplusplus
}
#endif

#endif // _VX_CEVA_NODES_H_
