//////////////////////////////////////////////////////////////////////////
//			Copyright (c) 2015 CEVA-DSP Inc
//////////////////////////////////////////////////////////////////////////
//	File Name	:	CDNNCommonInterface.h
//	Version		:	
//	Created	By	:	Erez Natan
//	Date		:	02/04/2016
//////////////////////////////////////////////////////////////////////////


/*!
* \file
* \brief External defines and types header file
* \author Erez Natan <erez.natan@ceva-dsp.com>
*
* \defgroup group_cdnn_interface User API 
* \brief This section defines the user interface functions. For an example of how to use the user interface, see \ref CDNNExampleApplication.
*
* To write code using the CDNN library, the developer should use the interface provided. This interface enables you to create a network object, attach input and output buffers to it, and then run it in run-time for each input frame/ROI while querying for the results from the CDNN (pointer to the output buffer, width, and height).
*
* \defgroup group_cdnn_typedef User Typedefs
* \brief This section defines the user typedefs.
*
* \defgroup group_cdnn_enumerations User Enumerations
* \brief This section defines the user enumerations used by the user API functions.
*/


//=============================================================================
//									Defines
//=============================================================================

#ifndef CDNN_COMMONINTERFACE_H__
#define CDNN_COMMONINTERFACE_H__


#define E_CDNN_CONST_DATA_EXTERNAL_ALIGNMENT	(15) //align CDNN file to 16 bytes in external memory
//=============================================================================
//								CDNN Enumeration
//=============================================================================

/*! \brief Describes the hardware configuration of the internal memory size that the CDNN will run on.
The user must define this size before running the CDNN; otherwise, the default 512 KB configuration will be used.
This value can be set via the \b CDNNSetAttribute API.
* \ingroup group_cdnn_enumerations
*/
enum E_CDNN_MEMORY_HARDWARE_SIZE
{
	/*! \brief 128 KB internal memory size*/
	E_CDNN_MEMORY_SIZE_128,
	/*! \brief 256 KB internal memory size*/
	E_CDNN_MEMORY_SIZE_256,
	/*! \brief 512 KB internal memory size*/
	E_CDNN_MEMORY_SIZE_512,
	/*! \brief 1024 KB internal memory size*/
	E_CDNN_MEMORY_SIZE_1024,
	/*! \brief 2048 KB internal memory size*/
	E_CDNN_MEMORY_SIZE_2048
};

/*! \brief Defines the runtime attributes
* \ingroup group_cdnn_enumerations
*/
enum E_CDNN_SET_ATTRIBUTES
{
	/*! \brief Enables debug mode with logging information. If an error occurs and this attribute is enabled, the CDNN prints a text line on the screen explaining the error that is defined by the error ID coming from the CDNN. */
	E_CDNN_ATTRIBUTE_DEBUG_ENABLE,
	/*! \brief Enables performance calculation (not currently supported)*/
	E_CDNN_ATTRIBUTE_PERFORMANCE_ENABLE,
	/*! \brief Enables run in floating point (not supported for release versions)*/
	E_CDNN_ATTRIBUTE_FLOAT_CONFIG,
	/*! \brief Disables the DMA while running */
	E_CDNN_ATTRIBUTE_DISABLE_DMA,
	/*! \brief Enables the statistic module, which logs the estimated cycles according to average cycles per operation calculated on the board. It also logs bandwidth in bytes and describes the 
	final structure of the network run in run time according to profiling level @ref E_CDNN_PROFILER_LEVEL.*/
	E_CDNN_ATTRIBUTE_PROFILER_ENABLE,
	/*! \brief Changes the default buffer that is used for user buffer memory allocation (input and output) to the user's own pointer. The user must provide the size of the buffer along with
	the pointer. Because the \B gu8MmeoryExternalAllocationReference buffer is not used, the user can reduce its size to zero when setting the pointer.
	Setting the pointer and size to a non-zero value is possible only before creating the network or any user buffer; otherwise, it will use the default "gu8MmeoryExternalAllocationReference" buffer.
	*/
	E_CDNN_ATTRIBUTE_SET_USER_BUFFERS_MEMORY_SPACE,
	/*! \brief Allocates memory from the CDNN external memory space to be used by the user. The size of the allocation will be in bytes and will be reduced from the total external memory available to 
	the CDNN at boot time. Once the memory is allocated there is no way to release it, but only by releasing the CDNN itself.
	*/
	E_CDNN_ATTRIBUTE_SET_USER_EXTERNAL_MEMORY_SPACE,
	/*! \brief Changes the internal memory size that the CDNN uses (applicable in Visual Studio only) according to @ref E_CDNN_MEMORY_HARDWARE_SIZE. 
	In simulation or emulation modes, the CDNN takes the memory configuration according to the end and start sections located in the \b lsf file.
	*/
	E_CDNN_ATTRIBUTE_SET_INTERNAL_MEMORY_SIZE
};

