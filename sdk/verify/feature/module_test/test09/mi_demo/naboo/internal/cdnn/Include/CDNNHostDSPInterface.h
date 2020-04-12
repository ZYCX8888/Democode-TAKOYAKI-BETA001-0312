//////////////////////////////////////////////////////////////////////////
//			Copyright (c) 2015 CEVA-DSP Inc
//////////////////////////////////////////////////////////////////////////
//	File Name	:	CDNNHostDSPInterface.h
//	Version		:	
//	Created	By	:	Erez Natan
//	Date		:	19/02/2017
//////////////////////////////////////////////////////////////////////////


/*!
* \file
* \brief External defines and types header file for host to dsp command
* \author Erez Natan <erez.natan@ceva-dsp.com>
*/


//=============================================================================
//									Defines
//=============================================================================

#ifndef __CDNN_HOSTDSPINTERFACE_H
#define __CDNN_HOSTDSPINTERFACE_H

/*! \brief Defines the the host commands
* \ingroup group_cdnn_hostToDspEnum
*/
enum E_CDNN_HOST_COMMAND
{
	/*! \brief Creates the CDNN framework */
	E_CDNN_CMD_CDNN_CREATE,
	/*! \brief Releases the CDNN framework */
	E_CDNN_CMD_CDNN_RELEASE,
	/*! \brief Initializes a created CDNN framework */
	E_CDNN_CMD_CDNN_INITIALIZE,
	/*! \brief Sets attributes to the created CDNN framework */
	E_CDNN_CMD_CDNN_SETATTRIBUTE,
	/*! \brief Sets attributes to the created CDNN framework */
	E_CDNN_CMD_CDNN_GETATTRIBUTE,
	/*! \brief Classifies using the created CDNN network*/
	E_CDNN_CMD_CDNN_NETWORKCLASSIFY,
	/*! \brief Creates the CDNN network*/
	E_CDNN_CMD_CDNN_CREATENETWORK,
	/*! \brief Releases the CDNN network*/
	E_CDNN_CMD_CDNN_RELEASENETWORK,
	/*! \brief Updates the parameters of the created CDNN network*/
	E_CDNN_CMD_CDNN_NETWORKUPDATEPARAMETER,
	/*! \brief Sets the attributes to the created CDNN network*/
	E_CDNN_CMD_CDNN_NETWORKSETATTRIBUTE,
	/*! \brief Queries the CDNN network*/
	E_CDNN_CMD_CDNN_QUERYNETWORK,
    /*! \brief Queries the CDNN network for a specific network tail (by index)*/
    E_CDNN_CMD_CDNN_NETWORK_GET_TAIL_BUFF_PARAMS_BY_INDEX,
    /*! \brief sets a network parameter (such as output buffers) */
    E_CDNN_CMD_CDNN_NETWORK_SET_PARAMETER,
	/*! \brief Accesses the CDNN network classes */
	E_CDNN_CMD_CDNN_ACCESSNETWORKCLASSES,
	/*! \brief Accesses the CDNN network profiler results */
	E_CDNN_CMD_CDNN_ACCESSNETWORKPROFILER,
	/*! \brief Creates a data buffer */
	E_CDNN_CMD_CDNN_CREATEDATABUFFER,
	/*! \brief Creates a data buffer from a handle */
	E_CDNN_CMD_CDNN_CREATEDATABUFFERFROMHANDLE,
	/*! \brief Releases the data buffer */
	E_CDNN_CMD_CDNN_RELEASEDATABUFFER,
	/*! \brief Queries the data buffer */
	E_CDNN_CMD_CDNN_QUERYDATABUFFER,
	/*! \brief Accesses the data buffer */
	E_CDNN_CMD_CDNN_ACCESSDATABUFFER,
	/*! \brief Copies data from the host */
	E_CDNN_CMD_CDNN_COPY_HOST_DATA
};

