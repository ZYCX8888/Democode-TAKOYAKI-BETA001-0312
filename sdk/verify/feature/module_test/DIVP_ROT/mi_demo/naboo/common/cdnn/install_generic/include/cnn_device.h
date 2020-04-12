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
#ifndef _I_CEVA_CNN_DEVICE_
#define _I_CEVA_CNN_DEVICE_

#include <vector>
#include <cassert>

#include "ceva_device.h"
#include "hw_task.h"
#include "cnn_tasks.h"

namespace CevaAcceleratorNS {

    class ICnnDevice : public CevaPlatform::IDevice {
    private:
    public:
        enum TaskStatus {
            OK,
            REGISTRY_CORRUPTED,
            ERROR_CONFIGURATION_MISMATCH,
            FAILED_TO_CREATE,
            FAILED_TO_INSERT,
            FAILED_TO_ANALYZE,
            NOT_ENOUGH_MEMORY,
            FAILED_TO_REORDER_WEIGHTS,
            FAILED_TO_BUILD_HW_TASKS,
            HW_NOT_READY,
            TASK_NOT_COMPLETED,
			INVALID_PARAMETERS,
			FAILED_TO_EXECUTE
        };

        class MissionAttributes // size: 44 Bytes
        {
        public:
            uint16_t getInputTileWidth() const { return inputTileWidth; }
            uint16_t getInputTileHeight() const { return inputTileHeight; }
            uint64_t getInputPtr() const { return inputPtr; }
            uint64_t getOutputPtr() const { return outputPtr; }
            uint16_t getOutputTileWidth() const { return outputTileWidth; }
            uint16_t getOutputTileHeight() const { return outputTileHeight; }
            uint16_t getOutputStrideX() const { return outputStrideX; }
            uint16_t getOutputStrideZ() const { return outputStrideZ; }
            Mission::TileDims getOutputTileDims() const { return outputTileDims; }
            Mission::TileLocation getOutputTileLocations() const { return outputTileLocations; }
            uint16_t getOutputMapsCnt() const { return outputMapsCnt; }			
        private:
            //data
            uint64_t inputPtr;                          //8 bytes
            uint64_t outputPtr;                         //8 bytes
            uint32_t inputMapsOffset;                   //4 bytes            
            uint32_t outputTrailingMapsOffset;          //4 bytes
            uint32_t outputStrideZ;                     //4 bytes
            Mission::TileDims outputTileDims;           //6 bytes
            uint16_t outputStrideX;                     //2 bytes
            Mission::TileLocation outputTileLocations;  //6 bytes
            uint16_t inputTileWidth;                    //2 bytes
            uint16_t inputTileHeight;                   //2 bytes
            uint16_t outputTileWidth;                   //2 bytes
            uint16_t outputTileHeight;                  //2 bytes
            uint16_t outputMapsCnt;                     //2 bytes
			uint32_t outputOffsetBetweenMaps;			//4 bytes
			uint32_t outputOffsetInsideMap;				//4 bytes
			uint8_t dummy[4];
			void setParams(const Task& task, const Mission& mission);
            friend class CnnDeviceImpl;
        };

        class TaskAttributes // size: 48 Bytes
        {
        public:
            uint16_t getNumOfTilesX() const { return numOfTilesX; }
            uint16_t getNumOfTilesY() const { return numOfTilesY; }
            uint16_t getNumOfTiles() const { return numOfTiles; }
            uint16_t getNumOfOutputMaps() const { return numOfOutputMaps; }
            Task::TilePartition getTilePartition() const { return tilePartition; }
            RunningMode getRunningMode() const { return runningMode; }            
            uint16_t getNoOfMissions() const { return noOfMissions; }
            uint32_t getBiasDataPtr() const { return biasDataPtr; }
            void setBiasDataPtr(uint32_t val) { biasDataPtr = val; }
			uint16_t getOriginInputStrideX() const { return originInputStrideX; }
			uint16_t getOriginInputStrideZ() const { return originInputStrideZ; }
			uint16_t getOriginOutputStrideX() const { return originOutputStrideX; }
			uint32_t getOriginOutputStrideZ() const { return originOutputStrideZ; }
        private:
            Task::TilePartition tilePartition;      //4 bytes
            RunningMode runningMode;                //4 bytes
            uint16_t numOfOutputMaps;               //2 bytes
            uint16_t numOfTilesX;                   //2 bytes
            uint16_t numOfTilesY;                   //2 bytes
            uint16_t numOfTiles;                    //2 bytes
            uint16_t noOfMissions;                  //2 bytes
            uint16_t nextMissionId; 	            //2 bytes
            uint32_t biasDataPtr;                   //4 bytes
			uint16_t originInputStrideX;			//2 bytes
			uint32_t originInputStrideZ;			//4 bytes
			uint16_t originOutputStrideX;			//2 bytes
			uint32_t originOutputStrideZ;			//4 bytes
			uint8_t dummy[12];
            void setParams(const Task& task, const Mission& mission)
            {
                tilePartition = task.getTilePartition();
                runningMode = task.data.runningMode;
                numOfTilesX = task.getNoOfTilesX();
                numOfTilesY = task.getNoOfTilesY();
                numOfTiles = task.getNoOfTiles();
                numOfOutputMaps = task.data.outputChannels;
                noOfMissions = (uint16_t)task.getNoOfMissions();
                nextMissionId = 0;
                biasDataPtr = task.offsetToBias;
				originInputStrideX = task.data.inputStrideX;
				originInputStrideZ = task.data.inputStrideZ;
				originOutputStrideX = task.data.outputStrideX;
				originOutputStrideZ = task.data.outputStrideZ;
            }
            friend class CnnDeviceImpl;
        };