/*! \brief Defines the runtime attributes
* \ingroup group_cdnn_enumerations
*/
enum E_CDNN_GET_ATTRIBUTES
{
	/*! \brief Gets the currently configured internal memory size used by the CDNN */
	E_CDNN_ATTRIBUTE_CONFIGURED_INTERNAL_MEMORY_SIZE,
	/*! \brief Gets the currently allocated external memory used by the CDNN */
	E_CDNN_ATTRIBUTE_USED_EXTERNAL_MEMORY_SIZE,
	/*! \brief Gets the currently allocated user memory for input and output buffers defined by the user */
	E_CDNN_ATTRIBUTE_USED_USER_MEMORY_SIZE
};

/*! \brief Defines the statistic-type module
* \ingroup group_cdnn_enumerations
*/
enum E_CDNN_PROFILER_LEVEL
{
	/*! \brief Dumps brief profiler information to an output file*/
	E_CDNN_PROFILER_LEVEL_SHORT,
	/*! \brief Dumps detailed profiler information to an output file*/
	E_CDNN_PROFILER_LEVEL_LONG,
	/*! \brief Dumps detailed development profiler information to an output file*/
	E_CDNN_PROFILER_LEVEL_DEV,
	/*! \brief Dumps all of the profiler information to an output file*/
	E_CDNN_PROFILER_MAX_LEVEL
};

//=============================================================================
//								CDNN Network Enumeration
//=============================================================================

/*! \brief Defines the network attributes
* \ingroup group_cdnn_enumerations
*/
enum E_CDNN_NETWORK_SET_ATTRIBUTES
{
	/*! \brief Sets a value (between 0 and 1) by which to optimize the bandwidth reduction value */
	E_CDNN_ATTRIBUTE_NETWORK_BWRVALUE,
	/*! \brief Decides whether or not to swap the channel order of the input image. */
	E_CDNN_ATTRIBUTE_NETWORK_CHANNEL_SWAP
};

/*! \brief Defines the class get attributes
* \ingroup group_cdnn_enumerations
*/


enum E_CDNN_NETWORK_GET_ATTRIBUTES
{
	/*! \brief Gets the number of classes the network supports */
	E_CDNN_ATTRIBUTE_NETWORK_CLASSES_SIZE,
	/*! \brief Gets the size of the class name buffer */
	E_CDNN_ATTRIBUTE_NETWORK_CLASSES_NAME_SIZE,
	/*! \brief Gets the profiler buffer memory size */
	E_CDNN_ATTRIBUTE_NETWORK_PROFILER_BUFFER_SIZE,
	/*! \brief Gets the network mode as configured by the generator */
	E_CDNN_ATTRIBUTE_NETWORK_NETWORK_MODE,
	/*! \brief Gets the number of user inputs to the network */
	E_CDNN_ATTRIBUTE_NETWORK_INPUT_BUFFER_NUMBER,
	/*! \brief Gets the number of user outputs to the network */
	E_CDNN_ATTRIBUTE_NETWORK_OUTPUT_BUFFER_NUMBER,
	/*! \brief Gets the network average performance in core cycles */
	E_CDNN_ATTRIBUTE_NETWORK_AVERAGE_PERFORMANCE,
	/*! \brief Gets the network heads and tails struct */
	E_CDNN_ATTRIBUTE_NETWORK_TAILS_COUNT
};

