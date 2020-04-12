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
***************************************************************************************
* Author: Adi Panzer                                                                  *
* Date  : 35/05/2016                                                                  *
\**************************************************************************************/

#ifndef _CNN_SIM_EXT
#define _CNN_SIM_EXT
#ifdef XM
#include <stdint.h>
#else
#include <cinttypes>
#endif
/*************************************************************************************\
*                                                                                     *
*                          includes                                                   *
*                                                                                     *
\**************************************************************************************/
#include "General_Defines.h"
#include "HardwareConfig.h"
#include "CDMA_LIB_ex.h"
#include <queue>
#include <deque>
/*************************************************************************************\
*                                                                                     *
*                          Defines                                                    *
*                                                                                     *
\**************************************************************************************/
typedef void(*isr_handler_t)(void);
typedef struct
{
	uint data[4];
} struct128;

typedef enum 
{
	CNN_NO_ERROR =	0,
	CNN_ERROR =		-1
} CNN_ErrorEnum;

typedef enum
{
	LIM = 1,
	SIM = 0
} CNN_InputModeEnum;

typedef enum 
{
	DATA_BLOCK1,
	DATA_BLOCK2,
	WEIGHT_BLOCK1,
	WEIGHT_BLOCK2,
	BIAS_BLOCK
} CNN_BlocksEnum;

typedef enum 
{
	NO_ACTIVATION = 0,
	RELU = 1,
	PARAMETRIC_RELU = 2,
	SQUARED = 3,
	BOUNDED = 4
} CNN_RectifierEnum;

/*************************************************************************************\
*                                                                                     *
*                          structs                                                    *
*                                                                                     *
\**************************************************************************************/

//#pragma pack(push)  /* push current alignment to stack */
//#pragma pack(1)     /* set alignment to 1 byte boundary */

typedef struct
{
	uchar         ConvPostShift;                    // convolution result post shift (0..32)
	uchar         ROInum;                           // number of ROI (1..64)
	uchar         ActivationType;                   // Activation type (None, ReLU, Parametric ReLU)

	uchar         ReLuPS;                           // ReLU post shift (0..32)
	short         ReLuParam;                        // Parametric ReLU slope/bound
	

	ushort        InputMapsCnt;						// Number of input maps
	ushort        OutputMapsCnt;                    // Number of output maps

	uchar         KernelHeight;						// Kernel Height (1..31)
	uchar         KernelWidth;						// Kernel Width (1..31)

	uchar         Num5x1;                           // Number of 5x1 Filters (Multi-Filter)
	uchar         Num4x1;                           // Number of 4x1 Filters (Multi-Filter)
	uchar         Num3x1;                           // Number of 3x1 Filters (Multi-Filter)
	uchar         Num2x1;                           // Number of 2x1 Filters (Multi-Filter)
	uchar         Num1x1;                           // Number of 1x1 Filters (Multi-Filter)

	uchar         OutMapsBatch;						// Output maps/batch
	ushort        OutMapsTrigger;                   // Out Maps per DMA write
	ushort        OutMapsWeights;                   // Output maps per weights block

	uchar         HorizontalStride;					// Filter horizontal Stride (1..7)
	uchar         VerticalStride;                   // Filter vertical Stride (1..7)

	uchar         InTopPad;                         // Top padding size
	uchar         InBotPad;                         // Bottom padding size
	uchar         InLeftPad;                        // Left padding size
	uchar         InRightPad;                       // Right padding size


	uchar  ReadHostControl;							// Selects if the weights are generated by the host
	uchar  WriteHostControl;						// Selects if the output maps are written to the host

	uchar  EndOfLayerPause;							// Pause at end of layer
	uchar  EndOfLayerInterrupt;						// Interrupt at end of layer


} ConvLayerParamsSt;
//#pragma pack(pop)   /* restore original alignment from stack */
typedef struct 
{
	uint		InPtr;					// Input pointer
	uint		OutPtr;					// Output pointer
	ushort		InMapHeight;			// Input maps height
	ushort		InMapWidth;				// Input maps width
	ushort		OutMapHeight;			// Output maps height
	ushort		OutMapWidth;			// Output maps width
	uchar		ElCyStart;				// Elements/cycle (start)
	uchar		ElCyMid;				// Elements/cycle (middle)
	uchar		ElCyEnd;				// Elements/cycle (end)
	uchar		MiddleBatchCnt;			// Number of middle batches
	uchar		SmLg;					// Small/large maps mode

	uchar		Dummy1;
	ushort		Dummy2;
	int			Dummy3;
	uint		Dummy4;
} ROIConfigSt;							// Total: 32B

typedef union 
{
	ConvLayerParamsSt		ConvLayerParams;
	ROIConfigSt				ROIConfig;
} CNNConfigUn;

typedef struct  
{
	// determined by HW configuration
	uint ReadDMAAddress;
	uint WriteDMAAddress;
	uint ConfigQueueAddress;
	uint DataBlock1Address;
	//uint DataBlock2Address;
	uint WeightsBlock1Address;
	uint WeightsBlock2Address;
	uint BiasBlockAddress;
	uint ReadDMABufferSize;
	uint WriteDMABufferSize;
	uint ConfigQueueSize;
	uint DataBlockSize;
	uint WeightsBlockSize;
	uint BiasBlockSize;
	uint NumberOfMultipliers;
	bool Use8x16Multipliers;

	// determined by driver
	uint ReadDMAFifoAddress;
	uint ReadDMAPredefinedAddress;
	uint WriteDMAFifoAddress;
	uint WriteDMAPredefinedAddress;
	uint ReadDMAFifoLength;
	uint WriteDMAFifoLength;
	uint MaxReadPredefinedNumber;
	uint MaxWritePredefinedNumber;
} HardwareConfigSt;

