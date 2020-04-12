#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>

#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<assert.h>

#include "mi_md.h"
#include "mi_od.h"
#include "mi_vdf.h"
#include "mi_vdf_datatype.h"
#include "mi_sys_datatype.h"

#define VIF_CHN0_ENABLE      1
#define VIF_CHN0_MD0_ENABLE  1   //VDFCHN = 0
#define VIF_CHN0_MD1_ENABLE  0   //VDFCHN = 1
#define VIF_CHN0_OD0_ENABLE  0   //VDFCHN = 2
#define VIF_CHN0_OD1_ENABLE  0   //VDFCHN = 3

#define VIF_CHN1_ENABLE      0
#define VIF_CHN1_OD0_ENABLE  0   //VDFCHN = 10
#define VIF_CHN1_OD1_ENABLE  0   //VDFCHN = 11
#define VIF_CHN1_MD0_ENABLE  0   //VDFCHN = 12
#define VIF_CHN1_MD1_ENABLE  0   //VDFCHN = 13

#define VIF_CHN2_ENABLE      0
#define VIF_CHN2_MD0_ENABLE  0   //VDFCHN = 20
#define VIF_CHN2_MD1_ENABLE  0   //VDFCHN = 21
#define VIF_CHN2_OD0_ENABLE  0   //VDFCHN = 22
#define VIF_CHN2_OD1_ENABLE  0   //VDFCHN = 23

#define VIF_CHN3_ENABLE      0
#define VIF_CHN3_OD0_ENABLE  0   //VDFCHN = 30
#define VIF_CHN3_OD1_ENABLE  0   //VDFCHN = 31
#define VIF_CHN3_MD0_ENABLE  0   //VDFCHN = 32
#define VIF_CHN3_MD1_ENABLE  0   //VDFCHN = 33


#define RAW_W		    320
#define RAW_H		    180
#define MD_DIV_W		12
#define MD_DIV_H		10
#define MD1_DIV_W		16
#define MD1_DIV_H		12
#define OD_DIV_W		3
#define OD_DIV_H		3
#define OD1_DIV_W		2
#define OD1_DIV_H		2

static MI_S32 g_width = RAW_W;
static MI_S32 g_height = RAW_H;
static MI_S32 g_code_mode = 1;


MI_VDF_ChnAttr_t stAttr[0x10] = { 0 };
MI_VDF_CHANNEL VdfChn[0x10] = { 0 };


MI_U8 g_exit = 0;



int vdf_set_MD_attr(MI_VDF_ChnAttr_t* pstAttr)
{
	pstAttr->enWorkMode = E_MI_VDF_WORK_MODE_MD;
	pstAttr->unAttr.stMdAttr.u8Enable    = 1;
	pstAttr->unAttr.stMdAttr.u8SrcChnNum = 0;
	pstAttr->unAttr.stMdAttr.u8MdBufCnt  = 3;
	pstAttr->unAttr.stMdAttr.u8VDFIntvl  = 0;

	pstAttr->unAttr.stMdAttr.ccl_ctrl.u16InitAreaThr = 8;
	pstAttr->unAttr.stMdAttr.ccl_ctrl.u16Step = 2;

    pstAttr->unAttr.stMdAttr.stMdDynamicParamsIn.sensitivity = 70;
    pstAttr->unAttr.stMdAttr.stMdDynamicParamsIn.learn_rate = 2000;
    pstAttr->unAttr.stMdAttr.stMdDynamicParamsIn.md_thr = 16;
    pstAttr->unAttr.stMdAttr.stMdDynamicParamsIn.obj_num_max = 0;

    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.width   = g_width;
    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.height  = g_height;
    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.color   = 1;
    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.mb_size = MDMB_MODE_MB_8x8;
    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.sad_out_ctrl = MDSAD_OUT_CTRL_8BIT_SAD;
    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.md_alg_mode  = MDALG_MODE_SAD;
    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.roi_md.num      = 4;
    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].x = 0;
    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].y = 0;
    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].x = g_width - 1;
    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].y = 0;
    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].x = g_width - 1; //必须被8*8整除
    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].y = g_height - 1;//必须被8*8整除
    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].x = 0;
    pstAttr->unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].y = g_height - 1;

	return 0;
}


int vdf_set_OD_attr(MI_VDF_ChnAttr_t* pstAttr)
{
    pstAttr->enWorkMode = E_MI_VDF_WORK_MODE_OD;
    pstAttr->unAttr.stOdAttr.u8SrcChnNum = 0;
    pstAttr->unAttr.stOdAttr.u8OdBufNum  = 16;
    pstAttr->unAttr.stOdAttr.u8VDFIntvl  = 5; 
/*
    pstAttr->unAttr.stOdAttr.stODObjCfg.u16LtX = 0;
    pstAttr->unAttr.stOdAttr.stODObjCfg.u16LtY = 0;
    pstAttr->unAttr.stOdAttr.stODObjCfg.u16RbX = g_width - 1;
    pstAttr->unAttr.stOdAttr.stODObjCfg.u16RbY = g_height - 1;
    pstAttr->unAttr.stOdAttr.stODObjCfg.u16ImgW = g_width;
    pstAttr->unAttr.stOdAttr.stODObjCfg.u16ImgH = g_height;
    
    pstAttr->unAttr.stOdAttr.stODRgnSet.u16WideDiv  = 3; // fill with endiv
    pstAttr->unAttr.stOdAttr.stODRgnSet.u16HightDiv = 3; // fill with endiv
    pstAttr->unAttr.stOdAttr.stODRgnSet.u32Enable = 0xFFFFFFFF;
    pstAttr->unAttr.stOdAttr.stODRgnSet.u8Col = 0;
    pstAttr->unAttr.stOdAttr.stODRgnSet.u8Row = 0;
*/
    pstAttr->unAttr.stOdAttr.stOdDynamicParamsIn.thd_tamper     = 3;
    pstAttr->unAttr.stOdAttr.stOdDynamicParamsIn.tamper_blk_thd = 1;
    pstAttr->unAttr.stOdAttr.stOdDynamicParamsIn.min_duration   = 15;

    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.inImgW = g_width;
    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.inImgH = g_height;
    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.nClrType = 1;
    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.div = OD_WINDOW_3X3;
	pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.alpha = 2;
    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.M = 120;
    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.MotionSensitivity = 0;
    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.roi_od.num = 4;
    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[0].x = 0;
    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[0].y = 0;
    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[1].x = g_width - 1;
    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[1].y = 0;
    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[2].x = g_width - 1;
    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[2].y = g_height - 1;
    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[3].x = 0;
    pstAttr->unAttr.stOdAttr.stOdStaticParamsIn.roi_od.pnt[3].y = g_height - 1;
  
	return 0;
}

void* vdf_test_loop(void *argu)
{
	char ch = '0';

	while(1)
	{
	    sleep(1);

        ch = getchar();

        if(('q' == ch) || ('Q' == ch))
        {
            g_exit = 1;
        }
	}

	return NULL;
}

void bind_vdec_to_vdf(void);
void StartVdec(void);

#define VI_CHN unsigned int

void display_help()
{
    printf("\n");
    printf("Usage: vdf_test [-w number] [-h number] [-c number]\n\n");
    printf("\t\t-H Displays this help\n");
    printf("\t\t-w        : The input image width,  sach as: 320, 640, 1280, 1920 ...\n");
    printf("\t\t-h        : The input image heitht, sach as: 240, 480, 720,  1080 ...\n");
    printf("\t\t-c        : [0] JPEG, [1] MJPEG, [2] H264, [3] H265 \n");
    printf("\n\n"); 
}

