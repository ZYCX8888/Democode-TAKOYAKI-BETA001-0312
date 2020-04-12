//////////////////////////////////////////////////////////////////////////
//			Copyright (c) 2016 CEVA DSP Inc
//////////////////////////////////////////////////////////////////////////
//	File Name	:	ImageReader.h
//	Version		:	
//	Created	By	:	Erez Natan
//	Date		:	27/09/2017
//////////////////////////////////////////////////////////////////////////

#ifndef CDNN_GLOBALAPPLICATIONDATA_H__
#define CDNN_GLOBALAPPLICATIONDATA_H__


//=============================================================================
//						Global Application Data Defines 
//=============================================================================
#ifdef WIN32
	#define	APPLICATION_NETWORK_FILES_MAX_SIZE (768*1024*1000)
	#define PRAGMA_DSECT_NO_LOAD(name)
	#define PRAGMA_DSECT_LOAD(name)
	#define PRAGMA_CSECT(name)
#elif !defined(X64) && !defined(LINUX_PLATFORM)
	#define	APPLICATION_NETWORK_FILES_MAX_SIZE (335*1024*1000)
	#define PRAGMA_DSECT_NO_LOAD(name)		__attribute__ ((section (".DSECT " name)))
	#define PRAGMA_DSECT_LOAD(name)			__attribute__ ((section (".DSECT " name)))
	#define PRAGMA_CSECT(name)				__attribute__ ((section (".CSECT " name)))
#else
	#define	APPLICATION_NETWORK_FILES_MAX_SIZE (1)
	#define PRAGMA_DSECT_NO_LOAD(name)
	#define PRAGMA_DSECT_LOAD(name)
	#define PRAGMA_CSECT(name)
#endif


#ifdef __cplusplus 
extern "C"{
#endif 
	extern unsigned char gu8ApplicationNetwork[APPLICATION_NETWORK_FILES_MAX_SIZE];
#ifdef __cplusplus 
}
#endif 


#endif //CDNN_GLOBALAPPLICATIONDATA_H__