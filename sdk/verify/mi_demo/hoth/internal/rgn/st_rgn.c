/* Copyright (c) 2018-2019 Sigmastar Technology Corp.
 All rights reserved.

  Unless otherwise stipulated in writing, any and all information contained
 herein regardless in any format shall remain the sole proprietary of
 Sigmastar Technology Corp. and be kept in strict confidence
 (ï¿½ï¿½Sigmastar Confidential Informationï¿½ï¿½) by the recipient.
 Any unauthorized act including without limitation unauthorized disclosure,
 copying, use, reproduction, sale, distribution, modification, disassembling,
 reverse engineering and compiling of the contents of Sigmastar Confidential
 Information is unlawful and strictly prohibited. Sigmastar hereby reserves the
 rights to any and all damages, losses, costs and expenses resulting therefrom.
*/

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>

#include "st_rgn.h"
#include "gb2312_code.h"

#define MAX_RGN_NUM     1024
#define MI_PI		    3.14159265358979323846

#define MAX_BUF_LEN                 1024
#define BITS_PER_PIXEL              16

#define MAX_LINES                   16

#define BMP_HEAD_LEN                54

#define ALIGN_UP(x, align)          (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(x, a)            (((x) / (a)) * (a))
#ifndef MAX
#define MAX(a,b)                    ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)                    ((a) < (b) ? (a) : (b))
#endif

#define ALIGN_MULTI(x, align)       (((x) % (align)) ? ((x) / (align) + 1) : ((x) / (align)))

#define DMF_FONT_PREFIX             "./"
#define DMF_FONT_PREFIX_ABSOLUTE    "/customer/mi_demo/"
#define DMF_FONT_ASCII_8x16         "ascii_8x16"
#define DMF_FONT_ASCII_16x32        "ascii_16x32"
#define DMF_FONT_ASCII_24x48        "ascii_24x48"
#define DMF_FONT_ASCII_32x64        "ascii_32x64"
#define DMF_FONT_HZ_16x16           "hanzi_16x16"
#define DMF_FONT_HZ_32x32           "hanzi_32x32"
#define DMF_FONT_HZ_48x48           "hanzi_48x48"
#define DMF_FONT_HZ_64x64           "hanzi_64x64"

typedef struct
{
    MI_RGN_HANDLE hHandle;
    MI_RGN_CanvasInfo_t stCanvasInfo;
    MI_RGN_PixelFormat_e ePixelFmt;
} ST_RGN_Info_T;

typedef enum
{
    UTF8_NO_BOM = 0,
    UTF8_WITH_BOM,
    GBK,

    Encoding_Butt,
} DMF_Encoding_Type_E;

typedef enum
{
    DMF_Font_Type_ASCII = 0,
    DMF_Font_Type_HZ,

    DMF_Font_Type_BUTT,
} DMF_Font_Type_E;

typedef struct
{
    char            szFile[64];
	char            szFileAbsolute[64];
    int             fd;
    uint8_t*        pBitMapAddr;
    struct stat     st;
    int             width;
    int             height;
} DMF_BitMapFile_S;

typedef struct
{
    int         charNumPerLine; // ??DD/¡§¡éD??¡§o?|¨¬??¨¢??¡è?¡§oy, ?D??¡§¡ã2¡§¡ã????¨¢??¡è?
    int         bgColor;        // ?¨¤3???¡§|?
    int         fgColor;        // ??????¡§|?
    int         leftMargin;     // margin ¡§o?D?
    int         rightMargin;
    int         upMargin;
    int         downMargin;
    int         verticalFlag;   // ¡§o¡§2?????
    int         charSpace;      // ?¨¢??¡è????¡§¡è
    int         lineSpace;      // DD???¡§¡è
} DMF_BitMapAttr_S;

static DMF_BitMapFile_S g_dmf_bitmapfile[DMF_Font_Type_BUTT][DMF_Font_Size_BUTT] =
{
    {
        {
            .szFile = DMF_FONT_PREFIX DMF_FONT_ASCII_8x16,
			.szFileAbsolute = DMF_FONT_PREFIX_ABSOLUTE DMF_FONT_ASCII_8x16,
            .fd = -1,
            .pBitMapAddr = NULL,
            .width = 8,
            .height = 16,
        },
        {
            .szFile = DMF_FONT_PREFIX DMF_FONT_ASCII_16x32,
			.szFileAbsolute = DMF_FONT_PREFIX_ABSOLUTE DMF_FONT_ASCII_16x32,
            .fd = -1,
            .pBitMapAddr = NULL,
            .width = 16,
            .height = 32,
        },
        {
            .szFile = DMF_FONT_PREFIX DMF_FONT_ASCII_24x48,
			.szFileAbsolute = DMF_FONT_PREFIX_ABSOLUTE DMF_FONT_ASCII_24x48,
            .fd = -1,
            .pBitMapAddr = NULL,
            .width = 24,
            .height = 48,
        },
        {
            .szFile = DMF_FONT_PREFIX DMF_FONT_ASCII_32x64,
			.szFileAbsolute = DMF_FONT_PREFIX_ABSOLUTE DMF_FONT_ASCII_32x64,
            .fd = -1,
            .pBitMapAddr = NULL,
            .width = 32,
            .height = 64,
        },
    },
    {
        {
            .szFile = DMF_FONT_PREFIX DMF_FONT_HZ_16x16,
			.szFileAbsolute = DMF_FONT_PREFIX_ABSOLUTE DMF_FONT_HZ_16x16,
            .fd = -1,
            .pBitMapAddr = NULL,
            .width = 16,
            .height = 16,
        },
        {
            .szFile = DMF_FONT_PREFIX DMF_FONT_HZ_32x32,
			.szFileAbsolute = DMF_FONT_PREFIX_ABSOLUTE DMF_FONT_HZ_32x32,
            .fd = -1,
            .pBitMapAddr = NULL,
            .width = 32,
            .height = 32,
        },
        {
            .szFile = DMF_FONT_PREFIX DMF_FONT_HZ_48x48,
			.szFileAbsolute = DMF_FONT_PREFIX_ABSOLUTE DMF_FONT_HZ_48x48,
            .fd = -1,
            .pBitMapAddr = NULL,
            .width = 48,
            .height = 48,
        },
        {
            .szFile = DMF_FONT_PREFIX DMF_FONT_HZ_64x64,
			.szFileAbsolute = DMF_FONT_PREFIX_ABSOLUTE DMF_FONT_HZ_64x64,
            .fd = -1,
            .pBitMapAddr = NULL,
            .width = 64,
            .height = 64,
        },
    },
};

