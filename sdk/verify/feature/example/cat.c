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
#include "cat1.h"
#include "cat2.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "busybox_wrapper.h"

shell_rcode cmd_cat1(const char *argv[], int count){
    printf("[%s]\n", __func__);
    return 0;
}

shell_rcode cmd_cat2(const char *argv[], int count){
    printf("[%s]\n", __func__);
    return 0;
}

shell_rcode cmd_cat3(const char *argv[], int count){
    printf("[%s]\n", __func__);
    return 0;
}

shell_rcode cmd_cat4(const char *argv[], int count){
    printf("[%s]\n", __func__);
    return 0;
}

static busybox_cmd_t cmds[] = {
    {"cat1", cmd_cat1},
    {"cat4", cmd_cat4},
    {"cat2", cmd_cat2},
    {"cat3", cmd_cat3},
};

int main(){
    struct rlimit limit;
    limit.rlim_cur = RLIM_INFINITY;
    limit.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &limit);
    //printf("cat1 test: %d\n", cat1(3, 5));
    //printf("cat2 test: %d\n", cat2());
    busybox_loop(cmds, sizeof(cmds)/sizeof(*cmds));
    return 0;
}
