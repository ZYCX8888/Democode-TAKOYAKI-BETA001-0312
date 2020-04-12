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
#ifndef cnn_device_impl_h__
#define cnn_device_impl_h__

#include "cnn_device.h"

namespace CevaAcceleratorNS {

    using namespace CevaPlatform;


    class CnnDeviceImpl : public DeviceImpl {
    public:
        //CnnDeviceImpl() : isInitialized(false), currntTasksNo(0){ setDecription("CNN Device - Not initialized\n"); }
        CnnDeviceImpl(const uint32_t ioBaseAddr, const uint32_t devBaseAddr, const uint32_t hostBaseAddr = 0);
#ifdef CNN_SIMULATION
        CnnDeviceImpl(void* pSim);
#endif //CNN_SIMULATION

        ~CnnDeviceImpl()
        {
            for (tasksIter tIter = tasks.begin(); tIter != tasks.end(); tIter++)
            {
                Task* currentTask = *tIter;
                delete currentTask;
            }
            tasks.clear();
        }

        const Version& getDriverVersion() const;
        const Version& getHwVersion() const;
        IDevice::Status initDeviceRuntime();
        IDevice::Status initDeviceGenerator(uint32_t dataMemSize,
            uint32_t biasMemSize,
            uint32_t weightsMemSize,
            uint32_t tasksMemSize,
            uint32_t multipliers,
            uint32_t weightsElmentSize,
            uint32_t writeDMAQueueSize,
            uint32_t readDMAQueueSize,
            uint32_t devId
            );
        IDevice::Status queryHWConfiguration();
        Task& createTask(TaskInitStruct* taskInit) const;

        template <typename T>
        ICnnDevice::TaskStatus buildHWTasks();
		ICnnDevice::Status reCalcOffsets(ICnnDevice::TaskAttributes& taskAttr, ICnnDevice::MissionAttributes& missAttr, HwTask& ht, 
			const uint32_t& inputStrideX, const uint32_t& inputStrideZ, const uint32_t& outputStrideX, const uint32_t& outputStrideZ);
		ICnnDevice::TaskStatus insertTask(Task& task, uint32_t& taskId);
        ICnnDevice::TaskStatus analyzeTask(Task& task);
        ICnnDevice::TaskStatus updateTask(const Task& task);
        IDevice::Status initTaskList(void* mem, uint32_t size, uint32_t noOfTasks, NetworkRoiMode roi);
		bool areStridesChanged(ICnnDevice::TaskAttributes& taskAttr, const uint32_t& inputStrideX, const uint32_t& inputStrideZ, const uint32_t& outputStrideX, const uint32_t& outputStrideZ);
		ICnnDevice::TaskStatus updateTaskBuffers(uint32_t taskId, const uint32_t& source, const uint32_t& inputStrideX, const uint32_t& inputStrideZ, const uint32_t& destination, const uint32_t& outputStrideX, const uint32_t& outputStrideZ);
        ICnnDevice::TaskStatus updateTaskBuffers(uint32_t taskId, const uint32_t& source, const uint32_t* destination, uint32_t destinationsSize);
        ICnnDevice::TaskStatus executeTask(uint32_t taskId, ICnnDevice::TaskAttributes*&, ICnnDevice::MissionAttributes*&);
        void resetTaskList();
        void createDescriptors(HwTask& ht, const Task& task, const Mission& mission, uint32_t weightPrecision);
        uint64_t getCyclesCounter() const;
        uint64_t getWaitsCyclesCounter() const;
        ICnnDevice::TaskStatus resetCyclesCounter();
        uint32_t getDataMemSize() const { return (isInitialized) ? dataMemSize : (uint32_t)-1; }
        uint32_t getNoOfMultipliers() const { return (isInitialized) ? multipliers : (uint32_t)-1; }
        uint32_t getWeightsElementSize() const { return (isInitialized) ? weightsElmentSize : (uint32_t)-1; }
        ICnnDevice::TaskStatus setHwTasksPtr(void* mem);
        uint32_t getNoOfTasks();
        uint32_t getBinaryStructureSize();
		ICnnDevice::TaskStatus updateConvolutionPostShift(uint32_t taskId, uint32_t psVal);
        ICnnDevice::TaskStatus waitTask(uint32_t taskId);
        ICnnDevice::TaskStatus getTaskBiasPtr(uint32_t taskId, uint32_t*& pBias);


    protected:
        int ElementsCalculationBruteForce(Task& task, Elements* elements);
        ICnnDevice::TaskStatus verifyHwConfiguration();
        uint32_t createHWTask(const Task& task, const Mission& mission, void* mem, uint32_t offset);
        void modifyHwTasksToSequentialRun(const Task& task);
		template <typename T>
		ICnnDevice::TaskStatus CopyWeightsAndBiases(uint8_t* mem, uint32_t* offsetToHWTasks);
		ICnnDevice::TaskStatus constructHWTasks(uint8_t* mem, uint32_t* totalOffsetFromStartMemory);
    protected:
//         AddressRange ioRange;
//         AddressRange dataRange;
//         AddressRange biasRange;
//         AddressRange taskRange;
        Version HWVersion;
        Version driverVersion;
        uint32_t multipliers;
        uint32_t dataMemSize;
        uint32_t biasMemSize;
        uint32_t tasksMemSize;
        uint32_t weightsMemSize;
        uint32_t weightsElmentSize;			//1: 8 bits, 0: 16 bits
        uint32_t writeDMAQueueSize;
        uint32_t readDMAQueueSize;
        uint32_t devId;
        Registry* registry;
        bool isInitialized;
        uint32_t currntTasksNo;
        uint32_t convTaskCounter;
        uint64_t cyclesCounter;
        uint64_t waitsCyclesCounter;
        typedef std::vector<Task*> tasksContainer;
        typedef tasksContainer::iterator tasksIter;
        tasksContainer tasks;

        enum RegsOffset {
            CNN_CTL_OFFSET = 0x00,
            CNN_STA_OFFSET = 0x04,
            CNN_ICU_OFFSET = 0X08,
            CNN_ICLR_OFFSET = 0x0C,
            CNN_TASK_OFFSET = 0x20,
            CNN_TASK_Q_TRACK_OFFSET = 0x24,
            CNN_PC1_OFFSET = 0x28,
            CNN_PC2_OFFSET = 0x2C,
            CNN_PC3_OFFSET = 0x30,
            CNN_PC4_OFFSET = 0x34,
            CNN_PC5_OFFSET = 0x38,
            CNN_PC6_OFFSET = 0x3C,
            CNN_CIMCN_OFFSET = 0x40,
            CNN_COMCN_OFFSET = 0x44,
            CNN_WD_TH_OFFSET = 0x48,
            CNN_VER_OFFSET = 0x4C
        };

        enum AddressMap {
            CNN_DATA_MEM_ADDR = 0x0,
            CNN_WEIGHTS_MEM_ADDR = 0x200000,
            CNN_BIAS_MEM_ADDR = 0x250000,
            CNN_TASK_MEM_ADDR = 0x240000,
            CNN_CDMAW_MEM_ADDR = 0x260000,
            CNN_CDMAR_MEM_ADDR = 0x264000
        };
		void* mpSim;		
    };
}
#endif // cnn_device_impl_h__
