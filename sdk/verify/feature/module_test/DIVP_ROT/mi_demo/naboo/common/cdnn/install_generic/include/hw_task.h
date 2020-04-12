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
#ifndef hw_task_h__
#define hw_task_h__

#include "cnn_cdma.h"
#include "convolution.h"

namespace CevaAcceleratorNS {
    class HwTask {
    private:
        DmaHwTask readRDmaConfig;
        DmaHwTask writeOutput;
        DmaHwTask writeTrailingOutput;
        ConvHwTask convolution;
        DmaHwTask readWDmaConfig;
        DmaHwTask readConvolutionConfig;
        DmaHwTask readInputMaps;
        DmaHwTask readBias;
        DmaHwTask readWeights;
        DmaHwTask readTrailingWeights;
        DmaHwTask dummyTask;
    public:
        DmaHwTask& getReadConfig() { return readRDmaConfig; }
        DmaHwTask& getWriteOutput() { return writeOutput; }
        DmaHwTask& getWriteTrailingOutput() { return writeTrailingOutput; }
        DmaHwTask& getReadWDmaConfig() { return readWDmaConfig; }
        DmaHwTask& getReadWConvolutionConfig() { return readConvolutionConfig; }
        DmaHwTask& getReadInputMaps() { return readInputMaps; }
        DmaHwTask& getReadWeights() { return readWeights; }
        DmaHwTask& getReadBias() { return readBias; }
        DmaHwTask& getReadTrailingWeights() { return readTrailingWeights; }        
        ConvHwTask& getConvolution() { return convolution; }
        DmaHwTask& getDummy() { return dummyTask; }
        void reset();
        uint32_t buildConfigurations();
        void* getTasks() { return &this->readRDmaConfig; }
		void rebuildConfigurations();
	};

}
#endif // hw_task_h__