/*! \brief Defines the network mode that can be configured
* \ingroup group_cdnn_enumerations
*/
enum E_CDNN_NETWORK_MODE_TYPE
{
	/*! \brief Sets the network to fully convolutional network inference mode. Input to the network is one image that contains multiple objects to detect.
	In this mode, the network executes all of the layers until the convolution layer, and then transforms the first fully connected layer after the convolution to a convolution
	layer that can pass the sliding window filter on the convolution map results. The result of this is the same as the output of the original fully connected layer after the last convolution layer output, as defined in the
	network structure (the fully connected layer scans the input image using a sliding window to extract potential object detection probabilities).*/
	E_CDNN_NETWORK_MODE_FULLYCONVOLUTIONAL = 0,
	/*! \brief Sets the network to fixed net size execution mode. The input to the network must be a fixed size; if not, it must pass resize to the fixed net size as described by the network structure.*/
	E_CDNN_NETWORK_MODE_FIXEDNETEXECUTION = 1
};

/*! \brief Describes the type of parameters the user must give the network structure
* \ingroup group_cdnn_enumerations
*/
enum E_CDNN_NETWORK_STRUCT_PARAMETERS
{
	/*! \brief An array holding pointers to the network input buffers */
	E_CDNN_NETWORK_STRUCT_INPUT_BUFFERS,
	/*! \brief An array holding pointers to the network output buffers */
	E_CDNN_NETWORK_STRUCT_OUTPUT_BUFFERS,
	/*! \brief Flag for resizing the input image */
	E_CDNN_NETWORK_DO_RESIZE_INDICATION,
	/*! \brief Network raw data memory  */
	E_CDNN_NETWORK_POINTER
};

//=============================================================================
//								CDNN Buffer Enumeration
//=============================================================================

/*! \brief Defines the data buffer attributes
* \ingroup group_cdnn_enumerations
*/
enum E_CDNN_BUFFER_ATTRIBUTES
{
	/*! \brief Gets the data buffer type */
	E_CDNN_BUFFER_ATTRIBUTE_DATA_TYPE,
	/*! \brief Gets the data buffer order type */
	E_CDNN_BUFFER_ATTRIBUTE_DATA_ORDER,
	/*! \brief Gets the data buffer width */
	E_CDNN_BUFFER_ATTRIBUTE_WIDTH,
	/*! \brief Gets the data buffer height */
	E_CDNN_BUFFER_ATTRIBUTE_HEIGHT,
	/*! \brief Gets the data buffer input size */
	E_CDNN_BUFFER_ATTRIBUTE_INPUTS,
	/*! \brief Gets the data buffer number of channels */
	E_CDNN_BUFFER_ATTRIBUTE_CHANNELS,
	/*! \brief Gets the number of bits dedicated to the fraction after the integer part in fixed point data elements representation. For example, if the 
	fraction bit is 8 with a data element of S16, it means that 8 high bits are for the integer and the last 8 bits are for fractions. */
	E_CDNN_BUFFER_ATTRIBUTE_FRACTION_BITS,
	/*! \brief Gets the data buffer size (in bytes) */
	E_CDNN_BUFFER_ATTRIBUTE_SIZE,
	/*! \brief Gets the element count */
	E_CDNN_BUFFER_ATTRIBUTE_ELEMENT_COUNT,
	/*! \brief Gets the element size (in bytes) */
	E_CDNN_BUFFER_ATTRIBUTE_ELEMENT_SIZE_IN_BYTES
};

/*! \brief Defines the order of the data memory in the buffer class, where N is the number of samples, C is the number of channels, H is the height, and W is the width
* \ingroup group_cdnn_enumerations
*/
enum E_CDNN_MEMORY_PUBLIC_DATA_ORDER
{
	/*! \brief Sets the data dimension order: inputs, width, height, channels */
	E_CDNN_MEMORY_DATAORDER_NWHC,
	/*! \brief Sets the data dimension order: inputs, height, width, channels */
	E_CDNN_MEMORY_DATAORDER_NHWC,
	/*! \brief Sets the data dimension order: channels, width, height, inputs */
	E_CDNN_MEMORY_DATAORDER_CWHN,
	/*! \brief Sets the data dimension order: channels, height, width, inputs */
	E_CDNN_MEMORY_DATAORDER_CHWN,
	/*! \brief Sets the data dimension order: inputs, channels, width, height */
	E_CDNN_MEMORY_DATAORDER_NCWH,
	/*! \brief Sets the data dimension order: inputs, channels, height, width */
	E_CDNN_MEMORY_DATAORDER_NCHW,
	/*! \brief Sets the data dimension order: channels, inputs, width, height */
	E_CDNN_MEMORY_DATAORDER_CNWH,
	/*! \brief Sets the data dimension order: channels, inputs, height, width */
	E_CDNN_MEMORY_DATAORDER_CNHW,
	/*! \brief Sets the data dimension order: width, height, channels, inputs */
	E_CDNN_MEMORY_DATAORDER_WHCN,
	/*! \brief Sets the data dimension order: height, width, channels, inputs */
	E_CDNN_MEMORY_DATAORDER_HWCN,
	/*! \brief Sets the data dimension order: width, height, inputs, channels */
	E_CDNN_MEMORY_DATAORDER_WHNC,
	/*! \brief Sets the data dimension order: height, width, inputs, channels */
	E_CDNN_MEMORY_DATAORDER_HWNC,
	/*! \brief Gets the data order enum size */
	E_CDNN_MEMORY_DATAORDER_PUBLIC_SIZE
};

