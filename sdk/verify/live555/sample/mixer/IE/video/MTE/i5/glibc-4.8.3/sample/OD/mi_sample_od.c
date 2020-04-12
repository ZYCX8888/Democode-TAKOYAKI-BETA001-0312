/*
* mi_sample_od.c- Sigmastar
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
#include <math.h>
#include "mi_od.h"

#define RAW_W		320
#define RAW_H		180
#define DIV_W		3
#define DIV_H		3


int main()
{
	OD_HANDLE OD_HANDLE_0;
	unsigned char input_y[RAW_W * RAW_H];	
	char infileName[512];
	FILE *fin;	
	int frm_counter = 0, x, y;
	int image_size = RAW_W * RAW_H;
	int tamper_result;

	

	sprintf(infileName, "test_img.y");	
	fin = fopen(infileName, "rb");
	if (!fin)
	{
		printf("the input file could not be open\n");
		return -1;
	}	
	
	OD_HANDLE_0 = MI_OD_Init(RAW_W, RAW_H, OD_Y, OD_WINDOW_3X3);
	MI_OD_SetAttr(OD_HANDLE_0, 3, 4, 15, 2, 120);

	while(fread(input_y, sizeof(char), image_size, fin))
	{
		frm_counter++;
		printf("frm=%d, blk result=(", frm_counter);
	/////////////////////////////////////////////////////////////////////////
		tamper_result = MI_OD_Run(OD_HANDLE_0, (const unsigned char*)(input_y));
		for (y = 0; y < DIV_H; y++)
		{
			for (x = 0; x < DIV_W; x++)
			{
				if((y == 0) && (x == 0))
					printf("%d",  MI_OD_GetWindowResult(OD_HANDLE_0, x, y));
				else
					printf(",%d", MI_OD_GetWindowResult(OD_HANDLE_0, x, y));
			}
		}			
	/////////////////////////////////////////////////////////////////////////
		printf("), tamper:%d\n", tamper_result);
	}	
	fclose(fin);		
	printf("Done\n");	
	
	MI_OD_Uninit(OD_HANDLE_0);
	return 0;

}