int main(int argc, char** argv)
{
	int i, j;
    pthread_t pThread;
    VI_CHN chn = 5;
    int result;
    MI_U32 u32VDFVersion[32];

	g_exit = 0;

    while((result = getopt(argc, argv, "w:h:c:H")) != -1)
    {
        switch(result)
        {
            case 'w':
                g_width = atoi(optarg);
                printf("\n	Input Image Width = %d\n", g_width);
                break;
                
			case 'h':
				g_height = atoi(optarg);
				printf("\n	Input Image Height = %d\n", g_height);
                break;

			case 'c':
				g_code_mode = atoi(optarg);
				switch(g_code_mode)
				{
				    case 0:
				        printf("\n	Input Image code Mode JPEG(%d)\n", g_code_mode);
				        break;
				    case 1:
				        printf("\n	Input Image code Mode MJPEG(%d)\n", g_code_mode);
				        break;
				    case 2:
				        printf("\n	Input Image code Mode H264(%d)\n", g_code_mode);
				        break;
				    case 3:
				        printf("\n	Input Image code Mode H265(%d)\n", g_code_mode);
				        break;
				    default:
				        printf("\n	Input wrong Image code Mode = %d\n", g_code_mode);
				}
                break;

			case 'H':
				display_help();
				break;
        }
    }

    printf("===== This APP is only run on I2 platform [");
    printf(__DATE__);
    printf("  ");
    printf(__TIME__);
    printf("] =====\n");

	pthread_create(&pThread, NULL, vdf_test_loop, NULL);
	pthread_setname_np(pThread, "vdf_test_loop");

//----------------- 1st: do system initial ------------------

//----------------- 2nd: do vdf initial ------------------

    StartVdec();

    MI_VDF_Init();

    //usleep(10*1000);

//============ VIF-chn=0 =============
#if (VIF_CHN0_ENABLE)  
#if (VIF_CHN0_MD0_ENABLE)
    // initial VDF_HDL(MD)=0
    VdfChn[0] = 0;

	vdf_set_MD_attr(&stAttr[0]);

	stAttr[0].unAttr.stMdAttr.u8MdBufCnt  = 3;
	stAttr[0].unAttr.stMdAttr.u8VDFIntvl  = 1;

    MI_VDF_CreateChn(VdfChn[0], &stAttr[0]);
	
    MI_VDF_EnableSubWindow(VdfChn[0], 0, 0, 1);
#endif //(VIF_CHN0_MD0_ENABLE)

#if (VIF_CHN0_MD1_ENABLE)
	VdfChn[1] = 1;
	vdf_set_MD_attr(&stAttr[1]);
	stAttr[1].unAttr.stMdAttr.u8Enable    = 1;
	stAttr[1].unAttr.stMdAttr.u8SrcChnNum = 0;
    stAttr[1].unAttr.stMdAttr.u8MdBufCnt  = 4;
    stAttr[1].unAttr.stMdAttr.u8VDFIntvl  = 2; 

    stAttr[1].unAttr.stMdAttr.stMdDynamicParamsIn.sensitivity = 80;
    stAttr[1].unAttr.stMdAttr.stMdDynamicParamsIn.learn_rate = 2000;
    stAttr[1].unAttr.stMdAttr.stMdDynamicParamsIn.md_thr = 16;
    stAttr[1].unAttr.stMdAttr.stMdDynamicParamsIn.obj_num_max = 0;

    stAttr[1].unAttr.stMdAttr.stMdStaticParamsIn.mb_size = MDMB_MODE_MB_8x8;
    stAttr[1].unAttr.stMdAttr.stMdStaticParamsIn.sad_out_ctrl = MDSAD_OUT_CTRL_8BIT_SAD;
    stAttr[1].unAttr.stMdAttr.stMdStaticParamsIn.md_alg_mode  = MDALG_MODE_FG;

	MI_VDF_CreateChn(VdfChn[1], &stAttr[1]);

	MI_VDF_EnableSubWindow(VdfChn[1], 0, 0, 1);
#endif //(VIF_CHN0_MD1_ENABLE)

#if (VIF_CHN0_OD0_ENABLE)
		VdfChn[2] = 2;
		vdf_set_OD_attr(&stAttr[2]);
		stAttr[2].enWorkMode = E_MI_VDF_WORK_MODE_OD;
		stAttr[2].unAttr.stOdAttr.u8SrcChnNum = 0;
		stAttr[2].unAttr.stOdAttr.u8OdBufNum = 10;
		stAttr[2].unAttr.stOdAttr.u8VDFIntvl = 5; 

	    stAttr[2].unAttr.stOdAttr.stOdDynamicParamsIn.thd_tamper     = 2;
	    stAttr[2].unAttr.stOdAttr.stOdDynamicParamsIn.tamper_blk_thd = 1;
	    stAttr[2].unAttr.stOdAttr.stOdDynamicParamsIn.min_duration   = 15;

	    stAttr[2].unAttr.stOdAttr.stOdStaticParamsIn.div = OD_WINDOW_3X3;
		stAttr[2].unAttr.stOdAttr.stOdStaticParamsIn.alpha = 2;
	    stAttr[2].unAttr.stOdAttr.stOdStaticParamsIn.M = 120;

		MI_VDF_CreateChn(VdfChn[2], &stAttr[2]);
	
		for (i = 0; i < OD_DIV_W; i++)
		{
			for (j = 0; j < OD_DIV_H; j++)
			{
				MI_VDF_EnableSubWindow(VdfChn[2], i, j, 1);
			}
		}
#endif //(VIF_CHN0_OD0_ENABLE)

#if (VIF_CHN0_OD1_ENABLE)
		VdfChn[3] = 3;
		vdf_set_OD_attr(&stAttr[3]);
		stAttr[3].enWorkMode = E_MI_VDF_WORK_MODE_OD;
		stAttr[3].unAttr.stOdAttr.u8SrcChnNum = 0;
		stAttr[3].unAttr.stOdAttr.u8OdBufNum = 8;
		stAttr[3].unAttr.stOdAttr.u8VDFIntvl = 1; 

	    stAttr[3].unAttr.stOdAttr.stOdDynamicParamsIn.thd_tamper     = 2;
	    stAttr[3].unAttr.stOdAttr.stOdDynamicParamsIn.tamper_blk_thd = 1;
	    stAttr[3].unAttr.stOdAttr.stOdDynamicParamsIn.min_duration   = 15;

	    stAttr[3].unAttr.stOdAttr.stOdStaticParamsIn.div = OD_WINDOW_2X2;
		stAttr[3].unAttr.stOdAttr.stOdStaticParamsIn.alpha = 2;
	    stAttr[3].unAttr.stOdAttr.stOdStaticParamsIn.M = 120;

		MI_VDF_CreateChn(VdfChn[3], &stAttr[3]);
	
		for (i = 0; i < OD1_DIV_W; i++)
		{
			for (j = 0; j < OD1_DIV_H; j++)
			{
				MI_VDF_EnableSubWindow(VdfChn[3], i, j, 1);
			}
		}
#endif //(VIF_CHN0_OD1_ENABLE)
#endif //(VIF_CHN0_ENABLE)

#if (VIF_CHN1_ENABLE)
#if (VIF_CHN1_OD0_ENABLE)
	VdfChn[4] = 10;
	vdf_set_OD_attr(&stAttr[4]);
	stAttr[4].enWorkMode = E_MI_VDF_WORK_MODE_OD;
	stAttr[4].unAttr.stOdAttr.u8SrcChnNum = 1;
	stAttr[4].unAttr.stOdAttr.u8OdBufNum = 4;
	stAttr[4].unAttr.stOdAttr.u8VDFIntvl = 5; 

	stAttr[4].unAttr.stOdAttr.stOdDynamicParamsIn.thd_tamper	 = 2;
	stAttr[4].unAttr.stOdAttr.stOdDynamicParamsIn.tamper_blk_thd = 1;
	stAttr[4].unAttr.stOdAttr.stOdDynamicParamsIn.min_duration	 = 15;
	
	stAttr[4].unAttr.stOdAttr.stOdStaticParamsIn.div = OD_WINDOW_3X3;
	stAttr[4].unAttr.stOdAttr.stOdStaticParamsIn.alpha = 2;
	stAttr[4].unAttr.stOdAttr.stOdStaticParamsIn.M = 120;

	MI_VDF_CreateChn(VdfChn[4], &stAttr[4]);

	for (i = 0; i < OD_DIV_W; i++)
	{
		for (j = 0; j < OD_DIV_H; j++)
		{
            MI_VDF_EnableSubWindow(VdfChn[4], i, j, 1);
        }
    }
#endif //(VIF_CHN1_OD0_ENABLE)

#if (VIF_CHN1_OD1_ENABLE)
	VdfChn[5] = 11;
	vdf_set_OD_attr(&stAttr[5]);
	stAttr[5].enWorkMode = E_MI_VDF_WORK_MODE_OD;
	stAttr[5].unAttr.stOdAttr.u8SrcChnNum = 1;
	stAttr[5].unAttr.stOdAttr.u8OdBufNum = 5;
	stAttr[5].unAttr.stOdAttr.u8VDFIntvl = 5; 

	stAttr[5].unAttr.stOdAttr.stOdDynamicParamsIn.thd_tamper	 = 2;
	stAttr[5].unAttr.stOdAttr.stOdDynamicParamsIn.tamper_blk_thd = 1;
	stAttr[5].unAttr.stOdAttr.stOdDynamicParamsIn.min_duration	 = 15;
	
	stAttr[5].unAttr.stOdAttr.stOdStaticParamsIn.div = OD_WINDOW_1X1;
	stAttr[5].unAttr.stOdAttr.stOdStaticParamsIn.alpha = 2;
	stAttr[5].unAttr.stOdAttr.stOdStaticParamsIn.M = 120;

	MI_VDF_CreateChn(VdfChn[5], &stAttr[5]);

	for (i = 0; i < OD1_DIV_W; i++)
	{
		for (j = 0; j < OD1_DIV_H; j++)
		{
            MI_VDF_EnableSubWindow(VdfChn[5], i, j, 1);
        }
    }
#endif //(VIF_CHN1_OD1_ENABLE)

#if (VIF_CHN1_MD0_ENABLE)
		VdfChn[6] = 12;
		vdf_set_MD_attr(&stAttr[6]);
		stAttr[6].unAttr.stMdAttr.u8Enable    = 1;
		stAttr[6].unAttr.stMdAttr.u8SrcChnNum = 1;
		stAttr[6].unAttr.stMdAttr.u8MdBufCnt  = 3;
		stAttr[6].unAttr.stMdAttr.u8VDFIntvl  = 5;
		
		stAttr[6].unAttr.stMdAttr.stMdDynamicParamsIn.sensitivity = 80;
		stAttr[6].unAttr.stMdAttr.stMdDynamicParamsIn.learn_rate = 2000;
		stAttr[6].unAttr.stMdAttr.stMdDynamicParamsIn.md_thr = 16;
		stAttr[6].unAttr.stMdAttr.stMdDynamicParamsIn.obj_num_max = 0;
		
		stAttr[6].unAttr.stMdAttr.stMdStaticParamsIn.mb_size = MDMB_MODE_MB_8x8;
		stAttr[6].unAttr.stMdAttr.stMdStaticParamsIn.sad_out_ctrl = MDSAD_OUT_CTRL_16BIT_SAD;
		stAttr[6].unAttr.stMdAttr.stMdStaticParamsIn.md_alg_mode  = MDALG_MODE_SAD;

		MI_VDF_CreateChn(VdfChn[6], &stAttr[6]);
	
		MI_VDF_EnableSubWindow(VdfChn[6], 0, 0, 1);
#endif //(VIF_CHN1_MD0_ENABLE)

#if (VIF_CHN1_MD1_ENABLE)
		VdfChn[7] = 13;
		vdf_set_MD_attr(&stAttr[7]);
		stAttr[7].unAttr.stMdAttr.u8Enable    = 1;
		stAttr[7].unAttr.stMdAttr.u8SrcChnNum = 1;
		stAttr[7].unAttr.stMdAttr.u8MdBufCnt  = 2;
		stAttr[7].unAttr.stMdAttr.u8VDFIntvl  = 2;

		stAttr[7].unAttr.stMdAttr.stMdDynamicParamsIn.sensitivity = 80;
		stAttr[7].unAttr.stMdAttr.stMdDynamicParamsIn.learn_rate = 2000;
		stAttr[7].unAttr.stMdAttr.stMdDynamicParamsIn.md_thr = 16;
		stAttr[7].unAttr.stMdAttr.stMdDynamicParamsIn.obj_num_max = 0;
		
		stAttr[7].unAttr.stMdAttr.stMdStaticParamsIn.mb_size = MDMB_MODE_MB_8x8;
		stAttr[7].unAttr.stMdAttr.stMdStaticParamsIn.sad_out_ctrl = MDSAD_OUT_CTRL_16BIT_SAD;
		stAttr[7].unAttr.stMdAttr.stMdStaticParamsIn.md_alg_mode  = MDALG_MODE_SAD_FG;

		MI_VDF_CreateChn(VdfChn[7], &stAttr[7]);
	
		MI_VDF_EnableSubWindow(VdfChn[7], 0, 0, 1);
#endif //(VIF_CHN1_MD1_ENABLE)
#endif //(VIF_CHN1_ENABLE)

#if (VIF_CHN2_ENABLE)
#if (VIF_CHN2_MD0_ENABLE)
    VdfChn[8] = 20;
	vdf_set_MD_attr(&stAttr[8]);
    stAttr[8].unAttr.stMdAttr.u8Enable    = 1;
    stAttr[8].unAttr.stMdAttr.u8SrcChnNum = 2;
    stAttr[8].unAttr.stMdAttr.u8MdBufCnt  = 5;
    stAttr[8].unAttr.stMdAttr.u8VDFIntvl  = 1; 

    stAttr[8].unAttr.stMdAttr.stMdDynamicParamsIn.sensitivity = 80;
    stAttr[8].unAttr.stMdAttr.stMdDynamicParamsIn.learn_rate = 2000;
    stAttr[8].unAttr.stMdAttr.stMdDynamicParamsIn.md_thr = 16;
    stAttr[8].unAttr.stMdAttr.stMdDynamicParamsIn.obj_num_max = 0;

    stAttr[8].unAttr.stMdAttr.stMdStaticParamsIn.mb_size = MDMB_MODE_MB_4x4;
    stAttr[8].unAttr.stMdAttr.stMdStaticParamsIn.sad_out_ctrl = MDSAD_OUT_CTRL_8BIT_SAD;
    stAttr[8].unAttr.stMdAttr.stMdStaticParamsIn.md_alg_mode  = MDALG_MODE_SAD_FG;

    MI_VDF_CreateChn(VdfChn[8], &stAttr[8]);
	
    MI_VDF_EnableSubWindow(VdfChn[8], 0, 0, 1);
#endif //(VIF_CHN2_MD0_ENABLE)

#if (VIF_CHN2_MD1_ENABLE)
    VdfChn[9] = 21;
	vdf_set_MD_attr(&stAttr[9]);
    stAttr[9].unAttr.stMdAttr.u8Enable    = 1;
    stAttr[9].unAttr.stMdAttr.u8SrcChnNum = 2;
    stAttr[9].unAttr.stMdAttr.u8MdBufCnt  = 8;
    stAttr[9].unAttr.stMdAttr.u8VDFIntvl  = 5;

    stAttr[9].unAttr.stMdAttr.stMdDynamicParamsIn.sensitivity = 80;
    stAttr[9].unAttr.stMdAttr.stMdDynamicParamsIn.learn_rate = 2000;
    stAttr[9].unAttr.stMdAttr.stMdDynamicParamsIn.md_thr = 16;
    stAttr[9].unAttr.stMdAttr.stMdDynamicParamsIn.obj_num_max = 0;

    stAttr[9].unAttr.stMdAttr.stMdStaticParamsIn.mb_size = MDMB_MODE_MB_4x4;
    stAttr[9].unAttr.stMdAttr.stMdStaticParamsIn.sad_out_ctrl = MDSAD_OUT_CTRL_16BIT_SAD;
    stAttr[9].unAttr.stMdAttr.stMdStaticParamsIn.md_alg_mode  = MDALG_MODE_SAD_FG;

    MI_VDF_CreateChn(VdfChn[9], &stAttr[9]);
	
    MI_VDF_EnableSubWindow(VdfChn[9], 0, 0, 1);
#endif //(VIF_CHN2_MD1_ENABLE)

#if (VIF_CHN2_OD0_ENABLE)
		VdfChn[10] = 22;
		vdf_set_OD_attr(&stAttr[10]);
		stAttr[10].enWorkMode = E_MI_VDF_WORK_MODE_OD;
		stAttr[10].unAttr.stOdAttr.u8SrcChnNum = 2;
		stAttr[10].unAttr.stOdAttr.u8OdBufNum = 10;
		stAttr[10].unAttr.stOdAttr.u8VDFIntvl = 5; 

	    stAttr[10].unAttr.stOdAttr.stOdDynamicParamsIn.thd_tamper     = 2;
	    stAttr[10].unAttr.stOdAttr.stOdDynamicParamsIn.tamper_blk_thd = 1;
	    stAttr[10].unAttr.stOdAttr.stOdDynamicParamsIn.min_duration   = 15;

	    stAttr[10].unAttr.stOdAttr.stOdStaticParamsIn.div = OD_WINDOW_2X2;
		stAttr[10].unAttr.stOdAttr.stOdStaticParamsIn.alpha = 2;
	    stAttr[10].unAttr.stOdAttr.stOdStaticParamsIn.M = 120;

		MI_VDF_CreateChn(VdfChn[10], &stAttr[10]);
	
		for (i = 0; i < OD_DIV_W; i++)
		{
			for (j = 0; j < OD_DIV_H; j++)
			{
				MI_VDF_EnableSubWindow(VdfChn[10], i, j, 1);
			}
		}
#endif //(VIF_CHN2_OD0_ENABLE)

#if (VIF_CHN2_OD1_ENABLE)
		VdfChn[11] = 23;
		vdf_set_OD_attr(&stAttr[11]);
		stAttr[11].enWorkMode = E_MI_VDF_WORK_MODE_OD;
		stAttr[11].unAttr.stOdAttr.u8SrcChnNum = 2;
		stAttr[11].unAttr.stOdAttr.u8OdBufNum = 11;
		stAttr[11].unAttr.stOdAttr.u8VDFIntvl = 5; 

	    stAttr[11].unAttr.stOdAttr.stOdDynamicParamsIn.thd_tamper     = 2;
	    stAttr[11].unAttr.stOdAttr.stOdDynamicParamsIn.tamper_blk_thd = 1;
	    stAttr[11].unAttr.stOdAttr.stOdDynamicParamsIn.min_duration   = 15;

	    stAttr[11].unAttr.stOdAttr.stOdStaticParamsIn.div = OD_WINDOW_1X1;
		stAttr[11].unAttr.stOdAttr.stOdStaticParamsIn.alpha = 2;
	    stAttr[11].unAttr.stOdAttr.stOdStaticParamsIn.M = 120;

		MI_VDF_CreateChn(VdfChn[11], &stAttr[11]);
	
		for (i = 0; i < OD1_DIV_W; i++)
		{
			for (j = 0; j < OD1_DIV_H; j++)
			{
				MI_VDF_EnableSubWindow(VdfChn[11], i, j, 1);
			}
		}
#endif //(VIF_CHN2_OD1_ENABLE)
#endif //(VIF_CHN2_ENABLE)

#if (VIF_CHN3_ENABLE)
#if (VIF_CHN3_OD0_ENABLE)
	VdfChn[12] = 30;
	vdf_set_OD_attr(&stAttr[12]);
	stAttr[12].enWorkMode = E_MI_VDF_WORK_MODE_OD;
	stAttr[12].unAttr.stOdAttr.u8SrcChnNum = 3;
	stAttr[12].unAttr.stOdAttr.u8OdBufNum = 2;
	stAttr[12].unAttr.stOdAttr.u8VDFIntvl = 5; 

	stAttr[12].unAttr.stOdAttr.stOdDynamicParamsIn.thd_tamper	 = 3;
	stAttr[12].unAttr.stOdAttr.stOdDynamicParamsIn.tamper_blk_thd = 1;
	stAttr[12].unAttr.stOdAttr.stOdDynamicParamsIn.min_duration	 = 15;
	
	stAttr[12].unAttr.stOdAttr.stOdStaticParamsIn.div = OD_WINDOW_3X3;
	stAttr[12].unAttr.stOdAttr.stOdStaticParamsIn.alpha = 2;
	stAttr[12].unAttr.stOdAttr.stOdStaticParamsIn.M = 120;

	MI_VDF_CreateChn(VdfChn[12], &stAttr[12]);

	for (i = 0; i < OD_DIV_W; i++)
	{
		for (j = 0; j < OD_DIV_W; j++)
		{
            MI_VDF_EnableSubWindow(VdfChn[12], i, j, 1);
        }
    }
#endif //(VIF_CHN3_OD0_ENABLE)

#if (VIF_CHN3_OD1_ENABLE)
	VdfChn[13] = 31;
	vdf_set_OD_attr(&stAttr[13]);
	stAttr[13].enWorkMode = E_MI_VDF_WORK_MODE_OD;
	stAttr[13].unAttr.stOdAttr.u8SrcChnNum = 3;
	stAttr[13].unAttr.stOdAttr.u8OdBufNum = 3;
	stAttr[13].unAttr.stOdAttr.u8VDFIntvl = 5; 

	stAttr[13].unAttr.stOdAttr.stOdDynamicParamsIn.thd_tamper	 = 3;
	stAttr[13].unAttr.stOdAttr.stOdDynamicParamsIn.tamper_blk_thd = 1;
	stAttr[13].unAttr.stOdAttr.stOdDynamicParamsIn.min_duration	 = 15;
	
	stAttr[13].unAttr.stOdAttr.stOdStaticParamsIn.div = OD_WINDOW_3X3;
	stAttr[13].unAttr.stOdAttr.stOdStaticParamsIn.alpha = 2;
	stAttr[13].unAttr.stOdAttr.stOdStaticParamsIn.M = 120;

	MI_VDF_CreateChn(VdfChn[13], &stAttr[13]);

	for (i = 0; i < OD1_DIV_W; i++)
	{
		for (j = 0; j < OD1_DIV_W; j++)
		{
            MI_VDF_EnableSubWindow(VdfChn[13], i, j, 1);
        }
    }
#endif //(VIF_CHN3_OD1_ENABLE)

#if (VIF_CHN3_MD0_ENABLE)
    VdfChn[14] = 32;
	vdf_set_MD_attr(&stAttr[14]);
    stAttr[14].unAttr.stMdAttr.u8Enable    = 1;
    stAttr[14].unAttr.stMdAttr.u8SrcChnNum = 3;
    stAttr[14].unAttr.stMdAttr.u8MdBufCnt  = 4;
    stAttr[14].unAttr.stMdAttr.u8VDFIntvl  = 5; 

    stAttr[14].unAttr.stMdAttr.stMdDynamicParamsIn.sensitivity = 80;
    stAttr[14].unAttr.stMdAttr.stMdDynamicParamsIn.learn_rate = 2000;
    stAttr[14].unAttr.stMdAttr.stMdDynamicParamsIn.md_thr = 16;
    stAttr[14].unAttr.stMdAttr.stMdDynamicParamsIn.obj_num_max = 0;

    stAttr[14].unAttr.stMdAttr.stMdStaticParamsIn.mb_size = MDMB_MODE_MB_16x16;
    stAttr[14].unAttr.stMdAttr.stMdStaticParamsIn.sad_out_ctrl = MDSAD_OUT_CTRL_8BIT_SAD;
    stAttr[14].unAttr.stMdAttr.stMdStaticParamsIn.md_alg_mode  = MDALG_MODE_FG;
    stAttr[14].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.num      = 4;
    stAttr[14].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].x = 0;
    stAttr[14].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].y = 0;
    stAttr[14].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].x = g_width - 1;
    stAttr[14].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].y = 0;
    stAttr[14].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].x = g_width - 1; //必须被16x16整除
    stAttr[14].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].y = g_height - 9;//必须被16x16整除
    stAttr[14].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].x = 0;
    stAttr[14].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].y = g_height - 9;

    MI_VDF_CreateChn(VdfChn[14], &stAttr[14]);
	
    MI_VDF_EnableSubWindow(VdfChn[14], 0, 0, 1);
