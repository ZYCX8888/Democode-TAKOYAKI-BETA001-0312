//////////////////////////////////////////////////////////////////////////
//			Copyright (c) 2016 CEVA DSP Inc
//////////////////////////////////////////////////////////////////////////
//	File Name	:	ImageReader.h
//	Version		:	
//	Created	By	:	Daniel Zelyanski
//	Date		:	11/01/2016
//////////////////////////////////////////////////////////////////////////

#ifndef CDNN_IMAGEREADER_H__
#define CDNN_IMAGEREADER_H__

//=============================================================================
//									INCLUDE
//=============================================================================
#include <cassert>
#include <string>
#include <vector>
#include <list>

#include "AppUtility.h"


//=============================================================================
//									UTILITY FUNCTION
//=============================================================================
/*! \brief read decode a jpeg buffer using libjpeg
* \param [in] pJpgBuffer								Pointer to the memory buffer.
* \param [in] bufSize									Memory buffer size.
* \param [out] s32ImageWidth							Decoded image's width.
* \param [out] s32ImageHeight							Decoded image's height.
* \param [out] s32Components							Decoded image's components number.
*/
extern "C" unsigned char*  libjpeg_decode_buffer(unsigned char* pJpgBuffer, int bufSize, int* s32ImageWidth, int* s32ImageHeight, int* s32Components);


//=============================================================================
//									CLASSES
//=============================================================================

/**
 *  ImageReader auxiliary class for reading all supported image formats (jpg/png/mat,etc...)
 *  and converting them to CDNN Data Buffers.
 *
 *
 *  Notes:
 *  * Jpeg decoding is done using LibJpeg 9 and is OpenCV 3.0-compatible.
 *    (Other formats standard image formats such as png are not guaranteed to be OpenCV compatible).
 *	* All files and folders are expected to be in ASCII characters.
 */
class ImageReader
{
public:
	const static int SIZE_OF_HEADER = 7 * sizeof(unsigned int); /**< expected head size of a .mat file */
	ImageReader(){	};
	virtual ~ImageReader() { FreeAll(); }
	/**
	* \brief Returns whether the specified points to a folder.
	* \param[in]	sPath		A string with the a full path to a file/folder.
	*/
	static bool IsFolder(const std::string& sPath);
	/**
	* \brief Returns a an array of all supported image files (jpg,png,mat) found in a folder.
	*
	*  In windows , this is done using system calls.
	*  In eclipse, the user must specify the first file in the folder and the using the format: <filename>_<first_index>.<extension>
	*  The function will find all files with a larger index than the specified file.
	*  For example, if the specified file was: cat_0.jpg, the function will try to add cat_1.jpg, cat_2.jpg, cat_3.jpg,...
	*  The function will try to find all of the subsequent files (even if some are missing) up to 1000 files total.
	* \param[in]	sFolder			The folder in which to search.
	* \param[out]	fileNames		The file names array.
	* \param[out]	filesCount		The files count
	*/
	E_IMAGE_READER_STATUS GetFilesInFolder(const char* sFolderName, char** fileNames[], int* filesCount);

	/**
	* \brief Returns an array of all files listed in a the text-file provided (a single file in each row)
	*
	* for example, if the input file is: /folderA/folderB/files.txt \n
	* and its content is: \n
	* A_0.jpg \n
	* A_1.jpg \n
	* The output array will be: [/folderA/folderB/A_0.jpg, /folderA/folderB/A_1.jpg]
	*
	* \param[in]	sFullPath		The file with all file names.
	* \param[out]	fileNames		The file names array.
	* \param[out]	filesCount		The files count
	*/
	E_IMAGE_READER_STATUS GetFilesInFile(const char* sFullPath, char** fileNames[], int* filesCount);
	
	/**
	* \brief 
	*
	*  In windows , this is done using system calls.
	*  In eclipse, the user must specify the first file in the folder and the using the format: <filename>_<first_index>.<extension>
	*  The function will find all files with a larger index than the specified file.
	*  For example, if the specified file was: cat_0.jpg, the function will try to add cat_1.jpg, cat_2.jpg, cat_3.jpg,...
	*  The function will try to find all of the subsequent files (even if some are missing) up to 1000 files total.
	* \param[in]	sFolder			The folder in which to search.
	* \param[out]	fileNames		The file names array.
	* \param[out]	filesCount		The files count
	*/
	E_IMAGE_READER_STATUS GetSubFoldersInFolder(const char* sFolder, char** foldersNames[], int* foldersCount);
	
	
	
