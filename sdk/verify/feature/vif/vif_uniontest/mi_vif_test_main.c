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
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <fcntl.h>
#include <stdlib.h>

#include "mi_vif_test_util.h"

typedef int (*start_test_func)(void);
typedef int (*stop_test_func)(void);

typedef struct vif_chn_op_s
{
    start_test_func start_test;
    stop_test_func stop_test;
} vif_chn_op_t;


static vif_chn_op_t all_vif_test[] =
{
    // vif
    {start_vif_1_1D1, stop_vif_1_1},
    {start_vif_1_4D1, stop_vif_1_4},
    {start_vif_4_1D1, stop_vif_4_1},
    {start_vif_4_4D1, stop_vif_4_4},
    {start_vif_1_1FHD, stop_vif_1_1},
    {start_vif_4_1FHD, stop_vif_4_1},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},

    // vif -> vpe
    {start_vpe_1_1D1, stop_vpe_1_1},
    {start_vpe_1_4D1, stop_vpe_1_4},
    {start_vpe_4_1D1, stop_vpe_4_1},
    {start_vpe_4_4D1, stop_vpe_4_4},
    {start_vpe_1_1FHD, stop_vpe_1_1},
    {start_vpe_4_1FHD, stop_vpe_4_1},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},

    // vif -> vpe -> venc
    {start_test_20, stop_test_20},
    {start_test_21, stop_test_21},
    {start_test_22, stop_test_22},
    {start_test_23, stop_test_23},
    {start_test_24, stop_test_24},
    {start_test_25, stop_test_25},
    {start_test_26, stop_test_26},
    {start_test_27, stop_test_27},
    {start_test_28, stop_test_28},
    {start_test_29, stop_test_29},


    // vif -> divp -> disp
    {start_disp_1_1D1, NULL},
    {start_disp_1_4D1, NULL},
    {start_disp_4_1D1, NULL},
    {start_disp_4_4D1, NULL},
    {start_disp_1_1FHD, NULL},
    {start_disp_4_1FHD, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},
    {NULL, NULL},

    //vif->divp->disp
    {start_divp_1_1D1_no_hvsp, stop_divp_1_1D1},//40
    {start_divp_1_1D1_hvsp, stop_divp_1_1D1},//41
    {NULL, NULL},
};

VIF_DBG_LEVEL_e vif_test_debug_level = VIF_TEST_DBG_INFO;
int vif_test_func_trace = 0;

int main(int argc, const char *argv[])
{
    char InputCmd[256] = { 0 };
    MI_U32 test_case_num;

    ExecFunc(MI_SYS_Init(), MI_SUCCESS);
    ExecFunc(vif_i2c_init(), 0);
    ExecFunc(vif_init_video_info(), 0);

    if(argc < 2)
    {
        printf("argc %d err\n", argc);
        return -1;
    }

    test_case_num = atoi(argv[1]);

    if(test_case_num >= 20 && test_case_num <= 29) //is VENC test cases
    {
        if(0 != set_extra_argv(argc, argv))
        {
            printf("venc %d err\n", argc);
            return -1;
        }
    }
    else if(argc != 2/*cases other than VENC*/)
    {
        printf("argc %d ERR\n", argc);
        return -1;
    }

    if(test_case_num > sizeof(all_vif_test) / sizeof(vif_chn_op_t))
    {
        DBG_ERR("set test case %d err\n", test_case_num);
        return -1;
    }

    DBG_DEBUG("set test case %d done\n", test_case_num);

    if(all_vif_test[test_case_num].start_test)
        all_vif_test[test_case_num].start_test();

    while(1)
    {
        printf("wait for command:\n");
        fgets((char *)(InputCmd), (sizeof(InputCmd) - 1), stdin);

        if(strncmp(InputCmd, "exit", 4) == 0)
        {
            printf("prepare to exit!\n\n");
            //goto EXIT_1;
            break;
        }
        else if(strncmp(InputCmd, "start", (strlen(InputCmd) - 1)) == 0)
        {
            all_vif_test[test_case_num].start_test();
        }
        else if(strncmp(InputCmd, "stop", (strlen(InputCmd) - 1)) == 0)
        {
            all_vif_test[test_case_num].stop_test();
            break;
        }
    }


    DBG_EXIT_OK();
    ExecFunc(MI_SYS_Exit(), MI_SUCCESS);

    DBG_ENTER();
    return 0;
}
