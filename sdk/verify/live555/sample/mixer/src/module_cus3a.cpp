#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include "isp_cus3a_if.h"
#include "mi_isp.h"
#include "module_common.h"

#define MOTOR_TEST 0

int mod1_isp_ae_init(void* pdata, ISP_AE_INIT_PARAM *init_state)
{
    printf("****[%s] ae_init ,shutter=%d,shutter_step=%d,sensor_gain_min=%d,sensor_gain_max=%d ****\n",
            __FUNCTION__,
            init_state->shutter,
            init_state->shutter_step,
            init_state->sensor_gain,
            init_state->sensor_gain_max
          );
    return 0;
}

void mod1_isp_ae_release(void* pdata)
{
    printf("****[%s] cus3e release ****\n", __FUNCTION__);
}

void mod1_isp_ae_run(void* pdata, const ISP_AE_INFO *info, ISP_AE_RESULT *result)
{
#define log_info 0

    // Only one can be chosen (the following three define)
#define shutter_test  0
#define gain_test     0
#define AE_sample     1


    static int AE_period = 4;
    static unsigned int fcount = 0;

    unsigned int max = info->AvgBlkY*info->AvgBlkX;
    unsigned int avg=0;
    unsigned int n;

    result->Change              = 0;
    result->u4BVx16384          = 16384;
    result->HdrRatio            = 10; //infinity5 //TBD //10 * 1024;   //user define hdr exposure ratio
    result->IspGain             = 1024;
    result->SensorGain          = 4096;
    result->Shutter             = 20000;
    result->IspGainHdrShort     = 1024;
    result->SensorGainHdrShort  = 1024;
    result->ShutterHdrShort     = 1000;
    //result->Size         = sizeof(CusAEResult_t);

    for(n=0;n<max;++n)
    {
        avg += info->avgs[n].y;
    }
    avg /= max;

    result->AvgY         = avg;
#if shutter_test // shutter test under constant sensor gain
    MI_U32 Shutter_Step = 100; //per frame
    MI_U32 Shutter_Max = 33333;
    MI_U32 Shutter_Min = 150;
    MI_U32 Gain_Constant = 10240;
    static int tmp=0;
    static unsigned int fcount = 0;
    result->SensorGain = Gain_Constant;
    result->Shutter = info->Shutter;

    if(++fcount%AE_period == 0)
    {
        if (tmp==0){
            result->Shutter = info->Shutter + Shutter_Step*AE_period;
            //printf("[shutter-up] result->Shutter = %d \n", result->SensorGain);
        }else{
            result->Shutter = info->Shutter - Shutter_Step*AE_period;
            //printf("[shutter-down] result->Shutter = %d \n", result->SensorGain);
        }
        if (result->Shutter >= Shutter_Max){
            result->Shutter = Shutter_Max;
            tmp=1;
        }
        if (result->Shutter <= Shutter_Min){
            result->Shutter = Shutter_Min;
            tmp=0;
        }
    }
#if log_info
    printf("fcount = %d, Image avg = 0x%X \n", fcount, avg);
    printf("tmp = %d, Shutter: %d -> %d \n", tmp, info->Shutter, result->Shutter);
#endif
#endif

#if gain_test // gain test under constant shutter
    MI_U32 Gain_Step = 1024; //per frame
    MI_U32 Gain_Max = 1024*100;
    MI_U32 Gain_Min = 1024*2;
    MI_U32 Shutter_Constant = 20000;
    static int tmp1=0;
    result->SensorGain = info->SensorGain;
    result->Shutter = Shutter_Constant;

    if(++fcount%AE_period == 0)
    {
        if (tmp1==0){
            result->SensorGain = info->SensorGain + Gain_Step*AE_period;
            //printf("[gain-up] result->SensorGain = %d \n", result->SensorGain);
        }else{
            result->SensorGain = info->SensorGain - Gain_Step*AE_period;
            //printf("[gain-down] result->SensorGain = %d \n", result->SensorGain);
        }
        if (result->SensorGain >= Gain_Max){
            result->SensorGain = Gain_Max;
            tmp1=1;
        }
        if (result->SensorGain <= Gain_Min) {
            result->SensorGain = Gain_Min;
            tmp1=0;
        }
    }
#if log_info
    printf("fcount = %d, Image avg = 0x%X \n", fcount, avg);
    printf("tmp = %d, SensorGain: %d -> %d \n", tmp, info->SensorGain, result->SensorGain);
#endif
#endif

#if AE_sample
    MI_U32 y_lower = 0x28;
    MI_U32 y_upper = 0x38;
    MI_U32 change_ratio = 10; // percentage
    MI_U32 Gain_Min = 1024*2;
    MI_U32 Gain_Max = 1024*1000;
    MI_U32 Shutter_Min = 150;
    MI_U32 Shutter_Max = 33333;

    result->SensorGain = info->SensorGain;
    result->Shutter = info->Shutter;
    result->SensorGainHdrShort = info->SensorGainHDRShort;
    result->ShutterHdrShort = info->ShutterHDRShort;

    if(++fcount%AE_period == 0)
    {
        if(avg<y_lower){
            if (info->Shutter<Shutter_Max){
                result->Shutter = info->Shutter + (info->Shutter*change_ratio/100);
                if (result->Shutter > Shutter_Max) result->Shutter = Shutter_Max;
            }else{
                result->SensorGain = info->SensorGain + (info->SensorGain*change_ratio/100);
                if (result->SensorGain > Gain_Max) result->SensorGain = Gain_Max;
            }
            result->Change = 1;
        }else if(avg>y_upper){
            if (info->SensorGain>Gain_Min){
                result->SensorGain = info->SensorGain - (info->SensorGain*change_ratio/100);
                if (result->SensorGain < Gain_Min) result->SensorGain = Gain_Min;
            }else{
                result->Shutter = info->Shutter - (info->Shutter*change_ratio/100);
                if (result->Shutter < Shutter_Min) result->Shutter = Shutter_Min;
            }
            result->Change = 1;
        }

        //hdr demo code
        result->SensorGainHdrShort = result->SensorGain;
        result->ShutterHdrShort = result->Shutter / result->HdrRatio;

    }

#if log_info
    printf("fcount = %d, Image avg = 0x%X \n", fcount, avg);
    printf("SensorGain: %d -> %d \n", info->SensorGain, result->SensorGain);
    printf("Shutter: %d -> %d \n", info->Shutter, result->Shutter);
    printf("SensorGainHDR: %d -> %d \n", info->SensorGainHDRShort, result->SensorGainHdrShort);
    printf("ShutterHDR: %d -> %d \n", info->ShutterHDRShort, result->ShutterHdrShort);
#endif

#endif

}

