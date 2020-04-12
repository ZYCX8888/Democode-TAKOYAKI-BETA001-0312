#ifndef __CSPOTTERSDK_API_H
#define __CSPOTTERSDK_API_H

#if defined(_WIN32)
	#ifdef CSpotterDll_EXPORTS
		#define CSPDLL_API __declspec(dllexport)
	#endif
#endif

#ifndef CSPDLL_API
#define CSPDLL_API
#endif

#include "base_types.h"
#include "CSpotterSDKApi_Const.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct _CSpotterSDKVersionInfo
{
	const char* pchSDKName;
	const char* pchSDKVersion;
	const char* pchSDKType;
	const char* pchReleaseDate;
	char* pchLicenseType;	
	BOOL  bTrialVersion;
} CSpotterSDKVersionInfo;



/************************************************************************/
//  Listener Callback Function
/************************************************************************/
//Purpose:				This callback function will be called when a recognition result is recognized, either accepted or rejected, is found, and some information about it will be passed to the application. This callback is usually used for debug purpose.
//nRecogType(OUT):		The recognition result type.
//pwcResult(OUT):		The string of the recognized command.
//nCmdIndex(OUT):		The index of the recognized command.
//nResGMM(OUT):			The result GMM (or background model) score.
//nResSG(OUT):			The result SG (Silence and Garbage models) score.
//nThresholdGMM(OUT):	The GMM score threshold.
//nThresholdSG(OUT):	The SG score threshold.
//lpParam(IN):			The user data.
typedef INT (*CSpotter_RecogInfo_Callback)(
	INT nRecogType,
	UNICODE *pwcResult,
	INT nCmdIndex,
	INT nResGMM,
	INT nResSG,
	INT nThresholdGMM,
	INT nThresholdSG,
	VOID *lpParam
);



/************************************************************************/
//  Initialization and Release Functions
/************************************************************************/

//Purpose:					Create a CSpotter recognizer handle.
//lpchEngineLibFile(IN):	The full path of engine library.
//lpchCommandFile(IN):		The full path of command model file.
//lpchLicenseFile(IN):		The full path of license file.
//lpnErr(OUT):				Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code. This parameter can be NULL.
//Return:					Return the CSpotter recognizer handle when success, Otherwise return NULL.
CSPDLL_API HANDLE CSpotter_InitWithFiles(
	char *lpchEngineLibFile,
	char *lpchCommandFile,
	char *lpchLicenseFile,
	INT *lpnErr
);

//Purpose:					Create a CSpotter recognizer handle with Filler model.
//lpchEngineLibFile(IN):	The full path of engine library.
//lpchCommandFile(IN):		The full path of command model file.
//lpchLicenseFile(IN):		The full path of license file.
//lpchFillerFile(IN):       The full path of filler file.
//lpnErr(OUT):				Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code. This parameter can be NULL.
//Return:					Return the CSpotter recognizer handle when success, Otherwise return NULL.
CSPDLL_API HANDLE CSpotter_InitWithFiles_Filler(
	char *lpchEngineLibFile,
	char *lpchCommandFile,
	char *lpchLicenseFile,
	char *lpchFillerFile,
	INT *lpnErr
);

//Purpose:					Create a CSpotter recognizer handle.
//lpchEngineLibFile(IN):	The full path of engine library. 
//lpbyBufCmdModel(IN):		The buffer of command model.
//nBufCmdModelSize(IN):		The size of command model.
//lpnErr(OUT):				Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code. This parameter can be NULL.
//Return:					Return the CSpotter recognizer handle when success, Otherwise return NULL.
CSPDLL_API HANDLE CSpotter_Init(
	char *lpchEngineLibFile,
	BYTE *lpbyBufCmdModel,
	INT nBufCmdModelSize,
	INT *lpnErr
);

//Purpose:			Release the CSpotter recognizer handle and its resources.
//hCSpotter(IN):	The CSpotter recognizer handle. 
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_Release(
	HANDLE hCSpotter
);



/************************************************************************/
//  Configuration Functions
/************************************************************************/

//Purpose:				Set callback function for getting information of command which was spotted.
//hCSpotter(IN):		The CSpotter recognizer handle.
//lpfnCallback(IN):		When there was a recognized command, either accepted or rejected, this callback function will be invoked to send information of command. This parameter can be NULL.
//lpParam(IN):			The user data.
//Return:				Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_DumpRecogInfo(
	HANDLE hCSpotter,
	CSpotter_RecogInfo_Callback lpfnCallback,
	VOID *lpParam
);

//Purpose:			Set timeout value to inform user that timeout situation occurred. When timeout occurred, function CSpotter_AddSample will return value CSPOTTER_ERR_Timeout once, and timeout mechanism will be closed.
//hCSpotter(IN):	The CSpotter recognizer handle.
//nTimeout(IN):		The timeout value in milliseconds. The default timeout value is infinite. 
//Return:			Return nTimeout when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_SetTimeout(
	HANDLE hCSpotter,
	INT nTimeout
);

//Purpose:				Set rejection level of the CSpotter recognizer handle. Higher rejection level can reduce false alarm rate, but on the other hand, it may probably reduce accuracy rate. 
//hCSpotter(IN):		The CSpotter recognizer handle.
//nRejectionLevel(IN):	The rejection level ranges from -100 to 100, and a reasonably suggested range is -20 to 20. The default value is 0.
//Return:				Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_SetRejectionLevel(
	HANDLE hCSpotter,
	INT nRejectionLevel
);

