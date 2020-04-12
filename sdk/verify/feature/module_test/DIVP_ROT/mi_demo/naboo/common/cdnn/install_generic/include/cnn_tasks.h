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
#ifndef cnn_tasks_h__
#define cnn_tasks_h__

//#include "cnn_device.h"
#include <list>
#ifdef XM
#include <inttypes.h>
#else
#include <cinttypes>
#endif
namespace CevaAcceleratorNS {
    class Registry;
    class Task;

    enum Activation {
        NONE,
        RELU
    };
    enum WeightsOrder {
        NCHW,
        NCWH,
        NHWC,
        NWHC,
        CNHW,
        CNWH
    };
    enum WeightsPrecision{
        WEIGHT16BIT,
        WEIGHT8BIT
    };

    enum NetworkRoiMode {
        FIXED,
        VARIABLE
    };

    enum SyncMissionOption {
        LAST,
        EACH_MISSION
    };

    enum InputMapScale
    {
        LARGEINPUTMAP = 1,
        SMALLINPUTMAP = 0
    };

    enum TileType
    {
        MIDDLE = 0,
        TOP = 1,
        BOTTOM = 2,
        LEFT = 4,
        RIGHT = 8,
        WHOLE_MAP = 16
    };

    enum RunningMode
    {
        SEQUENTIAL = 0,
        ONE_BY_ONE = 1
    };


    struct TaskInitStruct {
        uint32_t filterWidth;
        uint32_t filterHeight;
        uint8_t	Num5x1;					// Number of 5x1 Filters (Multi-Filter)
        uint8_t	Num4x1;					// Number of 4x1 Filters (Multi-Filter)
        uint8_t	Num3x1;					// Number of 3x1 Filters (Multi-Filter)
        uint8_t	Num2x1;					// Number of 2x1 Filters (Multi-Filter)
        uint8_t	Num1x1;					// Number of 1x1 Filters (Multi-Filter)
        uint32_t maxInputWidth;
        uint32_t maxInputHeight;
        uint32_t inputStrideX;
        uint32_t inputStrideZ;
        uint32_t inputChannels;
        uint32_t stepSizeX;
        uint32_t stepSizeY;
        uint32_t padLeft;
        uint32_t padRight;
        uint32_t padTop;
        uint32_t padBottom;
        uint32_t outputWidth;
        uint32_t outputHeight;
        uint32_t outputStrideX;
        uint32_t outputStrideZ;
        uint32_t outputChannels;
        uint32_t outputPostShift;
        Activation activation;
        uint32_t activationPostShift;
        WeightsOrder weightsOrder;
        WeightsPrecision weightsPrecision;
        uint32_t isVirtualInput;  //No read DMA
        uint32_t isVirtualOutput; //No write DMA
        RunningMode runningMode;
        void* weightsPtr;
        void* biasPtr;
    };

    struct Elements {
        uint32_t ElCyStart;				// Elements/cycle (start)
        uint32_t ElCyMid;				// Elements/cycle (middle)
        uint32_t ElCyEnd;				// Elements/cycle (end)
        uint32_t MiddleBatchCnt;		// Number of middle batches
    };

    class Mission {
    public:
        struct TileDims
        {
            uint16_t dimX;
            uint16_t dimY;
            uint16_t dimZ;
        };

        struct TileLocation
        {
            uint16_t x;
            uint16_t y;
            uint16_t z;
        };

