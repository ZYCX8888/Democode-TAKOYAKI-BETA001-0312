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
#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
//#include <fcntl.h>
#include <string.h>
//#include <sys/ioctl.h>
//#include <sys/types.h>
//#include <sys/stat.h>

#include "mi_lsd.h"


typedef enum {
    LSD_SRATE_8K  = 8000,
	LSD_SRATE_16K = 16000,
	LSD_SRATE_32K = 32000
} LsdSampleRate;

int main(int argc, char *argv[])
{
    short audio_input[2048];
    char input_file[512];
    char output_file[512];
    FILE *fin, *fout;
    int counter = 0;
    int i, ret;
	unsigned int tx[2];
    int agc_gain = 0;
    LSD_PARAMS lsd_params;
    int point_length, point_number;
    short lsd_dBResult;
	short lsd_result;

    LSD_HANDLE LSD_HANDLE0;
    /* AED parameter setting */
    lsd_params.sample_rate = LSD_SRATE_8K;
    lsd_params.channel = 1;

    switch (lsd_params.sample_rate) {
    case LSD_SRATE_8K:
        point_number = 256;
        break;
    case LSD_SRATE_16K:
        point_number = 512;
        break;
    case LSD_SRATE_32K:
        point_number = 1024;
        break;
    default:
        fprintf(stderr, "Unsupported sample rate\n");
        exit(EXIT_FAILURE);
    }

    /* AED init */

    LSD_HANDLE0 = MI_LSD_Init(&lsd_params, &point_length);

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

    while(fread(audio_input, sizeof(short), point_number*lsd_params.channel, fin))
    {
        counter++;

        /* Run LSD process */

		MI_LSD_GetdBResult(LSD_HANDLE0, audio_input, &lsd_dBResult);
	    MI_LSD_Run(LSD_HANDLE0, &lsd_dBResult);
        ret = MI_LSD_GetResult(LSD_HANDLE0, &lsd_result);

        if (ret < 0)
        {
            fprintf(stderr, "Error occured in AED\n");
            break;
        }
        if (lsd_result)
        {
            printf("current time = %f, loud sound detected!@#$*^?-~@/#&#~$^*!?-=~*%%#\n", (float)counter * (point_length / lsd_params.channel) / lsd_params.sample_rate);
        }


        fwrite(audio_input, sizeof(short), point_number*lsd_params.channel, fout);
    }


	fclose(fin);
    fclose(fout);

    MI_LSD_Uninit(LSD_HANDLE0);
    if (!ret)
    {
        printf("LSD done.\n");
    }
    else
    {
        printf("LSD is not exit successfully.\n");
    }

  return 0;
}