//Purpose:			Set the minimal energy for the recognized commands to be accepted. This can effectively reject commands falsely triggered by the background noise of relatively low volume. Higher threshold can result in less false triggers, but may require users to speak louder for their voice commands to pass the energy threshold.
//hCSpotter(IN):	The CSpotter recognizer handle.
//nEnergyTh(IN):	The energy threshold ranges from 0 to 8000. The default value is 0, which disables this rejection mechanism.
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_SetEnergyThreshold(
	HANDLE hCSpotter,
	INT nEnergyTh
);

//Purpose:			Switch engine mode (between CSPOTTER_Mode_TriggerCmd and CSPOTTER_Mode_KeywordSpotting).
//hCSpotter(IN):	The CSpotter recognizer handle.
//nMode(IN):		CSPOTTER_Mode_TriggerCmd(default) or CSPOTTER_Mode_KeywordSpotting.
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_SetKeywordSpottingMode(
	HANDLE hCSpotter,
	INT nMode
);

//Purpose:			Set score of a command
//hCSpotter(IN):	The CSpotter recognizer handle.
//nWordIdx(IN):		The command ID. It is zero based.
//nScore(IN):		The score ranges from -100 to 100. The higher value will increase the accuracy rate and false alarm rate.
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_SetCmdScore(
	HANDLE hCSpotter,
	INT nWordIdx,
	INT nScore
);

//Purpose:				Set identity level of a voice tag. This function only work on the voice tag which has SV feature.
//hCSpotter(IN):		The CSpotter recognizer handle.
//nIdentityLevel(IN):	The identity level ranges from -100 to 100. The default value is 0. The larger value, the harder to pass through.
//Return:				Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_SetIdentityLevel(
	HANDLE hCSpotter,
	INT nIdentityLevel
);

//Purpose:					Set acceleration level of engine. Please noted that it may reduce the recognition performance.
//hCSpotter(IN):			The CSpotter recognizer handle.
//nAccelerationLevel(IN):	The acceleration level ranges from 0 to 4. The default value is 0. The larger value, the faster computing speed it has.
//Return:					Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_SetAccelerationLevel(
	HANDLE hCSpotter,
	INT nAccelerationLevel
);

//Purpose:			Set rejected result threshold.
//hCSpotter(IN):	The CSpotter recognizer handle.
//nEnergyTh(IN):	The energy threshold ranges from 0 to 30000. The default value is 500. The higher value, the harder to pass through.
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_SetRejectedResultEnergyThreshold(
	HANDLE hCSpotter,
	INT nEnergyTh
);

//Purpose:			Set partial rejection threshold.
//hCSpotter(IN):	The CSpotter recognizer handle.
//nGapTh(IN):		The rejection gap ranges from 0 to 200. The smaller value, the stronger rejection ability. Assign negative value to disable this rejection mechanism and the default setting is disabled.
//					The recommended value is 75.
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_SetPartialRejectGap(
	HANDLE hCSpotter,
	INT nGapTh
);

//Purpose:			Set number of GMM Gauss of partial rejection.
//hCSpotter(IN):	The CSpotter recognizer handle.
//nStateNum(IN):	The number of GMM Gauss.
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
//Remark:			Internal debug.
CSPDLL_API INT CSpotter_SetPartialRejectStateNum(
	HANDLE hCSpotter,
	INT nStateNum
);

//Purpose:					Set setting file.
//hCSpotter(IN):			The CSpotter recognizer handle.
//lpchSettingFileName(IN):	The full path of setting file.
//Return:					Return TRUE when success, Otherwise return FALSE.
//Remark:					Internal debug.
CSPDLL_API BOOL CSpotter_SetSettingFile(
	HANDLE hCSpotter,
	char *lpchSettingFile
);



/************************************************************************/
//  Getting Functions
/************************************************************************/

//Purpose:			Get current energy threshold.
//hCSpotter(IN):	The CSpotter recognizer handle.
//Return:			Return energy threshold when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_GetEnergyThreshold(
	HANDLE hCSpotter
);

//Purpose:					Get several detailed information about SDK. 
//lpSDKVersionInfo(OUT):	There are following variables: pchSDKName, pchSDKVersion, pchSDKType, pchReleaseDate, pchLicenseType, and bTrialVersion to get SDK name, version number, SDK type, release date, license type, and version status respectively. For pchSDKType, the returned string is "Standard", "Advanced", or "Professional".
//Return:					Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_GetVersionInfo(
	char *lpchLicenseFile,
	CSpotterSDKVersionInfo *lpSDKVersionInfo
);

//Purpose:			Get string which is encoded with UTF-16 of a recognition command by using command ID.
//hCSpotter(IN):	The CSpotter recognizer handle.
//nID(IN):			The command ID. It is zero based.
//lpwBuffer(OUT):	The string(UTF-16) of recognition command whose the command ID is nID.
//Return:			Return length of string(UTF-16) of recognition command when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_GetCommand(
	HANDLE hCSpotter,
	INT nID,
	UNICODE *lpwBuffer
);

//Purpose:			Get string which is encoded with UTF-8 of a recognition command by using command ID.
//hCSpotter(IN):	The CSpotter recognizer handle.
//nID(IN):			The command ID.It is zero based.
//lpwBuffer(OUT):	The string(UTF-8) of recognition command whose the command ID is nID.
//Return:			Return length of string(UTF-8) of recognition command when success, Otherwise, return a negative error code.
CSPDLL_API INT CSpotter_GetUTF8Command(
	HANDLE hCSpotter,
	INT nID,
	BYTE *lpbyUTF8Buffer
);