		Mission() : padLeft(0), padRight(0), padTop(0), padBottom(0), noOfOutputMapsInWBlock(0), lastMission(false), inputMapsNeeded(false), trailing(false), trailingAlone(false)
		{
			offsetToInput = 0;
			offsetToOutput = 0;
			offsetToWeights = 0;
			offsetToBias = 0;
			startOutMap = 0;
			trailingWeightsMaps = 0;
			trailingOutputMaps = 0;
			parent = NULL;
			batchSize = 0;
			ElCyEnd = 0;
			ElCyMid = 0;
			ElCyStart = 0;
			MiddleBatchCnt = 0;
			outputMapsCount = 0;
			missionDataMemSize = 0;
			startOutMap = 0;
			tileOutputHeight = 0;
			tileOutputWidth = 0;
			tileInputHeight = 0;
			tileInputWidth = 0;
			tileType = WHOLE_MAP;
			trailingOutputMaps = 0;
			trailingWeightsMaps = 0;
			startOutLine = 0;
			noOfWBlockRepeat = 0;

		};
        ~Mission() { }
        uint32_t getMissionDataMemSize() const { return missionDataMemSize; }
        void setMissionDataMemSize(uint32_t val) { missionDataMemSize = val; }
        void setElements(Elements* elements);
        void setPadding(Task& task, uint32_t tileType);
        uint32_t getElCyStart() const { return ElCyStart; }
        uint32_t getElCyMid() const { return ElCyMid; }
        uint32_t getElCyEnd() const { return ElCyEnd; }
        uint32_t getMiddleBatchCnt() const { return MiddleBatchCnt; }
        uint32_t getPadTop() const { return padTop; }
        uint32_t getPadLeft() const { return padLeft; }
        uint32_t getPadRight() const { return padRight; }
        uint32_t getPadButtom() const { return padBottom; }
        void setOutputTileLoc(uint32_t x, uint32_t y, uint32_t z) { tileLoc.x = x; tileLoc.y = y; tileLoc.z = z; }
        uint32_t getTileInputWidth() const { return tileInputWidth; }
        void setTileInputWidth(uint32_t val) { tileInputWidth = val; }
        uint32_t getTileInputHeight() const { return tileInputHeight; }
        void setTileInputHeight(uint32_t val) { tileInputHeight = val; }
        uint32_t getTileOutputWidth() const { return tileOutputWidth; }
        void setTileOutputWidth(uint32_t val) { tileOutputWidth = val; }
        uint32_t getTileOutputHeight() const { return tileOutputHeight; }
        void setTileOutputHeight(uint32_t val) { tileOutputHeight = val; }
        uint32_t getBatchSize() const { return batchSize; }
        void setBatchSize(uint32_t val) { batchSize = val; }
        uint32_t getOutputMapsCount() const { return outputMapsCount; }
        void setOutputMapsCount(uint32_t val) { outputMapsCount = val; }
        bool isLastMission() const { return lastMission; }
        void setLastMission() { lastMission = true; }
        bool isInputMapsNeeded() const { return inputMapsNeeded; }
        void setInputMapsNeeded(bool val) { inputMapsNeeded = val; }
		bool isFirstSpatialTile() const { return (tileLoc.y == 0); }
        Mission::TileLocation getOutputTileLoc() const { return tileLoc; }
        void setNoOfOutputMapsInWBlock(uint32_t val) { noOfOutputMapsInWBlock = val; }
        uint32_t getNoOfOutputMapsInWeightsBlock() const { return noOfOutputMapsInWBlock; }
        TileDims getOutputTileDims() const
        {
            TileDims tileDims;
            tileDims.dimX = getTileOutputWidth();
            tileDims.dimY = getTileOutputHeight();
            tileDims.dimZ = getOutputMapsCount();
            return tileDims;
        }
        bool hasTrailing() const { return trailing; }
		bool isTrailingAlone() const { return trailingAlone; }
        
		uint32_t getTileType() const { return tileType; }
        void setTileType(uint32_t val) { tileType = val; }
        

        uint32_t getTrailingOutputMaps() const { return trailingOutputMaps; }
        void setTrailingOutputMaps(uint32_t val) { trailingOutputMaps = val; }

        uint32_t getOffsetToWeights() const { return offsetToWeights; }
        void setOffsetToWeights(uint32_t val) { offsetToWeights = val; }

		uint32_t getOffsetToOutput() const { return offsetToOutput; }
		void setOffsetToOutput(uint32_t val) { offsetToOutput = val; }

		uint32_t getTrailingWeightsMaps() const { return trailingWeightsMaps; }
		void setTrailingWeightsMaps(uint32_t val) { trailingWeightsMaps = val; }

