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
#include <string.h>

extern void do_cmd(const char *pCmd);
int main(char argc, char **argv)
{
	int i = 0;
	char cTransBuffer[TRANS_BUFFER];
	
	if (argc < 2)
	{
		printf("Arg is error\n");

		return -1;
	}

	memset(cTransBuffer, 0, TRANS_BUFFER);
	sprintf(cTransBuffer, "mdb");
	for (i = 1; i < argc; i++)
	{
		sprintf(cTransBuffer, "%s %s", cTransBuffer, argv[i]);
	}
	do_cmd(cTransBuffer);
	
	return 0;
}