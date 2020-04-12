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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(char argc, char **argv)
{
	char file_name[30];
	unsigned int buffer;
	int read_size = 0;
	
	memset(file_name, 0, 30);
	int fd = open(argv[1], O_RDONLY);
	if (fd == -1)
	{
		return 0;
	}
	sprintf(file_name, "%s_new", argv[1]);
	int fd1 = open(file_name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	
	while(1)
	{
		read_size = read(fd, &buffer, 4);
		buffer |= 0xFF000000;
		if(read_size)
		{
			write(fd1, &buffer, 4);			
		}
		else
			break;
	}
	close(fd);
	close(fd1);

	return;
}