static DMF_BitMapAttr_S g_dmf_bitmapattr =
{
    .charNumPerLine = 32,
    .bgColor = 0x2323,
    .fgColor = 0xFFFFFF,
    .leftMargin = 1,
    .rightMargin = 1,
    .upMargin = 1,
    .downMargin = 1,
    .verticalFlag = 0,
    //.verticalFlag = 1,
    .charSpace = 1,
    .lineSpace = 1,
};

static ST_RGN_Info_T g_stRgnInfo[MAX_RGN_NUM];
static MI_BOOL g_bInit = FALSE;
static pthread_mutex_t g_rgnOsd_mutex = PTHREAD_MUTEX_INITIALIZER;

#define ST_OSD_INIT_CHECK(handle) \
    do{  \
        if (hHandle < 0 || hHandle >= MAX_RGN_NUM) \
        { \
            return 1; \
        } \
        if (g_stRgnInfo[hHandle].hHandle == -1) \
        { \
            return 1; \
        } \
    }while(0);

MI_RGN_PaletteTable_t g_stPaletteTable =
{
    { //index0 ~ index15
         {255,   0,   0,   0}, {255, 255,   0,   0}, {255,   0, 255,   0}, {255,   0,   0, 255},
         {255, 255, 255,   0}, {255,   0, 112, 255}, {255,   0, 255, 255}, {255, 255, 255, 255},
         {255, 128,   0,   0}, {255, 128, 128,   0}, {255, 128,   0, 128}, {255,   0, 128,   0},
         {255,   0,   0,   0}, {255,   0, 128, 128}, {255, 128, 128, 128}, {255,  64,  64,  64},
         //index16 ~ index31
         {255,   0,   0,   0}, {  0,   0,   0,  30}, {  0,   0, 255,  60}, {  0, 128,   0,  90},
         {255,   0,   0, 120}, {  0, 255, 255, 150}, {255, 255,   0, 180}, {  0, 255,   0, 210},
         {255,   0, 255, 240}, {192, 192, 192, 255}, {128, 128, 128,  10}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index32 ~ index47
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index48 ~ index63
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index64 ~ index79
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index80 ~ index95
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index96 ~ index111
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index112 ~ index127
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index128 ~ index143
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index144 ~ index159
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index160 ~ index175
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index176 ~ index191
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index192 ~ index207
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index208 ~ index223
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index224 ~ index239
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         // (index236 :192,160,224 defalut colorkey)
         {192, 160, 224, 255}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         //index240 ~ index255
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0},
         {  0,   0,   0,   0}, {  0,   0,   0,   0}, {  0,   0,   0,   0}, {192, 160, 224, 255}
    }
};

MI_S32 ST_OSD_DrawPoint(MI_RGN_HANDLE hHandle, ST_Point_T stPoint, MI_U32 u32Color);

int _OSD_Readfully(int fd, void* buf, int nbytes)
{
	int nread;

	nread = 0;
	while ( nread < nbytes )
	{
		int r;

		r = read(fd, (char*) buf + nread, nbytes - nread);
		if (0 > r)
		{
			if (errno == EINTR || errno == EAGAIN)
			{
				sleep( 1 );
				continue;
			}
			else
			{
				return r;
			}
		}
		else if (0 == r)
		{
			break;
		}

		nread += r;
	}

	return nread;
}

DMF_BitMapFile_S* _OSD_BitmapFileInfo(DMF_Font_Type_E enType, DMF_Font_Size_E enSize)
{
    if (enType < DMF_Font_Type_ASCII || enType >= DMF_Font_Type_BUTT)
    {
        return NULL;
    }

    if (enSize < DMF_Font_Size_16x16 || enSize >= DMF_Font_Size_BUTT)
    {
        return NULL;
    }

    return &g_dmf_bitmapfile[enType][enSize];
}

void _OSD_BitmapFile_Init()
{
    int i = 0, j = 0;
    int fd = -1;
    uint8_t* pBitMapAddr = NULL;
    DMF_BitMapFile_S *pstDMFBitMapFile = NULL;

    for (i = 0; i < (int)DMF_Font_Type_BUTT; i ++)
    {
        for (j = 0; j < (int)DMF_Font_Size_BUTT; j ++)
        {
            pstDMFBitMapFile = &g_dmf_bitmapfile[i][j];

            fd = open(pstDMFBitMapFile->szFile, O_RDONLY);
            if (fd < 0)
            {
            	fd = open(pstDMFBitMapFile->szFileAbsolute, O_RDONLY);
				if (fd < 0)
				{
		            printf("%s %d, open %s fail\n", __func__, __LINE__, pstDMFBitMapFile->szFile);
		            continue;
				}
            }

            if (-1 == fstat(fd, &pstDMFBitMapFile->st))
            {
                printf("%s %d, fstat %s fail\n", __func__, __LINE__, pstDMFBitMapFile->szFile);
                close(fd);
                fd = -1;
                continue;
            }

            pBitMapAddr = (uint8_t *)mmap(NULL, (size_t)pstDMFBitMapFile->st.st_size,
                                            PROT_READ, MAP_PRIVATE, fd, (off_t)0);
            if (pBitMapAddr == MAP_FAILED)
            {
                close(fd);
                fd = -1;
                continue;
            }

            pstDMFBitMapFile->fd = fd;
            pstDMFBitMapFile->pBitMapAddr = pBitMapAddr;

            printf("%s %d, %s pBitMapAddr:%p, end:%p\n", __func__, __LINE__, pstDMFBitMapFile->szFile,
                        pstDMFBitMapFile->pBitMapAddr, pstDMFBitMapFile->pBitMapAddr + pstDMFBitMapFile->st.st_size);
        }
    }

    return;
}