/*************************************************************************************\
*                                                                                     *
*                          externals                                                  *
*                                                                                     *
\**************************************************************************************/


/*************************************************************************************\
*                                                                                     *
*                          functions                                                  *
*                                                                                     *
\**************************************************************************************/


class IConvHWSim
{

public:

	virtual ~IConvHWSim() {};

	/////////////////////////////////
	// CNN engine control function //
	/////////////////////////////////

	// Gets hardware configuration
	// HardwareConfigSt *config		struct holding the hardware config
	virtual int CNN_GetHardwareConfig(HardwareConfigSt *config) = 0;

	// Initializes CNN registers, add 1st DMA request to queue and starts DMA
	// uint *DMA_task				pointer to 1st DMA task
	virtual int	CNN_Init(
		uint *DMA_task,			
		uint dataMemSize,
		uint biasMemSize,
		uint weightsMemSize,
		uint tasksMemSize,
		uint multipliers,
		uint weightsElmentSize,
		uint writeDMAQueueSize,
		uint readDMAQueueSize) = 0;

	// Initializes CNN register
	virtual int	CNN_Init(
		uint dataMemSize,
		uint biasMemSize,
		uint weightsMemSize,
		uint tasksMemSize,
		uint multipliers,
		uint weightsElmentSize,
		uint writeDMAQueueSize,
		uint readDMAQueueSize) = 0;

// free memory
	virtual int	CNN_DeInit() = 0;

	// Start execution of CNN
	virtual int CNN_Process() = 0;

	// Sets interrupt service function for CNN
	// isr_handler_t isr_handler	pointer to function
	virtual int	CNN_SetIsrFunction(isr_handler_t isr_handler) = 0;

	// Returns address of internal CNN memory block
	// CNN_BLOCKS_enum block		selects the requested block
	virtual char *CNN_GetAddress(CNN_BlocksEnum block) = 0;

	// Sets configuration register of the CNN
	// uint address					register address
	// uint val						register value
	virtual int	CNN_SetRegister(uint address, uint val) = 0;

	// Reads register from the CNN
	// uint address					register address
	virtual uint CNN_ReadRegister(uint address) = 0;

	// Adds layer of ROI struct to the CNN configuration queue
	// uint len						number of elements to add
	virtual int	CNN_AddConfigMission(CNNConfigUn *buf, uint len) = 0;

	// Sets the read counter for host mode
	// int val						initial value of the counter
	virtual int CNN_SetReadHostCounter(int val) = 0;

	// Increase the read counter for host mode
	virtual int CNN_IncreaseReadHostCounter() = 0;

	// Sets the write counter for host mode
	// int val						initial value of the counter
	virtual int CNN_SetWriteHostCounter(int val) = 0;

	// Increase the write counter for host mode
	virtual int CNN_IncreaseWriteHostCounter() = 0;

	virtual void setExternalBuffer(bool isVirtualBuff, int extBuffOffset, int extMemBase, int extMemSize) = 0;

	//////////////////////
	// Read DMA control //
	//////////////////////

	// Adds DMA mission to read DMA
	// uint *buf					pointer to array of uint contains the DMA configuration
	// uint len						number of configuration words - must be multiply of 4
	virtual int		CNN_AddReadMission(uint *buf, uint len) = 0;
	virtual std::deque <struct128>* CNN_ReadDMA_GetQueue() = 0;
	// Triggers the read DMA
	virtual int		CNN_ReadDMA_Run() = 0;

	// Gets the number of bytes read by the DMA
	virtual int		CNN_ReadDMA_get_read_counter() = 0;

	// Gets the number of bytes written by the DMA
	virtual int		CNN_ReadDMA_get_write_counter() = 0;

	// Sets interrupt service function for the DMA
	// isr_handler_t handle			pointer to function
	virtual int		CNN_ReadDMA_SetISR(isr_handler_t handle) = 0;

	// Sets predefined request
	// DTC_REGISTER_MM3K reg		struct holding the DMA parameters
	// uint num						predefined number to be set
	virtual int		CNN_ReadDMA_set_predefined(DTC_REGISTER_MM3K reg, uint num) = 0;

	// Sets configuration register for the DMA
	// uint address					register address
	// uint val						register value
	virtual int		CNN_ReadDMA_SetRegister(uint address, uint val) = 0;

	// Reads register from the DMA
	// uint address					register address
	virtual uint	CNN_ReadDMA_ReadRegister(uint address) = 0;

	///////////////////////
	// Write DMA control //
	///////////////////////

	// same functionality as read DMA functions
	virtual	void	CNN_Reset() = 0;   // pop dummy tasks from head of the queue (simulator !=HW)
	virtual int		CNN_AddWriteMission(uint *buf, uint len) = 0;
	virtual int		CNN_WriteDMA_Run() = 0;
	virtual int		CNN_WriteDMA_get_read_counter() = 0;
	virtual int		CNN_WriteDMA_get_write_counter() = 0;
	virtual int		CNN_WriteDMA_SetISR(isr_handler_t handle) = 0;
	virtual int		CNN_WriteDMA_set_predefined(DTC_REGISTER_MM3K reg, uint num) = 0;
	virtual int		CNN_WriteDMA_SetRegister(uint address, uint val) = 0;
	virtual uint	CNN_WriteDMA_ReadRegister(uint address) = 0;
	virtual std::deque <struct128>* CNN_WriteDMA_GetQueue() = 0;
	static IConvHWSim* createSimulator();
	static void destroySimulator(IConvHWSim* pSim);
	

	virtual void	 CNN_ResetCounter() = 0;
	virtual uint64_t  CNN_GetCounter() = 0;

};


#endif	// _CNN_SIM_EXT
