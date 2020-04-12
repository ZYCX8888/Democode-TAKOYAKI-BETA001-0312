#ifndef __CSPOTTER_SDK_API_CONST_H__
#define __CSPOTTER_SDK_API_CONST_H__


#define CSPOTTER_SUCCESS						(     0 )
#define CSPOTTER_ERR_SDKError					( -2000 )
#define CSPOTTER_ERR_LexiconError				( -3000 )
#define CSPOTTER_ERR_EngineError				( -5000 )


/************************************************************************/
// Recognition type                                                                
/************************************************************************/
#define CSPOTTER_RecogType_Unknown				(0)
#define CSPOTTER_RecogType_Passed				(1)
#define CSPOTTER_RecogType_NotGoodEnough		(2)
#define CSPOTTER_RecogType_MissStartSyllable	(3)
#define CSPOTTER_RecogType_MissEndSyllable		(4)
#define CSPOTTER_RecogType_VolumeTooLow			(5)

/************************************************************************/
// Engine Type                                                                  
/************************************************************************/
#define CSPOTTER_Mode_TriggerCmd				(0)
#define CSPOTTER_Mode_KeywordSpotting			(1)


#define CSPOTTER_SampleRate_16k					(16000)
#define CSPOTTER_SampleRate_8k					(8000)


/************************************************************************/
// Error code                                                                  
/************************************************************************/

#define CSPOTTER_ERR_IllegalHandle				( CSPOTTER_ERR_SDKError -   1 )
#define CSPOTTER_ERR_IllegalParam				( CSPOTTER_ERR_SDKError -   2 )
#define CSPOTTER_ERR_LeaveNoMemory				( CSPOTTER_ERR_SDKError -   3 )
#define CSPOTTER_ERR_LoadDLLFailed				( CSPOTTER_ERR_SDKError -   4 )
#define CSPOTTER_ERR_LoadModelFailed			( CSPOTTER_ERR_SDKError -   5 )
#define CSPOTTER_ERR_GetFunctionFailed			( CSPOTTER_ERR_SDKError -   6 )
#define CSPOTTER_ERR_ParseEINFailed				( CSPOTTER_ERR_SDKError -   7 )
#define CSPOTTER_ERR_OpenFileFailed				( CSPOTTER_ERR_SDKError -   8 )
#define CSPOTTER_ERR_NeedMoreSample				( CSPOTTER_ERR_SDKError -   9 )
#define CSPOTTER_ERR_Timeout					( CSPOTTER_ERR_SDKError -   10 )
#define CSPOTTER_ERR_InitWTFFailed				( CSPOTTER_ERR_SDKError -   11 )
#define CSPOTTER_ERR_AddSampleFailed			( CSPOTTER_ERR_SDKError -   12 )
#define CSPOTTER_ERR_BuildUserCommandFailed	    ( CSPOTTER_ERR_SDKError -   13 )
#define CSPOTTER_ERR_MergeUserCommandFailed	    ( CSPOTTER_ERR_SDKError -   14 )
#define CSPOTTER_ERR_IllegalUserCommandFile     ( CSPOTTER_ERR_SDKError -   15 )
#define CSPOTTER_ERR_IllegalWaveFile			( CSPOTTER_ERR_SDKError -   16 )
#define CSPOTTER_ERR_BuildCommandFailed			( CSPOTTER_ERR_SDKError -   17 )
#define CSPOTTER_ERR_InitFixNRFailed			( CSPOTTER_ERR_SDKError -   18 )
#define CSPOTTER_ERR_ExceedNRBufferSize			( CSPOTTER_ERR_SDKError -   19 )
#define CSPOTTER_ERR_Rejected					( CSPOTTER_ERR_SDKError -   20 )
#define CSPOTTER_ERR_ModelVerMismatch			( CSPOTTER_ERR_SDKError -	21 )

#define CSPOTTER_ERR_InitCSVGMMFailed			( CSPOTTER_ERR_SDKError -	30 )
#define CSPOTTER_ERR_TrainCSVGMMFailed			( CSPOTTER_ERR_SDKError -	31 )
#define CSPOTTER_ERR_WaveToFeaFailed			( CSPOTTER_ERR_SDKError -	32 )
#define CSPOTTER_ERR_InitSpeakerFailed			( CSPOTTER_ERR_SDKError -	33 )
#define CSPOTTER_ERR_SpeakerAddFeaFailed		( CSPOTTER_ERR_SDKError -	34 )
#define CSPOTTER_ERR_SpeakerGetScoreFailed		( CSPOTTER_ERR_SDKError -	35 )
#define CSPOTTER_ERR_TooManySpeakers			( CSPOTTER_ERR_SDKError -	36 )
#define CSPOTTER_ERR_NotGetSpeaker				( CSPOTTER_ERR_SDKError -	37 )
#define CSPOTTER_ERR_InsideRecognitionFailed	( CSPOTTER_ERR_SDKError -	38 )

#define CSPOTTER_ERR_Expired					( CSPOTTER_ERR_SDKError - 100 )
#define CSPOTTER_ERR_LicenseFailed				( CSPOTTER_ERR_SDKError - 200 )








#endif //__CSPOTTER_SDK_API_CONST_H__