/*! \brief Defines the host command event retrieval
* \ingroup group_cdnn_hostToDspEnum
*/
enum E_CDNN_HOST_EVENT
{
	/*! \brief Indicates that the event succeeded*/
	E_CDNN_EVENT_CDNN_COMMON,
	/*! \brief Indicates that the event failed */
	E_CDNN_EVENT_CDNN_NETWORK_EXECUTE,
	/*! \brief Indicates the number of the event */
	E_CDNN_EVENT_CDNN_EVENT_NUMBER
};

/*! \brief Defines the host command return values
* \ingroup group_cdnn_hostToDspEnum
*/
enum E_CDNN_HOST_RETURN_VALUE
{
	/*! \brief Indicates that the function returned successfully */
	E_CDNN_RETVAL_SUCCEED = 0x0000,
	/*! \brief Indicates that the address received by the DSP is invalid */
	E_CDNN_RETVAL_FAILED_INVALID_ADDRESS = 0x0001,
	/*! \brief Indicates that the DSP does not have enough free memory to preform the copy*/
	E_CDNN_RETVAL_FAILED_NOT_ENOUGH_MEMORY = 0x0002,
	/*! \brief Indicates that a general failure occurred*/
	E_CDNN_RETVAL_FAILED_GENERAL = 0xffff
};

//=============================================================================
//				CDNN Host To DSP Message Interface
//=============================================================================

/*! \brief The CDNN create struct
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnCreate
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief CDNN framework handle */
	unsigned int u32CDNNHandle;
	/*! \brief Shared memory pointer*/
	void *pSharedMemory;
	/*! \brief Shared memory size*/
	unsigned int SharedMemorySize;
	/*! \brief Pointer to the initial target information struct */
	void *ptr;
	/*! \brief Return value of cdnnCreate */
	int retVal;
} cdnnCreate_st;

/*! \brief The CDNN release struct
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnRelease
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief CDNN handle */
	unsigned int u32CDNNHandle;
	/*! \brief Return value of CDNNCreate */
	int retVal;
} cdnnRelease_st;

/*! \brief The CDNN initialize function parameter
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnInitialize
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief CDNN handle */
	unsigned int u32CDNNHandle;
	/*! \brief Return value of CDNNCreate */
	int retVal;
} cdnnInitialize_st;

/*! \brief The CDNN set attribute struct
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnSetAttribute
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief CDNN handle */
	unsigned int u32CDNNHandle;
	/*! \brief Attribute type */
	int s32Attribute;
	/*! \brief Pointer to the attribute */
	void *ptr;
	/*! \brief Parameter size */
	int size;
	/*! \brief Return value */
	int u32retval;
} cdnnSetAttribute_st;

/*! \brief The CDNN create data buffer struct
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnDataBuffFromHandle
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief CDNN handle */
	unsigned int u32CDNNHandle;
	/*! \brief Pointer to the buffer parameters */
	void *pBufferParams;
	/*! \brief Pointer to the data buffer */
	void *ptr;
	/*! \brief Data buffer handle */
	unsigned int dataBuffHandle;
} cdnnDataBuff_st;

/*! \brief The CDNN create network struct
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnCreateNetwork
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief Network parameters struct */
	void *pNetworkParams;
	/*! \brief CDNN handle */
	unsigned int u32CDNNHandle;
	/*! \brief Network handle */
	unsigned int NetworkHandle;
} cdnnCreateNetwork_st;

/*! \brief The copy data from host to DSP struct
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnCopyHostData
{
	/*! \brief Size */
	unsigned int	size;
	/*! \brief Data buffer handle */
	unsigned char*	dataBuffer;
	/*! \brief Return value */
	unsigned int	retVal;
	/*! \brief CDNN handle */
	unsigned int	u32CDNNHandle;
	/*! \brief Network size */
	unsigned int	u32NetSize;
	/*! \brief network memory space */
	unsigned int	networkMemSpace;
}cdnnCopyHostData_st;

