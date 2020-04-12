/*************************************************************************************\
*                                                                                     *
* Copyright (C) CEVA Inc. All rights reserved                                         *
*                                                                                     *
*                                                                                     *
* THIS PRODUCT OR SOFTWARE IS MADE AVAILABLE EXCLUSIVELY TO LICENSEES THAT HAVE       *
* RECEIVED EXPRESS WRITTEN AUTHORIZATION FROM CEVA TO DOWNLOAD OR RECEIVE THE         *
* PRODUCT OR SOFTWARE AND HAVE AGREED TO THE END USER LICENSE AGREEMENT (EULA).       *
* IF YOU HAVE NOT RECEIVED SUCH EXPRESS AUTHORIZATION AND AGREED TO THE               *
* CEVA EULA, YOU MAY NOT DOWNLOAD, INSTALL OR USE THIS PRODUCT OR SOFTWARE.           *
*                                                                                     *
* The information contained in this document is subject to change without notice and  *
* does not represent a commitment on any part of CEVA®, Inc. CEVA®, Inc. and its      *
* subsidiaries make no warranty of any kind with regard to this material, including,  *
* but not limited to implied warranties of merchantability and fitness for a          *
* particular purpose whether arising out of law, custom, conduct or otherwise.        *
*                                                                                     *
* While the information contained herein is assumed to be accurate, CEVA®, Inc.       *
* assumes no responsibility for any errors or omissions contained herein, and         *
* assumes no liability for special, direct, indirect or consequential damage,         *
* losses, costs, charges, claims, demands, fees or expenses, of any nature or kind,   *
* which are incurred in connection with the furnishing, performance or use of this    *
* material.                                                                           *
*                                                                                     *
* This document contains proprietary information, which is protected by U.S. and      *
* international copyright laws. All rights reserved. No part of this document may be  *
* reproduced, photocopied, or translated into another language without the prior      *
* written consent of CEVA®, Inc.                                                      *
*                                                                                     *
\**************************************************************************************/
#ifndef convolution_h__
#define convolution_h__
#ifdef XM
#include <inttypes.h>
#else
#include <cinttypes>
#endif
namespace CevaAcceleratorNS {

    typedef struct
    {
        uint8_t         ConvPostShift;                    // convolution result post shift (0..32)
        uint8_t         ROInum;                           // number of ROI (1..64)
        uint8_t         ActivationType;                   // Activation type (None, ReLU, Parametric ReLU)

        uint8_t         ReLuPS;                           // ReLU post shift (0..32)
        int16_t         ReLuParam;                        // Parametric ReLU slope/bound


        uint16_t        InputMapsCnt;                     // Number of input maps
        uint16_t        OutputMapsCnt;                    // Number of output maps

        uint8_t         KernelHeight;                     // Kernel Height (1..31)
        uint8_t         KernelWidth;                      // Kernel Width (1..31)

        uint8_t         Num5x1;                           // Number of 5x1 Filters (Multi-Filter)
        uint8_t         Num4x1;                           // Number of 4x1 Filters (Multi-Filter)
        uint8_t         Num3x1;                           // Number of 3x1 Filters (Multi-Filter)
        uint8_t         Num2x1;                           // Number of 2x1 Filters (Multi-Filter)
        uint8_t         Num1x1;                           // Number of 1x1 Filters (Multi-Filter)

        uint8_t         OutMapsBatch;                     // Output maps/batch
        uint16_t        OutMapsTrigger;                   // Out Maps per DMA write
        uint16_t        OutMapsWeights;                   // Output maps per weights block

        uint8_t         HorizontalStride;                 // Filter horizontal Stride (1..7)
        uint8_t         VerticalStride;                   // Filter vertical Stride (1..7)

        uint8_t         InTopPad;                         // Top padding size
        uint8_t         InBotPad;                         // Bottom padding size
        uint8_t         InLeftPad;                        // Left padding size
        uint8_t         InRightPad;                       // Right padding size


        uint8_t  ReadHostControl;                         // Selects if the weights are generated by the host
        uint8_t  WriteHostControl;                        // Selects if the output maps are written to the host

        uint8_t  EndOfLayerPause;                         // Pause at end of layer
        uint8_t  EndOfLayerInterrupt;                     // Interrupt at end of layer


    } ConvLayerParamsSt;

    typedef struct
    {
        uint32_t        InPtr;                  // Input pointer
        uint32_t        OutPtr;                 // Output pointer
        uint16_t        InMapHeight;            // Input maps height
        uint16_t        InMapWidth;             // Input maps width
        uint16_t        OutMapHeight;           // Output maps height
        uint16_t        OutMapWidth;            // Output maps width
        uint8_t         ElCyStart;              // Elements/cycle (start)
        uint8_t         ElCyMid;                // Elements/cycle (middle)
        uint8_t         ElCyEnd;                // Elements/cycle (end)
        uint8_t         MiddleBatchCnt;         // Number of middle batches
        uint8_t         SmLg;                   // Small/large maps mode
        uint8_t         Dummy[11];
    } ROIConfigSt;                              // Total: 32B

    typedef union
    {
        ConvLayerParamsSt       ConvLayerParams;
        ROIConfigSt             ROIConfig;
    } CNNConfigUn;

    class ConvHwTask
    {
    private:
        //CNNConfigUn params;
        ConvLayerParamsSt       convolutionParams;
        ROIConfigSt             roiConfig;
    public:
        void reset();
        void setConvolutionRightPostShift(uint32_t);
        void setActivationType(uint32_t);
        void setTotalInputMaps(uint32_t);
        void setTotalOutputMaps(uint32_t);
        void setFilterWidth(uint32_t);
        void setFilterHeight(uint32_t);
        void setBatchOutputMapsCount(uint32_t);
        void setWriteDmaOutputMapsCount(uint32_t);
        void setWeightsOutputMapsCount(uint32_t);
        void setFilterHorizontalStride(uint32_t);
        void setFilterVerticalStride(uint32_t);
        void setInputMapTopPad(uint32_t);
        void setInputMapButtomPad(uint32_t);
        void setInputMapLeftPad(uint32_t);
        void setInputMapRightPad(uint32_t);
        void setStopEndOfLayer();
        void setInterruptEndOfLayer();
        void setInputMapsAddress(uint32_t);
        void setOutputMapsAddress(uint32_t);
        void setInputMapsWidth(uint32_t);
        void setInputMapsHeight(uint32_t);
        void setOutputMapsWidth(uint32_t);
        void setOutputMapsHeight(uint32_t);
        void setElementsStartCount(uint32_t);
        void setElementsMiddleCount(uint32_t);
        void setElementsEndCount(uint32_t);
        void setMiddleBatchCount(uint32_t);
        void setMapSize(uint32_t);
        uint16_t getInputMapsCount() { return convolutionParams.InputMapsCnt; }
        uint16_t getInputMapsWidth() { return roiConfig.InMapWidth; }
        uint16_t getInputMapsHeight() { return roiConfig.InMapHeight; }
        uint16_t getOutputMapsCount() { return convolutionParams.OutputMapsCnt; }
        uint16_t getOutputMapsWidth() { return roiConfig.OutMapWidth; }
        uint16_t getOutputMapsHeight() { return roiConfig.OutMapHeight; }
        uint16_t getKernelWidth() { return convolutionParams.KernelWidth; }
        uint16_t getKernelHeight() { return convolutionParams.KernelHeight; }
    };

}
#endif // convolution_h__
