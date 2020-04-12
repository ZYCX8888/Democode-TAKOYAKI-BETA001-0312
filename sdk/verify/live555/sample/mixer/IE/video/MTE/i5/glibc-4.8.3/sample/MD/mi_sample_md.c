/*
* mi_sample_md.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: XXXX <XXXX@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
#include <stdio.h>
#include <stdlib.h>
#include "mi_md.h"

#define RAW_W		320
#define RAW_H		240
#define DIV_W		10
#define DIV_H		12
#define MULTI_REGION

unsigned char RAW_A[RAW_W*RAW_H];
unsigned char RAW_B[RAW_W*RAW_H];

int MMPF_OsCounterGetMs(void)
{
	return 10000;
}

#define MD_INPUT_ARGUMENT_NUM (5)

int main(int argc, char *argv[])
{
	MD_HANDLE MD_HANDLE_0;

	char * infileName = "0000.raw";
	char * infileName2 = "0030.raw";
	int t, old_t;
	static int cnt = 0;
	unsigned char *RAW_PTR;
	int result, result_2, i, j, ret;
	FILE *pFile, *pFile_2;

	// read raw images
	pFile = fopen ( infileName , "rb" );
	pFile_2 = fopen ( infileName2 , "rb" );
	if ( (pFile==NULL) || (pFile_2==NULL) ) {printf("File open error!!!\n"); return (-1);}
	result = fread (RAW_A, 1, RAW_W*RAW_H, pFile);
	result_2 = fread (RAW_B, 1, RAW_W*RAW_H, pFile_2);
	if ( (result != RAW_W*RAW_H) || (result_2 != RAW_W*RAW_H) ) {printf("File read error!!!\n"); return (-1);}
	fclose(pFile);
	fclose(pFile_2);

#ifndef MULTI_REGION
	MDParamsOut_t param_out;
	MDParamsIn_t param_in = {1, 2, 100, 70, 2000};

	MD_HANDLE_0 = MI_MD_Init(RAW_W, RAW_H, 1, DIV_W, DIV_H);
	MI_MD_SetDetectWindow(0, 0, RAW_W-1, RAW_H-1, DIV_W, DIV_H);

	for (i = 0; i < DIV_W; i++)
	{
		for (j = 0; j < DIV_H; j++)
		{
			MI_MD_SetWindowParamsIn(i, j, &param_in);
			MI_MD_GetWindowParamsIn(i, j, &param_in);
			printf("IN: [%d,%d]=(%d,%d,%d,%d,%d)\n", i, j, param_in.enable, param_in.size_perct_thd_min, param_in.size_perct_thd_max,
				param_in.learn_rate, param_in.sensitivity);
		}
	}

	RAW_PTR = RAW_A;

	for (i = 0; i < 2; i ++)
	{
		t = MMPF_OsCounterGetMs(); // get system time in ms
		if (cnt > 0)
		{
			// run MD second time
			RAW_PTR = RAW_B;
			MI_MD_SetTime(t - old_t); // need to take care of timer overflow
		}
		ret = MI_MD_Run(RAW_PTR);
		old_t = t;
		cnt ++;
	}

	if (ret == -1)
		printf("MD error!!!\n");
	else if (ret == 0)
		printf("No motion detected.\n");
	else if (ret > 0)
	{
		for (i = 0; i < DIV_W; i++)
		{
			for (j = 0; j < DIV_H; j++)
			{

				MI_MD_GetWindowParamsOut(i, j, &param_out);
				if(param_out.md_result)
					printf("OUT: [%d,%d]=(%d,%d)\n", i, j, param_out.md_result, param_out.obj_cnt);
			}
		}
	}

#else
	int k;
	int region_num = 4;
	MDParamsOut_t param_out[region_num];
	MDParamsIn_t param_in[region_num];
	MDBlockInfo_t info[region_num];

	for(i=0; i<region_num; i++)
	{
		param_in[i].enable = 1;
		param_in[i].size_perct_thd_min = 3;
		param_in[i].size_perct_thd_max = 100;
		param_in[i].sensitivity = 70;
		param_in[i].learn_rate = 2000;
	}

	/*param_in[0].enable = 1;
	param_in[0].size_perct_thd_min = 2;
	param_in[0].size_perct_thd_max = 100;
	param_in[0].sensitivity = 70;
	param_in[0].learn_rate = 2000;
	param_in[1].enable = 1;
	param_in[1].size_perct_thd_min = 4;
	param_in[1].size_perct_thd_max = 100;
	param_in[1].sensitivity = 70;
	param_in[1].learn_rate = 3000;
	param_in[2].enable = 1;
	param_in[2].size_perct_thd_min = 10;
	param_in[2].size_perct_thd_max = 100;
	param_in[2].sensitivity = 80;
	param_in[2].learn_rate = 2500;     */

	/*for(i=0; i<region_num; i++)
	{
		info[i].st_x = 0 + (i%8) * 40;
		info[i].end_x = info[i].st_x + 31;
		info[i].st_y = 0 + (i/8) * 30;
		info[i].end_y = info[i].st_y + 23;
	}*/

	info[0].st_x =  30;
	info[0].st_y =  30;
	info[0].end_x = 300;
	info[0].end_y = 120;
	info[1].st_x =  30;
	info[1].st_y =  30;
	info[1].end_x = 150;
	info[1].end_y = 220;
	info[2].st_x =  50;
	info[2].st_y =  60;
	info[2].end_x = 180;
	info[2].end_y = 190;
	info[3].st_x =  165;
	info[3].st_y =  150;
	info[3].end_x = 310;
	info[3].end_y = 230;


	MD_HANDLE_0 = MI_MD_Init(RAW_W, RAW_H, 1, 1, 1);
	MI_MD_SetRegionInfo(region_num, info);

	MI_MD_SetWindowParamsIn(0, 0, param_in);
	MI_MD_GetWindowParamsIn(0, 0, param_in);
	for(k = 0 ; k < region_num ; k++)
		printf("IN: region %d =(%d,%d,%d,%d,%d)\n", k, param_in[k].enable, param_in[k].size_perct_thd_min, param_in[k].size_perct_thd_max,
			param_in[k].learn_rate, param_in[k].sensitivity);


	RAW_PTR = RAW_A;

	for (i = 0; i < 2; i ++)
	{
		t = MMPF_OsCounterGetMs(); // get system time in ms
		if (cnt > 0)
		{
			// run MD second time
			RAW_PTR = RAW_B;
			MI_MD_SetTime(t - old_t); // need to take care of timer overflow
		}
		ret = MI_MD_Run(RAW_PTR);
		old_t = t;
		cnt ++;
	}

	if (ret == -1)
		printf("MD error!!!\n");
	else if (ret == 0)
		printf("No motion detected.\n");
	else if (ret > 0)
	{
		for (i = 0; i < 1; i++)
		{
			for (j = 0; j < 1; j++)
			{
				MI_MD_GetWindowParamsOut(i, j, param_out);
				for(k = 0 ; k < region_num ; k++){
					if(param_out[k].md_result)
						printf("OUT: region %d =(%d,%d)\n", k, param_out[k].md_result, param_out[k].obj_cnt);
				}
			}
		}
	}
#endif

	MI_MD_Uninit(MD_HANDLE_0);
	return 0;
}
