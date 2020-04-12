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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

typedef int LONG;
typedef unsigned int DWORD;
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int Uint32;

typedef struct __attribute__ ((__packed__)){
    WORD    bfType;
    DWORD   bfSize;
    WORD    bfReserved1;
    WORD    bfReserved2;
    DWORD   bfOffBits;
}BMPFILEHEADER_T;

typedef struct __attribute__ ((__packed__)){
    DWORD      biSize;
    LONG       biWidth;
    LONG       biHeight;
    WORD       biPlanes;
    WORD       biBitCount;
    DWORD      biCompression;
    DWORD      biSizeImage;
    LONG       biXPelsPerMeter;
    LONG       biYPelsPerMeter;
    DWORD      biClrUsed;
    DWORD      biClrImportant;
}BMPINFOHEADER_T;
typedef struct __attribute__ ((__packed__))
{
    BYTE    rgbBlue;
    BYTE    rgbGreen;
    BYTE    rgbRed;
    BYTE    rgbReserved; // 0
}RGBQUAD_T;
static BMPFILEHEADER_T bmpFileInfo;
static BMPINFOHEADER_T bmpInfoheadr;

int main(int argc, char **argv)
{
	int fd = 0;
	int fd_out_argb1555 = 0, fd_out_argb4444 = 0, fd_out_rgb565= 0, fd_out_argb8888 = 0, fd_pos = 0,fd_pos_argb8888 = 0;
	unsigned char red = 0, green = 0, blue = 0;
	unsigned int read_cnt = 0, cnt = 0;
	unsigned int color_data = 0, width = 0, height = 0;
	unsigned short out_data = 0;
	unsigned int out_data32 = 0;
	char file_name[50];

	
	if (argc != 2)
		return 0;
	
	fd = open(argv[1], O_RDONLY);
	if (fd == -1)
	{
		return 0;
	}


	read(fd, &bmpFileInfo, sizeof(BMPFILEHEADER_T));
	read(fd, &bmpInfoheadr, sizeof(BMPINFOHEADER_T));
	printf("bmp width %d  height %d\n", bmpInfoheadr.biWidth, bmpInfoheadr.biHeight);
	sprintf(file_name, "%dX%d.argb1555", bmpInfoheadr.biWidth, bmpInfoheadr.biHeight);
    fd_out_argb1555 = open(file_name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd_out_argb1555 < 0)
    {
        perror("open");
        return -1;
    }
	sprintf(file_name, "%dX%d.argb4444", bmpInfoheadr.biWidth, bmpInfoheadr.biHeight);
    fd_out_argb4444 = open(file_name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd_out_argb4444 < 0)
    {
        perror("open");
        return -1;
    }
	sprintf(file_name, "%dX%d.rgb565", bmpInfoheadr.biWidth, bmpInfoheadr.biHeight);
    fd_out_rgb565 = open(file_name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd_out_rgb565 < 0)
    {
        perror("open");
        return -1;
    }
	sprintf(file_name, "%dX%d.argb8888", bmpInfoheadr.biWidth, bmpInfoheadr.biHeight);
    fd_out_argb8888 = open(file_name, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd_out_argb8888 < 0)
    {
        perror("open");
        return -1;
    }
	fd_pos = bmpInfoheadr.biWidth * (bmpInfoheadr.biHeight - 1) * 2;
	fd_pos_argb8888 = bmpInfoheadr.biWidth * (bmpInfoheadr.biHeight - 1) * 4;
	lseek(fd_out_argb1555, fd_pos, SEEK_SET);
	lseek(fd_out_argb4444, fd_pos, SEEK_SET);
	lseek(fd_out_rgb565, fd_pos, SEEK_SET);
	lseek(fd_out_argb8888, fd_pos_argb8888, SEEK_SET);
	while (1)
	{
		read(fd, &color_data, 3);
		cnt++;
		blue = color_data & 0xFF;
		red = (color_data & 0xFF0000) >> 16;
		green = (color_data & 0xFF00) >> 8;
		out_data = 0x8000 | ((red & 0xF8) << 7) | ((green & 0xF8) << 2) | ((blue & 0xF8) >> 3);
		write(fd_out_argb1555, &out_data, 2);
		out_data = 0xF000 | ((red & 0xF0) << 4) | ((green & 0xF0)) | ((blue & 0xF0) >> 4);
		write(fd_out_argb4444, &out_data, 2);
		out_data = ((red & 0xF8) << 8) | ((green & 0xFC) << 3) | ((blue & 0xF8) >> 3);
		write(fd_out_rgb565, &out_data, 2);
		out_data32 = 0xFF000000 | ((red & 0xFF) << 16) | ((green & 0xFF) << 8) | ((blue & 0xFF));
		write(fd_out_argb8888, &out_data32, 4);
		if (cnt == bmpInfoheadr.biHeight * bmpInfoheadr.biWidth)
		{
			break;
		}
		if (cnt / bmpInfoheadr.biWidth != (cnt - 1) / bmpInfoheadr.biWidth)
		{
			fd_pos = bmpInfoheadr.biWidth * (bmpInfoheadr.biHeight - (cnt / bmpInfoheadr.biWidth + 1)) * 2;
			lseek(fd_out_argb1555, fd_pos, SEEK_SET);
			lseek(fd_out_argb4444, fd_pos, SEEK_SET);
			lseek(fd_out_rgb565, fd_pos, SEEK_SET);
			fd_pos_argb8888 = bmpInfoheadr.biWidth * (bmpInfoheadr.biHeight - (cnt / bmpInfoheadr.biWidth + 1)) * 4;
			lseek(fd_out_argb8888, fd_pos_argb8888, SEEK_SET);
		}
	}
	close(fd_out_argb1555);
	close(fd_out_argb4444);
	close(fd_out_rgb565);
	close(fd_out_argb8888);
	close(fd);

	return 0;
}