		void setWeights(const uint32_t maxNoOfOutputMapsInWBlock, const uint32_t noOfOutputMapsInWBlock, const uint32_t trailingWeightsMaps);
		uint32_t getOffsetBetweenMaps() const;
		uint32_t getOffsetInsideMap() const;
		uint32_t getBiasOffset() const;
		uint32_t getTrailingWeightsOffset(const Task& task, const uint32_t weightsShift) const;
		uint32_t getNoOfTrailingWeightsElements() const;
		bool isFirstTile();

		void setOffsetToInput(uint32_t offset)	{ offsetToInput = offset; }

		uint32_t getOffsetToInput() const		{ return offsetToInput; }

		uint32_t getStartOutMap() const { return startOutMap; }
		void setStartOutMap(uint32_t val) { startOutMap = val; }
		uint32_t getStartOutLine() const { return startOutLine; }
		void setStartOutLine(uint32_t val) { startOutLine = val; }

		void setParent(Task* parent)	{ this->parent = parent; }		

		uint32_t getNoOfWBlockRepeat() const { return noOfWBlockRepeat; }
		void setNoOfWBlockRepeat(uint32_t val) { noOfWBlockRepeat = val; }

	private:
        uint32_t padLeft;
        uint32_t padRight;
        uint32_t padTop;
        uint32_t padBottom;
        uint32_t ElCyStart;										// Elements/cycle (start)
        uint32_t ElCyMid;										// Elements/cycle (middle)
        uint32_t ElCyEnd;										// Elements/cycle (end)		
        uint32_t MiddleBatchCnt;								// Number of middle batches			
        TileLocation tileLoc;
        uint32_t missionDataMemSize;							// required data memory size of the current mission
        uint32_t outputMapsCount;								// output maps count for specific missions
        uint32_t tileInputWidth;								// input tile width
        uint32_t tileInputHeight;								// input tile height
        uint32_t tileOutputWidth;								// output tile width
        uint32_t tileOutputHeight;								// output tile height
        uint32_t batchSize;										// basic batch size 		
        uint32_t noOfOutputMapsInWBlock;						// how many batches enter in weights memory block
        bool	lastMission;									// flag indicates last mission for HW
        bool	inputMapsNeeded;								// flag indicates read DMA needed for bringing new input maps data
        bool	trailing;										// flag indicates if trailing weights is required
        bool	trailingAlone;								    // flag indicates only trailing requests without regular weights requests		
        uint32_t tileType;										// tile type
        uint32_t trailingOutputMaps;
        uint32_t offsetToWeights;
		uint32_t trailingWeightsMaps;
		uint32_t offsetToInput;
		uint32_t offsetToOutput;
		uint32_t offsetToBias;
		uint32_t startOutMap;
		uint32_t startOutLine;
		uint32_t noOfWBlockRepeat;
		Task*	 parent;		

	}; //Mission

    class Task {
    public:
        enum TilePartition
        {
            NONE = 0,
            DEPTH = 1,
            SPATIAL = 2,
            BOTH = 3
        };

		Task();
        Task(TaskInitStruct* data);
        ~Task()
        {
            //std::cout << "delete task\n";
			for (missionIter mIter = missions.begin(); mIter != missions.end(); mIter++)
            {
				Mission* m = *mIter;
                delete m;
            }
            missions.clear();
        }

		class Statistics
		{
		public:
			Statistics() : bestUtilization(0), outputUtilization(0), outMapsUtilization(0), noOfMultiplesPerLayerDesired(0) {};
			int32_t dumpStatisticsToFile(const char* fileName, Registry* reg, Task& task);
			uint32_t getNoOfMultiplesPerLayer() const { return noOfMultiplesPerLayerDesired; }
			void setNoOfMultiplesPerLayer(uint32_t val) { noOfMultiplesPerLayerDesired = val; }
			uint32_t getOutMapsUtilization() const { return outMapsUtilization; }
			void setOutMapsUtilization(uint32_t val) { outMapsUtilization = val; }
			uint32_t getOutputUtilization() const { return outputUtilization; }
			void setOutputUtilization(uint32_t val) { outputUtilization = val; }
			uint32_t getMACUtilization() const { return MACUtilization; }
			void setMACUtilization(uint32_t val) { MACUtilization = val; }
			uint32_t getBestUtilization() const { return bestUtilization; }
			void setBestUtilization(uint32_t val) { bestUtilization = val; }
			uint32_t getBatchSize() const { return batchSize; }
			void setBatchSize(uint32_t val) { batchSize = val; }
			CevaAcceleratorNS::Elements getElements() const { return elements; }
			void setElements(CevaAcceleratorNS::Elements val) { elements = val; }
			uint32_t getNoOfMultiplesPerLayerActual() const { return noOfMultiplesPerLayerActual; }
			void setNoOfMultiplesPerLayerActual(uint32_t val) { noOfMultiplesPerLayerActual = val; }