void _OSD_BitmapFile_UnInit()
{
    int i = 0, j = 0;
    int fd = -1;
    uint8_t* pBitMapAddr = NULL;
    DMF_BitMapFile_S *pstDMFBitMapFile = NULL;

    for (i = 0; i < (int)DMF_Font_Type_BUTT; i ++)
    {
        for (j = 0; j < (int)DMF_Font_Size_BUTT; j ++)
        {
            pstDMFBitMapFile = &g_dmf_bitmapfile[i][j];

            if (pstDMFBitMapFile->pBitMapAddr != NULL)
            {
                munmap(pstDMFBitMapFile->pBitMapAddr, (size_t)pstDMFBitMapFile->st.st_size);
                pstDMFBitMapFile->pBitMapAddr = NULL;
            }

            if (pstDMFBitMapFile->fd > 0)
            {
                close(pstDMFBitMapFile->fd);
                pstDMFBitMapFile->fd = -1;
            }
        }
    }

    UNUSED(pBitMapAddr);
    UNUSED(fd);
}

extern int _dmf_GetUtf8Length(const uint8_t *src);
int _OSD_Utf8ToUnicode(const uint8_t *src, uint8_t *dst)
{
    int length;
	uint8_t unicode[2] = {0}; // D???D¡§¡ã

    length = _dmf_GetUtf8Length(src);
    if (length < 0)
    {
        return -1;
    }

	switch (length)
    {
	case 1:
		*dst = *src;
		*(dst + 1) = 0;
		return 1;
	case 2:
		unicode[0] = *(src + 1) & 0x3f;
		unicode[0] += (*src & 0x3) << 6;
		unicode[1] = (*src & 0x7 << 2) >> 2;
		break;
	case 3:
		unicode[0] = *(src + 2) & 0x3f;
		unicode[0] += (*(src + 1) & 0x3) << 6;
		unicode[1] = (*(src + 1) & 0xF << 2) >> 2;
		unicode[1] += (*src & 0xf) << 4;
		break;
	case 4:
		/* not support now */
		return -1;
	}

	*dst = unicode[0];
	*(dst + 1) = unicode[1];

	return length;
}

int _OSD_UnicodeToGb2312(uint16_t unicode, const uint16_t *mem_gb2312, int gb2312_num)
{
    int i = 0;

    for (i = 0; i < gb2312_num; i++)
    {
        if (mem_gb2312[2 * i] == unicode)
        {
            return mem_gb2312[2 * i + 1];
        }
    }

	return -1;
}

extern int _dmf_IsUtf8(unsigned char *string, int len);
DMF_Encoding_Type_E _OSD_DetectEncodingType(const char *string)
{
    DMF_Encoding_Type_E enEncodIngType = UTF8_NO_BOM;
    int len = 0;
    int ret = 0;

    if (string == NULL)
    {
        return Encoding_Butt;
    }

    len = strlen(string);

    // printf("%s %d, len:%d\n", __func__, __LINE__, len);

    if (len >= 3 && string[0] == 0xef && string[1] == 0xbb && string[2] == 0xbf)
    {
        enEncodIngType = UTF8_WITH_BOM;
    }
    else
    {
        // printf("%s %d\n", __func__, __LINE__);
        ret = _dmf_IsUtf8((unsigned char *)string, len);
		if (ret > 0)
        {
			enEncodIngType = GBK;
		}
        else if (ret == -1)
        {
			enEncodIngType = UTF8_NO_BOM;
		}
    }

    return enEncodIngType;
}

int _OSD_CalcCharNumGetGb2312Code(const char *string, int *charTotalNum, uint8_t *gb2312buf, int bufLen)
{
    DMF_Encoding_Type_E enEncodIngType = UTF8_NO_BOM;
    int charNum = 0, strLen = 0, dealLen = 0;
    int ret = 0;
    uint8_t unicode[2] = {0};
    const uint8_t *ptr = NULL;
    uint16_t gb2312_code;
    int gb2312codeLen = sizeof(gb2312code) / sizeof(gb2312code[0]) / 3;
    uint8_t *ptr_gb2312;

    if (string == NULL)
    {
        return 0;
    }

    strLen = strlen(string);
    ptr = (uint8_t *)string;
    ptr_gb2312 = gb2312buf;

    enEncodIngType = _OSD_DetectEncodingType(string);

    // ST_DBG("enEncodIngType:%d\n", enEncodIngType);

    if (enEncodIngType == UTF8_NO_BOM ||
        enEncodIngType == UTF8_WITH_BOM)
    {
        do
        {
            ret = _OSD_Utf8ToUnicode(ptr, unicode);
            // printf("%s %d, 0x%X, 0x%X\n", __func__, __LINE__, unicode[0], unicode[1]);
            if (ret < 0)
            {
                return -1;
            }

            ptr += ret;
            dealLen += ret;
            charNum ++;

            gb2312_code = _OSD_UnicodeToGb2312(unicode[0] +
							        unicode[1] * 0x100, gb2312code, gb2312codeLen);

            // printf("%s %d, 0x%X, %p\n", __func__, __LINE__, gb2312_code, ptr_gb2312);
            ptr_gb2312[0] = gb2312_code % 0x100;
			if (gb2312_code / 0x100 > 0)
            {
				ptr_gb2312[1] = gb2312_code / 0x100;
				ptr_gb2312 += 2;
			}
            else
            {
                ptr_gb2312 += 1;
            }
        } while (dealLen < strLen);
    }
    else if (enEncodIngType == GBK)
    {
        charNum += 2;
        dealLen += 2;
        ptr += 2;
        do
        {
            enEncodIngType = _OSD_DetectEncodingType((const char *)ptr);
            if (enEncodIngType == GBK)
            {
                charNum += 2;
                dealLen += 2;
                ptr += 2;
            }
            else
            {
                charNum ++;
                dealLen ++;
                ptr ++;
            }
        } while (dealLen < strLen);

        charNum = snprintf((char *)gb2312buf, bufLen - 1, "%s", string);
    }

    *charTotalNum = charNum;

    return 0;
}