	/**
	* \brief Read a single image file
	* \param[in]	psFilePath			A pointer to the full file path.
	* \param[in]	eExtractLayer		Enumerator specifying whether or not the the layer name and the buffer id should be extracted from the file's name. for example: layer10_12.jpg -> layer name = layer10, buffer id = 12.
	* \param[out]	pDataBuffer			The pointer to data buffer struct for the read file.
	*/
	E_IMAGE_READER_STATUS ReadFile(const char* sFullFilePath, E_IMAGE_READER_EXTRACT_LAYER_FROM_FILE_NAME eExtractLayer, DataBuffer_t** ppDataBuffer);
	/**
	* \brief Delete a single file (free all resources)
	* param[in]		pDataBuffer			The pointer to data buffer struct obtained using cdnnReadFile().
	*/
	E_IMAGE_READER_STATUS DeletFile(DataBuffer_t* pDataBuffer);
	/**
	 * \brief Frees all allocated memory.
	 *  Frees all memory allocated for the data buffers as well as for the the folders and files lists.
	*/
	void FreeAll();

private:

	typedef unsigned char uchar;
	typedef unsigned int uint;
	typedef E_IMAGE_READER_STATUS(*pfFileNameParser)(const std::string& sFileName, cdnnDatabufferParameters_t&);  
	/**
	*	\brief Read .Mat file
	*	A .Mat file must contain a header followed by raw data.
	*	The format of the header is expected to be as follows:
	*		byte	|     0-3		|	  4-19			|  20-23	|	24-27					| 28...EOF 
	*		content	|	DataOrder	|  ImageDimensions	| DataType	| Precision (Fraction Bits)	| RawData
	*	* Each field has a size of 4 bytes and type of unsigned integer\integer.
	*	* The values of the DataOrder, DataType and Precision must have the values as defined in the 
	*	  enums E_CDNN_MEMORY_PUBLIC_DATA_ORDER,E_CDNN_BUFFER_DATA_TYPE and E_CDNN_BUFFER_BIT_PRECISION_TYPE 
	*	  respectively (see CDNNCommonInterface.h)
	*
	*/
	E_IMAGE_READER_STATUS ReadMatFile(const std::string& sFilePath);
	/**
	*	\brief Read JPEG file using open source LibJpeg 9 (OpenCV compatible).
	*	\param[in]	sFilePath		A string with the full file path.
	*/
	E_IMAGE_READER_STATUS ReadJPEGFile(const std::string& sFilePath);
	/**
	*	\brief Read all image files excluding .jpg and .mat files using stb library.
	*	\param[in]	sFilePath		A string with the full file path.
	*/
	E_IMAGE_READER_STATUS ReadOtherImageFile(const std::string& sFilePath);
	/**
	* \brief Parses the file and, dispatches the decoding according to the file's format .
	* \param[in]	sFilePath		A string with the full file path.
	* \param[in]	pfPraser		A function pointer for file name parsing.
	*/
	E_IMAGE_READER_STATUS ReadImageFileWithParser(const std::string& sFilePath, pfFileNameParser pfPraser);
	/**
	*	\brief Delete a DataBuffer using an iterator and return the iterator to the next item.
	*	\param[in]	it		An iterator to the DataBuffer to delete
	*/
	std::list<DataBuffer_t>::iterator DeleteDataBuffer(std::list<DataBuffer_t>::iterator it);
	
	/** 
	*	A strings array class for managing the folder/file names. 
	*/
	class StringsArray
	{
	public:
		char** ptr;
		int cnt;
		StringsArray();
		~StringsArray();
		
		/* 
		\brief Free all resources used for holding the strings.
		*/
		void Clear();
		/*
		*	\brief Copy all vector content into a dynamically allocated array.
		*	\param[in]	v	A source vector of strings.
		*/
		E_IMAGE_READER_STATUS CopyFromStringVec(std::vector<std::string>& v);
	};
	
	/*! An array of file names,   filled by GetFilesInFolder() */
	StringsArray mLastFiles;	
	/*! An array of folder names, filled by GetSubFoldersInFolder() */
	StringsArray mLastFolders;	
	/*! A vector of all allocated DataBuffers */
	std::list<DataBuffer_t> mvDataBuffers;	
};



#endif //CDNN_IMAGEREADER_H__
