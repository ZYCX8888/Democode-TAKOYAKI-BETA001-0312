//////////////////////////////////////////////////////////////////////////
//			Copyright (c) 2015 CEVA-DSP Inc
//////////////////////////////////////////////////////////////////////////
//	File Name	:	CDNNUserInterface.h
//	Version		:	
//	Created	By	:	Erez Natan
//	Date		:	02/25/2015
//////////////////////////////////////////////////////////////////////////

#ifndef CDNN_TARGETINTERFACE_H__
#define CDNN_TARGETINTERFACE_H__

//=============================================================================
//									Include
//=============================================================================
#include <CDNNCommonInterface.h>

//=============================================================================
//									API
//=============================================================================

/**
 *  \defgroup cdnn_context CDNN Context API
 *  \brief Creates, initializes, and destroys CDNN context
 *  \ingroup group_cdnn_interface 
*/

/**
*  \defgroup cdnn_buffer CDNN Buffer API
*  \brief Provides input data and gets output data from the CDNN framework
*  \ingroup group_cdnn_interface
*/

/**
*  \defgroup cdnn_network CDNN Network API
*  \brief Creates, executes, and destroys a CDNN network
*  \ingroup group_cdnn_interface
*/

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

    //=========================================================================
    //								CDNN CONTEXT
    //=========================================================================
    /** \addtogroup cdnn_context
    *  @{
    */

	/*! \brief Creates a CDNN platform. Only one network at a time can be created under the CDNN framework (this will be resolved in future versions of the CDNN).
	* \param[out] handle Handle to the created CDNN framework
	* \param[in] pBringUpInfo Brings up info for CDNN creation
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNCreate(cdnnHandle_t **handle, cdnnBringUpInfo_st *pBringUpInfo);
	/*! \brief Releases the CDNN framework
	* \param [in] handle Handle to the created CDNN framework
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNRelease(cdnnHandle_t *handle);
	/*! \brief Initializes the network parameters configured by the user
	* \param [in] handle Handle to the created CDNN framework
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNInitialize(cdnnHandle_t *handle);
	/*! \brief Sets the CDNN attributes (as described in @ref E_CDNN_SET_ATTRIBUTES)
	* \param [in] handle Handle to the created CDNN framework
	* \param [in] attribute Network attribute
	* \param [in] ptr Pointer to the value to be set
	* \param [in] size Size of the value, in bytes
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNSetAttribute(cdnnHandle_t *handle, int attribute, void *ptr, int size);
	/*! \brief Gets the CDNN parameter by attribute (as described in @ref E_CDNN_GET_ATTRIBUTES)
	* \param [in] handle Handle to the created CDNN framework
	* \param [in] attribute Network attribute
	* \param [in] ptr Pointer to the value to be set
	* \param [in] size Size of the value, in bytes
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNGetAttribute(cdnnHandle_t *handle, int attribute, void *ptr, int size);
    
    /** @}*/
    //=========================================================================
    //								CDNN NETWORK
    //=========================================================================
	
    /** \addtogroup cdnn_network
    *  @{
    */
	/*! \brief Executes the network as initialized by the user
	* \param [in] handle Handle to the CDNN object
	* \param [in] network Handle to the network object
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNNetworkInference(cdnnHandle_t *handle, cdnnNetwork network);
	/*! \brief Creates a network from the file configuration for training
	* \param [in] handle Handle to the created CDNN framework
	* \param [in] pNetworkParams Pointer to the network user parameters (see @ref cdnnNetworkParams_st)
	* \retval Handle to the created CDNN network
	*/
	cdnnNetwork CDNNCreateNetwork(cdnnHandle_t *handle, cdnnNetworkParams_st *pNetworkParams);
	/*! \brief Releases the network attached to the CDNN framework
	* \param [in] handle Handle to the created CDNN framework
	* \param [in] network Handle to the created CDNN network
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNReleaseNetwork(cdnnHandle_t *handle, cdnnNetwork *network);
	/*! \brief Sets a network parameter (currently, only setting the output buffers is allowed)
	*	\param [in] network  Handle to the created CDNN network
	*	\param [in] ref		Handle to the changed reference
	*	\param [in] paramIdx The parameter to set (currently, only @ref E_CDNN_NETWORK_STRUCT_OUTPUT_BUFFERS is valid)
	*/
	int CDNNNetworkSetParameter(cdnnNetwork network, cdnnReference ref, cdnnEnum paramIdx);
	/*! \brief Updates a network parameter
	* \param [in] network Handle to the created CDNN network
	* \param [in] ref Handle to the changed reference
	* \param [in] paramIdx The name of the parameter to be updated in the <b>@ref E_CDNN_NETWORK_STRUCT_PARAMETERS</b> network struct
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNNetworkUpdateParameter(cdnnNetwork network, cdnnReference ref, cdnnEnum paramIdx);
	/*! \brief Sets the CDNN attributes for the network (as described in @ref E_CDNN_NETWORK_SET_ATTRIBUTES)
	* \param [in] network Handle to the created CDNN network
	* \param [in] attribute Network attribute
	* \param [in] ptr Pointer to the value to be set
	* \param [in] size Size of the value, in bytes
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNNetworkSetAttribute(cdnnNetwork network, int attribute, void *ptr, int size);
	/*! \brief Gets information from the network by setting an attribute (as described in @ref E_CDNN_NETWORK_GET_ATTRIBUTES)
	* \param [in] handle Handle to the created CDNN network
	* \param [in] attribute Network attribute
	* \param [in] ptr Pointer to the parameter
	* \param [in] size Size of the parameter
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNNetworkGetAttribute(cdnnNetwork handle, cdnnEnum attribute, void *ptr, int size);
	/*! \brief Queries the network class labels
	* \param [in] handle Handle to the created CDNN network
	* \param [in] pLabels Pointer to the class memories
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNAccessNetworkClassesLabels(cdnnNetwork handle, char **pLabels);

	/*! \brief Queries the network profiler results (see @ref prof)
	* \param [in] handle Handle to the created CDNN network
	* \param [out] pProfilerResult Pointer to the profiler result memory
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNAccessNetworkProfilerResults(cdnnNetwork handle, char **pProfilerResult);

    /*! \brief Gets network tail data-buffer parameters according to the tail index
    *
    * This method is intended for iterating over the network tails, obtaining their parameters (layer-name, width, height, and so on),
    * and then creating a cdnnDatab using CDNNCreateDataBufferFromHandle().
    * \param [in] handle Handle to the created CDNN network
    * \param [in] tailIndex Index of the tail in the network
    * \param [out] pDataBuffParams Pointer to the data buffer parameter struct
    * \retval 0 for the correct behavior; otherwise, see @ref debug
    */
    int CDNNNetworkGetTailBuffParamsByIndex(cdnnNetwork handle, int tailIndex, cdnnDatabufferParameters_t* pDataBuffParams);
    /** @}*/
    //=========================================================================
    //								CDNN BUFFER 
    //=========================================================================
    /** \addtogroup cdnn_buffer
    *  @{
    */
    /*! \brief Creates a data buffer object with default parameters according to @ref cdnnDatabufferParameters_t
    * \param [in] handle Handle to the CDNN network
    * \param [in] pBufferParams Buffer parameters
	* \retval returns a handle to the CDNN data buffer object
    */
	cdnnDatab CDNNCreateDataBuffer(cdnnHandle_t *handle, cdnnDatabufferParameters_t *pBufferParams);

	/*! \brief Creates a data buffer from an input pointer, and then returns a handle for it according to @ref cdnnDatabufferParameters_t \n 
	*  /b Note: After providing the CDNN with a pointer to the memory, the user should not use this memory during CDNN execution.
    * \param [in] handle Handle to the CDNN network
	* \param [in] pBufferParams Buffer parameters
	* \param [in] ptr Pointer to the external buffer
	* \retval An opaque pointer to a data buffer object
	*/
	cdnnDatab CDNNCreateDataBufferFromHandle(cdnnHandle_t *handle, cdnnDatabufferParameters_t *pBufferParams, void *ptr);
	/*! \brief Releases a reference input data buffer from the network manager
    * \param [in] handle Handle to the CDNN network
	* \param [in] datab Handle to the CDNN data buffer
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNReleaseDataBuffer(cdnnHandle_t *handle, cdnnDatab *datab);
	/*! \brief Queries the data buffer for information about the buffer (see @ref E_CDNN_BUFFER_ATTRIBUTES)
	* \param [in] buffer Handle to the data buffer
	* \param [in] attribute Attribute
	* \param [in] ptr Pointer to the destination
	* \param [in] size Size of the query element
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNQueryDataBuffer(cdnnDatab buffer, cdnnEnum attribute, void *ptr, int size);
	/*! \brief Accesses the memory buffer pointer
	* \param [in] buffer Handle to the data buffer
	* \param [in] ptr Pointer to the memory buffer
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNAccessDataBuffer(cdnnDatab buffer, void **ptr);
	/* \brief Creates a layer from the layer parameters (this option is not currently enabled)
    * \param [in] handle Handle to the CDNN network
	* \param [in] layerType Layer type 
	* \param [in] pLayerParam Layer parameters
	* \retval Object handle
	*/
    
    /** @}*/

	cdnnLayer CDNNCreateLayer(cdnnHandle_t *handle, cdnnLayerParams_st *pLayerParam);
	/* \brief Creates a layer object (this option is not currently enabled)
    * \param [in] handle Handle to the CDNN network
	* \param [in] layer Handle to the layer
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNLayerExecute(cdnnHandle_t *handle, cdnnLayer layer);
	/* \brief Releases a layer object (this option is not currently enabled)
    * \param [in] handle Handle to the CDNN network
	* \param [in] layer Handle to the layer
	* \retval 0 for the correct behavior; otherwise, see @ref debug
	*/
	int CDNNRelaseLayer(cdnnHandle_t *handle, cdnnLayer *layer);
	
	
#if 0 //todo: implement 
	int CDNNGetDataTailBuffParamsByName(cdnnHandle_t *handle, const char* pLayerName, cdnnDatabufferParameters_t* pDataBuffParams);
#endif 

#ifdef __cplusplus
}
#endif // __cplusplus

#endif //CDNN_TARGETINTERFACE_H