//Purpose:			Get number of recognition commands of recognition command model.
//hCSpotter(IN):	The CSpotter recognizer handle.
//Return:			Return number of recognition commands, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_GetCommandNumber(
	HANDLE hCSpotter
);

//Purpose:			Get sampling rate of recognition command model. Only supported 16KHz and 8KHz.
//hCSpotter(IN):	The CSpotter recognizer handle.
//Return:			Return sampling rate of recognition command model when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_GetSampleRate(
	HANDLE hCSpotter
);

//Purpose:			Get partial rejection threshold.
//hCSpotter(IN):	The CSpotter recognizer handle.
//Return:			Return rejection gap when success, Otherwise return a negative error code. If the return value equals -1, it means that the rejection mechanism is disabled.
CSPDLL_API INT CSpotter_GetPartialRejectGap(
	HANDLE hCSpotter
);

//Purpose:			Get score of a command.
//hCSpotter(IN):	The CSpotter recognizer handle.
//nWordIdx(IN):		The command ID.It is zero based.
//lpnErr(OUT):		Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
//Return:			Return score of a command.
CSPDLL_API INT CSpotter_GetCmdScore(
	HANDLE hCSpotter,
	INT nWordIdx,
	INT *lpnErr
);

//Purpose:					Get information of model file. These values are pre-build in the model file.
//							Only Support model version >= 7 !!!!
//hCSpotter(IN):			The CSpotter recognizer handle.
//lpnPlatForm(OUT):			The platform.
//lpnSampleRate(OUT):		The sampling rate.
//lpnFrameSize(OUT):		The frame size.
//lpnFrameStep(OUT):		The frame step.
//lpnDim(OUT):				The dimension.
//lpnRejectLevel(OUT):		The reject level.
//lpnResponseTime(OUT):		The response time.
//lpnFreqWarp(OUT):			The frequency warp factor.
//lpnEnergyThreshold(OUT):	The energy threshold.
//Return:					Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_GetModelInfo(
	HANDLE hCSpotter,
	INT *lpnPlatForm,
	INT *lpnSampleRate,
	INT *lpnFrameSize,
	INT *lpnFrameStep,
	INT *lpnDim,
	INT *lpnRejectLevel,
	INT *lpnResponseTime,
	INT *lpnFreqWarp,
	INT *lpnEnergyThreshold
);

//Purpose:			Get number of GMM Gauss
//hCSpotter(IN):	The CSpotter recognizer handle.
//Return:			Return number of GMM Gauss when success, Otherwise return a negative error code.
//Remark:			Internal debug.
CSPDLL_API INT CSpotter_GetGMMGaussNum(
	HANDLE hCSpotter
);

//Purpose:			Get ThrdAlphaGap. 
//					After invoking function CSpotter_SetRejectionLevel, ThrdAlphaGap value should have changed.
//					The higher rejection level, the higher AlphaGap value.
//hCSpotter(IN):	The CSpotter recognizer handle.
//lpnErr(OUT):		Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
//Return:			Return ThrdAlphaGap value.
//Remark:			Internal debug.
CSPDLL_API INT CSpotter_GetThrdAlphaGap(
	HANDLE hCSpotter,
	INT *lpnErr
);

//Purpose:			Get number of GMM Gauss of partial rejection.
//hCSpotter(IN):	The CSpotter recognizer handle.
//Return:			Return number of GMM Gauss when success, Otherwise return a negative error code.
//Remark:			Internal debug.
CSPDLL_API INT CSpotter_GetPartialRejectStateNum(
	HANDLE hCSpotter
);



/************************************************************************/
//  Recognition Functions
/************************************************************************/

//Purpose:			Clear recognition status. To invoke this function after invoking function CSpotter_InitWithFiles.
//hCSpotter(IN):	The CSpotter recognizer handle.
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_Reset(
	HANDLE hCSpotter
);

//Purpose:			Transfer voice samples to the recognizer handle for recognition.
//hCSpotter(IN):	The CSpotter recognizer handle.
//lpsSample(IN):	The buffer of voice data.
//nNumSamples(IN):	The number of voice data(The size of a sample is 2 bytes).
//Return:			There are return types as follows:
//					CSPOTTER_SUCCESS -> This indicates a voice command was recognized. Application can invoke function CSpotter_GetResult to get the result.
//					CSPOTTER_ERR_NeedMoreSample -> This indicates no recognition result yet, and needs to invoke this function again to add more samples to the recognizer.
//					CSPOTTER_ERR_Timeout -> This indicates timeout situation occurred.
//					Other negative error code.
CSPDLL_API INT CSpotter_AddSample(
	HANDLE hCSpotter,
	SHORT *lpsSample,
	INT nNumSamples
);
CSPDLL_API INT CSpotter_AddSample_SilCMSOnly(
	HANDLE hCSpotter,
	SHORT *lpsSample,
	INT nNumSamples
);

//Purpose:			Add voice features to the recognizer handle for recognition.
//hCSpotter(IN):	The CSpotter recognizer handle.
//lpnFeature(IN):	The buffer of voice features.
//nFeatureLen(IN):	The number of voice features.
//lpnRMS(IN):		The buffer of voice RMS.
//Result:			There are return types as follows:
//					CSPOTTER_SUCCESS -> This indicates a voice command was recognized. Application can invoke function CSpotter_GetResult to get the result.
//					CSPOTTER_ERR_NeedMoreSample -> This indicates no recognition result yet, and needs to invoke this function again to add more features to the recognizer.
//					Other negative error code. 
CSPDLL_API INT CSpotter_AddFeature(
	HANDLE hCSpotter,
	INT *lpnFeature,
	INT nFeatureLen,
	INT *lpnRMS
);

