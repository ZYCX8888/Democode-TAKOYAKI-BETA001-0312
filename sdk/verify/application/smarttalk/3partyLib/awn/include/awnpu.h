#ifndef __AWNPU_H__
#define __AWNPU_H__
#ifdef __cplusplus
extern "C" {
#endif
#include "awnpudef.h"
#include "stdio.h"


#ifndef API_EXPORT
#ifdef AW_API_EXPORT
#define API_EXPORT __attribute__ ((visibility("default")))
#else
#define API_EXPORT
#endif
#endif


API_EXPORT AW_HANDLE AW_Init(int flag);
API_EXPORT int AW_Uninit(AW_HANDLE handle);
API_EXPORT int AW_Util_GetVersion(void);
API_EXPORT int AW_SetIntParameter(AW_HANDLE handle, int paramID, int paramValue);
API_EXPORT int AW_GetIntParameter(AW_HANDLE handle, int paramID);
API_EXPORT int AW_SetFloatParameter(AW_HANDLE handle, int paramID, float paramValue);
API_EXPORT float AW_GetFloatParameter(AW_HANDLE handle, int paramID);
API_EXPORT int AW_SetStringParameter(AW_HANDLE handle, int paramID, const char *paramValue);
API_EXPORT const char *AW_GetStringParameter(AW_HANDLE handle, int paramID);


API_EXPORT AW_FD_HANDLE AW_FD_Create(AW_HANDLE handle, int flag);
API_EXPORT int AW_FD_Release(AW_FD_HANDLE handle);
API_EXPORT int AW_FD_GetPreferInputSize(AW_FD_HANDLE handle, int *width, int *height);
API_EXPORT int AW_FD_ProcessStream(AW_FD_HANDLE handle, unsigned int addr, int flag);
API_EXPORT int AW_FD_ProcessYUV(AW_FD_HANDLE handle, unsigned char *yuvData, int width, int height, int flag);
API_EXPORT int AW_FD_ProcessRGB(AW_FD_HANDLE handle, unsigned char *rgbData, int width, int height, int flag);
API_EXPORT int AW_FD_ProcessResult(AW_FD_HANDLE handle);
API_EXPORT int AW_FD_GetFaceBoxNum(AW_FD_HANDLE handle);
API_EXPORT int AW_FD_GetFaceBox(AW_FD_HANDLE handle, int index, AW_RECT *rect);
API_EXPORT int AW_FD_GetFaceExtraData(AW_FD_HANDLE handle, int cmd, int index, void *data, int *size);
API_EXPORT int AW_FD_GetAlignedFace(AW_FD_HANDLE handle, int index, unsigned char *rgbData, int width, int height, unsigned char *dstDataPtr, int dstWidth, int dstHeight, int flag);


API_EXPORT AW_FR_HANDLE AW_FR_Create(AW_HANDLE handle, int flag);
API_EXPORT AW_FR_HANDLE AW_FR_CreateWithFD(AW_HANDLE handle, AW_FD_HANDLE handleFD);
API_EXPORT AW_FD_HANDLE AW_FR_GetFDHandle(AW_FR_HANDLE handle);
API_EXPORT int AW_FR_Release(AW_FR_HANDLE handle);
API_EXPORT int AW_FR_GetPreferInputSize(AW_FR_HANDLE handle, int *width, int *height);
API_EXPORT int AW_FR_GetFDPreferInputSize(AW_FR_HANDLE handle, int *width, int *height);
API_EXPORT int AW_FR_ProcessYUV(AW_FR_HANDLE handle, unsigned char *yuvData, int width, int height, int flag);
API_EXPORT int AW_FR_ProcessRGB(AW_FR_HANDLE handle, unsigned char *rgbData, int width, int height, int flag);
API_EXPORT int AW_FR_ProcessStream(AW_FR_HANDLE handle, unsigned int addr, int flag);
API_EXPORT int AW_FR_ProcessResult(AW_FR_HANDLE handle);
API_EXPORT int AW_FR_GetFaceBoxNum(AW_FR_HANDLE handle);
API_EXPORT int AW_FR_GetFaceBox(AW_FR_HANDLE handle, int index, AW_RECT *rect);
API_EXPORT int AW_FR_GetFeatureType(AW_FR_HANDLE handle);
API_EXPORT int AW_FR_GetFeatureSize(AW_FR_HANDLE handle);
API_EXPORT int AW_FR_GetFeatureNum(AW_FR_HANDLE handle);
API_EXPORT int AW_FR_GetFeature(AW_FR_HANDLE handle, int index, void *data, int size);


API_EXPORT AW_PD_HANDLE AW_PD_Create(AW_HANDLE handle, int flag);
API_EXPORT int AW_PD_Release(AW_PD_HANDLE handle);
API_EXPORT int AW_PD_GetPreferInputSize(AW_PD_HANDLE handle, int *width, int *height);
API_EXPORT int AW_PD_ProcessStream(AW_PD_HANDLE handle, unsigned int addr, int flag);
API_EXPORT int AW_PD_ProcessYUV(AW_PD_HANDLE handle, unsigned char *yuvData, int width, int height, int flag);
API_EXPORT int AW_PD_ProcessRGB(AW_PD_HANDLE handle, unsigned char *rgbData, int width, int height, int flag);
API_EXPORT int AW_PD_ProcessResult(AW_PD_HANDLE handle);
API_EXPORT int AW_PD_GetPedestrianNum(AW_PD_HANDLE handle);
API_EXPORT int AW_PD_GetPedestrian(AW_PD_HANDLE handle, int index, AW_RECT *rect);


API_EXPORT AW_FTMAN_HANDLE AW_FeatMan_Create(AW_HANDLE handle, int featureType, int featureSize, int flag);
API_EXPORT AW_FTMAN_HANDLE AW_FeatMan_Create2(AW_HANDLE handle, const char *dir, int featureType, int featureSize, int extraDataSize, int maxFeatureNumber, int flag);
API_EXPORT int AW_FeatMan_Release(AW_FTMAN_HANDLE handle);
API_EXPORT int AW_FeatMan_Register(AW_FTMAN_HANDLE handle, int id, void *feature, void *extraData, int extraLen, int flag);
API_EXPORT int AW_FeatMan_Deregister(AW_FTMAN_HANDLE handle, int id);
API_EXPORT int AW_FeatMan_DeregisterAll(AW_FTMAN_HANDLE handle);
API_EXPORT float AW_FeatMan_CalcSimilarity(AW_FTMAN_HANDLE handle, void *feature1, void *feature2);
API_EXPORT float AW_FeatMan_CalcSimilarity2(AW_FTMAN_HANDLE handle, int featureType, int featureSize, void *feature1, void *feature2);
API_EXPORT int AW_FeatMan_FindSimilar(AW_FTMAN_HANDLE handle, void *feature, float threshold, AW_FeatueResult *found, int foundSize, int *foundCount);
API_EXPORT int AW_FeatMan_GetFeatureInfoById(AW_FTMAN_HANDLE handle, int id, AW_FeatueInfo *info);
API_EXPORT int AW_FeatMan_GetFeatureInfoByIndex(AW_FTMAN_HANDLE handle, int index, AW_FeatueInfo *info);
API_EXPORT int AW_FeatMan_GetFeatureInfoCount(AW_FTMAN_HANDLE handle);
API_EXPORT int AW_FeatMan_LoadFP(AW_FTMAN_HANDLE handle, FILE *fp, int *readBytes);
API_EXPORT int AW_FeatMan_SaveFP(AW_FTMAN_HANDLE handle, FILE *fp, int *writeBytes);
API_EXPORT int AW_FeatMan_LoadFile(AW_FTMAN_HANDLE handle, const char *filename);
API_EXPORT int AW_FeatMan_SaveFile(AW_FTMAN_HANDLE handle, const char *filename);


API_EXPORT AW_LIVE_HANDLE AW_LIVE_Create(AW_HANDLE handle, int flag);
API_EXPORT AW_LIVE_HANDLE AW_LIVE_CreateWithFD(AW_HANDLE handle, AW_FD_HANDLE handleFD);
API_EXPORT AW_FD_HANDLE AW_LIVE_GetFDHandle(AW_LIVE_HANDLE handle);
API_EXPORT int AW_LIVE_Release(AW_LIVE_HANDLE handle);
API_EXPORT int AW_LIVE_GetPreferInputSize(AW_LIVE_HANDLE handle, int *width, int *height);
API_EXPORT int AW_LIVE_ProcessYUV(AW_LIVE_HANDLE handle, unsigned char *yuvData, int width, int height, int flag);
API_EXPORT int AW_LIVE_ProcessRGB(AW_LIVE_HANDLE handle, unsigned char *rgbData, int width, int height, int flag);
API_EXPORT int AW_LIVE_GetFaceBoxNum(AW_LIVE_HANDLE handle);
API_EXPORT int AW_LIVE_GetFaceBox(AW_LIVE_HANDLE handle, int index, AW_RECT *rect);
API_EXPORT int AW_LIVE_GetLiveInfoNum(AW_LIVE_HANDLE handle);
API_EXPORT int AW_LIVE_IsLive(AW_LIVE_HANDLE handle, int index);


API_EXPORT AW_GD_HANDLE AW_GD_Create(AW_HANDLE handle, int flag);
API_EXPORT AW_GD_HANDLE AW_GD_CreateWithFD(AW_HANDLE handle, AW_FD_HANDLE handleFD);
API_EXPORT AW_FD_HANDLE AW_GD_GetFDHandle(AW_GD_HANDLE handle);
API_EXPORT int AW_GD_Release(AW_GD_HANDLE handle);
API_EXPORT int AW_GD_GetPreferInputSize(AW_GD_HANDLE handle, int *width, int *height);
API_EXPORT int AW_GD_GetFDPreferInputSize(AW_GD_HANDLE handle, int *width, int *height);
API_EXPORT int AW_GD_ProcessYUV(AW_GD_HANDLE handle, unsigned char *yuvData, int width, int height, int flag);
API_EXPORT int AW_GD_ProcessRGB(AW_GD_HANDLE handle, unsigned char *rgbData, int width, int height, int flag);
API_EXPORT int AW_GD_ProcessStream(AW_GD_HANDLE handle, unsigned int addr, int flag);
API_EXPORT int AW_GD_ProcessResult(AW_GD_HANDLE handle);
API_EXPORT int AW_GD_GetFaceBoxNum(AW_GD_HANDLE handle);
API_EXPORT int AW_GD_GetFaceBox(AW_GD_HANDLE handle, int index, AW_RECT *rect);
API_EXPORT int AW_GD_GetGenderInfoNum(AW_GD_HANDLE handle);
API_EXPORT int AW_GD_IsMale(AW_GD_HANDLE handle, int index);


API_EXPORT AW_AGE_HANDLE AW_AGE_Create(AW_HANDLE handle, int flag);
API_EXPORT AW_AGE_HANDLE AW_AGE_CreateWithFD(AW_HANDLE handle, AW_FD_HANDLE handleFD);
API_EXPORT AW_FD_HANDLE AW_AGE_GetFDHandle(AW_AGE_HANDLE handle);
API_EXPORT int AW_AGE_Release(AW_AGE_HANDLE handle);
API_EXPORT int AW_AGE_GetPreferInputSize(AW_AGE_HANDLE handle, int *width, int *height);
API_EXPORT int AW_AGE_GetFDPreferInputSize(AW_AGE_HANDLE handle, int *width, int *height);
API_EXPORT int AW_AGE_ProcessYUV(AW_AGE_HANDLE handle, unsigned char *yuvData, int width, int height, int flag);
API_EXPORT int AW_AGE_ProcessRGB(AW_AGE_HANDLE handle, unsigned char *rgbData, int width, int height, int flag);
API_EXPORT int AW_AGE_ProcessStream(AW_AGE_HANDLE handle, unsigned int addr, int flag);
API_EXPORT int AW_AGE_ProcessResult(AW_AGE_HANDLE handle);
API_EXPORT int AW_AGE_GetFaceBoxNum(AW_AGE_HANDLE handle);
API_EXPORT int AW_AGE_GetFaceBox(AW_AGE_HANDLE handle, int index, AW_RECT *rect);
API_EXPORT int AW_AGE_GetAgeInfoNum(AW_AGE_HANDLE handle);
API_EXPORT int AW_AGE_GetAge(AW_AGE_HANDLE handle, int index);


#ifdef __cplusplus
}
#endif
#endif//__AWNPU_H__


