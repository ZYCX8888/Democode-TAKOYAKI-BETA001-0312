#if MIXER_SED_ENABLE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>

#include "module_common.h"
#include "mid_common.h"
#include "mid_vpe.h"
#include "mi_sys_datatype.h"
#include "mi_vdf.h"
#include "mi_vdf_datatype.h"

#include "mid_divp.h"

#include "mi_sed_datatype.h"
#include "mi_sed.h"
#include "mid_sed.h"
#include "mid_osd.h"
#include "mid_dla.h"
static pthread_t g_SedGetResultThred;

extern pthread_mutex_t g_mutex_UpadteOsdState;  
extern pthread_cond_t  g_cond_UpadteOsdState;  

BOOL g_SedCnnobjDetectState = FALSE;  
extern MI_U32 g_UpadteOsdState;
extern int DLAtoRECT(MI_VENC_CHN s32VencChn, int recCnt, ST_DlaRectInfo_T* pRecInfo, MI_BOOL bShow, MI_BOOL bShowBorder);

static void *startSedCnnobjDetectGetResultThred(void *arg)
{  
    MI_SED_CHN SedChn = (MI_SED_CHN)arg;
	MI_SED_RectInfo_t pstRectInfo;
	MI_S32 s32Ret = 0;
	ST_DlaRectInfo_T pRecInfo[SED_MAX_ROI_NUM_PER_CHN];
	memset(&pRecInfo,0,sizeof(ST_DlaRectInfo_T)*SED_MAX_ROI_NUM_PER_CHN);
	memset(&pstRectInfo,0,sizeof(MI_SED_RectInfo_t));
    while(g_SedCnnobjDetectState)
   	{
   	    s32Ret = MI_SED_GetRect(SedChn,&pstRectInfo);
	    if(MI_SUCCESS == s32Ret)
	  	{
	  		for(MI_U16 i=0; i < pstRectInfo.u32RectCount; i++)
	  		{
	  		  memset(&pRecInfo[i].rect,0,sizeof(ST_Rect_T));
	  		  memcpy(&pRecInfo[i].rect,&pstRectInfo.stRect[i],sizeof(ST_Rect_T)); 
	  		}
	   		DLAtoRECT(0, pstRectInfo.u32RectCount, pRecInfo, TRUE, TRUE);
	 		pthread_mutex_lock(&g_mutex_UpadteOsdState);  
			g_UpadteOsdState++;
	   	 	pthread_cond_signal(&g_cond_UpadteOsdState);	
			pthread_mutex_unlock(&g_mutex_UpadteOsdState);	
	   }
	   else
	   {
	     MIXER_DBG("SedChn=%d\n",SedChn);
	     usleep(100);
	   }
   	}
}
void startSedCnnobjDetect(MI_SED_CHN sedChn)
{
    pthread_attr_t attr; 
	MI_S32 policy = 0;
    pthread_attr_init(&attr);
	policy = SCHED_FIFO;
    Mixer_set_thread_policy(&attr,policy);
	struct sched_param s_parm;
    s_parm.sched_priority = sched_get_priority_max(SCHED_FIFO);
    pthread_attr_setschedparam(&attr, &s_parm);
	policy = Mixer_get_thread_policy(&attr);
    Mixer_show_thread_priority(&attr, policy);
    Mixer_get_thread_priority(&attr);
	g_SedCnnobjDetectState = TRUE;
    pthread_create(&g_SedGetResultThred, &attr, startSedCnnobjDetectGetResultThred, (void*)((MI_SED_CHN)sedChn));
    pthread_setname_np(g_SedGetResultThred , "OSD_Task");
    MIXER_INFO("initOsdModule()\n");
}
void stopSedCnnobjDetect(void)
{
   g_SedCnnobjDetectState = FALSE;
   pthread_join(g_SedGetResultThred,NULL);
}
#endif