//Purpose:				Get recognized command which is encoded with UTF-16.
//hCSpotter(IN):		The CSpotter recognizer handle.
//lpnResultID(OUT):		The ID of recognized command. It can be NULL.
//lpwcResult(OUT):		The recognized command which is encoded with UTF-16. Please noted that length of buffer must greater to length of result, and this buffer is allocated by outside. It can be NULL.
//Return:				Return length of recognized command, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_GetResult(
	HANDLE hCSpotter,
	INT *lpnResultID,
	UNICODE *lpwcResult
);

//Purpose:				Get recognized command which is encoded with UTF-8.
//hCSpotter(IN):		The CSpotter recognizer handle.
//lpnResultID(OUT):		The ID of recognized command. It can be NULL.
//lpbyUTF8Result(OUT):	The recognized command which is encoded with UTF-8. Please noted that length of buffer must greater to length of result, and this buffer is allocated by outside. It can be NULL.
//Return:				Return length of the recognized command, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_GetUTF8Result(
	HANDLE hCSpotter,
	INT *lpnResultID,
	BYTE *lpbyUTF8Result
);



/************************************************************************/
//  Voice Detection Functions
/************************************************************************/

//Purpose:			A voice activity detection algorithm has been embedded internally in the CSpotter engine. Application can invoke this function to check if there has been any voice spoken by the user since last time starting recognition or function CSpotter_ResetVAD was invoked.
//hCSpotter(IN):	The CSpotter recognizer handle.
//Return:			Return 1 if voice has been detected, return 0 if no voice is detected, or return negative error code.
CSPDLL_API INT CSpotter_HasVoice(
	HANDLE hCSpotter
);

//Purpose:			Invoke this function to reset the voice activity detection.
//hCSpotter(IN):	The CSpotter recognizer handle.
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT  CSpotter_ResetVAD(
	HANDLE hCSpotter
);



/************************************************************************/
//  Command Creation Functions
/************************************************************************/

//Purpose:				Create a command model file by inputting text commands which are encoded with UTF-16. This function is available only in the professional version.
//lpchLicenseFile(IN):  The full path of license file.
//lpchLibPath(IN):		The directory path of Lexicon library.
//lpchDataPath(IN):		The directory path for CSpotter language data, in which there is a list of folders, and each folder contains the data files for a language. 
//lpchEinFile(IN):		The full path of CSpotter language file(*.ein).
//lppwcCommands(IN):	An array of strings which are encoded with UTF-16 of the commands, for which acoustic models are created.
//nCmdNum(IN):			The number of commands.
//lpchOutBinFile(OUT):	The full path of output command model. This is a normal command model file that can be used to initialize a CSpotter handle and merged with other command model using function CSpotter_MergeCommand.
//Return:				Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_BuildCommandWithFiles(
	char *lpchLicenseFile,
	char *lpchLibPath,
	char *lpchDataPath,
	char *lpchEinFile,
	UNICODE **lppwcCommands,
	INT nCmdNum,
	char *lpchOutBinFile
);

//Purpose:					Create a command model file by inputting text commands which are encoded with UTF-8. This function is available only in the professional version.
//lpchLicenseFile(IN):		The full path of license file.
//lpchLibPath(IN):			The directory path of lexicon library.
//lpchDataPath(IN):			The directory path for CSpotter language data, in which there is a list of folders, and each folder contains the data files for a language. 
//lpchEinFile(IN):			The full path of CSpotter language file(*.ein).
//lppbyUTF8Commands(IN):	An array of strings which are encoded with UTF-8 of the commands, for which acoustic models are created.
//nCmdNum(IN):				The number of commands.
//lpchOutBinFile(OUT):		The full path of output command model. This is a normal command model file that can be used to initialize a CSpotter handle and merged with other command model using function CSpotter_MergeCommand.
//Return:					Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_BuildUTF8CommandWithFiles(
	char *lpchLicenseFile,
	char *lpchLibPath,
	char *lpchDataPath,
	char *lpchEinFile,
	BYTE **lppbyUTF8Commands,
	INT nCmdNum,
	char *lpchOutBinFile
);

//Purpose:							Create a command model file by inputting wave files. This function is available in the advanced version and professional version.
//lpchLicenseFile(IN):				The full path of license file.
//lpchCVTLibFile(IN):				The full path of voice tag library.
//lpchNINJALibFile(IN):				The full path of engine library. 
//lpchToolDefCmdFile(IN):			The full path of the tool-defined command model file.
//lpchCMSFile(IN):					The full path of the CMS file. It can be NULL.
//lppchWaveFiles(IN):				An array of full path of the wave files, for which acoustic models are created.
//nNumWaveFile(IN):					The number of wave files.
//lpwcVoiceTagName(IN):				The name which is encoded with UTF-16 of user-defined command, for which acoustic models are created.
//lpchOutputUserDefCmdFile(IN):		The full path of output command model. This is a normal command model file that can be used to initialize a CSpotter handle and merged with other command model using function CSpotter_MergeCommand.
//nSampleRate(IN):					The recorded sampling rate of wave files. It must be same as that of the tool-defined command model file.
//bUseSV(IN):						The flag of feature of speaker verification.
//Return:							Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT  CSpotter_BuildUserDefCommandWithFiles(
	char *lpchLicenseFile,
	char *lpchCVTLibFile,
	char *lpchNINJALibFile,
	char *lpchToolDefCmdFile,
	char *lpchCMSFile,
	char *lppchWaveFiles[],
	INT nNumWaveFile,
	UNICODE *lpwcVoiceTagName,
	char *lpchOutputUserDefCmdFile,
	INT nSampleRate,
	BOOL bUseSV
);

