/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.
  
  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (��Sigmastar Confidential Information��) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/
//============================================================================
// Name        : VG_Sample_Code.c
// Author      : Chiao Su
// Description : Virtual Gate
//============================================================================
#include <stdio.h>
#include <stdlib.h>
#include "mi_vg.h"
#include <stdint.h>

#define width               (320)
#define height              (180)
#define size width * height
#define total_frame         (80)

#define size_thd            (5)
#define user_line_number    (2)
#define environment_state   (1)

unsigned char input[size];
unsigned char test_result[size];

int main()
{
	FILE *read;
	S32  frame;

	MI_VgResult_t cross_alarm;
	MI_VgSet_t user_info;
	MI_VG_HANDLE vg_handle;
	MI_VgLine_t line1, line2;

	MI_VG_SetObjSizeThd(&user_info, size_thd);  //Object Size Threshold

	MI_VG_SetScene(&user_info, environment_state);  //environment state (indoor or outdoor)

	MI_VG_SetLineNumber(&user_info,user_line_number);  //Line Number

	if( user_info.line_number >= 1 )
	{
		//First Line
		line1.px.x = 15;
		line1.px.y = 95;
		line1.py.x = 115;
		line1.py.y = 95;
		line1.pdx.x = 50;
		line1.pdx.y = 150;
		line1.pdy.x = 50;
		line1.pdy.y = 15;
		MI_VG_SetLineAndDir(&user_info, &line1, 1);
	}

	if( user_info.line_number == 2 )
	{
		//Second Line
        line2.px.x = 220;
        line2.px.y = 100;
        line2.py.x = 225;
        line2.py.y = 50;
        line2.pdx.x = 220;
        line2.pdx.y = 50;
        line2.pdy.x = 230;
        line2.pdy.y = 50;
        MI_VG_SetLineAndDir(&user_info, &line2, 2);
	}

	if( NULL == (vg_handle = MI_VG_Init(&user_info, width, height)) )
	{
		printf("MI VG initial err\n");
		vg_handle = NULL;
	}

	if((read = fopen("./Test_VG_Video.yuv","rb")) == NULL)
	{
		printf("input video could not be opened.\n");
		return -1;
	}

	for( frame = 0 ; frame < total_frame ; frame++ )
	{
		printf("frame = %d\n",frame);

		fread(input,size,1,read);

		MI_VG_Run(vg_handle, input);

		MI_VG_GetResult(vg_handle , &cross_alarm);

		if( cross_alarm.alarm1  == 1 )
		{
			printf("\n");
			printf("First Line Alarm!!!\n");
			printf("\n");
		}

		if( user_info.line_number == 2 )
		{
			if( cross_alarm.alarm2  == 1 )
			{
				printf("\n");
				printf("Second Line Alarm!!!\n");
				printf("\n");
			}
		}
	}

	MI_VG_Uninit(vg_handle);

	return 0;
}