void _OSD_CalcBMPWH(int charTotalNum, int *bmpWidth, int *bmpHeight, uint8_t *pGb2312)
{
    DMF_BitMapFile_S *pstDMFBitMapFile = &g_dmf_bitmapfile[DMF_Font_Type_ASCII][DMF_Font_Size_16x16];
    DMF_BitMapAttr_S *pstDMFBitMapAttr = &g_dmf_bitmapattr;

    int width = 0, height = 0;
    int lines = 0, i = 0, offset = 0;
    int charNumPerLine = 0;

    charNumPerLine = MIN(charTotalNum, pstDMFBitMapAttr->charNumPerLine);

    lines = ALIGN_MULTI(charTotalNum, pstDMFBitMapAttr->charNumPerLine);

    offset = 0;
    if (pstDMFBitMapAttr->verticalFlag == 0)
    {
        for (i = 0; i < charNumPerLine; i ++)
        {
            if (pGb2312[offset] > 0xA0 &&
                pGb2312[offset]  < 0xff)
            {
                width += pstDMFBitMapFile->width;

                offset += 2;
            }
            else if (pGb2312[offset] > 0x1f &&
                    pGb2312[offset] < 0x80)
            {
                width += pstDMFBitMapFile->width;

                offset ++;
            }
        }

        for (i = 0; i < lines; i ++)
        {
            height += pstDMFBitMapFile->height;
        }
    }
    else if (pstDMFBitMapAttr->verticalFlag == 1)
    {
        for (i = 0; i < charNumPerLine; i ++)
        {
            height += pstDMFBitMapFile->height;
        }

        for (i = 0; i < lines; i ++)
        {
            width += pstDMFBitMapFile->width;
        }
    }

    if (pstDMFBitMapAttr->verticalFlag == 0)
    {
        // o¡§¡é??
        width += pstDMFBitMapAttr->leftMargin +
                pstDMFBitMapAttr->charSpace * (charTotalNum - 1) +
                pstDMFBitMapAttr->rightMargin;

        height += pstDMFBitMapAttr->upMargin +
                pstDMFBitMapAttr->lineSpace * (lines - 1) +
                pstDMFBitMapAttr->downMargin;
    }
    else if (pstDMFBitMapAttr->verticalFlag == 1)
    {
        // ¡§o¡§2??
        width += pstDMFBitMapAttr->leftMargin +
                pstDMFBitMapAttr->lineSpace * (lines - 1) +
                pstDMFBitMapAttr->rightMargin;

        height += pstDMFBitMapAttr->upMargin +
                pstDMFBitMapAttr->charSpace * (charTotalNum - 1) +
                pstDMFBitMapAttr->downMargin;
    }

    *bmpWidth = width;
    *bmpHeight = height;
}

void _OSD_FontDataToCanvas(const uint8_t *pFontdata, int x, int y, int width,
                        int height, MI_RGN_HANDLE hHandle, MI_U32 u32Color)
{
    int i = 0, j = 0;
    int char_num;
	int char_bit;
	char bit;
    uint8_t *pFontdataTemp = NULL;
    ST_Point_T stPoint;

    if ((pFontdata == NULL))
    {
        return;
    }

#if 0
    for (i = 0; i < height; i ++) // |¨¬1¡§¡é?¨¦|¨¬???¡§a?
#else
    for (i = height - 1; i >= 0; i--) // ?y¡§¡é?¨¦|¨¬???¡§a?
    {
        pFontdataTemp = (uint8_t *)pFontdata + (width + 7) / 8 * i;

        for (j = 0; j < width; j ++)
        {
            char_num = j / 8;
		    char_bit = 7 - j % 8;
            bit = pFontdataTemp[char_num] & (1 << char_bit);

            stPoint.u32X = x + j;
            stPoint.u32Y = y + i;

            if (bit)
            {
                ST_OSD_DrawPoint(hHandle, stPoint, u32Color);
            }
            else
            {
            }
        }
    }
#endif
}