//Purpose:							Create a command model file by inputting wave files. This function is available in the advanced version and professional version.
//lpchLicenseFile(IN):				The full path of license file.
//lpchCVTLibFile(IN):				The full path of voice tag library.
//lpchNINJALibFile(IN):				The full path of engine library. 
//lpchToolDefCmdFile(IN):			The full path of the tool-defined command model file.
//lpchCMSFile(IN):					The full path of the CMS file. It can be NULL.
//lppchWaveFiles(IN):				An array of full path of the wave files, for which acoustic models are created.
//nNumWaveFile(IN):					The number of wave files.
//lpbyUTF8VoiceTagName(IN):			The name which is encoded with UTF-8 of user-defined command, for which acoustic models are created.
//lpchOutputUserDefCmdFile(IN):		The full path of output command model. This is a normal command model file that can be used to initialize a CSpotter handle and merged with other command model using function CSpotter_MergeCommand.
//nSampleRate(IN):					The recorded sampling rate of wav files. It must be same as that of the tool-defined command model file.
//bUseSV(IN):						The flag of feature of speaker verification.
//Return:							Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT  CSpotter_BuildUTF8UserDefCommandWithFiles(
	char *lpchLicenseFile,
	char *lpchCVTLibFile,
	char *lpchNINJALibFile,
	char *lpchToolDefCmdFile,
	char *lpchCMSFile,
	char *lppchWaveFiles[],
	INT nNumWaveFile,
	BYTE *lpbyUTF8VoiceTagName,
	char *lpchOutputUserDefCmdFile,
	INT nSampleRate,
	BOOL bUseSV
);

//Purpose:								Create a command model for the input wave streams. This function is available in the advanced version and professional version.
//lpchCVTLibFile(IN):					The full path of voice tag library.
//lpchNINJALibFile(IN):					The full path of engine library. 
//lpbyBufToolDefModel(IN):				The buffer of tool-defined model.
//nBufToolDefModelSize(IN):				The size of tool-defined model.
//lppbyBufWave(IN):						An array of wave streams which are included wave header.
//lpnBufWaveSize(IN):					The size of wave streams.
//nNumBufWave(IN):						The number of wave streams.
//lpwcVoiceTagName(IN):					The name which is encoded with UTF-16 of user-defined command, for which acoustic models are created.
//lpbyBufOutputUserDefCmdModel(OUT):	The buffer of output command model. This is a normal command model that can be used to initialize a CSpotter handle and merged with other command model using class function CSpotter_MergeCommandModel.
//nBufOutputUserDefCmdModelSize(OUT):	The size of output command model.
//nSampleRate(IN):						The recorded sampling rate of wave files. It must be same as that of the tool-defined command model.
//bUseSV(IN):							The flag of feature of speaker verification.
//Return:								Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT  CSpotter_BuildUserDefCommand(
	char *lpchCVTLibFile,
	char *lpchNINJALibFile,
	BYTE *lpbyBufToolDefModel,
	INT nBufToolDefModelSize,
	BYTE *lppbyBufWave[],
	INT *lpnBufWaveSize,
	INT nNumBufWave,
	UNICODE *lpwcVoiceTagName,
	BYTE *lpbyBufOutputUserDefCmdModel,
	INT *lpnBufOutputUserDefCmdModelSize,
	INT nSampleRate,
	BOOL bUseSV
);

//Purpose:					Merge two command model files into one. The input command model files can be created by CSpotter Model Tool,  by command creation function CSpotter_Build[UTF8]UserDefCmdFile, or by command creation function CSpotter_Build[UTF8]Command.
//lpchLicenseFile(IN):		The full path of license file.
//lpchNINJALibFile(IN):		The full path of engine library. 
//lpchCmdFile1:				The full path of the first input command model file.
//lpchCmdFile2:				The full path of the second input command model file.
//lpchOutputMergedCmdFile:	The full path of the output merged command model file.
//Return:					Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT  CSpotter_MergeCommandWithFiles(
	char *lpchLicenseFile,
	char *lpchNINJALibFile,
	char *lpchCmdFile1,
	char *lpchCmdFile2,
	char *lpchOutputMergedCmdFile
);

//Purpose:								Merge two command models into one. The input command models can be created by CSpotter Model Tool,  by command creation function CSpotter_BuildUserDefCommandModel.
//lpchNINJALibFile(IN):					The full path of engine library. 
//lpbyBufCmdModel1(IN):					The buffer of the first input command model.
//nBufCmdModel1Size(IN):				The size of the first input command model.
//lpbyBufCmdModel2(IN):					The buffer of the second input command model.
//nBufCmdModel2Size(IN):				The size of the second input command model.
//lpbyBufOutputMergedCmdModel(OUT):		The buffer of output merged command model.
//nBufOutputMergedCmdModelSize(OUT):	The size of output merged command model.
//Return:								Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT  CSpotter_MergeCommand(
	char *lpchNINJALibFile,
	BYTE *lpbyBufCmdModel1,
	INT nBufCmdModel1Size,
	BYTE *lpbyBufCmdModel2,
	INT nBufCmdModel2Size,
	BYTE *lpbyBufOutputMergedCmdModel,
	INT *lpnBufOutputMergedCmdModelSize
);