/*! \brief Defines the size of the supported memory element (in bits)
* \ingroup group_cdnn_enumerations
*/
enum E_CDNN_BUFFER_BIT_PRECISION_TYPE
{
	/*! \brief Sets an 8-bit element size */
	E_CDNN_PRECISION_8BIT = 8,
	/*! \brief Sets a 16-bit element size */
	E_CDNN_PRECISION_16BIT = 16,
	/*! \brief Sets a 32-bit element size */
	E_CDNN_PRECISION_32BIT = 32,
	/*! \brief Sets a 64-bit element size */
	E_CDNN_PRECISION_64BIT = 64
};

/*! \brief Defines the buffer data type, where U is unsigned, S is signed, and F is floating
* \ingroup group_cdnn_enumerations
*/
enum E_CDNN_BUFFER_DATA_TYPE
{
	/*! \brief Sets an 8-bit unsigned data type*/
	E_CDNN_DATATYPE_U8,
	/*! \brief Sets an 8-bit signed data type*/
	E_CDNN_DATATYPE_S8,
	/*! \brief Sets a 16-bit unsigned data type*/
	E_CDNN_DATATYPE_U16,
	/*! \brief Sets a 16-bit signed data type*/
	E_CDNN_DATATYPE_S16,
	/*! \brief Sets a 32-bit unsigned data type*/
	E_CDNN_DATATYPE_U32,
	/*! \brief Sets a 32-bit signed data type*/
	E_CDNN_DATATYPE_S32,
	/*! \brief Sets a floating single precision data type  */
	E_CDNN_DATATYPE_F32,
	/*! \brief Sets a floating double precision data type  */
    E_CDNN_DATATYPE_F64
};

//=============================================================================
//								CDNN Layer Enumeration
//=============================================================================


/*! \brief Describes the type of layers supported by the current version of the CDNN
* \ingroup group_cdnn_enumerations
*/
typedef enum E_CDNN_LAYER_TYPES
{
	/*! \brief */
	E_CDNN_LAYER_NOTDEFINED, //reserved to 0 do not change
	/*! \brief */
	E_CDNN_LAYER_CONV,
	/*! \brief */
	E_CDNN_LAYER_DROPOUT,
	/*! \brief */
	E_CDNN_LAYER_INNERPRODUCT,
	/*! \brief */
	E_CDNN_LAYER_NORMALIZE,
	/*! \brief */
	E_CDNN_LAYER_POOL,
	/*! \brief */
	E_CDNN_LAYER_INPUT,
	/*! \brief */
	E_CDNN_LAYER_UPSAMPLE,
	/*! \brief */
	E_CDNN_LAYER_RESHAPE,
	/*! \brief */
	E_CDNN_LAYER_CONCAT,
	/*! \brief */
	E_CDNN_LAYER_BN,
	/*! \brief */
	E_CDNN_LAYER_BATCHNORM,
	/*! \brief */
	E_CDNN_LAYER_SCALE,
	/*! \brief */
	E_CDNN_LAYER_RELU,
	/*! \brief */
	E_CDNN_LAYER_PRELU,
	/*! \brief */
	E_CDNN_LAYER_ABS,
	/*! \brief */
	E_CDNN_LAYER_POWER,
	/*! \brief */
	E_CDNN_LAYER_TANH,
	/*! \brief */
	E_CDNN_LAYER_SIGMOID,
	/*! \brief */
	E_CDNN_LAYER_DECONV,
	/*! \brief */
	E_CDNN_LAYER_SOFTMAX,
	/*! \brief */
	E_CDNN_LAYER_ARGMAX,
	/*! \brief */
	E_CDNN_LAYER_ELTWISE,
	/*! \brief */
	E_CDNN_LAYER_PROPOSAL,
	/*! \brief */
	E_CDNN_LAYER_ROI_POOLING,
	/*! \brief */
	E_CDNN_LAYER_NOTSUPPORTED
}E_CDNN_LAYER_TYPES;