extern uint32_t _dmf_Gb2312codeToFontoffset(uint32_t gb2312code, uint32_t font_height);
extern uint32_t _dmf_AsciiToFontoffset(uint32_t ascii, uint32_t width, uint32_t height);
void _OSD_DrawTextToCanvas(MI_RGN_HANDLE hHandle, ST_Point_T stPoint, const char *szString, MI_U32 u32Color, DMF_Font_Size_E enSize)
{
    DMF_BitMapFile_S *pstDMFBitMapFile = NULL;
    DMF_BitMapAttr_S *pstDMFBitMapAttr = &g_dmf_bitmapattr;

    int charTotalNum = 0;
    uint8_t gb2312buf[MAX_BUF_LEN * 2];
    int totalLines = 0;
    int charNumPerLine[MAX_LINES];
    int charRemainNum = 0;
    int i = 0;
    int j = 0;
    int bmpWidth = 0;
    int bmpHeight = 0;
    int gb2312Offset = 0;
    int gb2312LineOffset = 0;
    uint8_t *pGb2312Line = NULL;
    int fontoffset = 0;
    int fontWidth = 0;
    int fontHeight = 0;
    uint8_t *fontAddr = NULL;
    int fontTotalWidth = 0;
    int fontTotalHeight = 0;
    int xpos = 0;
    int ypos = 0;

    if (szString == NULL)
    {
        return;
    }

    memset(gb2312buf, 0, sizeof(gb2312buf));
    _OSD_CalcCharNumGetGb2312Code(szString, &charTotalNum, gb2312buf, MAX_BUF_LEN * 2);

    totalLines = ALIGN_MULTI(charTotalNum, pstDMFBitMapAttr->charNumPerLine);
    totalLines = MIN(totalLines, MAX_LINES);

    memset(&charNumPerLine, 0, sizeof(charNumPerLine));
    charRemainNum = charTotalNum;
    for (i = 0; i < totalLines; i ++)
    {
        charNumPerLine[i] = MIN(charRemainNum, pstDMFBitMapAttr->charNumPerLine);

        charRemainNum -= charNumPerLine[i];
    }

    _OSD_CalcBMPWH(charTotalNum, &bmpWidth, &bmpHeight, gb2312buf);

#if 0
    printf("string:\t%s\n", szString);
    printf("charTotalNum:\t%d\n", charTotalNum);
    printf("charNumPerLine:\t");
    for (i = 0; i < totalLines; i ++)
    {
        printf("%d ", charNumPerLine[i]);
    }
    printf("\n");
    printf("totalLines:\t%d\n", totalLines);
    printf("bmpWidth:\t%d\n", bmpWidth);
    printf("bmpHeight:\t%d\n", bmpHeight);;
#endif

    gb2312Offset = 0;
    for (i = 0; i < totalLines; i ++)
    {
        pGb2312Line = gb2312buf + gb2312Offset;
        gb2312LineOffset = 0;

        // printf("%s %d, %p\n", __func__, __LINE__, pGb2312Line);

        for (j = 0; j < charNumPerLine[i]; j ++)
        {
            if (pGb2312Line[gb2312LineOffset] > 0xA0 &&
                pGb2312Line[gb2312LineOffset]  < 0xff)
            {
                pstDMFBitMapFile = _OSD_BitmapFileInfo(DMF_Font_Type_HZ, enSize);
                if (pstDMFBitMapFile == NULL)
                {
                    continue;
                }

                fontoffset = _dmf_Gb2312codeToFontoffset(pGb2312Line[gb2312LineOffset] +
						        0x100 * pGb2312Line[gb2312LineOffset + 1],
						        pstDMFBitMapFile->height);

                fontWidth = pstDMFBitMapFile->width;
                fontHeight = pstDMFBitMapFile->height;

                gb2312LineOffset += 2;

                fontAddr = pstDMFBitMapFile->pBitMapAddr;
            }
            else if (pGb2312Line[gb2312LineOffset] > 0x1f &&
                        pGb2312Line[gb2312LineOffset] < 0x80)
            {
                pstDMFBitMapFile = _OSD_BitmapFileInfo(DMF_Font_Type_ASCII, enSize);
                if (pstDMFBitMapFile == NULL)
                {
                    continue;
                }

                fontWidth = pstDMFBitMapFile->width;
                fontHeight = pstDMFBitMapFile->height;

                fontoffset = _dmf_AsciiToFontoffset(pGb2312Line[gb2312LineOffset], fontWidth, fontHeight);

                gb2312LineOffset ++;

                fontAddr = pstDMFBitMapFile->pBitMapAddr;
            }
            else
            {
                continue;
            }

            if (pstDMFBitMapAttr->verticalFlag == 0)
            {
                // o¡§¡é??
                xpos = pstDMFBitMapAttr->leftMargin + fontTotalWidth +
                        j * pstDMFBitMapAttr->lineSpace;
                ypos = pstDMFBitMapAttr->upMargin + i * pstDMFBitMapFile->height +
                        i * pstDMFBitMapAttr->lineSpace;

                fontTotalWidth += fontWidth;
            }
            else if (pstDMFBitMapAttr->verticalFlag == 1)
            {
                // ¡§o¡§2??
                xpos = pstDMFBitMapAttr->leftMargin + i * pstDMFBitMapFile->width +
                        i * pstDMFBitMapAttr->lineSpace;
                ypos = pstDMFBitMapAttr->upMargin + fontTotalHeight +
                        j * pstDMFBitMapAttr->lineSpace;

                fontTotalHeight += fontHeight;
            }

            // printf("xpos:%d, ypos:%d, gb2312LineOffset:%d, fontoffset:%d, fontWidth:%d,fontHeight:%d, 0x%x\n",
            //            xpos, ypos, gb2312LineOffset, fontoffset, fontWidth, fontHeight, (fontAddr + fontoffset)[0]);
            if (fontAddr != NULL)
                _OSD_FontDataToCanvas(fontAddr + fontoffset,
                                xpos + stPoint.u32X, ypos + stPoint.u32Y, fontWidth, fontHeight, hHandle, u32Color);
        }

        fontTotalWidth = 0;
        fontTotalHeight = 0;

        gb2312Offset += gb2312LineOffset;
    }
}