int mod1_isp_awb_init(void *pdata)
{
    printf("****[%s] awb_init ****\n", __FUNCTION__);
    return 0;
}

void mod1_isp_awb_run(void* pdata, const ISP_AWB_INFO *info, ISP_AWB_RESULT *result)
{
#define log_info 0

    static u32 count = 0;
    MI_U32 avg_r = 0;
    MI_U32 avg_g = 0;
    MI_U32 avg_b = 0;
    MI_U32 tar_rgain = 1024;
    MI_U32 tar_bgain = 1024;
    MI_U32 x = 0;
    MI_U32 y = 0;

    result->R_gain = info->CurRGain;
    result->G_gain = info->CurGGain;
    result->B_gain = info->CurBGain;
    result->Change = 0;
    result->ColorTmp = 6000;

    if (count++ % 4 == 0)
    {
        //center area YR/G/B avg
        for (y = 30; y<60; ++y)
        {
            for (x = 32; x<96; ++x)
            {
                avg_r += info->avgs[info->AvgBlkX*y + x].r;
                avg_g += info->avgs[info->AvgBlkX*y + x].g;
                avg_b += info->avgs[info->AvgBlkX*y + x].b;
            }
        }
        avg_r /= 30 * 64;
        avg_g /= 30 * 64;
        avg_b /= 30 * 64;

        if (avg_r <1)
            avg_r = 1;
        if (avg_g <1)
            avg_g = 1;
        if (avg_b <1)
            avg_b = 1;

#if log_info
        printf("AVG R / G / B = %d, %d, %d \n", avg_r, avg_g, avg_b);
#endif

        // calculate Rgain, Bgain
        tar_rgain = avg_g * 1024 / avg_r;
        tar_bgain = avg_g * 1024 / avg_b;

        if (tar_rgain > info->CurRGain) {
            if (tar_rgain - info->CurRGain < 384)
                result->R_gain = tar_rgain;
            else
                result->R_gain = info->CurRGain + (tar_rgain - info->CurRGain)/10;
        }else{
            if (info->CurRGain - tar_rgain < 384)
                result->R_gain = tar_rgain;
            else
                result->R_gain = info->CurRGain - (info->CurRGain - tar_rgain)/10;
        }

        if (tar_bgain > info->CurBGain) {
            if (tar_bgain - info->CurBGain < 384)
                result->B_gain = tar_bgain;
            else
                result->B_gain = info->CurBGain + (tar_bgain - info->CurBGain)/10;
        }else{
            if (info->CurBGain - tar_bgain < 384)
                result->B_gain = tar_bgain;
            else
                result->B_gain = info->CurBGain - (info->CurBGain - tar_bgain)/10;
        }

        result->Change = 1;
        result->G_gain = 1024;

        if (count == 1) {
            result->R_gain = tar_rgain;
            result->B_gain = tar_bgain;
        }

#if log_info
        printf("[current] r=%ld, g=%d, b=%ld \n", info->CurRGain, info->CurGGain, info->CurBGain);
        printf("[result] r=%ld, g=%d, b=%ld \n", result->R_gain, result->G_gain, result->B_gain);
#endif
    }
}

