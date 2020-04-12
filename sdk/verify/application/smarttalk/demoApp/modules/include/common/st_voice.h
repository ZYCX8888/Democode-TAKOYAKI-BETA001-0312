#ifndef _ST_VOICE_H_
#define _ST_VOICE_H_

#ifdef __cplusplus
extern "C"{
#endif	// __cplusplus

#include <time.h>

#include "mi_common_datatype.h"

MI_S32 ST_AudioInStart();
MI_S32 ST_AudioInStop();

MI_S32 ST_VoiceAnalyzeStart();
MI_S32 ST_VoiceAnalyzeStop();

MI_S32 ST_VoiceLearnStart();
MI_S32 ST_VoiceLearnStop();

MI_S32 ST_VoiceAnalyzeInit();
MI_S32 ST_VoiceAnalyzeDeInit();

#ifdef __cplusplus
}
#endif	// __cplusplus

#endif //_ST_VOICE_H_