MI_S32 ST_OSD_Init(void)
{
    MI_U32 i = 0;

    pthread_mutex_lock(&g_rgnOsd_mutex);
    if (g_bInit == FALSE)
    {
        g_bInit = TRUE;
        for (i = 0; i < MAX_RGN_NUM; i ++)
        {
            g_stRgnInfo[i].hHandle = -1;
        }

        _OSD_BitmapFile_Init();

    	ExecFunc(MI_RGN_Init(&g_stPaletteTable), MI_RGN_OK);
        // MI_RGN_Init(&stPaletteTable);
    }
    pthread_mutex_unlock(&g_rgnOsd_mutex);

    return MI_RGN_OK;
}
MI_S32 ST_OSD_Deinit(void)
{
    MI_U32 i = 0;

    pthread_mutex_lock(&g_rgnOsd_mutex);
    if (g_bInit == TRUE)
    {
        g_bInit = FALSE;
        for (i = 0; i < MAX_RGN_NUM; i ++)
        {
            g_stRgnInfo[i].hHandle = -1;
        }

        _OSD_BitmapFile_UnInit();

    	ExecFunc(MI_RGN_DeInit(), MI_RGN_OK);
        // MI_RGN_Init(&stPaletteTable);
    }
    pthread_mutex_unlock(&g_rgnOsd_mutex);

    return MI_RGN_OK;
}
MI_S32 ST_OSD_Create(MI_RGN_HANDLE hHandle, MI_RGN_Attr_t *pstRegion)
{
    ExecFunc(MI_RGN_Create(hHandle, pstRegion), MI_RGN_OK);
    g_stRgnInfo[hHandle].hHandle = hHandle;
    g_stRgnInfo[hHandle].ePixelFmt = pstRegion->stOsdInitParam.ePixelFmt;
    memset(&g_stRgnInfo[hHandle].stCanvasInfo, 0, sizeof(MI_RGN_CanvasInfo_t));

    return MI_RGN_OK;
}
MI_S32 ST_OSD_Destroy(MI_RGN_HANDLE hHandle)
{
    ExecFunc(MI_RGN_Destroy(hHandle), MI_RGN_OK);
    memset(&g_stRgnInfo[hHandle], 0, sizeof(ST_RGN_Info_T));
    g_stRgnInfo[hHandle].hHandle = MI_RGN_HANDLE_NULL;

    return MI_RGN_OK;
}
MI_S32 ST_OSD_DrawPoint(MI_RGN_HANDLE hHandle, ST_Point_T stPoint, MI_U32 u32Color)
{
    MI_U16 *pDst = NULL;
    MI_U32 u32Stride = 0;
    MI_U8 u8Value = 0;

    ST_OSD_INIT_CHECK(hHandle);

    // ST_DBG("point(%d,%d), size(%dx%d)\n", stPoint.u32X, stPoint.u32Y,
    //    g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width, g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height);

    if (stPoint.u32X < 0 || stPoint.u32X >= g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width ||
        stPoint.u32Y < 0 || stPoint.u32Y >= g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height)
    {
        return 1;
    }

    pDst = (MI_U16 *)g_stRgnInfo[hHandle].stCanvasInfo.virtAddr;
    u32Stride = g_stRgnInfo[hHandle].stCanvasInfo.u32Stride;

    // ST_DBG("pDst:%p, u32Stride:%d, point(%d,%d)\n", pDst, u32Stride, stPoint.u32X, stPoint.u32Y);

    if (g_stRgnInfo[hHandle].ePixelFmt == E_MI_RGN_PIXEL_FORMAT_ARGB1555)
    {
        *(pDst + (u32Stride / 2 * stPoint.u32Y) + stPoint.u32X) = u32Color & 0xffff;
    }
    else if (g_stRgnInfo[hHandle].ePixelFmt == E_MI_RGN_PIXEL_FORMAT_I4)
    {
        // *(pDst + (u32Stride / 2 * stPoint.u32Y) + stPoint.u32X) = u32Color & 0xffff;
        //*(pDst + (u32Stride / 2 * stPoint.u32Y) + stPoint.u32X) = u32Color & 0x0f;
        if (stPoint.u32X % 2)
        {
            u8Value = (*((MI_U8 *)pDst + (u32Stride * stPoint.u32Y) + stPoint.u32X / 2) & 0x0F) | ((u32Color & 0x0f) << 4);
            // *((MI_U8 *)pDst + (u32Stride * stPoint.u32Y) + stPoint.u32X / 2) |= u32Color & 0x0f;
            // *((MI_U8 *)pDst + (u32Stride * stPoint.u32Y) + stPoint.u32X / 2) |= (u32Color & 0x0f) << 4;
            *((MI_U8 *)pDst + (u32Stride * stPoint.u32Y) + stPoint.u32X / 2) = u8Value;
        }
        else
        {
            // u8Value = ;
            // *((MI_U8 *)pDst + (u32Stride * stPoint.u32Y) + stPoint.u32X / 2) |= (u32Color & 0x0f) << 4;
            // *((MI_U8 *)pDst + (u32Stride * stPoint.u32Y) + stPoint.u32X / 2) |= u32Color & 0x0f;
            u8Value = (*((MI_U8 *)pDst + (u32Stride * stPoint.u32Y) + stPoint.u32X / 2) & 0xF0) | (u32Color & 0x0f);
            *((MI_U8 *)pDst + (u32Stride * stPoint.u32Y) + stPoint.u32X / 2) = u8Value;
        }
    }

    return MI_RGN_OK;
}

MI_S32 ST_OSD_DrawLine(MI_RGN_HANDLE hHandle, ST_Point_T stPoint0, ST_Point_T stPoint1, MI_U8 u8BorderWidth, MI_U32 u32Color)
{
    int x, y;
    int i = 0, j = 0;
	float k, e, dx, dy;
    ST_Point_T stPoint;

    // ST_DBG("point0(%d,%d), point1(%d,%d)\n", stPoint0.u32X, stPoint0.u32Y, stPoint1.u32X, stPoint1.u32Y);

    ST_OSD_INIT_CHECK(hHandle);

    stPoint0.u32X = (stPoint0.u32X < 0) ? 0 : stPoint0.u32X;
    stPoint0.u32X = (stPoint0.u32X > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width) ?
                        g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width : stPoint0.u32X;
    stPoint0.u32Y = (stPoint0.u32Y < 0) ? 0 : stPoint0.u32Y;
    stPoint0.u32Y = (stPoint0.u32Y > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height) ?
                        g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height: stPoint0.u32Y;

    stPoint1.u32X = (stPoint1.u32X < 0) ? 0 : stPoint1.u32X;
    stPoint1.u32X = (stPoint1.u32X > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width) ?
                        g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width : stPoint1.u32X;
    stPoint1.u32Y = (stPoint1.u32Y < 0) ? 0 : stPoint1.u32Y;
    stPoint1.u32Y = (stPoint1.u32Y > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height) ?
                        g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height: stPoint1.u32Y;

    if (stPoint1.u32X > stPoint0.u32X)
    {
        dx = stPoint1.u32X - stPoint0.u32X;
        x = stPoint0.u32X;
    }
    else
    {
        dx = stPoint0.u32X - stPoint1.u32X;
        x = stPoint1.u32X;
    }

	dy = (float)(stPoint1.u32Y - stPoint0.u32Y);

    if (stPoint1.u32Y > stPoint0.u32Y)
    {
        dy = stPoint1.u32Y - stPoint0.u32Y;
        y = stPoint0.u32Y;
    }
    else
    {
        dy = stPoint0.u32Y - stPoint1.u32Y;
        y = stPoint1.u32Y;
    }

    // draw v line
	if (dx == 0)
	{
	    for (j = 0; j < u8BorderWidth; j ++)
        {
            for (i = 0; i < dy; i ++)
    		{
    		    stPoint.u32X = x + j;
                stPoint.u32Y = y + i;
    			ST_OSD_DrawPoint(hHandle, stPoint, u32Color);
    		}
        }

		return MI_RGN_OK;
	}

    // draw h line
    if (dy == 0)
    {
        for (j = 0; j < u8BorderWidth; j ++)
        {
            for (i = 0; i < dx; i ++)
            {
                stPoint.u32X = x + i;
                stPoint.u32Y = y + j;
    			ST_OSD_DrawPoint(hHandle, stPoint, u32Color);
            }
        }

        return MI_RGN_OK;
    }

    // draw slant line
    e = -0.5;
	k = (float)(dy /dx);

    // for (j = 0; j < u8BorderWidth; j ++)
    {
        for (i = 0; i < dx; i ++)
    	{
    	    stPoint.u32X = x + j;
            stPoint.u32Y = y + j;

    		ST_OSD_DrawPoint(hHandle, stPoint, u32Color);
    		x ++;
    		e += k;
    		if (e >= 0)
    		{
    			y ++;
    			e --;
    		}
    	}
    }

    return MI_RGN_OK;
}