/*! \brief Defines the Eltwise layer operation type
* \ingroup group_cdnn_enumerations
*/
enum E_CDNN_LAYER_ELTWISE_OP_TYPE
{
	/*! \brief Product   */
	E_CDNN_ELTWISE_OP_PROD,
	/*! \brief Summation */
	E_CDNN_ELTWISE_OP_ADD,
	/*! \brief Maximum   */
	E_CDNN_ELTWISE_OP_MAX,
	/*! \brief Number of Eltwise operations supported  */
	E_CDNN_ELTWISE_OP_CNT
};

/*! \brief Defines the user device type supported by the CDNN
* \ingroup group_cdnn_defines
*/
typedef enum E_CDNN_USER_DEVICE_TYPE {
	/*! \brief CEVA-XM4-based device */
	E_CDNN_DEVICE_USER_XM4,
	/*! \brief CEVA-XM4 + CNN HWA-based device */
	E_CDNN_DEVICE_USER_XM4HWA,
	/*! \brief CEVA-XM6-based device */
	E_CDNN_DEVICE_USER_XM6,
	/*! \brief CEVA-XM6 + CNN HWA-based device */
	E_CDNN_DEVICE_USER_XM6HWA
}E_CDNN_USER_DEVICE_TYPE;

//=============================================================================
//							CDNN User Interface
//=============================================================================

/*! \brief An opaque reference to the CDNN object
* \ingroup group_cdnn_typedef
*/
typedef void cdnnHandle_t;

/*! \brief The CDNN return status value
* \ingroup group_cdnn_typedef
*/
typedef int cdnnStatus_t;

/*! \brief An opaque reference to a data buffer
* \ingroup group_cdnn_typedef
*/
typedef struct _cdnnDatab *cdnnDatab;

/*! \brief A generic opaque reference to any object within the CDNN API library
* \ingroup group_cdnn_typedef
*/
typedef struct _cdnnReference *cdnnReference;

/* \brief An opaque reference to a layer object
* \ingroup group_cdnn_typedef
*/
typedef struct _cdnnLayer *cdnnLayer;

/* \brief An opaque reference to a network object
* \ingroup group_cdnn_typedef
*/
typedef struct _cdnnNetwork *cdnnNetwork;

/*! \brief Sets the standard enumeration type size to a fixed quantity.
* \details All enumerable fields must use this type as the container to
* enforce enumeration ranges and sizeof() operations.
* \ingroup group_cdnn_typedef
*/
typedef int cdnnEnum;

/*! \brief A handle representing a device. Used by a device driver to control the device.
*/
typedef void* cdnnDeviceHandle;

//=============================================================================
//									CDNN Reference Types
//=============================================================================

/*! \brief Includes the CDNN target information needed to initialize the CDNN with the loaded targets
*/
typedef struct _cdnnTargetInfo {
	/*! \brief Device type */
	E_CDNN_USER_DEVICE_TYPE	eDeviceType;
	unsigned int u32NumOfCNNHA;
	cdnnDeviceHandle* hCNNHADevices;
}cdnnTargetInfo_st;

/*! \brief Includes the CDNN information needed to create the CDNN memory space, target, and devices.
This information will be used to create the CDNN context and set the base device the manager is working on (including other targets the CDNN needs to connect to)
*/
typedef struct _cdnnBringUpInfo {
	/*! \brief Input buffer handle */
	int				  NumberOfTargets;
	/*! \brief Number of input buffers */
	cdnnTargetInfo_st *pTargetsInformation;
}cdnnBringUpInfo_st;