        //ICnnDevice();
        ICnnDevice(const uint32_t, const uint32_t, const uint32_t = 0);
		ICnnDevice(void* pSim);
        CevaPlatform::IDevice::Status initDeviceRuntime();
        CevaPlatform::IDevice::Status initDeviceGenerator(uint32_t dataMemSize,
            uint32_t biasMemSize,
            uint32_t weightsMemSize,
            uint32_t tasksMemSize,
            uint32_t multipliers,
            uint32_t weightsElmentSize,
            uint32_t writeDMAQueueSize,
            uint32_t readDMAQueueSize,
            uint32_t devId = 1
            );

        CevaPlatform::IDevice::Status initTaskList(void* mem, uint32_t size, uint32_t noOfTasks, NetworkRoiMode roi);
        void resetTaskList();
        Task& createTask(TaskInitStruct* taskInit);
        TaskStatus analyzeTask(Task& task);
        //TaskStatus updateTask(Task& task);
        TaskStatus insertTask(Task& task, uint32_t& taskId);
        TaskStatus buildHwTasks();
        TaskStatus setHwTasksPtr(void* mem);
        TaskStatus updateTaskBuffers(uint32_t taskId, const uint32_t& source, const uint32_t& inputStrideX, const uint32_t& inputStrideZ, const uint32_t& destination, const uint32_t& outputStrideX, const uint32_t& outputStrideZ);
//        TaskStatus updateTaskBuffers(uint32_t taskId, const uint32_t& source, const uint32_t* destination, uint32_t destinationsSize);
        TaskStatus executeTask(uint32_t, TaskAttributes*&, MissionAttributes*&);
        TaskStatus waitTask(uint32_t taskId);
        uint32_t getDataMemSize() const;
        uint32_t getNoOfMultipliers() const;
        uint32_t getWeightsElementSize() const;
        uint32_t getNoOfTasks();
        uint32_t getBinaryStructureSize();
        uint64_t getCyclesCounter() const;
        uint64_t getWaitsCyclesCounter() const;
        TaskStatus resetCyclesCounter();
		TaskStatus updateConvolutionPostShift(uint32_t taskId, uint32_t psVal);
        TaskStatus getTaskBiasPtr(uint32_t taskId, uint32_t*& pBias);
	};

    class Registry {
    public:
        enum Signature {
            NOT_INITIALIZED = 0x0,
            INITIALIZED     = 0x900d900d,
            VALID           = 0x5aa5a55a
        };
        class Header {
        public:
            uint32_t driverVersion;
            uint32_t hwVersion;
            NetworkRoiMode roiMode;
            uint32_t noOfTasks;
            WeightsPrecision weightsPrecision;
            struct HwConfig{
                uint32_t multipliers;
                uint32_t dataMemSize;
                uint32_t biasMemSize;
                uint32_t taskMemSize;
                uint32_t weightsMemSize;
                uint32_t weightsElmentSize;
                uint32_t writeDMAQueueSize;
                uint32_t readDMAQueueSize;
            } hwConfig;
        } header;
        uint32_t allocatedSize;
        uint32_t totalSize;
        uint32_t isVerified;
        Signature signature;
        uint32_t offsetTask0;
    public:
        /*inline */HwTask* const getHwTask(uint32_t taskId, uint32_t missionId) const;
        /*inline */ICnnDevice::TaskAttributes& getTaskAttributes(uint32_t taskId) const;
        ICnnDevice::MissionAttributes& getMissionAttributes(uint32_t taskId, uint32_t missionId) const;
        ICnnDevice::TaskStatus swapOffsetsToPtr(int32_t addrTrans = 0);
    };

}

#endif // !_I_CEVA_CNN_DEVICE_