		private:
			uint32_t bestUtilization;
			uint32_t MACUtilization;
			uint32_t outputUtilization;
			uint32_t outMapsUtilization;
			uint32_t noOfMultiplesPerLayerDesired;
			uint32_t noOfMultiplesPerLayerActual;
			uint32_t batchSize;
			Elements elements;			
		};

        // public members
        TaskInitStruct data;
        uint32_t nextMission;
        InputMapScale inputMapScale;
        uint32_t offsetToWeights;
        uint32_t offsetToBias;
        bool	isValid;
        uint32_t id;
		//Statistics stats;

        //methods		
        void addMissionToTask(Mission* mission);
		void makeInputTileHeightDivByStride(int32_t* inputTileHeight);

		//Getters and Setters
        uint32_t getNoOfTiles() const { return noOfTiles; }
        void setNoOfTiles(uint32_t val) { noOfTiles = val; }
        uint32_t getNoOfMissions() const { return noOfMissions; }
        void setNoOfMissions(uint32_t val) { noOfMissions = val; }
        uint32_t getOutputMapWidth() const { return data.outputWidth; }
        uint32_t getOutputMapHeight() const { return data.outputHeight; }
        uint32_t getOutputMapStrideX() const { return data.outputStrideX; }
        uint32_t getOutputMapStrideZ() const { return data.outputStrideZ; }
        void setOutputMapTileWidth(uint32_t val) {}
        void setOutputMapTileHeight(uint32_t val) {}
        void setOutputMapWidth(uint32_t val) { data.outputWidth = val; }
        void setOutputMapHeight(uint32_t val) { data.outputHeight = val; }
        Mission& getMissionByIndex(uint32_t index) const;
        void setSummeryValues(uint32_t x, uint32_t y);
        uint32_t getNoOfTilesX() const { return noOfTilesX; }
        void setNoOfTilesX(uint32_t val) { noOfTilesX = val; }
        uint32_t getNoOfTilesY() const { return noOfTilesY; }
        void setNoOfTilesY(uint32_t val) { noOfTilesY = val; }
        void setTilePartition(TilePartition val) { tilePartition = val; }
        TilePartition getTilePartition() const { return tilePartition; }
        typedef std::list<Mission*> missionContainer;
        typedef std::list<Mission*>::const_iterator missionIter;
        missionContainer& getMissions() { return missions; }        
        uint32_t getWeightsOffset(const Mission& mission, const uint32_t weightsPrec) const;		
		uint32_t setTileTypeAndSize(int32_t currentWidth, int32_t* currentHeight, uint32_t cols, uint32_t rows, uint32_t* outputH, uint32_t* outputW, uint32_t* support, uint32_t isOdd, uint32_t depth);
		uint32_t getWeightsPerSingleOutputMap() const { return weightsPerSingleOutputMap; }
        void setWeightsPerSingleOutputMap(uint32_t val) { weightsPerSingleOutputMap = val; }
		uint32_t getBiasOffset()	{ return offsetToBias; }

	private:
        uint32_t noOfTiles;
        TilePartition tilePartition;							// type of tile partitioning (none \ spatial \ depth \ both)
        missionContainer missions;								// missions container
        uint32_t noOfTilesX;
        uint32_t noOfTilesY;
        uint32_t noOfMissions;        
		uint32_t weightsPerSingleOutputMap;

	};

}
#endif // cnn_tasks_h__
