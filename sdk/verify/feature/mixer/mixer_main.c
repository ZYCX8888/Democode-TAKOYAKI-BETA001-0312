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
#include "mixer.h"

static mixer_test_func mixer_test[] =
{
    mixer_test_0,
};

static struct option long_options[] =
{
    /* These options set a flag. */
    {"device",     required_argument, 0, 'd'},
    {"framerate",  required_argument, 0, 'f'},
    {"path"  ,     required_argument, 0, 'p'},
    {"trace",      no_argument,       0, 't'},
    {"verbose",    required_argument, 0, 'v'},
    {0, 0, 0, 0}
};

int main(int argc, char **argv)
{
    int c;
    char InputCmd[256] = { 0 };

    ExecFunc(mixer_init_video_info(), 0);

    while(1)
    {

        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long(argc, argv, "d:f:p:tv:",
                        long_options, &option_index);

        /* Detect the end of the options. */
        if(c == -1)
            break;

        switch(c)
        {
            case 0:

                /* If this option set a flag, do nothing else now. */
                if(long_options[option_index].flag != 0)
                    break;

                printf("option %s", long_options[option_index].name);

                if(optarg)
                    printf(" with arg %s", optarg);

                printf("\n");
                break;

            case 'd':
                ExecFunc(mixer_device_set_mode_all(optarg), 0);
                break;

            case 'f':
                ExecFunc(mixer_path_set_all_eframerate(optarg), 0);
                break;

            case 'p':
                ExecFunc(mixer_path_set_all(optarg), 0);
                break;

            case 'v':
                mixer_util_set_debug_level(atoi(optarg));
                break;

            case 't':
                mixer_util_set_func_trace(TRUE);
                break;

            default:
                abort();
        }
    }

    /* Print any remaining command line arguments (not options). */
    if(optind < argc)
    {
        printf("non-option ARGV-elements: ");

        while(optind < argc)
            printf("%s ", argv[optind++]);

        putchar('\n');
    }

    ExecFunc(MI_SYS_Init(), MI_SUCCESS);
    ExecFunc(mixer_i2c_init(), 0);
    //exit(0);

    //ExecFunc(mixer_chn_dump_video_info(), 0);
    //exit(0);
    while(1)
    {
        memset(InputCmd,0,sizeof(InputCmd));
        printf("wait for command:\n");

        fgets((char *)(InputCmd), (sizeof(InputCmd) - 1), stdin);

        if(strlen(InputCmd) == 1)
            continue;

        if(strncmp(InputCmd, "s", (strlen(InputCmd) - 1)) == 0)
        {
            ExecFunc(mixer_path_destroy_all(), 0);
        }
        else if(strncmp(InputCmd, "b", (strlen(InputCmd) - 1)) == 0)
        {
            ExecFunc(mixer_device_create_all(), 0);
            ExecFunc(mixer_path_create_all(), 0);
        }
        else if(strncmp(InputCmd, "q", (strlen(InputCmd) - 1)) == 0)
        {
            break;
        }
        else if(strncmp(InputCmd, "t", (strlen(InputCmd) - 1)) == 0)
        {
            MI_U32 u32TestCase;
            printf("please input case num:\n");
            scanf("%d", &u32TestCase);
            mixer_test[u32TestCase]();
        }
    }


    ExecFunc(MI_SYS_Exit(), MI_SUCCESS);

    DBG_EXIT_OK();
    return 0;
}