/************************************************************************/
//  Speaker Functions
/************************************************************************/

//Purpose:						Set trained speaker model files to enable feature of speaker verification. To invoke this function after invoking function CSpotter_InitWithFiles.
//hCSpotter(IN):				The CSpotter recognizer handle.
//lpchCSVGMMLibFile(IN):		The full path of CSVGMM library.
//lpchSIModelFile(IN):			The full path of base speaker model file CSVGMM_SI_16k.mod.
//lppchSpeakerModelFile(IN):	An array of full path of trained speaker model files.
//nNumSpeaker(IN):				The number of trained speaker model files.
//nSampleRate(IN):				Only support 16KHz sampling rate at present.
//Return:						Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_Speaker_SetSpeakerWithFiles(
	HANDLE hCSpotter,
	char *lpchCSVGMMLibFile,
	char *lpchSIModelFile,
	char *lppchSpeakerModelFile[],
	INT nNumSpeaker,
	INT nSampleRate
);

//Purpose:						Set trained speaker models to enable feature of speaker verification. To invoke this function after invoking function CSpotter_Init.
//hCSpotter(IN):				The CSpotter recognizer handle.
//lpchCSVGMMLibFile(IN):		The full path of CSVGMM library.
//lpbySIModel(IN): 				The buffer of base speaker model
//lppbySpeakerModel(IN):		An array of buffer of trained speaker models.
//lpnSpeakerModelSize(IN):		The size of trained speaker models.
//nNumSpeaker(IN):				The number of trained speaker models.
//nSampleRate(IN):				Only support 16KHz sampling rate at present.
//Return:						Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_Speaker_SetSpeaker(
	HANDLE hCSpotter,
	char *lpchCSVGMMLibFile,
	BYTE *lpbySIModel,
	BYTE *lppbySpeakerModel[],
	INT  *lpnSpeakerModelSize,
	INT nNumSpeaker,
	INT nSampleRate
);

//Purpose:					Get speaker tag which is encoded with UTF-16 of verified speaker whenever a voice command was recognized. To invoke this function after invoking function CSpotter_GetResult.
//hCSpotter(IN):			The CSpotter recognizer handle. 
//lpnSpeakerID(OUT):		The ID of verified speaker. It can be NULL.
//lpwcSpeakerTag(OUT):		The tag which is encoded with UTF-16 of verified speaker. It can be NULL.
//lpnScore(OUT):			The best score. It can be NULL.
//Return:					Return length of speaker tag of verified speaker, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_Speaker_GetSpeaker(
	HANDLE hCSpotter,
	INT *lpnSpeakerID,
	UNICODE *lpwcSpeakerTag,
	INT *lpnScore
);

//Purpose:					Get speaker tag which is encoded with UTF-8 of verified speaker whenever a voice command was recognized. To invoke this function after invoking function CSpotter_GetResult.
//hCSpotter(IN):			The CSpotter recognizer handle. 
//lpnSpeakerID(OUT):		The ID of verified speaker. It can be NULL.
//lpbyUTF8SpeakerTag(OUT):	The tag which is encoded with UTF-8 of verified speaker. It can be NULL.
//lpnScore(OUT):			The best score. It can be NULL.
//Return:					Return length of speaker tag of verified speaker, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_Speaker_GetUTF8Speaker(
	HANDLE hCSpotter,
	INT *lpnSpeakerID,
	BYTE *lpbyUTF8SpeakerTag,
	INT *lpnScore
);

//Purpose:			Enable speaker verification.
//hCSpotter(IN):	The CSpotter recognizer handle. 
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_Speaker_Enable(
	HANDLE hCSpotter
);

//Purpose:			Disable speaker verification.
//hCSpotter(IN):	The CSpotter recognizer handle.
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_Speaker_Disable(
	HANDLE hCSpotter
);

//Purpose:				Set rejection threshold of speaker verification.
//hCSpotter(IN):		The CSpotter recognizer handle.
//nThreshold(INT):		The rejection threshold ranges from -50 to 50. The default value is 0. The larger value, the harder to pass through.
//Return:				Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_Speaker_SetRejectThreshold(
	HANDLE hCSpotter,
	INT nThreshold
);

//Purpose:			Get rejection threshold of speaker verification.
//hCSpotter(IN):	The CSpotter recognizer handle.
//Return:			Return value of rejection threshold when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_Speaker_GetRejectThreshold(
	HANDLE hCSpotter
);

//Purpose:			Detect whether a speaker was recognized or not.
//hCSpotter(IN):	The CSpotter recognizer handle.
//Return:			Return TRUE or FALSE when success, Otherwise return a negative error code.
CSPDLL_API BOOL CSpotter_Speaker_IsGetSpeaker(
	HANDLE hCSpotter
);