#endif //(VIF_CHN3_MD0_ENABLE)

#if (VIF_CHN3_MD1_ENABLE)
    VdfChn[15] = 33;
	vdf_set_MD_attr(&stAttr[15]);
    stAttr[15].unAttr.stMdAttr.u8Enable    = 1;
    stAttr[15].unAttr.stMdAttr.u8SrcChnNum = 3;
    stAttr[15].unAttr.stMdAttr.u8MdBufCnt  = 5;
    stAttr[15].unAttr.stMdAttr.u8VDFIntvl  = 5;

    stAttr[15].unAttr.stMdAttr.stMdDynamicParamsIn.sensitivity = 80;
    stAttr[15].unAttr.stMdAttr.stMdDynamicParamsIn.learn_rate = 2000;
    stAttr[15].unAttr.stMdAttr.stMdDynamicParamsIn.md_thr = 16;
    stAttr[15].unAttr.stMdAttr.stMdDynamicParamsIn.obj_num_max = 0;

    stAttr[15].unAttr.stMdAttr.stMdStaticParamsIn.mb_size = MDMB_MODE_MB_16x16;
    stAttr[15].unAttr.stMdAttr.stMdStaticParamsIn.sad_out_ctrl = MDSAD_OUT_CTRL_8BIT_SAD;
    stAttr[15].unAttr.stMdAttr.stMdStaticParamsIn.md_alg_mode  = MDALG_MODE_SAD_FG;
    stAttr[15].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.num      = 4;
    stAttr[15].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].x = 0;
    stAttr[15].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[0].y = 0;
    stAttr[15].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].x = g_width - 1;
    stAttr[15].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[1].y = 0;
    stAttr[15].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].x = g_width - 1; //必须被16x16整除
    stAttr[15].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[2].y = g_height - 9;//必须被16x16整除
    stAttr[15].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].x = 0;
    stAttr[15].unAttr.stMdAttr.stMdStaticParamsIn.roi_md.pnt[3].y = g_height - 9;

    MI_VDF_CreateChn(VdfChn[15], &stAttr[15]);
	
    MI_VDF_EnableSubWindow(VdfChn[15], 0, 0, 1);