/*! \brief The CDNN network inference struct
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnNetworkInference
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief Network handle */
	unsigned int u32network;
	/*! \brief CDNN handle */
	unsigned int u32CDNNHandle;
	/*! \brief Return value of the network inference */
	int retVal;
} cdnnNetworkInference_st;

/*! \brief The CDNN query data buffer struct
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnQueryDataBuffer
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief Buffer handle */
	unsigned int bufferHandle;
	/*! \brief Attribute type */
	unsigned int attribute;
	/*! \brief Return value (by user) */
	void *ptr;
	/*! \brief Size of the return value */
	int size;
	/*! \brief Return value */
	int retVal;
} cdnnQueryDataBuffer_st;

/*! \brief The CDNN access data buffer struct
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnAccessDataBuffer
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief Buffer handle */
	unsigned int bufferHandle;
	/*! \brief Pointer to the shared memory buffer shared by the CDNN and the user */
	void **ptr;
	/*! \brief Return value */
	int retVal;
} cdnnAccessDataBuffer_st;

/*! \brief The CDNN query Network struct
* \ingroup group_cdnn_hostToDsp
*/
typedef struct _cdnnQueryNetwork
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief Network handle */
	unsigned int NetworkHandle;
	/*! \brief Attribute type */
	unsigned int attribute;
	/*! \brief Return value (by user) */
	void *ptr;
	/*! \brief Size of the return value */
	int size;
	/*! \brief Return value */
	int retVal;
} cdnnQueryNetwork_st;

/*! \brief The CDNN release data buffer struct
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnReleaseDataBuffer
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief CDNN handle */
	unsigned int u32CDNNHandle;
	/*! \brief Data buffer handle */
	cdnnDatab *datab;
	/*! \brief Return value */
	int retVal;
} cdnnReleaseDataBuffer_st;

/*! \brief The Network access labels struct
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnNetworkAccessLabels
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief Network handle */
	unsigned int NetworkHandle;
	/*! \brief Pointer to labels */
	void *pLabels;
	/*! \brief Size of the labels (in bytes) */
	unsigned int size;
	/*! \brief Return value */
	int retVal;
} cdnnNetworkAccessLabels_st;

/*! \brief The CDNN release network struct
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnReleaseNetwork
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief CDNN handle */
	unsigned int cdnnHandle;
	/*! \brief Network handle */
	cdnnNetwork *network;
	/*! \brief Return value */
	int retVal;
} cdnnReleaseNetwork_st;

/*! \brief The CDNN network update struct
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnNetworkUpdateParameter
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief Network handle */
	cdnnNetwork network;
	/*! \brief Reference to be updated */
	cdnnReference ref;
	/*! \brief Network parameter index */
	unsigned int paramIdx;
	/*! \brief Return value */
	int retVal;
} cdnnNetworkUpdateParameter_st;

/*! \brief The CDNN network set attribute struct
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnNetworkSetAttribute
{
	/*! \brief Message-type ID */
	unsigned int msgId;
	/*! \brief CDNN network handle */
	cdnnNetwork network;
	/*! \brief Attribute type */
	int s32attribute;
	/*! \brief Pointer to the attribute */
	void *ptr;
	/*! \brief Parameter size */
	int size;
	/*! \brief Return value */
	int u32retval;
} cdnnNetworkSetAttribute_st;


/*! \brief The CDNN network get tail databuffer params
* \ingroup group_cdnn_hostToDspTypes
*/
typedef struct _cdnnNetworkGetTailBuffParamsByIndex
{
    /*! \brief Message-type ID */
    unsigned int msgId;
    /*! \brief CDNN network handle */
    unsigned int networkHandle;
    /*! \brief tail index */
    int tailIdx;
    /*! \brief Pointer to the attribute */
    void *ptr;
    /*! \brief Return value */
    int retVal;
} cdnnNetworkGetTailBuffParamsByIndex_st;



#endif//endif