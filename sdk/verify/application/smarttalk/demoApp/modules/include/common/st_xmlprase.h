#ifndef _ST_XMLPRASE_H_
#define _ST_XMLPRASE_H_

#include "st_common.h"

#ifdef  __cplusplus
extern "C"
{
#endif

MI_S32 ST_XmlAddSmartBDCfg(SMART_BD_Machine_CFG_T *pstDeviceCfg);
MI_S32 ST_XmlPraseBDCfg(const MI_U8 *pu8CfgFile, MI_U8 *pu8CallID, MI_U8 *puIPaddr);
MI_S32 ST_XmlGetIPaddrByCallID(const MI_U8 *pu8CfgFile, MI_U8 *pu8CallID, MI_U8 *pu8IPaddr);
MI_S32 ST_XmlPraseBD_MyInfo(const MI_U8 *pu8CfgFile, MI_U8 *pu8DeviceID);
MI_S32 ST_XmlPraseUiCfg(const MI_U8 *pu8CfgFile, const MI_U8 *LayoutName, ST_Rect_T  stUiItemArea[]);
MI_S32 ST_XmlParseDevVolumeCfg(const MI_U8 *pu8DevVolume, MI_S32 *ps32VolValue, MI_BOOL *pbMute);
MI_S32 ST_XmlUpdateDevVolumeCfg(const MI_U8 *pu8DevVolume, MI_S32 s32VolValue, MI_BOOL bMute);
MI_S32 ST_XmlParsePlayFileCfg(MI_U8 *pu8Path, MI_S32 s32PathLen);
MI_S32 ST_XmlUpdatePlayFileCfg(MI_U8 *pu8SavePath);
#ifdef  __cplusplus
}
#endif

#endif //_ST_XMLPRASE_H_