//Purpose:							Train speaker verification model file for one person.
//lpchLicenseFile(IN):				The full path of license file.
//lpchNINJALibFile(IN):				The full path of NINJA library.
//lpchCSVGMMLibFile(IN):			The full path of CSVGMM library.
//lpchSIModelFile(IN):				The full path of SI model file.
//lpchCommandFile(IN):				The full path of command file.
//lpchCMSFile(IN):					The full path of CMS file. It can be NULL.
//lppchWaveFiles(IN):				An array of full path of the wave files, for which speaker model is created.
//nNumWaveFile(IN):					The number of wave files.
//lpchOutputSpeakerModelFile(IN):	The full path of output trained model file.
//lpwcSpeakerTag(IN):				The speaker tag which is encoded with UTF-16, and its maximal length is 255.
//nSampleRate(IN):					Only support 16KHz sampling rate at present.
//Return:							Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_Speaker_TrainWithFiles(
	char *lpchLicenseFile,
	char *lpchNINJALibFile,
	char *lpchCSVGMMLibFile,
	char *lpchSIModelFile,
	char *lpchCommandFile,
	char *lpchCMSFile,
	char *lppchWaveFiles[],
	INT nNumWaveFile,
	char *lpchOutputSpeakerModelFile,
	UNICODE *lpwcSpeakerTag,
	INT nSampleRate
);

//Purpose:							Train speaker verification model file for one person.
//lpchLicenseFile(IN):				The full path of license file.
//lpchNINJALibFile(IN):				The full path of NINJA library.
//lpchCSVGMMLibFile(IN):			The full path of CSVGMM library.
//lpchSIModelFile(IN):				The full path of SI model file.
//lpchCommandFile(IN):				The full path of command file.
//lpchCMSFile(IN):					The full path of CMS file. It can be NULL.
//lppchWaveFiles(IN):				An array of full path of the wave files, for which speaker model is created.
//nNumWaveFile(IN):					The number of wave files.
//lpchOutputSpeakerModelFile(IN):	The full path of output trained model file.
//lpbyUTF8SpeakerTag(IN):			The speaker tag which is encoded with UTF-8, and its maximal length is 255.
//nSampleRate(IN):					Only support 16KHz sampling rate at present.
//Return:							Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_Speaker_UTF8TrainWithFiles(
	char *lpchLicenseFile,
	char *lpchNINJALibFile,
	char *lpchCSVGMMLibFile,
	char *lpchSIModelFile,
	char *lpchCommandFile,
	char *lpchCMSFile,
	char *lppchWaveFiles[],
	INT nNumWaveFile,
	char *lpchOutputSpeakerModelFile,
	BYTE *lpbyUTF8SpeakerTag,
	INT nSampleRate
);

//Purpose:							Train speaker verification model for one person.
//lpchNINJALibFile(IN):				The full path of NINJA library.
//lpchCSVGMMLibFile(IN):			The full path of CSVGMM library.
//lpbySIModel(IN):					The buffer of SI Model.
//nSIModelSize(IN):					The size of SI model.
//lpbyCommandModel(IN):				The buffer of command model.
//nCommandModelSize(IN):			The size of command model.
//lppbyBufWave(IN):					An array of wave streams which are included wave header.
//lpnBufWaveSize(IN):				The size of wave streams.
//nNumBufWave(IN):					The number of wave streams.
//lpbyOutputSpeakerModel(OUT):		The The buffer of output trained model.
//lpnOutputSpeakerModelSize(OUT):	The size of output trained model.
//lpwcSpeakerTag(IN):				The speaker tag which is encoded with UTF-16, and its maximal length is 255.
//nSampleRate(IN):					Only support 16KHz sampling rate at present.
//Return:							Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_Speaker_Train(
	char *lpchNINJALibFile,
	char *lpchCSVGMMLibFile,
	BYTE *lpbySIModelFile,
	INT nSIModelSize,
	BYTE *lpbyCommandModel,
	INT nCommandModelSize,
	BYTE *lppbyBufWave[],
	INT *lpnBufWaveSize,
	INT nNumBufWave,
	BYTE *lpbyOutputSpeakerModel,
	INT *lpnOutputSpeakerModelSize,
	UNICODE *lpwcSpeakerTag,
	INT nSampleRate
);



/************************************************************************/
//  Read/Write CMS Functions
/************************************************************************/
//Purpose:				Get CMS data.
//hCSpotter(IN):		The CSpotter recognizer handle.
//lpnMean(OUT):			An array of CMS. If lpnMean is NULL, the function will return length of CMS.
//lpnTypeVoice(OUT):	An array of type voice, and the array element is 1.
//Return:				Return length of CMS, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_GetCMS(
	HANDLE hCSpotter,
	INT *lpnMean,
	INT *lpnTypeVoice
);

//Purpose:			Set CMS data.
//hCSpotter(IN):	The CSpotter recognizer handle.
//lpnMean(OUT):		An array of CMS.
//nTypeVoice(IN):	The type voice.
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative integer error code.
CSPDLL_API INT CSpotter_SetCMS(
	HANDLE hCSpotter,
	INT *lpnMean,
	INT nTypeVoice
);

//Purpose:				Save CMS data to file.
//hCSpotter(IN):		The CSpotter recognizer handle.
//lpchCMSFile(IN):		The full path of CMS file.
//Return:				Return CSPOTTER_SUCCESS when success, Otherwise return a negative integer error code.
CSPDLL_API INT CSpotter_SaveCMSToFile(
	HANDLE hCSpotter,
	char *lpchCMSFile
);

//Purpose:				Load CMS data from file.
//hCSpotter(IN):		The CSpotter recognizer handle.
//lpchCMSFile(IN):		The full path of CMS file.
//Return:				Return CSPOTTER_SUCCESS when success, Otherwise return a negative integer error code.
CSPDLL_API INT CSpotter_LoadCMSFromFile(
	HANDLE hCSpotter,
	char *lpchCMSFile
);