#endif //(VIF_CHN3_MD1_ENABLE)
#endif //(VIF_CHN3_ENABLE)


    MI_VDF_GetLibVersion(VdfChn[0], u32VDFVersion);

	MI_VDF_Run(E_MI_VDF_WORK_MODE_MD);
	MI_VDF_Run(E_MI_VDF_WORK_MODE_OD);

    sleep(1);

    bind_vdec_to_vdf();

//----------------- 3th: get MD/OD result ------------------
	while(0 == g_exit)
	{
		MI_S32 ret = 0;
		MI_U8* pu8MdRstData = NULL;
        MI_VDF_Result_t stVdfResult = { 0 };
        
	    usleep(1000*10);

#if (VIF_CHN0_ENABLE) // VIF-Chn=0
#if (VIF_CHN0_MD0_ENABLE)
        memset(&stVdfResult, 0x00, sizeof(MI_VDF_Result_t));
		stVdfResult.u8SrcChnNum = 0;
        stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_MD;
		ret = MI_VDF_GetResult(VdfChn[0], &stVdfResult, 0);
        if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
        {
            pu8MdRstData = stVdfResult.unVdfResult.stMdResult.pu8MdResult;
            printf("[MD_TEST][HDL=00] pts=0x%llx [WorkMode=%d, Enable=%d] Get MD-Rst data:\n",
                                stVdfResult.unVdfResult.stMdResult.u64Pts, stVdfResult.enWorkMode, \
                                stVdfResult.unVdfResult.stMdResult.u8Enable);

			printf("    0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x  ",	
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++,
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
			printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, 
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);

            MI_VDF_PutResult(VdfChn[0], &stVdfResult);
        }
		else
        {
            printf("[MD_TEST][HDL=00] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }
#endif //(VIF_CHN0_MD0_ENABLE)

#if (VIF_CHN0_MD1_ENABLE)
		memset(&stVdfResult, 0x00, sizeof(MI_VDF_Result_t));
		stVdfResult.u8SrcChnNum = 0;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_MD;
		ret = MI_VDF_GetResult(VdfChn[1], &stVdfResult, 0);
		if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
		{
			pu8MdRstData = stVdfResult.unVdfResult.stMdResult.pu8MdResult;
			printf("[MD_TEST][HDL=01] pts=0x%llx [WorkMode=%d, Enable=%d] Get MD-Rst data:\n",
                                stVdfResult.unVdfResult.stMdResult.u64Pts,   	\
                                stVdfResult.enWorkMode,                      	\
                                stVdfResult.unVdfResult.stMdResult.u8Enable);
			printf("    0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x  ",	
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++,
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
			printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, 
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
			MI_VDF_PutResult(VdfChn[1], &stVdfResult);
		}
		else
        {
            printf("[MD_TEST][HDL=01] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }
#endif //(VIF_CHN0_MD1_ENABLE)

#if (VIF_CHN0_OD0_ENABLE)
		memset(&stVdfResult, 0x00, sizeof(stVdfResult));
		stVdfResult.u8SrcChnNum = 0;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_OD;
		ret = MI_VDF_GetResult(VdfChn[2], &stVdfResult, 0);
		if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
		{
			printf("[OD_TEST][HDL=02] pts=0x%llx [WorkMode=%d, Enable=%d, RmsData=%u (%d, %d)] Get OD-Rst data: ",
								stVdfResult.unVdfResult.stOdResult.u64Pts, 		\
								stVdfResult.enWorkMode, 						\
								stVdfResult.unVdfResult.stOdResult.u8Enable, 	\
								stVdfResult.unVdfResult.stOdResult.u8DataLen, 	\
								stVdfResult.unVdfResult.stOdResult.u8WideDiv, 	\
								stVdfResult.unVdfResult.stOdResult.u8HightDiv);
			printf("{%u %u %u  %u %u %u  %u %u %u}\n", 
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][2]);
			MI_VDF_PutResult(VdfChn[2], &stVdfResult);
		}
		else
        {
            printf("[OD_TEST][HDL=02] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }
#endif //(VIF_CHN0_OD0_ENABLE)

#if (VIF_CHN0_OD1_ENABLE)
		memset(&stVdfResult, 0x00, sizeof(stVdfResult));
		stVdfResult.u8SrcChnNum = 0;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_OD;
		ret = MI_VDF_GetResult(VdfChn[3], &stVdfResult, 0);
		if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
		{
			printf("[OD_TEST][HDL=03] pts=0x%llx [WorkMode=%d, Enable=%d, RmsData=%u (%d, %d)] Get OD-Rst data: ",
								stVdfResult.unVdfResult.stOdResult.u64Pts, 		\
								stVdfResult.enWorkMode, 						\
								stVdfResult.unVdfResult.stOdResult.u8Enable, 	\
								stVdfResult.unVdfResult.stOdResult.u8DataLen, 	\
								stVdfResult.unVdfResult.stOdResult.u8WideDiv, 	\
								stVdfResult.unVdfResult.stOdResult.u8HightDiv);
			printf("{%u %u %u  %u %u %u  %u %u %u}\n\n", 
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][2]);
			MI_VDF_PutResult(VdfChn[3], &stVdfResult);
		}
        else
        {
            printf("[OD_TEST][HDL=03] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }
#endif //(VIF_CHN0_OD0_ENABLE)
#endif //(VIF_CHN0_ENABLE)

#if (VIF_CHN1_ENABLE)
#if (VIF_CHN1_OD0_ENABLE)
		memset(&stVdfResult, 0x00, sizeof(stVdfResult));
		stVdfResult.u8SrcChnNum = 1;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_OD;
		ret = MI_VDF_GetResult(VdfChn[4], &stVdfResult, 0);
		if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
		{
			printf("[OD_TEST][HDL=10] pts=0x%llx [WorkMode=%d, Enable=%d, RmsData=%u (%d, %d)] Get OD-Rst data: ",
								stVdfResult.unVdfResult.stOdResult.u64Pts, 		\
								stVdfResult.enWorkMode, 						\
								stVdfResult.unVdfResult.stOdResult.u8Enable, 	\
								stVdfResult.unVdfResult.stOdResult.u8DataLen, 	\
								stVdfResult.unVdfResult.stOdResult.u8WideDiv, 	\
								stVdfResult.unVdfResult.stOdResult.u8HightDiv);
			printf("{%u %u %u  %u %u %u  %u %u %u}\n",	
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][2]);
			MI_VDF_PutResult(VdfChn[4], &stVdfResult);
		}
        else
        {
            printf("[OD_TEST][HDL=10] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }
#endif //(VIF_CHN1_OD0_ENABLE)

#if (VIF_CHN1_OD1_ENABLE)
		memset(&stVdfResult, 0x00, sizeof(stVdfResult));
		stVdfResult.u8SrcChnNum = 1;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_OD;
		ret = MI_VDF_GetResult(VdfChn[5], &stVdfResult, 0);
		if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
		{
			printf("[OD_TEST][HDL=11] pts=0x%llx [WorkMode=%d, Enable=%d, RmsData=%u (%d, %d)] Get OD-Rst data: ",
								stVdfResult.unVdfResult.stOdResult.u64Pts, 		\
								stVdfResult.enWorkMode, 						\
								stVdfResult.unVdfResult.stOdResult.u8Enable, 	\
								stVdfResult.unVdfResult.stOdResult.u8DataLen, 	\
								stVdfResult.unVdfResult.stOdResult.u8WideDiv, 	\
								stVdfResult.unVdfResult.stOdResult.u8HightDiv);
			printf("{%u %u %u  %u %u %u  %u %u %u}\n",	
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][2]);
			MI_VDF_PutResult(VdfChn[5], &stVdfResult);
		}
        else
        {
            printf("[OD_TEST][HDL=11] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }
#endif //(VIF_CHN1_OD1_ENABLE)

#if (VIF_CHN1_MD0_ENABLE)
		memset(&stVdfResult, 0x00, sizeof(MI_VDF_Result_t));
		stVdfResult.u8SrcChnNum = 1;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_MD;
		ret = MI_VDF_GetResult(VdfChn[6], &stVdfResult, 0);
		if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
		{
			pu8MdRstData = stVdfResult.unVdfResult.stMdResult.pu8MdResult;
			printf("[MD_TEST][HDL=12] pts=0x%llx [WorkMode=%d, Enable=%d, RmsData=%u] Get MD-Rst data:\n",
                                stVdfResult.unVdfResult.stMdResult.u64Pts,   	\
                                stVdfResult.enWorkMode,                      	\
                                stVdfResult.unVdfResult.stMdResult.u8Enable);
			printf("    0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x  ",	
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++,
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
			printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, 
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
			MI_VDF_PutResult(VdfChn[6], &stVdfResult);
		}
        else
        {
            printf("[MD_TEST][HDL=12] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }
#endif //(VIF_CHN1_MD0_ENABLE)

#if (VIF_CHN1_MD1_ENABLE)
        memset(&stVdfResult, 0x00, sizeof(stVdfResult));
        stVdfResult.u8SrcChnNum = 1;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_MD;
		ret = MI_VDF_GetResult(VdfChn[7], &stVdfResult, 0);
        if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
        {
            pu8MdRstData = stVdfResult.unVdfResult.stMdResult.pu8MdResult;
            printf("[MD_TEST][HDL=13] pts=0x%llx [WorkMode=%d, Enable=%d] Get MD-Rst data:\n",
                                stVdfResult.unVdfResult.stMdResult.u64Pts,   	\
                                stVdfResult.enWorkMode,                      	\
                                stVdfResult.unVdfResult.stMdResult.u8Enable);
			printf("    0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x  ",	
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++,
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
			printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, 
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
            MI_VDF_PutResult(VdfChn[7], &stVdfResult);
        }
        else
        {
            printf("[MD_TEST][HDL=13] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }
#endif //(VIF_CHN1_MD1_ENABLE)
#endif //VIF_CHN1_ENABLE

#if (VIF_CHN2_ENABLE)
#if (VIF_CHN2_MD0_ENABLE)
        memset(&stVdfResult, 0x00, sizeof(stVdfResult));
        stVdfResult.u8SrcChnNum = 2;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_MD;
		ret = MI_VDF_GetResult(VdfChn[8], &stVdfResult, 0);
        if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
        {
            pu8MdRstData = stVdfResult.unVdfResult.stMdResult.pu8MdResult;
            printf("[MD_TEST][HDL=20] pts=0x%llx [WorkMode=%d, Enable=%d] Get MD-Rst data:\n",
                                stVdfResult.unVdfResult.stMdResult.u64Pts,   	\
                                stVdfResult.enWorkMode,                      	\
                                stVdfResult.unVdfResult.stMdResult.u8Enable);
			printf("    0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x  ",	
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++,
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
			printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, 
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
            MI_VDF_PutResult(VdfChn[8], &stVdfResult);
        }
        else
        {
            printf("[MD_TEST][HDL=20] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }        
#endif //(VIF_CHN2_MD0_ENABLE)

#if (VIF_CHN2_MD1_ENABLE)
		memset(&stVdfResult, 0x00, sizeof(stVdfResult));
		stVdfResult.u8SrcChnNum = 2;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_MD;
		ret = MI_VDF_GetResult(VdfChn[9], &stVdfResult, 0);
		if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
		{
			pu8MdRstData = stVdfResult.unVdfResult.stMdResult.pu8MdResult;
			printf("[MD_TEST][HDL=21] pts=0x%llx [WorkMode=%d, Enable=%d] Get MD-Rst data:\n",
                                stVdfResult.unVdfResult.stMdResult.u64Pts,   	\
                                stVdfResult.enWorkMode,                      	\
                                stVdfResult.unVdfResult.stMdResult.u8Enable);
			printf("    0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x  ",	
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++,
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
			printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, 
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
			MI_VDF_PutResult(VdfChn[9], &stVdfResult);
		}
        else
        {
            printf("[MD_TEST][HDL=21] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }
#endif //(VIF_CHN2_MD1_ENABLE)

#if (VIF_CHN2_OD0_ENABLE)
		memset(&stVdfResult, 0x00, sizeof(stVdfResult));
		stVdfResult.u8SrcChnNum = 2;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_OD;
		ret = MI_VDF_GetResult(VdfChn[10], &stVdfResult, 0);
		if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
		{
			printf("[OD_TEST][HDL=22] pts=0x%llx [WorkMode=%d, Enable=%d, RmsData=%u (%d, %d)] Get OD-Rst data: ",
								stVdfResult.unVdfResult.stOdResult.u64Pts, 		\
								stVdfResult.enWorkMode, 						\
								stVdfResult.unVdfResult.stOdResult.u8Enable, 	\
								stVdfResult.unVdfResult.stOdResult.u8DataLen, 	\
								stVdfResult.unVdfResult.stOdResult.u8WideDiv, 	\
								stVdfResult.unVdfResult.stOdResult.u8HightDiv);
			printf("{%u %u %u  %u %u %u  %u %u %u}\n", 
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][2]);
			MI_VDF_PutResult(VdfChn[10], &stVdfResult);
		}
        else
        {
            printf("[OD_TEST][HDL=22] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }	   
#endif //(VIF_CHN2_OD0_ENABLE)

#if (VIF_CHN2_OD1_ENABLE)
		memset(&stVdfResult, 0x00, sizeof(stVdfResult));
		stVdfResult.u8SrcChnNum = 2;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_OD;
		ret = MI_VDF_GetResult(VdfChn[11], &stVdfResult, 0);
		if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
		{
			printf("[OD_TEST][HDL=23] pts=0x%llx [WorkMode=%d, Enable=%d, RmsData=%u (%d, %d)] Get OD-Rst data: ",
								stVdfResult.unVdfResult.stOdResult.u64Pts, 		\
								stVdfResult.enWorkMode, 						\
								stVdfResult.unVdfResult.stOdResult.u8Enable, 	\
								stVdfResult.unVdfResult.stOdResult.u8DataLen, 	\
								stVdfResult.unVdfResult.stOdResult.u8WideDiv, 	\
								stVdfResult.unVdfResult.stOdResult.u8HightDiv);
			printf("{%u %u %u  %u %u %u  %u %u %u}\n\n", 
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][2]);
			MI_VDF_PutResult(VdfChn[11], &stVdfResult);
		}
        else
        {
            printf("[OD_TEST][HDL=23] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }   
#endif //(VIF_CHN2_OD1_ENABLE)
#endif //(VIF_CHN2_ENABLE)

#if (VIF_CHN3_ENABLE)
#if (VIF_CHN3_OD0_ENABLE)
        memset(&stVdfResult, 0x00, sizeof(stVdfResult));
        stVdfResult.u8SrcChnNum = 3;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_OD;
		ret = MI_VDF_GetResult(VdfChn[12], &stVdfResult, 0);
        if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
        {
            printf("[OD_TEST][HDL=30] pts=0x%llx [WorkMode=%d, Enable=%d, RmsData=%u (%d, %d)] Get OD-Rst data: ",
								stVdfResult.unVdfResult.stOdResult.u64Pts, 		\
								stVdfResult.enWorkMode, 						\
								stVdfResult.unVdfResult.stOdResult.u8Enable, 	\
								stVdfResult.unVdfResult.stOdResult.u8DataLen, 	\
								stVdfResult.unVdfResult.stOdResult.u8WideDiv, 	\
								stVdfResult.unVdfResult.stOdResult.u8HightDiv);
            printf("{%u %u %u  %u %u %u  %u %u %u}\n",	
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][2]);
            MI_VDF_PutResult(VdfChn[12], &stVdfResult);
        }
        else
        {
            printf("[OD_TEST][HDL=30] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }
#endif //(VIF_CHN3_OD0_ENABLE)
		
#if (VIF_CHN3_OD1_ENABLE)
		memset(&stVdfResult, 0x00, sizeof(stVdfResult));
		stVdfResult.u8SrcChnNum = 3;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_OD;
		ret = MI_VDF_GetResult(VdfChn[13], &stVdfResult, 0);
		if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
		{
			printf("[OD_TEST][HDL=31] pts=0x%llx [WorkMode=%d, Enable=%d, RmsData=%u (%d, %d)] Get OD-Rst data: ",
								stVdfResult.unVdfResult.stOdResult.u64Pts, 		\
								stVdfResult.enWorkMode, 						\
								stVdfResult.unVdfResult.stOdResult.u8Enable, 	\
								stVdfResult.unVdfResult.stOdResult.u8DataLen, 	\
								stVdfResult.unVdfResult.stOdResult.u8WideDiv, 	\
								stVdfResult.unVdfResult.stOdResult.u8HightDiv);
			printf("{%u %u %u  %u %u %u  %u %u %u}\n", 
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[0][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[1][2],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][0],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][1],
								stVdfResult.unVdfResult.stOdResult.u8RgnAlarm[2][2]);
			MI_VDF_PutResult(VdfChn[13], &stVdfResult);
		}
        else
        {
            printf("[OD_TEST][HDL=31] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }	   
#endif //(VIF_CHN3_OD1_ENABLE)

#if (VIF_CHN3_MD0_ENABLE)
		memset(&stVdfResult, 0x00, sizeof(stVdfResult));
		stVdfResult.u8SrcChnNum = 3;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_MD;
		ret = MI_VDF_GetResult(VdfChn[14], &stVdfResult, 0);
		if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
		{
		    pu8MdRstData = stVdfResult.unVdfResult.stMdResult.pu8MdResult;
			printf("[MD_TEST][HDL=32] pts=0x%llx [WorkMode=%d, Enable=%d] Get MD-Rst data:\n",
                                stVdfResult.unVdfResult.stMdResult.u64Pts,   	\
                                stVdfResult.enWorkMode,                      	\
                                stVdfResult.unVdfResult.stMdResult.u8Enable);
			printf("    0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x  ",	
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++,
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
			printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, 
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
			MI_VDF_PutResult(VdfChn[14], &stVdfResult);
		}
        else
        {
            printf("[MD_TEST][HDL=32] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }
#endif //(VIF_CHN3_MD0_ENABLE)

#if (VIF_CHN3_MD1_ENABLE)
		memset(&stVdfResult, 0x00, sizeof(stVdfResult));
		stVdfResult.u8SrcChnNum = 3;
		stVdfResult.enWorkMode = E_MI_VDF_WORK_MODE_MD;
		ret = MI_VDF_GetResult(VdfChn[15], &stVdfResult, 0);
		if((0 == ret) && (1 == stVdfResult.unVdfResult.stMdResult.u8Enable))
		{
			pu8MdRstData = stVdfResult.unVdfResult.stMdResult.pu8MdResult;
			printf("[MD_TEST][HDL=33] pts=0x%llx [WorkMode=%d, Enable=%d] Get MD-Rst data:\n",
                                stVdfResult.unVdfResult.stMdResult.u64Pts,   	\
                                stVdfResult.enWorkMode,                      	\
                                stVdfResult.unVdfResult.stMdResult.u8Enable);
			printf("    0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x  ",	
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++,
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
			printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n",
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, 
								*pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++, *pu8MdRstData++);
			MI_VDF_PutResult(VdfChn[9], &stVdfResult);
		}
        else
        {
            printf("[MD_TEST][HDL=33] line%d call MI_VDF_GetResult() fail(%d)\n\n",__LINE__, ret);
        }
#endif //(VIF_CHN3_MD1_ENABLE)
#endif //(VIF_CHN3_ENABLE)
	}


//----------------- 4th: do vdf un-initial ------------------
    for (i = 0; i < MD_DIV_W; i++)
	{
		for (j = 0; j < MD_DIV_H; j++)
		{
		    MI_VDF_ChnAttr_t stAttr_tmp = { 0 };
            
            MI_VDF_EnableSubWindow(VdfChn[0], i, j, 0);

            memset(&stAttr_tmp, 0x00, sizeof(stAttr_tmp));
            MI_VDF_GetChnAttr(VdfChn[0], &stAttr_tmp);
		}
	}
        
    MI_VDF_DestroyChn(VdfChn[0]);
    MI_VDF_Stop(E_MI_VDF_WORK_MODE_MD);
    MI_VDF_Uninit();

    printf("vdf example exit\n");
    return 0;
}



/////////////////////////////////////////////////////
///shadow
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "mi_shadow.h"
#include "mi_sys.h"

//////////////////////////
///vdec create
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>

#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<assert.h>

#include "mi_sys.h"
#include "mi_vdec.h"
#include "mi_divp_datatype.h"
//#include "mi_common_datatype.h"
#include "mi_divp.h"

#define ExecFunc(_func_, _ret_) \
    if (_func_ != _ret_)\
    {\
        printf("[%d]exec function failed\n", __LINE__);\
        return 1;\
    }\
    else\
    {\
        printf("(%d)exec function pass\n", __LINE__);\
    }

#define VDEC_CHN_MAX (33)
#define MI_U32VALUE(pu8Data, index) (pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3])
#define VESFILE_READER_BATCH (1024*128)


static pthread_t _thrPush[VDEC_CHN_MAX];
static pthread_t _thrGet[VDEC_CHN_MAX];
static MI_BOOL _bRun[VDEC_CHN_MAX];
static MI_S32 _hFile[VDEC_CHN_MAX];
static MI_SYS_ChnPort_t _astChnPort;
static MI_VDEC_CodecType_e _aeCodecType[VDEC_CHN_MAX][8];


#define _CHECKPOINT_ printf("xxxxxxxxx [%s][%d] xxxxxxxx\n", __FUNCTION__, __LINE__);
MI_U64 get_pts(MI_U32 u32FrameRate)
{
    if (0 == u32FrameRate)
    {
        return (MI_U64)(-1);
    }

    return (MI_U64)(1000 / u32FrameRate);
}

void *push_stream(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufConf_t stBufConf;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;

    MI_U8 *pu8Buf = NULL;
    MI_U32 u32Len = 0;
    MI_U32 u32FrameLen = 0;
    MI_U64 u64Pts = 0;
    MI_U8 au8Header[16] = {0};
    MI_U32 u32Pos = 0;
    MI_VDEC_ChnStat_t stChnStat;

    MI_VDEC_CHN VdecChn = (MI_VDEC_CHN)p;
    memset(&stChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
    stChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stChnPort.u32DevId = 0;
    stChnPort.u32ChnId = VdecChn;
    stChnPort.u32PortId = 0;

    memset(&stBufConf, 0x0, sizeof(MI_SYS_BufConf_t));
    stBufConf.eBufType = E_MI_SYS_BUFDATA_RAW;
    stBufConf.u64TargetPts = 0;
    pu8Buf = malloc(VESFILE_READER_BATCH);
    while (_bRun[VdecChn])
    {
        usleep(30 * 1000);

        memset(au8Header, 0, 16);
        u32Pos = lseek(_hFile[VdecChn], 0L, SEEK_CUR);
        u32Len = read(_hFile[VdecChn], au8Header, 16);
        if(u32Len <= 0)
        {
            lseek(_hFile[VdecChn], 0, SEEK_SET);
            continue;
        }

        u32FrameLen = MI_U32VALUE(au8Header, 4);
        if(u32FrameLen > VESFILE_READER_BATCH)
        {
            lseek(_hFile[VdecChn], 0, SEEK_SET);
            continue;
        }

        u32Len = read(_hFile[VdecChn], pu8Buf, u32FrameLen);
        if(u32Len <= 0)
        {
        	//ѭ����ȡjpeg.es��������
            lseek(_hFile[VdecChn], 0, SEEK_SET);
            continue;
        }

        stBufConf.stRawCfg.u32Size = u32Len;

        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        hSysBuf = MI_HANDLE_NULL;
        if (MI_SUCCESS != MI_SYS_ChnInputPortGetBuf(&stChnPort, &stBufConf, &stBufInfo, &hSysBuf,0))
        {
            lseek(_hFile[VdecChn], u32Pos, SEEK_SET);
            continue;
        }

        memcpy(stBufInfo.stRawData.pVirAddr, pu8Buf, u32Len);

        stBufInfo.eBufType = E_MI_SYS_BUFDATA_RAW;
        stBufInfo.bEndOfStream = FALSE;
        stBufInfo.u64Pts = u64Pts;
        stBufInfo.stRawData.bEndOfFrame = TRUE;
        stBufInfo.stRawData.u32ContentSize = u32Len;
        if (MI_SUCCESS != MI_SYS_ChnInputPortPutBuf(hSysBuf, &stBufInfo,FALSE))
        {
            lseek(_hFile[VdecChn], u32Pos, SEEK_SET);
            continue;
        }

        u64Pts = u64Pts + get_pts(30);

        memset(&stChnStat, 0x0, sizeof(stChnStat));
        MI_VDEC_GetChnStat(VdecChn, &stChnStat);
        printf("Chn(%d)_%s_Codec:%d, Frame Dec:%d\n", VdecChn, _aeCodecType[VdecChn], stChnStat.eCodecType, stChnStat.u32DecodeStreamFrames);
    }

    free(pu8Buf);
    return NULL;
}


void save_yuv422_data(MI_U8 *pYuv422Data, MI_U32 u32Width, MI_U32 u32Height, MI_U32 u32Pitch, MI_U32 u32Chn)
{
    FILE *fp = NULL;
    char aName[128];
    MI_U32 u32Length = u32Width * u32Height * 2;
    static MI_U32 u32Frmcnt[33] = {0};

    if (u32Frmcnt[u32Chn] > 5)
    {
        printf("get frame count:%d\n", u32Frmcnt[u32Chn]++);
        return;
    }
    memset(aName, 0x0, sizeof(aName));
    sprintf(aName, "/mnt/app_chn_%d_jpeg_dump_vdec[%d_%d_%d]_%d.yuv", u32Chn, u32Width, u32Height, u32Pitch, u32Frmcnt[u32Chn]);
    fp = fopen(aName, "wb+");
    if (!fp)
    {
        printf("Open File Faild\n");
        return;
    }

    lseek(fp, 0, SEEK_SET);
    if(fwrite(pYuv422Data, 1, u32Length, fp) != u32Length)
    {
        printf("fwrite %s failed\n", aName);
        goto _END;
    }

    printf("dump file(%s) ok ..............[len:%d]\n", aName, u32Length);
    u32Frmcnt[u32Chn]++;

_END:
    fclose(fp);
    fp = NULL;
}

void *get_frame_data(void* p)
{
    MI_SYS_ChnPort_t stChnPort;
    MI_SYS_BufInfo_t stBufInfo;
    MI_SYS_BUF_HANDLE hSysBuf;
    MI_U32 u32CheckSum = 0;
    MI_VDEC_CHN VdecChn = 0;

    memcpy(&stChnPort, p, sizeof(MI_SYS_ChnPort_t));
    VdecChn = stChnPort.u32ChnId;
    MI_SYS_SetChnOutputPortDepth(&stChnPort,5,10);
    while (_bRun[VdecChn])
    {
        hSysBuf = MI_HANDLE_NULL;
        usleep(30 * 1000);
        memset(&stBufInfo, 0x0, sizeof(MI_SYS_BufInfo_t));
        if (MI_SUCCESS != MI_SYS_ChnOutputPortGetBuf(&stChnPort, &stBufInfo, &hSysBuf))
        {
            continue;
        }

        if (stBufInfo.eBufType != E_MI_SYS_BUFDATA_FRAME)
        {
            printf("error eBufType:%d\n", stBufInfo.eBufType);
        }
        else
        {
            save_yuv422_data(stBufInfo.stFrameData.pVirAddr[0], stBufInfo.stFrameData.u16Width, stBufInfo.stFrameData.u16Height, stBufInfo.stFrameData.u32Stride[0], VdecChn);
        }

        if (MI_SUCCESS != MI_SYS_ChnOutputPortPutBuf(hSysBuf))
        {
            continue;
        }
    }

    return NULL;
}

void create_push_stream_thread(MI_VDEC_CHN VdecChn, MI_VDEC_CodecType_e eCodecType, MI_BOOL bCreateGetThead)
{
    MI_U8 pu8FileName[64];
    _bRun[VdecChn] = 1;
    if (eCodecType == E_MI_VDEC_CODEC_TYPE_H264)
    {
        sprintf(pu8FileName, "/mnt/h264.es", VdecChn);
        sprintf(_aeCodecType[VdecChn], "h264");
    }
    else if (eCodecType == E_MI_VDEC_CODEC_TYPE_H265)
    {
        sprintf(pu8FileName, "/mnt/h265.es", VdecChn);
        sprintf(_aeCodecType[VdecChn], "h265");
    }
    else if (eCodecType == E_MI_VDEC_CODEC_TYPE_JPEG)
    {
        sprintf(pu8FileName, "/mnt/jpeg.es", VdecChn);
        sprintf(_aeCodecType[VdecChn], "jpeg");
    }
    else
    {
        return;
    }

    printf("%s\n", pu8FileName);
    _hFile[VdecChn] = open(pu8FileName, O_RDONLY, 0);
    if (_hFile[VdecChn] >= 0)
    {
        if (pthread_create(&_thrPush[VdecChn], NULL, push_stream, VdecChn))
        {
            assert(0);
        }
        if (!bCreateGetThead)
        {
            return;
        }

        if (eCodecType == E_MI_VDEC_CODEC_TYPE_JPEG)
        {
            memset(&_astChnPort, 0x0, sizeof(MI_SYS_ChnPort_t));
            _astChnPort.eModId = E_MI_MODULE_ID_VDEC;
            _astChnPort.u32DevId = 0;
            _astChnPort.u32ChnId = VdecChn;
            _astChnPort.u32PortId = 0;
            if (pthread_create(&_thrGet[VdecChn], NULL, get_frame_data, &_astChnPort))
            {
                assert(0);
            }
        }
    }
}

void destroy_push_stream_thread(MI_VDEC_CHN VdecChn)
{
    if (_bRun[VdecChn])
    {
        _bRun[VdecChn] = 0;
        if (_thrPush[VdecChn])
        {
            pthread_join(_thrPush[VdecChn], NULL);
        }

        if (_thrGet[VdecChn])
        {
            pthread_join(_thrGet[VdecChn], NULL);
        }
    }

    if (_hFile[VdecChn] >= 0)
    {
        close(_hFile[VdecChn]);
    }
}

void StartTestCaseJPEG(MI_BOOL bCreateVdecDumpThread)
{
    MI_VDEC_ChnAttr_t stChnAttr;
    MI_VDEC_CHN VdecChn = 0;
    MI_U32 u32TestCaseNum = 0;
    MI_BOOL bCreateGetThead = TRUE;

    memset(_thrPush, 0x0, sizeof(_thrPush));
    memset(_thrGet, 0x0, sizeof(_thrGet));
    memset(_bRun, 0x0, sizeof(_bRun));

	switch(g_code_mode)
	{
		case 0:
		case 1:
			stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_JPEG;
			break;
		case 2:
			stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_H264;
			break;
		case 3:
			stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_H265;
			break;
		default:
			printf("\n	Get wrong Image code Mode = %d\n", g_code_mode);
	}

    //stChnAttr.eCodecType = E_MI_VDEC_CODEC_TYPE_JPEG;
    stChnAttr.stVdecVideoAttr.u32RefFrameNum = 7;
    stChnAttr.eVideoMode   = E_MI_VDEC_VIDEO_MODE_FRAME;
    stChnAttr.u32BufSize   = 1024 * 1024;
    stChnAttr.u32PicWidth  = 720; //g_width; //720;
    stChnAttr.u32PicHeight = 576; //g_height; //576;
    stChnAttr.u32Priority  = 0;
    ExecFunc(MI_VDEC_CreateChn(0, &stChnAttr), MI_SUCCESS);
    ExecFunc(MI_VDEC_StartChn(0), MI_SUCCESS);
    create_push_stream_thread(0, stChnAttr.eCodecType, bCreateVdecDumpThread);
}

void StopTestCaseJPEG(void)
{
    destroy_push_stream_thread(0);
    ExecFunc(MI_VDEC_StopChn(0), MI_SUCCESS);
    ExecFunc(MI_VDEC_DestroyChn(0), MI_SUCCESS);
}

void StartVdec(void)
{
    //StartTestCaseJPEG(FALSE);
    StartTestCaseJPEG(TRUE);
}

void bind_vdec_to_vdf(void)
{
    MI_U16 u32ChnId = 0;
    MI_U16 u32PortId = 0;
    MI_SYS_ChnPort_t stOutputChnPort;
    MI_SYS_ChnPort_t stInputChnPort;

    stOutputChnPort.eModId = E_MI_MODULE_ID_VDEC;
    stOutputChnPort.u32DevId = 0;
    stOutputChnPort.u32ChnId = u32ChnId;
    stOutputChnPort.u32PortId = u32PortId;

    for (u32ChnId=0; u32ChnId < MI_VDF_CHANNEL_NUM_MAX; u32ChnId++) {
	    stInputChnPort.eModId = E_MI_MODULE_ID_VDF;
	    stInputChnPort.u32DevId = 0;
	    stInputChnPort.u32ChnId = u32ChnId;
	    stInputChnPort.u32PortId = u32PortId;
	    ExecFunc(MI_SYS_BindChnPort(&stOutputChnPort, &stInputChnPort, 30, 30), MI_SUCCESS);
    }
}

////////////////////////////////////////////////////