void mod1_isp_awb_release(void *pdata)
{
    printf("****[%s] awb_release ****\n", __FUNCTION__);
}


#if MOTOR_TEST
int af_init_done=0;
extern int af_pos1;
extern int af_zoom1;
#define RANGE(val, range) ((val>=range-5) && (val<=range+5))

void mod1_isp_af_motor_init()
{
    //init motor
    MI_S32 moto[1];
    moto[0] = 0;
    mixer_send_cmd(CMD_MOTOR_INIT, (MI_S8 *) moto, sizeof(moto));

    //set delay time, and move to zoom pos=1000
    moto[0] = 10000;
    mixer_send_cmd(CMD_MOTOR_DelayMs, (MI_S8 *) moto, sizeof(moto));
    moto[0] = 2;
    mixer_send_cmd(CMD_MOTOR_CONTROL, (MI_S8 *) moto, sizeof(moto));
}
#endif

int mod1_isp_af_init(void *pdata, ISP_AF_INIT_PARAM *param)
{
    MI_U32 u32ch = 0;
    MI_U8 u8win_idx = 16;
    CusAFRoiMode_t taf_roimode;

    printf("************ af_init **********\n");

#if 1

    //Init Normal mode setting
    taf_roimode.mode = AF_ROI_MODE_NORMAL;
    taf_roimode.u32_vertical_block_number = 1;
    MI_ISP_CUS3A_SetAFRoiMode(u32ch, &taf_roimode);

    static CusAFWin_t afwin[16] =
    {
        // x_start need to start from 16

        //{ 0, {   16,    0,  1023,  1023}},  //for full image
        { 0, {  16,    0,  255,  255}},
        { 1, { 256,    0,  511,  255}},
        { 2, { 512,    0,  767,  255}},
        { 3, { 768,    0, 1023,  255}},
        { 4, {  16,  256,  255,  511}},
        { 5, { 256,  256,  511,  511}},
        { 6, { 512,  256,  767,  511}},
        { 7, { 768,  256, 1023,  511}},
        { 8, {  16,  512,  255,  767}},
        { 9, { 256,  512,  511,  767}},
        {10, { 512,  512,  767,  767}},
        {11, { 768,  512, 1023,  767}},
        {12, {  16,  768,  255, 1023}},
        {13, { 256,  768,  511, 1023}},
        {14, { 512,  768,  767, 1023}},
        {15, { 768,  768, 1023, 1023}}
    };
    for(u8win_idx = 0; u8win_idx < 16; ++u8win_idx)
    {
        MI_ISP_CUS3A_SetAFWindow(u32ch, &afwin[u8win_idx]);
    }

#else
	//Init Matrix mode setting
	taf_roimode.mode = AF_ROI_MODE_MATRIX;
	taf_roimode.u32_vertical_block_number = 16; //16xN, N=16
	MI_ISP_CUS3A_SetAFRoiMode(u32ch, &taf_roimode);

	static CusAFWin_t afwin[16] =
	{
        // x_start need to start from 16

		//full image, equal divide to 16x16
		{0, {16, 0, 63, 63}},
		{1, {64, 64, 127, 127}},
		{2, {128, 128, 191, 191}},
		{3, {192, 192, 255, 255}},
		{4, {256, 256, 319, 319}},
		{5, {320, 320, 383, 383}},
		{6, {384, 384, 447, 447}},
		{7, {448, 448, 511, 511}},
		{8, {512, 512, 575, 575}},
		{9, {576, 576, 639, 639}},
		{10, {640, 640, 703, 703}},
		{11, {704, 704, 767, 767}},
		{12, {768, 768, 831, 831}},
		{13, {832, 832, 895, 895}},
		{14, {896, 896, 959, 959}},
		{15, {960, 960, 1023, 1023}}

		/*
		//use two row only => 16x2 win
		{0, {16, 0, 63, 63}},
		{1, {64, 64, 127, 127}},
		{2, {128, 0, 191, 2}},		//win2 v_str, v_end doesn't use, set to (0, 2)
		{3, {192, 0, 255, 2}},
		{4, {256, 0, 319, 2}},
		{5, {320, 0, 383, 2}},
		{6, {384, 0, 447, 2}},
		{7, {448, 0, 511, 2}},
		{8, {512, 0, 575, 2}},
		{9, {576, 0, 639, 2}},
		{10, {640, 0, 703, 2}},
		{11, {704, 0, 767, 2}},
		{12, {768, 0, 831, 2}},
		{13, {832, 0, 895, 2}},
		{14, {896, 0, 959, 2}},
		{15, {960, 0, 1023, 2}}
		*/
	};

	for(u8win_idx = 0; u8win_idx < 16; ++u8win_idx)
	{
		MI_ISP_CUS3A_SetAFWindow(u32ch, &afwin[u8win_idx]);
	}
#endif

#if TARGET_CHIP_I6E
	//set AF Filter
    static CusAFFilter_t affilter =
    {
        //filter setting with sign value
        //{s9, s10, s9, s7, s7}
        //high: 37, 0, -37, 83, 40; 37, 0, -37, -54, 34; 32, 0, -32, 14, 0
        //low:  15, 0, -15, -79, 44; 15, 0, -15, -115, 55; 14, 0, -14, -91, 37

        //convert to hw format (sign bit with msb)
        37, 0, 37+512, 83, 40, 0, 1023, 0, 1023,	//high
        15, 0, 15+512, 79+128, 44, 0, 1023, 0, 1023,	//low
		1, 37, 0, 37+512,  54+128, 34, 1, 32, 0, 32+512, 14, 0,	//high-e1, e2
		1, 15, 0, 15+512, 115+128, 55, 1, 14, 0, 14+512, 91+128, 37			//low-e1, e2
    };
#else
	//set AF Filter
    static CusAFFilter_t affilter =
    {
		//filter setting with sign value
		//{63, -126, 63, -109, 48, 0, 320, 0, 1023},
		//{63, -126, 63, 65, 55, 0, 320, 0, 1023}

		//convert to hw format (sign bit with msb)
		63, 126 + 1024, 63, 109 + 128, 48, 0, 320, 0, 1023,
		63, 126 + 1024, 63, 65, 55, 0, 320, 0, 1023,
    };
#endif

    MI_ISP_CUS3A_SetAFFilter(0, &affilter);

	//set AF Sq
/*	CusAFFilterSq_t sq = {
		.bSobelYSatEn = 0,
		.u16SobelYThd = 1023,
		.bIIRSquareAccEn = 0,
		.bSobelSquareAccEn = 0,
		.u16IIR1Thd = 0,
		.u16IIR2Thd = 0,
		.u16SobelHThd = 0,
		.u16SobelVThd = 0,
		.u8AFTblX = {6,7,7,6,6,6,7,6,6,7,6,6,},
		.u16AFTblY = {0,1,53,249,431,685,1023,1999,2661,3455,5487,6749,8191},
	};
	*/

	//MI_ISP_CUS3A_SetAFFilterSq(0, &sq);

#if MOTOR_TEST
    mod1_isp_af_motor_init();
#endif

    printf("****[%s] af_init done ****\n", __FUNCTION__);

    return 0;
}