MI_S32 ST_OSD_Update(MI_RGN_HANDLE hHandle)
{
    ST_OSD_INIT_CHECK(hHandle);

    // ExecFunc(MI_RGN_UpdateCanvas(hHandle), MI_RGN_OK);
    memset(&g_stRgnInfo[hHandle].stCanvasInfo, 0, sizeof(MI_RGN_CanvasInfo_t));
    MI_RGN_UpdateCanvas(hHandle);

    return MI_RGN_OK;
}

MI_S32 ST_OSD_GetCanvasInfo(MI_RGN_HANDLE hHandle, MI_RGN_CanvasInfo_t** ppstCanvasInfo)
{
    MI_S32 sRet = MI_RGN_OK;

    ST_OSD_INIT_CHECK(hHandle);
    sRet = MI_RGN_GetCanvasInfo(hHandle, &g_stRgnInfo[hHandle].stCanvasInfo);
    if (sRet == MI_RGN_OK)
    {
        *ppstCanvasInfo = &g_stRgnInfo[hHandle].stCanvasInfo;
    }

    return sRet;
}
static void ST_OSD_PrepareLine(MI_U8 u8BorderWidth, MI_U16 u16PixW, MI_U8 u8Color, MI_U8 *pu8Data)
{
    MI_U32 i = 0;

    for (i = 0; i < u8BorderWidth/2; i++)
    {
        pu8Data[i] = u8Color;
    }
    i += (u16PixW >> 1) - i * 2;
    memcpy(&pu8Data[i], pu8Data, u8BorderWidth / 2);
}

MI_S32 ST_OSD_ClearRectFast(MI_RGN_HANDLE hHandle, ST_Rect_T stRect)
{
    MI_U8 *pu8StartAddr = NULL;
    MI_U32 i = 0;

    ST_OSD_INIT_CHECK(hHandle);
    if (stRect.u32X > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width
        || stRect.u32Y > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height)
        return MI_ERR_RGN_ILLEGAL_PARAM;

    stRect.u32X = (stRect.u32X < 0) ? 0 : stRect.u32X;
    stRect.u32Y = (stRect.u32Y < 0) ? 0 : stRect.u32Y;
    stRect.u16PicW = ((stRect.u32X + stRect.u16PicW) > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width) ?
                        (g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width - stRect.u32X) : stRect.u16PicW;
    stRect.u16PicH = ((stRect.u32Y + stRect.u16PicH) > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height) ?
                        (g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height - stRect.u32Y) : stRect.u16PicH;
    pu8StartAddr = (MI_U8 *)(g_stRgnInfo[hHandle].stCanvasInfo.virtAddr + stRect.u32Y * g_stRgnInfo[hHandle].stCanvasInfo.u32Stride + (stRect.u32X >> 1));
    for (i = 0; i < stRect.u16PicH; i++)
    {
        memset(pu8StartAddr, 0, (stRect.u16PicW >> 1));
        pu8StartAddr += g_stRgnInfo[hHandle].stCanvasInfo.u32Stride;
    }

    return MI_RGN_OK;
}
MI_S32 ST_OSD_DrawRectFast(MI_RGN_HANDLE hHandle, ST_Rect_T stRect, MI_U8 u8BorderWidth, MI_U32 u32Color)
{
    MI_U32 i = 0;
    MI_U8 u8Color = 0;
    MI_U8 *pu8StartAddr = NULL;

    ST_OSD_INIT_CHECK(hHandle);
    if (stRect.u32X > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width
        || stRect.u32Y > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height)
        return MI_ERR_RGN_ILLEGAL_PARAM;

    stRect.u32X = (stRect.u32X < 0) ? 0 : stRect.u32X;
    stRect.u32Y = (stRect.u32Y < 0) ? 0 : stRect.u32Y;
    stRect.u16PicW = ((stRect.u32X + stRect.u16PicW) > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width) ?
                        (g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width - stRect.u32X) : stRect.u16PicW;
    stRect.u16PicH = ((stRect.u32Y + stRect.u16PicH) > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height) ?
                        (g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height - stRect.u32Y) : stRect.u16PicH;
    u8Color = (u32Color & 0xF) | ((u32Color & 0xF) << 4);
    pu8StartAddr = (MI_U8 *)(g_stRgnInfo[hHandle].stCanvasInfo.virtAddr + stRect.u32Y * g_stRgnInfo[hHandle].stCanvasInfo.u32Stride + (stRect.u32X >> 1));
    for (i = 0; i < u8BorderWidth; i++)
    {
        memset(pu8StartAddr, u8Color, (stRect.u16PicW >> 1));
        pu8StartAddr += g_stRgnInfo[hHandle].stCanvasInfo.u32Stride;
    }
    for (i = 0; i < stRect.u16PicH - u8BorderWidth * 2; i++)
    {
        ST_OSD_PrepareLine(u8BorderWidth, stRect.u16PicW, u8Color, pu8StartAddr);
        pu8StartAddr += g_stRgnInfo[hHandle].stCanvasInfo.u32Stride;
    }
    for (i = 0; i < u8BorderWidth; i++)
    {
        memset(pu8StartAddr, u8Color, (stRect.u16PicW >> 1));
        pu8StartAddr += g_stRgnInfo[hHandle].stCanvasInfo.u32Stride;
    }

    return MI_RGN_OK;
}

