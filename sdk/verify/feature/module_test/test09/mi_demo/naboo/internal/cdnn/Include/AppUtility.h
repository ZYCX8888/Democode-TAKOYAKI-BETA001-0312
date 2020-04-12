//////////////////////////////////////////////////////////////////////////
//			Copyright (c) 2015 CEVA DSP Inc
//////////////////////////////////////////////////////////////////////////
//	File Name	:	fileUtility.h
//	Version		:	
//	Created	By	:	Erez Natan
//	Date		:	02/26/2015
//////////////////////////////////////////////////////////////////////////



#ifndef CDNN_APPUTILITY_H__
#define CDNN_APPUTILITY_H__

//=========================================================================
//								PLATFORM  
//=========================================================================

#include <CDNNCommonInterface.h>
#include <inputParsing.h>
#include <GlobalAppData.h>

//=========================================================================
//								 DEFINES
//=========================================================================

#define E_CDNN_NETWORK_MAX_LAYER_NAME_SIZE 128


#ifdef CEVA_HOST_IO_ENABLED
#	if defined(XM4)
#      	include "hostio_api.h"
#		define  CEVAFILE				unsigned int
#		define	 CEVAFOPEN(a,b)			ceva_fopen(a,b);
#		define	 CEVAFCLOSE(a)			ceva_fclose(a)
#		define  CEVAFPRINTF				ceva_fprintf
#		define  CEVAFREAD(a,b,c,d)		ceva_fread_std(a,b,c,d)
#		define	 CEVAFSEEK(a,b,c)		ceva_fseek(a,b,c)
#		define  CEVAFWRITE(a,b,c,d)		ceva_fwrite_std(a,b,c,d)
#		define  CEVAFPUTC(a,b)			ceva_fputc(a,b)
#		define  CEVAFGETC(a)			ceva_fgetc(a)
#		define  CEVAFPUS(a,b)			ceva_fputs(a,b)
#		define  CEVAFGETS(a,b,c)		ceva_fgets(a,b,c)
#		define  CEVAFEOF(a)				ceva_feof(a)
#		define  CEVAFFLUSH(a)			ceva_fflush(a)
#		define  CEVAFTELL(a)			ceva_ftell(a)
#		define  CEVAREWIND(a)			ceva_rewind(a)
#		define  CEVAFSCANF				ceva_fscanf
#		define  CEVAPRINTF				printf
#		define CEVASTDOUT				1
#		define CEVASTDERR				2
#
#   elif defined(XM6)
#       include "xm6_hostio_api.h"
#		define  CEVAFILE				unsigned int
#		define	 CEVAFOPEN(a,b)			ceva_fopen(a,b);
#		define	 CEVAFCLOSE(a)			ceva_fclose(a)
#		define  CEVAFPRINTF				ceva_fprintf
#		define  CEVAFREAD(a,b,c,d)		ceva_fread_std(a,b,c,d)
#		define	 CEVAFSEEK(a,b,c)		ceva_fseek(a,b,c)
#		define  CEVAFWRITE(a,b,c,d)		ceva_fwrite_std(a,b,c,d)
#		define  CEVAFPUTC(a,b)			ceva_fputc(a,b)
#		define  CEVAFGETC(a)			ceva_fgetc(a)
#		define  CEVAFPUS(a,b)			ceva_fputs(a,b)
#		define  CEVAFGETS(a,b,c)		ceva_fgets(a,b,c)
#		define  CEVAFEOF(a)				ceva_feof(a)
#		define  CEVAFFLUSH(a)			ceva_fflush(a)
#		define  CEVAFTELL(a)			ceva_ftell(a)
#		define  CEVAREWIND(a)			ceva_rewind(a)
#		define  CEVAFSCANF				ceva_fscanf
#		define  CEVAPRINTF				printf
#   endif
#else // !defined(CEVA_HOST_IO_ENABLED)
#	define  CEVAFILE				FILE*

#	define	CEVAFOPEN(a,b)			fopen(a,b);
#	define	CEVAFCLOSE(a)			fclose(a)
#	define  CEVAFPRINTF				fprintf
#	define  CEVAFREAD(a,b,c,d)		fread(a,b,c,d)
#	define  CEVAFSEEK(a,b,c)		fseek(a,b,c)
#	define  CEVAFWRITE(a,b,c,d)		fwrite(a,b,c,d)
#	define  CEVAFPUTC(a,b)			fputc(a,b)
#	define  CEVAFGETC(a)			fgetc(a)
#	define  CEVAFPUS(a,b)			fputs(a,b)
#	define  CEVAFGETS(a,b,c)		fgets(a,b,c)
#	define  CEVAFEOF(a)				feof(a)
#	define  CEVAFFLUSH(a)			fflush(a)
#	define  CEVAFTELL(a)			ftell(a)
#	define  CEVAREWIND(a)			rewind(a)
#	define  CEVAFSCANF				fscanf
#	define	CEVAPRINTF				printf

