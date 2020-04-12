#ifndef _ST_USB_H_
#define _ST_USB_H_

#include "st_common.h"

#ifdef  __cplusplus
extern "C"
{
#endif

#define ST_DetectUsbDev     ST_InitUsbDev

MI_S32 ST_InitUsbDev(char *pDirName, int nLen);
MI_S32 ST_DeinitUsbDev();
#ifdef  __cplusplus
}
#endif

#endif //_ST_XMLPRASE_H_