/*!
* \brief Defines the data buffer parameters (this structure is used only by the host). The size of the buffer is limited to 64 MB when running on Visual Studio, and to 16 MB when running a DSP-to-Host mode application (due to the size of the shared memory).
*/
typedef struct _cdnnDatabufferParameters_t {
	/*! Buffer width (in elements) */
	unsigned int width;
	/*! \brief Buffer height (in elements) */
	unsigned int height;
	/*! \brief Buffer element data type (for example, single precision float, singed short, and so on) */
	unsigned int dataType;
	/*! \brief Element size (in bits) */
	unsigned int depth;
	/*! \brief Number of channels */
	unsigned int nChannels;
	/*! \brief Represents the number of bits dedicated for the fraction after the integer part if the fixed-point result is defined (S/U16, S/U8, S/U32).
	The user must define it to a reasonable value. Defining as a negative value for the output network buffer indicates that the user does not want to change the CDNN precision output. */
	int fractionBits;
	/*! \brief enum of data order: NCHW, NCWH, and so on*/
	unsigned int dataOrder;
	/*! \brief Number of input maps */
	unsigned int nInputs;
	/*! \brief Flips the data*/
	unsigned int dataFlipped;
	/*! \brief Transposes the data*/
	unsigned int dataTransposed;
	/*! \brief Margin dimensions (in elements), where 0 = left, 1 = right, 2 = top, 3 = bottom */
	int padding[4];
	/*! \brief The name of the layer the buffer belongs to. The name must be the same as the name of the layer as described in the network file provided by the developer.*/
	char *pLayerName;
	/*! \brief The buffer parameter index in the layer it belongs to  */
	unsigned int bufferId;
} cdnnDatabufferParameters_t;


/** 
*	\brief A structure for holding an array of cdnnDatab* and its size 
*/
typedef struct _cdnnDataBuffersArray {
	/*! \brief Array size*/
	int arrSize;
	/*! \brief Array of cdnnDatab */
	cdnnDatab* pArray;
} cdnnDataBuffersArray;


/*! \brief The network parameters that must be set by the user for the network initialization to succeed. 
To avoid configuration mistakes, the user must initialize the created structure to a zero value. 
During the initialization of the network, the CDNN allocates the maximum amount of external memory to support the input and output buffer size; any use of larger buffers in real-time will cause the CDNN to return a failure.
*/
typedef struct _cdnnNetworkParams {
	/*! \brief Array of handles to input buffers before initialization*/
	cdnnDataBuffersArray* pInDataBuffersArray;
	/*! \brief Indicates whether a resize to the input image is needed*/
	unsigned int	bResizeInput;
	/*!	\brief Pointer to a memory containing the data from the two files created by the CDNN Network Generator, qcdata and qcnet, according to the following structure: 
	{4 bytes for file type (float = 1 or fixed = 0);  4 bytes for network structure file memory size, in bytes (qcnet size); 4 bytes for network data file memory size, in bytes (qcdata size);
	network structure description (qcnet); network data memory (qcdata)}.<br>
	At the beginning of the qcnet network structure memory, the first 4 bytes must be filled again with the size of the qcnet file size.<br> 
	It is the responsibility of the user to take these two files and load them to the memory according to this structure and provide the pointer to this memory.*/
	char*			pNetworkFilePointer;
}cdnnNetworkParams_st;

/** 
* \cond INTERNAL
* \brief Contains the layer parameters for single-layer execution (set by the user interface)
*/
typedef struct _cdnnLayerParams {
	/*! \brief Input buffer handle */
	void*	pInputBuffer;
	/*! \brief Number of input buffers */
	int		numberOfInputs;
	/*! \brief Weight buffer handle */
	void*	pInputWeights;
	/*! \brief Bias buffer handle */
	void*	pInputBias;
	/*! \brief Output buffer handle */
	void*	pOutputBuffer;
	/*! \brief Number of output buffers */
	int		numberOfOutput;
	/*! \brief The layer type */
	E_CDNN_LAYER_TYPES	eLayerType;
	/*! \brief Padding size */
	int		s32padW;
	/*! \brief Padding size */
	int		s32padH;
	/*! \brief Step size between operations */
	int		s32StrideW;
	/*! \brief Step size between operations */
	int		s32StrideH;
	/*! \brief Kernel size */
	int		s32KernelSize[2];
	/*! \brief Bandwidth reduction threshold in float (between 0 and 1)*/
	float	f32BWRTH;
}cdnnLayerParams_st;
/**
*	\endcond INTERNAL
*/





#endif //__CDNN_COMPONENTCOMMON_H