#	define CEVASTDOUT				stdout
#	define CEVASTDERR				stderr
#	endif //CEVA_HOST_IO_ENABLED


#ifdef __cplusplus
extern "C" {
#endif

	/*!
		\defgroup appUtility Application Utility 
		\brief The Application Utility library is \b not part of the CDNNTarget.
	*/

	/*!
	*  \defgroup imageReaderUtilityAPI Image Reader Utility API
	*  \ingroup appUtility 
	*  \brief Reads and decodes image files
	*	
	* \b Important:
	*  * The utility is provided as part of the CDNNExampleApplication and is \b not part of the CDNNTarget.
	*  * All files and folders \b must be in ASCII characters.

	*   ### Decoding
	*	JPEG decoding is done using LibJpeg 9 (http://www.ijg.org/) and is OpenCV 3.0 compatible. \n
	*	Other image formats (such as \b PNG and \b BMP) are decoded using STB 1.33 (https://github.com/nothings/stb) and are not guaranteed to be OpenCV compatible.

	*   ### .mat Files
	*	A **.mat** file is a special file that contains a header followed by raw data. \n
	*	It can be used for writing an intermediate network buffer, storing images with non-standard formats, and so on. 
	*	The format of the header is expected to be as follows: \n 
	*		Byte	|     0-3		|	  4-19			|  20-23	|	24-27					| 28...EOF
	*		:------:|:-------------:|:-----------------:|:---------:|:-------------------------:|:--------:
	*		Content	|	DataOrder	|  ImageDimensions	| DataType	| Precision (Fraction Bits)	| RawData
	*	\n
	*	* Each field has a size of an unsigned integer (that is, 4 bytes).
	*	* The values of \b DataOrder, \b DataType, and \b Precision must have the values as defined in the
	*			\ref E_CDNN_MEMORY_PUBLIC_DATA_ORDER,\ref E_CDNN_BUFFER_DATA_TYPE, and \ref E_CDNN_BUFFER_BIT_PRECISION_TYPE enums
	*			respectively (for more details, see CDNNCommonInterface.h).
	*	* \b ImageDimensions contains four unsigned integers that are ordered according to the \b DataOrder field. \n
	*		For example, if \b DataOrder = E_CDNN_MEMORY_DATAORDER_NCHW, then \b ImageDimensions will contain <b>| N(4bytes) | C(4bytes) | H(4bytes) | W(4bytes)|</b>
	*	For details about saving **.mat** files, see WriteMatFile().
	
	*   ### Memory Management
	*	The user controls the lifetime of the reader by calling the iruCreateImageReader() and iruDestroyImageReader() functions.
	*	All memory is managed by the utility.
	*  
	* \b Important: All \b malloc() and \b free() calls on the files and the filenames will lead to undefined behavior, and must be avoided.
	*	
	*   ### Typical Usage
	*	\code{.c}
	*	const char* sFolderName = ... ; //some folder
	*	int fileIdx = 0;
	*	char** fileNames=NULL;
	*	int filesCount=0;
	*	unsigned char bKeepFileInMem = 0;
	*
	*	//create reader
	*	ImageReader_t* pReader = iruCreateImageReader();
	*	//get all files within a folder (no user memory management is required)
	*	assert (iruGetFilesInFolder(pReader, sFolderName, &fileNames, &filesCount) == E_IR_SUCCESS);
	*	for (fileIdx = 0; fileIdx < filesCount; fileIdx++)
	*	{
	*		DataBuffer_t* pDataBuffer = NULL;
	*		//read a single file, parse its name to extract layer-name and buffer-id. Place the result in pDataBuffer.
	*		assert (iruReadFile(pReader, fileNames[fileIdx], E_EXTRACT_LAYER_FROM_FILENAME, &pDataBuffer) == E_IR_SUCCESS);
	*		//do something with pDataBuffer...
	*		
	*		if (!bKeepFileInMem)
	*			//free all resources associated with this file
	*			iruDeleteFile(pReader,pDataBuffer);
	*	}
	*   //free all resources
	*	iruDestroyImageReader(pReader);
	*	\endcode
	*  @{
	*/

	/*! \brief Returns the status from the ReaderAPI */
	typedef enum
	{
		E_IR_SUCCESS,					/*!< Success */
		E_IR_NO_FILE_OR_FOLDER,			/*!< Unable to read file or access folder */
		E_IR_CORRUPT_HEADER,			/*!< The MAT file header is corrupt. */
		E_IR_UNSUPPORTED_DATA_TYPE,		/*!< The MAT file data type is not supported. */
		E_IR_FILE_SIZE_ERROR,			/*!< The MAT file size does not match the expected size. */
		E_IR_NO_MEMORY,					/*!< Insufficient memory*/
		E_IR_CORRUPT_FILE_NAME,			/*!< The filename is corrupt. */
		E_IR_CORRUPT_BUFFER,			/*!< The provided buffer to iruDeletFile() is corrupt. */
		E_IR_FAILURE					/*!< Unexpected failure */
	} E_IMAGE_READER_STATUS;

	/*! \brief Determines if the reader should extract the layer name and the buffer ID from the filename*/
	typedef enum
	{
		E_EXTRACT_LAYER_FROM_FILENAME,		/*!< Extract the layer name and buffer ID. */
		E_DONT_EXTRACT_LAYER_FROM_FILENAME  /*!< Do not extract the layer name and buffer ID. */
	}E_IMAGE_READER_EXTRACT_LAYER_FROM_FILE_NAME;

	/*! \brief Defines the encapsulation of all data required for creating a CDNNDataBuffer_t */
	typedef struct
	{
		/*! Pointer to the raw image data  */
		void* pRawData;
		/*! Pointer to the struct with all of the data buffer parameters */
		cdnnDatabufferParameters_t* pstParams;
	} DataBuffer_t;



	struct ImageReader_st;
	/*! \brief Opaque struct, accesses and reads image files */
	typedef struct ImageReader_st ImageReader_t;

	/*! \brief A struct with all required meta-data for creating .mat files*/
	typedef struct
	{
		/*! Dimension order; can be either NCHW or NHCW */
		enum E_CDNN_MEMORY_PUBLIC_DATA_ORDER u32DataOrder;
		/*! Number of input images (N) */
		unsigned int u32NInputs;
		/*! Number of channels (C) */
		unsigned int u32NChannels;
		/*! Image height (H) */
		unsigned int u32Height;
		/*! Image width (W) */
		unsigned int u32Width;
		/*! Underlying data type: U8, S16,...  */
		enum  E_CDNN_BUFFER_DATA_TYPE u32DataType;
		/*! The number of fractional bits (only relevant for fixed-point data types) */
		int s32FractionBits;
	} MatWriterInfo_t;

	#ifndef LINUX_PLATFORM
	/*!
	* \brief Initializes all of the hardware devices
	* \param[out]	pCnnHardwareAcceleratorHandle	Handle to a CNN hardware accelerator
	*/
	int CevaHardwareInit(cdnnDeviceHandle* pCnnHardwareAcceleratorHandle, E_CDNN_USER_DEVICE_TYPE PlatformType);
	#endif//end if

	/*!
	* \brief De-initializes all of the hardware devices
	*/
	int CevaHardwareDeinit();

	/*!
	* \brief Creates an image reader object
	*/
	ImageReader_t* iruCreateImageReader();

	/*!
	* \brief Destroys an image reader object and frees all of the resources
	* \param[in]	pReader			Pointer to the image reader struct created using iruCreateImageReader()
	*/
	void iruDestroyImageReader(ImageReader_t* pReader);

	/*!
	* \brief Reads a single image file
	* \param[in]	pReader				Pointer to the image reader struct created using \b iruCreateImageReader()
	* \param[in]	psFilePath			Pointer to the full file path
	* \param[in]	eExtractLayer		Enumerator specifying whether the layer name and the buffer ID should be extracted from the filename
	* \param[out]	pDataBuffer			Pointer for the returned data buffer struct
	* 
	* All files and folders \b must be in ASCII characters.
	*/
	E_IMAGE_READER_STATUS iruReadFile(ImageReader_t* pReader, const char* psFilePath, E_IMAGE_READER_EXTRACT_LAYER_FROM_FILE_NAME eExtractLayer, DataBuffer_t** pDataBuffer);

	/*!
	* \brief Deletes a single file (frees all resources)
	* \param[in]	pReader			Pointer to the image reader struct created using \b iruCreateImageReader()
	* \param[in]	pDataBuffer		Pointer to data buffer struct obtained using \b iruReadFile()
	*/
	E_IMAGE_READER_STATUS iruDeleteFile(ImageReader_t* pReader, DataBuffer_t* pDataBuffer);

	/*!
	* \brief Returns an array of all of the supported image files (\b JPG, \b PNG, and \b MAT) found in a folder
	* 
	*  In \b Windows, this is done using system calls. \n
	*  \b Note: Because the sort order for files and folders whose names contain numerals differs between Windows versions, this function cannot guarantee any order for the file list. \n
	*  \n
	*  In <b>CEVA-Toolbox (SDT)</b>, the user must specify the first file in the folder using the format <b><filename>_<first_index>.<extension></b>. \n
	* \n
	*  This function finds all files with a larger index than the specified file (up to 1000 files in total). The indexes do not need to be continuous. \n
	*  For example, if the specified file is \b cat_0.jpg, then this function will attempt to add \b cat_1.jpg, \b  cat_2.jpg, \b cat_3.jpg, and so on. \n
	*
	* \param[in]	pReader			Pointer to the image reader struct created using \b iruCreateImageReader()
	* \param[in]	sFolder			Folder in which to search
	* \param[out]	fileNames		Filename array
	* \param[out]	filesCount		File count
	*/
	E_IMAGE_READER_STATUS iruGetFilesInFolder(ImageReader_t* pReader, const char* sFolder, char** fileNames[], int* filesCount);

	/**
	* \brief Returns an array of all files listed in the provided text file (a single file in each row)
	*
	* For example, if the input file is <b>/folderA/folderB/files.txt</b> and its contents are: \n
	*	* \b A_0.jpg \n
	* 	* \b A_1.jpg \n
	* Then the output array will be <b>[/folderA/folderB/A_0.jpg, /folderA/folderB/A_1.jpg]</b>
	*
	* \param[in]	pReader			Pointer to the image reader struct created using \b iruCreateImageReader()
	* \param[in]	sFullPath		File with all filenames
	* \param[out]	fileNames		Filename array
	* \param[out]	filesCount		File count
	*/
	E_IMAGE_READER_STATUS iruGetFilesInFile(ImageReader_t* pReader, const char* sFullPath, char** fileNames[], int* filesCount);
	/**
	* \brief Finds all files with an index larger than the specified file
	*
	*  In \b Windows, this is done using system calls. \n
	* \n
	*  In <b>CEVA-Toolbox (SDT)</b>, the user must specify the first file in the folder using the format <b><filename>_<first_index>.<extension></b>. \n
	* \n
	*  This function finds all files with an index larger than the specified file. \n
	*  For example, if the specified file is \b cat_0.jpg, then this function will attempt to add \b cat_1.jpg, \b cat_2.jpg, \b cat_3.jpg, and so on. \n
	*  This function attempts to find all of the subsequent files (up to 1000 files in total), even if some are missing.
    * \param[in]	pReader			Pointer to the image reader struct created using \b iruCreateImageReader()
	* \param[in]	sFolder			Folder in which to search
	* \param[out]	fileNames		Filename array
	* \param[out]	filesCount		File count
	*/
	E_IMAGE_READER_STATUS iruGetSubFoldersInFolder(ImageReader_t* pReader, const char* sFolder, char** fileNames[], int* folderCount);
	/*!
	* \brief Returns whether the specified pointer points to a file/folder
	* \param[in]	sPath		A string with the a full path to a file/folder
	*/
	unsigned char iruIsFolder(const char* sPath);

	/*!
	* @}
	*/ //end of imageReaderAPI

	/*!
	* \defgroup utilityFunctions Utility Functions
	*  \brief Parses example application CFG files, prints results, and so on
	* \ingroup appUtility 
	* @{
	*/

	/**
	* \brief Prepares a test struct and adds it to g_ppApplicationParameters
	*  The following cases are supported:
	*  * If no parameters are supplied, a default \b InputConfiguration.cfg file is used as input.
	*  * A <b>-cdnnconfig \<configuration_file_name\>.cfg</b> file.
	*  * A configuration line.
	* \param [in] argc argc
	* \param [in] argv argv
	* \ingroup group_input_parsing
	*/
	int AddPrepareTests(int argc, char *argv[]);


	/*! \brief Prints prediction results
	* \param [in] psInputFileName Input filename
	* \param [in] psResultsFileName Output filename
	* \param [in] s64Duration Inference duration, in milliseconds
	* \param [in] psPredictionLabels Pointer to prediction labels
	* \param [in] u32LabelsNamelength Number of labels
	* \param [in] pf32Predictions Pointer to prediction values
	* \param [in] u32PredictionLength Number of actual predictions
	* \ingroup group_cdnn_debugger    */
	int printResults(char* psInputFileName, char* psResultsFileName, double s64Duration, char* psPredictionLabels, unsigned int u32LabelsNamelength, float *pf32Predictions, unsigned int u32PredictionLength);

	/*! \brief Dumps a buffer to a file
	* \param [in] pFileName Pointer to the output file
	* \param [in] pData Pointer to the data
	* \param [in] ElementsNumber Number of elements
	* \param [in] BytesPerElement Bytes per element
	* \ingroup group_cdnn_buffer
	*/
	int appendToFile(char* pFileName, char *pData, int ElementsNumber, int BytesPerElement);
	/*! \brief Dumps a buffer to a profiler file
	* \param [in] pFileName Pointer to the output file
	* \param [in] pData Pointer to the data
	* \param [in] BytesPerElement Bytes per element
	* \ingroup group_cdnn_buffer
	*/
	int PrintToProfilerFile(char* pFileName, char *pData, int BytesPerElement);

	/*! \brief Dumps to a file
	* \param [in] pLayerName Layer name
	* \param [in] pbufferName Buffer name
	* \param [in] pcPath Path to the file
	* \param [in] i File index
	* \param [in] pData Data pointer
	* \param [in] buffDataOrder Data order type
	* \param [in] fileDataOrder File order type
	* \param [in] nInputs Number of inputs
	* \param [in] nChannels Number of nChannels
	* \param [in] height Height
	* \param [in] width Width
	* \param [in] elementSize Size of the element
	* \ingroup group_cdnn_buffer
	*/
	void dumpToFile(char* pLayerName, char* pbufferName, char* pcPath, int i, void *pData, int buffDataOrder, int fileDataOrder, int nInputs, int nChannels, int height, int width, int elementSize);

	/*! \brief Resets the file
	* \param [in] pf Pointer to the file
	* \ingroup group_cdnn_buffer
	*/
	int resetFile(char *pf);

	/*! \brief Prints the usage if an error occurs in an input command argument
	* \param [in] argv Pointer to the argument buffer
	* \ingroup group_input_parsing
	*/
	void ApplicationPrintUsageAndExit(char* argv);


	/*! \brief Loads a network from a file
	* \param [in] pNetworkFileName Pointer to the network filename
	* \param [in] bQuant Quantization parameter
	* \param [in] pNetworkRawData Pointer to the network raw data
	* \param [in] s32NetworkRawDataCapacity Size of the network raw data
	* \ingroup group_input_parsing
	*/
	int applicationLoadNetworkFromFile(const char *pNetworkFileName, unsigned char bQuant, void *pNetworkRawData, unsigned int s32NetworkRawDataCapacity, unsigned int *pu32NetSize);
	
	/*! \brief Starts a process using WinAPI
	* \param [in] psExecutionStr The process to execute (with all arguments)
	* \param [in] bInheritConsole Redirects the output of the subprocess into the current process
	* \return The exit code of the process
	*/
	int startWinProcess(const char* psExecutionStr, unsigned char bInheritConsole);

	/*! \brief Starts a process using WinAPI
	* \param [in] sFilePath		The output filename 
	* \param [in] pInfo			The struct containing the meta-data (for example, dimensions and data type) of the .mat file		
	* \param [in] pRawData		The data
	* \return 0 if successful, -1 on error
	*/
	int WriteMatFile(const char* sFilePath, const MatWriterInfo_t* pInfo, const unsigned char* pRawData);
	/*! \brief Runs the CDNN Network Generator
	* \param [in] pApplicationParameters		Struct of application parameters
	* \param [in] pReader			Image reader pointer
	* \return 0 if successful, -1 on error
	*/
	int runCDNNNetworkGenerator(InputTestParameters_t *pApplicationParameters, ImageReader_t* pReader);

	/*!
	* @}
	*/ //end of utilityFunctions



#ifdef __cplusplus
}
#endif

#endif //CDNN_APPUTILITY_H__
