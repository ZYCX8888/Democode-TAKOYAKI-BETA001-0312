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
#include "mi_vpe.h"
#include "mi_vpe_test.h"
typedef int (*vpe_test_func)(int argc, const char *argv[]);
typedef struct {
    const char *testCaseName;
    vpe_test_func func;
    const char *testDecription;
} test_vpe_FuncManager;

static test_vpe_FuncManager testVpeCase[] = {
    TEST_VPE_FUNC(001, test_vpe_TestCase001_main, VPE_TEST_001_DESC),
    TEST_VPE_FUNC(002, test_vpe_TestCase002_main, VPE_TEST_002_DESC),
    TEST_VPE_FUNC(003, test_vpe_TestCase003_main, VPE_TEST_003_DESC),
    TEST_VPE_FUNC(004, test_vpe_TestCase004_main, VPE_TEST_004_DESC),
    TEST_VPE_FUNC(005, test_vpe_TestCase005_main, VPE_TEST_005_DESC),
    TEST_VPE_FUNC(006, test_vpe_TestCase006_main, VPE_TEST_006_DESC),
    TEST_VPE_FUNC(007, test_vpe_TestCase007_main, VPE_TEST_007_DESC),
    TEST_VPE_FUNC(008, test_vpe_TestCase008_main, VPE_TEST_008_DESC),
};

static void showTestCase(const char *name)
{
    int i;
    char buff[256];
    for (i = 0; i < sizeof(testVpeCase)/sizeof(testVpeCase[0]); i++)
    {
        sprintf(buff, "ln -s /bin/%s /bin/%s 2>/dev/null", name, testVpeCase[i].testCaseName);
        system(buff);
        printf("%s -- %s\n", testVpeCase[i].testCaseName, testVpeCase[i].testDecription);
    }
}

int main(int argc, const char *argv[])
{
    int i;
    MI_BOOL bFind = FALSE;
    for (i = 0; i < sizeof(testVpeCase)/sizeof(testVpeCase[0]); i++)
    {
        if (strcmp(argv[0], testVpeCase[i].testCaseName) ==0 )
        {
            testVpeCase[i].func(argc, argv);
            printf("%s()pass exit\n", testVpeCase[i].testCaseName);
            bFind = TRUE;
            break;
        }
    }

    if (bFind == FALSE)
    {
        showTestCase(argv[0]);
    }

    return 0;
}