/************************************************************************/
//  WaveToFea Module Functions
/************************************************************************/

//Purpose:		Create a WaveToFea handle.
//nSampleRate:	The recorded sampling rate of Audio. It must be same as that of the command model file.
//Return:		Return the WaveToFea handle when success, Otherwise return NULL.
CSPDLL_API HANDLE CSpotter_WaveToFeaInit(
	INT nSampleRate
);

//Purpose:			Release the WaveToFea handle and its resources.
//hWaveToFea(IN):	The WaveToFea handle. 
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_WaveToFeaRelease(
	HANDLE hWaveToFea
);

//Purpose:			Start WaveToFea module.
//hWaveToFea(IN):	The WaveToFea handle. 
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_WaveToFeaStartWaveToFea(
	HANDLE hWaveToFea
);

//Purpose:			Add voice samples to WaveToFea module.
//hWaveToFea(IN):	The WaveToFea handle. 
//lpsSample(IN):	The buffer of voice data.
//nNumSamples(IN):	The number of voice data(The size of a sample is 2 bytes).
//Return:			Return non negative number when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_WaveToFeaAddSample(
	HANDLE hWaveToFea,
	SHORT *lpsSample,
	INT nNumSamples
);

//Purpose:			Detect whether start point is found or not.
//hWaveToFea(IN):	The WaveToFea handle. 
//Return:			Return TRUE when success, Otherwise return FALSE.
CSPDLL_API BOOL CSpotter_WaveToFeaIsStart(
	HANDLE hWaveToFea
);

//Purpose:			Detect whether end point is found or not.
//hWaveToFea(IN):	The WaveToFea handle. 
//Return:			Return TRUE for success, Otherwise return FALSE.
CSPDLL_API BOOL CSpotter_WaveToFeaIsFinish(
	HANDLE hWaveToFea
);

//Purpose:			Set length of end point.
//hWaveToFea(IN):	The WaveToFea handle. 
//nFrame(IN):		The length of end point(A unit is 10ms).
//Return:			Return non negative number when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_WaveToFeaSetCheckLengthEPD(
	HANDLE hWaveToFea,
	INT nFrame
);

//Purpose:			Add voice samples to WaveToFea module and get voice features and RMS value.
//hWaveToFea(IN):	The WaveToFea handle. 
//lpsSample(IN):	The buffer of voice data.
//nNumSample(IN):	The number of voice data(The size of a sample is 2 bytes).
//lpnFea(OUT):		The buffer of voice features.
//					If lpnFea is NULL, the function will return the required nNumFea.
//					lpnC0, lpnSpec and lpnRMS can be NULL.
//Return:			Return non negative number when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_WaveToFeaAddSampleAndGetFeaWithRMS(
	HANDLE hWaveToFea,
	SHORT *lpsSample,
	INT nNumSample,
	INT *lpnFea,
	INT *lpnC0,
	INT *lpnSpec,
	INT *lpnVoiceType,
	INT *lpnRMS
);

CSPDLL_API INT CSpotter_WaveToFeaAddSampleAndGetFeaWithRMS_SilCMSOnly(
	HANDLE hWaveToFea,
	SHORT *lpsSample,
	INT nNumSample,
	INT *lpnFea,
	INT *lpnC0,
	INT *lpnSpec,
	INT *lpnVoiceType,
	INT *lpnRMS
);

//Purpose:			Get feature dimension.
//hWaveToFea(IN):	The WaveToFea handle. 
//Return:			Return feature dimension when success, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_WaveToFeaGetDim(
	HANDLE hWaveToFea
);

//Purpose:				Get CMS data.
//hWaveToFea(IN):		The WaveToFea handle. 
//lpnMean(OUT):			An array of CMS. If lpnMean is NULL, the function will return length of CMS.
//lpnTypeVoice(OUT):	An array of type voice, and the array element is 1.
//Return:				Return length of CMS, Otherwise return a negative error code.
CSPDLL_API INT CSpotter_WaveToFeaGetCMS(
	HANDLE hWaveToFea,
	INT *lpnMean,
	INT *lpnTypeVoice
);

//Purpose:			Set CMS data.
//hWaveToFea(IN):	The WaveToFea handle. 
//lpnMean(OUT):		An array of CMS.
//nTypeVoice(IN):	The type voice.
//Return:			Return CSPOTTER_SUCCESS when success, Otherwise return a negative integer error code.
CSPDLL_API INT CSpotter_WaveToFeaSetCMS(
	HANDLE hWaveToFea,
	INT *lpnMean,
	INT nTypeVoice
);

//Purpose:				Save CMS data to file.
//hWaveToFea(IN):		The WaveToFea handle. 
//lpchCMSFile(IN):		The full path of CMS file.
//Return:				Return CSPOTTER_SUCCESS when success, Otherwise return a negative integer error code.
CSPDLL_API INT CSpotter_WaveToFeaSaveCMSToFile(
	HANDLE hWaveToFea,
	char *lpchCMSFile
);

//Purpose:				Load CMS data from file.
//hWaveToFea(IN):		The WaveToFea handle. 
//lpchCMSFile(IN):		The full path of CMS file.
//Return:				Return CSPOTTER_SUCCESS when success, Otherwise return a negative integer error code.
CSPDLL_API INT CSpotter_WaveToFeaLoadCMSFromFile(
	HANDLE hWaveToFea,
	char *lpchCMSFile
);


#ifdef __cplusplus
}
#endif

#endif // __CSPOTTERSDK_API_H
