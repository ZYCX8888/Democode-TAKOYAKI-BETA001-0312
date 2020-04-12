/*
* sample.c- Sigmastar
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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mi_aed.h"

typedef enum {
    AED_SRATE_8K  = 8000,
    AED_SRATE_16K = 16000,
    AED_SRATE_32K = 32000
} AedSampleRate;

int main(int argc, char *argv[])
{
    short audio_input[2048];
    char input_file[512];
    char output_file[512];
    FILE *fin, *fout;
    int counter = 0;
    int i, ret, lsd_result, aed_result;

    int agc_gain = 0;
    AedParams aed_params;
    int point_length, point_number;

    /* AED parameter setting */
    aed_params.sample_rate = AED_SRATE_8K;
    aed_params.enable_nr = 1;
    aed_params.channel = 1;

    switch (aed_params.sample_rate) {
    case AED_SRATE_8K:
        point_number = 256;
        break;
    case AED_SRATE_16K:
        point_number = 512;
        break;
    case AED_SRATE_32K:
        point_number = 1024;
        break;
    default:
        fprintf(stderr, "Unsupported sample rate\n");
        exit(EXIT_FAILURE);
    }

    /* AED init */
    AED_HANDLE AED_HANDLE0;
    AED_HANDLE0 = MI_AED_Init(&aed_params, &point_length);
    MI_AED_SetSensitivity(AED_HANDLE0, AED_SEN_MID);
    MI_AED_SetOperatingPoint(AED_HANDLE0, 0);

    if (argc > 1)
        strcpy(input_file, argv[1]);
    else
        sprintf(input_file, "%s", "AFE_8K.wav");

    sprintf(output_file, "%s", "AFE_8K_out.wav");

    fin = fopen(input_file, "rb");
    if (!fin)
    {
        fprintf(stderr, "Error opening file: %s\n", input_file);
        exit(EXIT_FAILURE);
    }

    fout = fopen(output_file, "wb");
    if (!fout)
    {
        fprintf(stderr, "Error opening file: %s\n", output_file);
        exit(EXIT_FAILURE);
    }

    i = fread(audio_input, sizeof(char), 46, fin);  // read header 46 bytes
    fwrite(audio_input, sizeof(char), 46, fout);    // write 46 bytes output

    while(fread(audio_input, sizeof(short), point_number*aed_params.channel, fin))
    {
        counter++;

        /* Run LSD process */
        ret = MI_AED_RunLsd(AED_HANDLE0, audio_input, agc_gain);
        lsd_result = MI_AED_GetLsdResult(AED_HANDLE0);

        if (ret < 0)
        {
            fprintf(stderr, "Error occured in AED\n");
            break;
        }
        if (lsd_result)
        {
            printf("current time = %f, loud sound detected!@#$*^?-~@/#&#~$^*!?-=~*%%#\n", (float)counter * (point_length / aed_params.channel) / aed_params.sample_rate);
        }

        /* Run AED process */
        ret = MI_AED_Run(AED_HANDLE0, audio_input);
        aed_result = MI_AED_GetResult(AED_HANDLE0);

        if (ret < 0)
        {
            fprintf(stderr, "Error occured in AED\n");
            break;
        }
        if (aed_result)
        {
            printf("current time = %f, baby is crying! >___________________________< \n", (float)counter * (point_length / aed_params.channel) / aed_params.sample_rate);
        }
        fwrite(audio_input, sizeof(short), point_number*aed_params.channel, fout);
    }
    fclose(fin);
    fclose(fout);

    MI_AED_Uninit(AED_HANDLE0);
    if (!ret)
    {
        printf("AED done.\n");
    }
    else
    {
        printf("AED is not exit successfully.\n");
    }

  return 0;
}