MI_S32 ST_OSD_DrawRect(MI_RGN_HANDLE hHandle, ST_Rect_T stRect, MI_U8 u8BorderWidth, MI_U32 u32Color)
{
    ST_Point_T stPoint0, stPoint1;

    ST_OSD_INIT_CHECK(hHandle);
    if (stRect.u32X > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width
        || stRect.u32Y > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height)
        return MI_ERR_RGN_ILLEGAL_PARAM;

    stRect.u32X = (stRect.u32X < 0) ? 0 : stRect.u32X;
    stRect.u32Y = (stRect.u32Y < 0) ? 0 : stRect.u32Y;
    stRect.u16PicW = ((stRect.u32X + stRect.u16PicW) > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width) ?
                        (g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width - stRect.u32X) : stRect.u16PicW;
    stRect.u16PicH = ((stRect.u32Y + stRect.u16PicH) > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height) ?
                        (g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height - stRect.u32Y) : stRect.u16PicH;

    stPoint0.u32X = stRect.u32X;
    stPoint0.u32Y = stRect.u32Y;

    stPoint1.u32X = stRect.u32X + stRect.u16PicW;
    stPoint1.u32Y = stRect.u32Y;
    ST_OSD_DrawLine(hHandle, stPoint0, stPoint1, u8BorderWidth, u32Color);

    stPoint0.u32X = stRect.u32X + stRect.u16PicW;
    stPoint0.u32Y = stRect.u32Y;

    stPoint1.u32X = stRect.u32X + stRect.u16PicW;
    stPoint1.u32Y = stRect.u32Y + stRect.u16PicH;
    ST_OSD_DrawLine(hHandle, stPoint0, stPoint1, u8BorderWidth, u32Color);

    stPoint0.u32X = stRect.u32X + stRect.u16PicW;
    stPoint0.u32Y = stRect.u32Y + stRect.u16PicH;

    stPoint1.u32X = stRect.u32X;
    stPoint1.u32Y = stRect.u32Y + stRect.u16PicH;
    ST_OSD_DrawLine(hHandle, stPoint1, stPoint0, u8BorderWidth, u32Color);

    stPoint0.u32X = stRect.u32X;
    stPoint0.u32Y = stRect.u32Y + stRect.u16PicH;

    stPoint1.u32X = stRect.u32X;
    stPoint1.u32Y = stRect.u32Y;
    ST_OSD_DrawLine(hHandle, stPoint1, stPoint0, u8BorderWidth, u32Color);

    return MI_RGN_OK;
}

MI_S32 ST_OSD_FillRect(MI_RGN_HANDLE hHandle, ST_Rect_T stRect, MI_U32 u32Color)
{
    int i = 0;
    ST_Point_T stPoint0, stPoint1;

    ST_OSD_INIT_CHECK(hHandle);
    if (stRect.u32X > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width
        || stRect.u32Y > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height)
        return MI_ERR_RGN_ILLEGAL_PARAM;

    stRect.u32X = (stRect.u32X < 0) ? 0 : stRect.u32X;
    stRect.u32Y = (stRect.u32Y < 0) ? 0 : stRect.u32Y;
    stRect.u16PicW = ((stRect.u32X + stRect.u16PicW) > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width) ?
                        (g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Width - stRect.u32X) : stRect.u16PicW;
    stRect.u16PicH = ((stRect.u32Y + stRect.u16PicH) > g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height) ?
                        (g_stRgnInfo[hHandle].stCanvasInfo.stSize.u32Height - stRect.u32Y) : stRect.u16PicH;

    for (i = 0; i < stRect.u16PicH; i ++)
    {
        stPoint0.u32X = stRect.u32X;
        stPoint0.u32Y = stRect.u32Y + i;

        stPoint1.u32X = stRect.u32X + stRect.u16PicW;
        stPoint1.u32Y = stRect.u32Y + i;

        ST_OSD_DrawLine(hHandle, stPoint0, stPoint1, 1, u32Color);
    }

    return MI_RGN_OK;
}

// from: angle, end:angle
MI_S32 ST_OSD_DrawCircle(MI_RGN_HANDLE hHandle, ST_Point_T stPoint, int radii, int from, int end, MI_U8 u8BorderWidth, MI_U32 u32Color)
{
	MI_S32 i = 0, j = 0;
	MI_S32 ix = 0, iy = 0;
    ST_Point_T stPoint1;

    ST_OSD_INIT_CHECK(hHandle);

    for (j = 0; j < u8BorderWidth; j ++)
    {
        for (i = from; i <= end; i ++)
    	{
    		ix = radii * cos(((double)i / 180) * MI_PI);
    		iy = radii * sin(((double)i / 180) * MI_PI);

            stPoint1.u32X = stPoint.u32X + ix - j;
            stPoint1.u32Y = stPoint.u32Y + iy - j;

    		ST_OSD_DrawPoint(hHandle, stPoint1, u32Color);
    	}
    }

	return MI_RGN_OK;
}

// has some error
MI_S32 ST_OSD_FillCircle(MI_RGN_HANDLE hHandle, ST_Point_T stPoint, int radii, int from, int end, MI_U32 u32Color)
{
    int i = 0;
	int ix = 0, iy = 0;
    ST_Point_T stPoint1;

    ST_OSD_INIT_CHECK(hHandle);

	for (i = from; i <= end; i ++)
	{
		ix = radii * cos(((double)i / 180) * MI_PI);
		iy = radii * sin(((double)i / 180) * MI_PI);

        stPoint1.u32X = stPoint.u32X + ix;
        stPoint1.u32Y = stPoint.u32Y + iy;

		ST_OSD_DrawLine(hHandle, stPoint, stPoint1, 1, u32Color);
	}

    return MI_RGN_OK;
}

MI_S32 ST_OSD_DrawText(MI_RGN_HANDLE hHandle, ST_Point_T stPoint, const char *szString, MI_U32 u32Color, DMF_Font_Size_E enSize)
{
    ST_OSD_INIT_CHECK(hHandle);

    _OSD_DrawTextToCanvas(hHandle, stPoint, szString, u32Color, enSize);

    return MI_RGN_OK;
}

MI_S32 ST_OSD_Clear(MI_RGN_HANDLE hHandle, ST_Rect_T *pstRect)
{
    MI_RGN_CanvasInfo_t *pstRgnCanvasInfo = NULL;

    ST_OSD_INIT_CHECK(hHandle);

    pstRgnCanvasInfo = &g_stRgnInfo[hHandle].stCanvasInfo;

    if (g_stRgnInfo[hHandle].ePixelFmt == E_MI_RGN_PIXEL_FORMAT_ARGB1555)
    {
        memset((void*)pstRgnCanvasInfo->virtAddr, 0x23, pstRgnCanvasInfo->stSize.u32Height*pstRgnCanvasInfo->u32Stride);
    }
    else if (g_stRgnInfo[hHandle].ePixelFmt == E_MI_RGN_PIXEL_FORMAT_I4)
    {
        memset((void*)pstRgnCanvasInfo->virtAddr, 0, pstRgnCanvasInfo->stSize.u32Height*pstRgnCanvasInfo->u32Stride);
    }

    return MI_RGN_OK;
}