void mod1_isp_af_release(void *pdata)
{
    printf("****[%s] af_release ****\n", __FUNCTION__);
}

void mod1_isp_af_run(void *pdata, const ISP_AF_INFO *af_info, ISP_AF_RESULT *result)
{

#if MOTOR_TEST

    static MI_S32 moto[1];
    static int frame_cnt = 0;
    int i = 0, x = 0;

    int af_pos = af_pos1;
    int af_zoom = af_zoom1;

    switch(frame_cnt%5)
    {
        case 0:
        {
            //zoom out
            if(af_init_done == 1)
            {
                if (af_zoom < 1800)
                {
                    moto[0] = 100;
                    mixer_send_cmd(CMD_MOTOR_DelayMs, (MI_S8 *) moto, sizeof(moto));

                    moto[0] = 2; //++
                    mixer_send_cmd(CMD_MOTOR_CONTROL, (MI_S8 *) moto, sizeof(moto));
                }
            }
            //zoom middle
            else if(af_init_done == 2)
            {
                if (af_zoom > 1100)
                {
                    moto[0] = 100;
                    mixer_send_cmd(CMD_MOTOR_DelayMs, (MI_S8 *) moto, sizeof(moto));

                    //base on zoom out
                    moto[0] = 3; //--
                    mixer_send_cmd(CMD_MOTOR_CONTROL, (MI_S8 *) moto, sizeof(moto));
                }
            }
            //zoom in
            else if(af_init_done == 3)
            {
                if (af_zoom > 100)
                {
                    moto[0] = 100;
                    mixer_send_cmd(CMD_MOTOR_DelayMs, (MI_S8 *) moto, sizeof(moto));

                    moto[0] = 3; //--
                    mixer_send_cmd(CMD_MOTOR_CONTROL, (MI_S8 *) moto, sizeof(moto));
                }
            }
            else if(af_init_done == 6)
            {
                //++
                int val, speed;
                if (RANGE(af_zoom,100))          {val = 360; speed = 100;}
                else if (RANGE(af_zoom,1100))    {val = 310; speed = 100;}
                else if (RANGE(af_zoom,1800))    {val = 440; speed = 100;}

                if (af_pos < val)
                {
                    moto[0] = speed;
                    mixer_send_cmd(CMD_MOTOR_DelayMs, (MI_S8 *) moto, sizeof(moto));

                    moto[0] = 1; //++
                    mixer_send_cmd(CMD_MOTOR_CONTROL, (MI_S8 *) moto, sizeof(moto));
                }
            }
            //back to wide
            else if(af_init_done == 7)
            {
                //--
                int val, speed;
                if (RANGE(af_zoom,100))          {val = 130; speed = 100;}
                else if (RANGE(af_zoom,1100))    {val = 230; speed = 100;}
                else if (RANGE(af_zoom,1800))    {val = 380; speed = 100;}

                if (af_pos > val)
                {
                    moto[0] = speed;
                    mixer_send_cmd(CMD_MOTOR_DelayMs, (MI_S8 *) moto, sizeof(moto));

                    moto[0] = 0; //--
                    mixer_send_cmd(CMD_MOTOR_CONTROL, (MI_S8 *) moto, sizeof(moto));
                }
            }
        }
        break;

        case 4:
        {
            if ( af_init_done == 9)
                printf("\n[moto]pos:%d,zoom:%d", af_pos, af_zoom);

            if ( ((RANGE(af_zoom,100))  && af_pos > 130 && af_pos < 360) ||
                 ((RANGE(af_zoom,1100)) && af_pos > 230 && af_pos < 310) ||
                 ((RANGE(af_zoom,1800)) && af_pos > 380 && af_pos < 440) ||
                 af_init_done == 9)
            printf("--[AF]win%d-%d iir0:0x%02x%02x%02x%02x%02x, iir1:0x%02x%02x%02x%02x%02x, sobelh:0x%02x%02x%02x%02x%02x, sobelv:0x%02x%02x%02x%02x%02x ",
               x, i,
               af_info->af_stats.stParaAPI[x].high_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].low_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].sobel_h[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[0 + i * 5],
               af_info->af_stats.stParaAPI[x].sobel_v[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[0 + i * 5]
              );
        }
        break;
    }

    frame_cnt++;
#else

    #define log_info 0

    #if log_info
    int i = 0, x = 0;

    //printf("\n\n");

    //print row0 16wins
    x = 0;
    for (i = 0; i < 16; i++)
    {
        printf("\n[AF]win%d-%d iir0:0x%02x%02x%02x%02x%02x, iir1:0x%02x%02x%02x%02x%02x, luma:0x%02x%02x%02x%02x, sobelh:0x%02x%02x%02x%02x%02x, sobelv:0x%02x%02x%02x%02x%02x ysat:0x%02x%02x%02x",
               x, i,
               af_info->af_stats.stParaAPI[x].high_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].low_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].luma[3 + i * 4], af_info->af_stats.stParaAPI[x].luma[2 + i * 4], af_info->af_stats.stParaAPI[x].luma[1 + i * 4], af_info->af_stats.stParaAPI[x].luma[0 + i * 4],
               af_info->af_stats.stParaAPI[x].sobel_h[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[0 + i * 5],
               af_info->af_stats.stParaAPI[x].sobel_v[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[0 + i * 5],
               af_info->af_stats.stParaAPI[x].ysat[2 + i * 3], af_info->af_stats.stParaAPI[x].ysat[1 + i * 3], af_info->af_stats.stParaAPI[x].ysat[0 + i * 3]
              );
    }

    //print row15 16wins
    x = 15;
    for (i = 0; i < 16; i++)
    {
        printf("[AF]win%d-%d iir0: 0x%02x%02x%02x%02x%02x, iir1:0x%02x%02x%02x%02x%02x, luma:0x%02x%02x%02x%02x, sobelh:0x%02x%02x%02x%02x%02x, sobelv:0x%02x%02x%02x%02x%02x ysat:0x%02x%02x%02x\n",
               x, i,
               af_info->af_stats.stParaAPI[x].high_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].high_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].low_iir[4 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[3 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[2 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[1 + i * 5], af_info->af_stats.stParaAPI[x].low_iir[0 + i * 5],
               af_info->af_stats.stParaAPI[x].luma[3 + i * 4], af_info->af_stats.stParaAPI[x].luma[2 + i * 4], af_info->af_stats.stParaAPI[x].luma[1 + i * 4], af_info->af_stats.stParaAPI[x].luma[0 + i * 4],
               af_info->af_stats.stParaAPI[x].sobel_h[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_h[0 + i * 5],
               af_info->af_stats.stParaAPI[x].sobel_v[4 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[3 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[2 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[1 + i * 5], af_info->af_stats.stParaAPI[x].sobel_v[0 + i * 5],
               af_info->af_stats.stParaAPI[x].ysat[2 + i * 3], af_info->af_stats.stParaAPI[x].ysat[1 + i * 3], af_info->af_stats.stParaAPI[x].ysat[0 + i * 3]
              );
    }
    #endif

#endif
}

int mod1_isp_af_ctrl(void *pdata, ISP_AF_CTRL_CMD cmd, void* param)
{
    return 0;
}


/************************************************************ CUS3A UT ************************************************************/
int ut_isp_ae_init(void* pdata, ISP_AE_INIT_PARAM *init_state)
{
    printf("AeInitParam:\n"
            " shutter=%d\n"
            " shutter_step=%d\n"
            " shutter_min=%d\n"
            " shutter_max=%d\n"
            " sensor_gain=%d\n"
            " sensor_gain_min=%d\n"
            " sensor_gain_max=%d\n"
            " isp_gain=%d\n"
            " isp_gain_max=%d\n"
            " FNx10=%d\n"
            " fps=%d\n",
            init_state->shutter,
            init_state->shutter_step,
            init_state->shutter_min,
            init_state->shutter_max,
            init_state->sensor_gain,
            init_state->sensor_gain_min,
            init_state->sensor_gain_max,
            init_state->isp_gain,
            init_state->isp_gain_max,
            init_state->FNx10,
            init_state->fps
          );
    printf(" shutterHDRShort_step=%d\n"
            " shutterHDRShort_min=%d\n"
            " shutterHDRShort_max=%d\n"
            " sensor_gainHDRShort_min=%d\n"
            " sensor_gainHDRShort_max=%d\n"
            " AvgBlkX=%d\n"
            " AvgBlkY=%d\n",
            init_state->shutterHDRShort_step,
            init_state->shutterHDRShort_min,
            init_state->shutterHDRShort_max,
            init_state->sensor_gainHDRShort_min,
            init_state->sensor_gainHDRShort_max,
            init_state->AvgBlkX,
            init_state->AvgBlkY
            );
    return 0;
}

void ut_isp_ae_release(void* pdata)
{
    printf("****[%s] cus3e release ****\n", __FUNCTION__);
}

void ut_isp_ae_run(void* pdata, const ISP_AE_INFO *info, ISP_AE_RESULT *result)
{
    unsigned int *pAeCnt = (unsigned int*) pdata;
    printf("AeParam:\n"
           " hist1=0x%X\n"
           " hist2=0x%X\n"
           " AvgBlkX=%d\n"
           " AvgBlkY=%d\n"
           " avgs=0x%X\n"
           " Shutter=%d\n"
           " SensorGain=%d\n"
           " IspGain=%d\n"
           " ShutterHdrShort=%d\n"
           " SensorGainHdrShort=%d\n"
           " IspGainHdrShort=%d\n"
           " PreAvgY=%d\n",
           (unsigned int)info->hist1,
           (unsigned int)info->hist2,
           info->AvgBlkX,
           info->AvgBlkY,
           (unsigned int)info->avgs,
           info->Shutter,
           info->SensorGain,
           info->IspGain,
           info->ShutterHDRShort,
           info->SensorGainHDRShort,
           info->IspGainHDRShort,
           info->PreAvgY
    );
    printf(" HDRCtlMode=%d\n"
           " FNx10=%d\n"
           " CurFPS=%d\n"
           " PreWeightY=%d\n",
           info->HDRCtlMode,
           info->FNx10,
           info->CurFPS,
           info->PreWeightY
    );

    if(pAeCnt==0)
    {
        result->Change              = 0;
        result->Shutter             = 15000;
        result->SensorGain          = 2048;
        result->IspGain             = 1024;
        result->ShutterHdrShort     = 1000;
        result->SensorGainHdrShort  = 1024;
        result->IspGainHdrShort     = 1024;
        result->u4BVx16384          = 16384;
        result->AvgY                = 64;
        result->HdrRatio            = 10;
        result->FNx10               = 18;
        result->DebandFPS           = 25;
        result->WeightY             = 1024;
    }
    else
    {
        result->Change              = 0;
        result->Shutter             = info->Shutter + 1000;
        result->SensorGain          = info->SensorGain + 1024;
        result->IspGain             = info->IspGain + 1024;
        result->ShutterHdrShort     = info->ShutterHDRShort + 1000;
        result->SensorGainHdrShort  = info->SensorGainHDRShort + 1000;
        result->IspGainHdrShort     = info->IspGainHDRShort + 1024;
        result->u4BVx16384          = 16384;
        result->AvgY                = 64;
        result->HdrRatio            = 10;
        result->FNx10               = 18;
        result->DebandFPS           = 25;
        result->WeightY             = 1024;
    }
    if(pAeCnt)pAeCnt++;
}

int ut_isp_awb_init(void *pdata)
{
    printf("****[%s] awb_init ****\n", __FUNCTION__);
    return 0;
}

void ut_isp_awb_run(void* pdata, const ISP_AWB_INFO *info, ISP_AWB_RESULT *result)
{
    //unsigned int *pAwbCnt = (unsigned int) pdata;

    printf("AwbParam:\n"
            " AvgBlkX=%d\n"
            " AvgBlkY=%d\n"
            " Rgain=%d\n"
            " Ggain=%d\n"
            " Bgain=%d\n"
            " avgs=0x%X\n",
            info->AvgBlkX,
            info->AvgBlkY,
            info->CurRGain,
            info->CurGGain,
            info->CurBGain,
            (unsigned int)info->avgs
    );
    printf( " HDRMode=%d\n"
            " pAwbStatisShort=0x%X\n"
            " u4BVx16384=%d\n"
            " WeightY=%d\n",
            info->HDRMode,
            (unsigned int)info->pAwbStatisShort,
            info->u4BVx16384,
            info->WeightY
    );

    result->R_gain = info->CurRGain;
    result->G_gain = info->CurGGain;
    result->B_gain = info->CurBGain;
    result->Change = 0;
    result->ColorTmp = 6000;

    //pAwbCnt++;
}

void ut_isp_awb_release(void *pdata)
{
    printf("****[%s] awb_release ****\n", __FUNCTION__);
}
