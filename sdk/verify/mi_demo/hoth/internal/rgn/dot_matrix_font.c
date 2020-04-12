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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

#include "dot_matrix_font.h"
#include "gb2312_code.h"

#define MAX_BUF_LEN                 1024
#define BITS_PER_PIXEL              16

#define MAX_LINES                   16

#define BMP_HEAD_LEN                54

#define ALIGN_UP(x, align)          (((x) + ((align) - 1)) & ~((align) - 1))
#define ALIGN_DOWN(x, a)            (((x) / (a)) * (a))
#define MAX(a,b)                    ((a) > (b) ? (a) : (b))
#define MIN(a,b)                    ((a) < (b) ? (a) : (b))

#define ALIGN_MULTI(x, align)       (((x) % (align)) ? ((x) / (align) + 1) : ((x) / (align)))

typedef enum
{
    UTF8_NO_BOM = 0,
    UTF8_WITH_BOM,
    GBK,

    Encoding_Butt,
} DMF_Encoding_Type_E;

typedef struct
{
    char            szFile[64];
    int             fd;
    unsigned char*  pBitMapAddr;
    struct stat     st;
    int             width;
    int             height;
} DMF_BitMapFile_S;

typedef struct
{
    int             width;
    int             height;
    int             bgColor;
    int             fgColor;
    uint32_t*       pData;
} DMF_BMP_S;

static DMF_BitMapFile_S g_dmf_bitmapfile =
{
    .fd = -1,
    .pBitMapAddr = NULL,
    .width = 16,
    .height = 16,
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

/*
 * see: https://github.com/JulienPalard/is_utf8/blob/master/is_utf8.c
 * Check if the given unsigned char * is a valid utf-8 sequence.
 *
 * Return value :
 * If the string is valid utf-8, 0 is returned.
 * IF -1 returned, it is very likely valid utf-8.
 * Else the position, starting from 1, is returned.
 *
 * Valid utf-8 sequences look like this :
 * 0xxxxxxx
 * 110xxxxx 10xxxxxx
 * 1110xxxx 10xxxxxx 10xxxxxx
 * 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 * 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 * 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
 */
int _dmf_IsUtf8(unsigned char *string, int len)
{
    size_t i = 0;
	size_t continuation_bytes = 0;
	int quick_flag = 0;
    int count = 0;

	while (i < len)
    {
        // printf("%s %d, 0x%X, len:%d\n", __func__, __LINE__, string[i], len);
		switch (string[i])
        {
		case 0x0 ... 0x7f:
			continuation_bytes = 0;
			break;
		case 0xC0 ... 0xDF:
			continuation_bytes = 1;
			break;
		case 0xE0 ... 0xEF:
			continuation_bytes = 2;
			break;
		case 0xF0 ... 0xF4: /* Cause of RFC 3629 */
			continuation_bytes = 3;
			quick_flag = 1;
			break;
		default:
			return i + 1;
		}
		i += 1;
		while (i < len && continuation_bytes > 0
			&& string[i] >= 0x80
			&& string[i] <= 0xBF)
        {
			i += 1;
			continuation_bytes -= 1;

            count ++;
		}

		if (continuation_bytes != 0)
			return i + 1;
		else if (quick_flag)
			return -1;
	}

	return count;
}

DMF_Encoding_Type_E _dmf_DetectEncodingType(const char *string)
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

int _dmf_GetUtf8Length(const uint8_t *src)
{
	switch (*src)
    {
	case 0x0 ... 0x7f:
		return 1;
	case 0xC0 ... 0xDF:
		return 2;
	case 0xE0 ... 0xEF:
		return 3;
	case 0xF0 ... 0xF7:
		return 4;
	default:
		return -1;
	}
}

int _dmf_Utf8ToUnicode(const uint8_t *src, uint8_t *dst)
{
    int length;
	uint8_t unicode[2] = {0}; // Ð¡¶ËÐò

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

int _dmf_UnicodeToGb2312(uint16_t unicode, const uint16_t *mem_gb2312, int gb2312_num)
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

int _dmf_CalcCharNumGetGb2312Code(const char *string, int *charTotalNum, uint8_t *gb2312buf, int bufLen)
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

    enEncodIngType = _dmf_DetectEncodingType(string);

    if (enEncodIngType == UTF8_NO_BOM ||
        enEncodIngType == UTF8_WITH_BOM)
    {
        do
        {
            ret = _dmf_Utf8ToUnicode(ptr, unicode);
            // printf("%s %d, 0x%X, 0x%X\n", __func__, __LINE__, unicode[0], unicode[1]);
            if (ret < 0)
            {
                return -1;
            }

            ptr += ret;
            dealLen += ret;
            charNum ++;

            gb2312_code = _dmf_UnicodeToGb2312(unicode[0] +
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

    }

    *charTotalNum = charNum;

    return 0;
}

uint32_t _dmf_Gb2312codeToFontoffset(uint32_t gb2312code, uint32_t font_height)
{
    uint32_t fontoffset;

	fontoffset = (gb2312code % 0x100 - 0xA1) * 94
		            + (gb2312code / 0x100 - 0xA1);
	fontoffset *= (font_height * font_height / 8);

	return fontoffset;
}

uint32_t _dmf_AsciiToFontoffset(uint32_t ascii, uint32_t width, uint32_t height)
{
    uint32_t size = 0;

    size = (width / 8) * height;

    return ascii * size;
    // return (ascii)* 16;
	// return (ascii * 16) + 1;
}

void _dmf_CalcBMPWH(int charTotalNum, int *bmpWidth, int *bmpHeight, uint8_t *pGb2312)
{
    DMF_BitMapFile_S *pstDMFBitMapFile = &g_dmf_bitmapfile;
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
                width += 8;

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
        // ºáÅÅ
        width += pstDMFBitMapAttr->leftMargin +
                pstDMFBitMapAttr->charSpace * (charTotalNum - 1) +
                pstDMFBitMapAttr->rightMargin;

        height += pstDMFBitMapAttr->upMargin +
                pstDMFBitMapAttr->lineSpace * (lines - 1) +
                pstDMFBitMapAttr->downMargin;
    }
    else if (pstDMFBitMapAttr->verticalFlag == 1)
    {
        // ÊúÅÅ
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

DMF_BMP_S* _dmf_BMPCreate(int width, int height, int bgColor, int fgColor)
{
    DMF_BMP_S *pstBMP = NULL;

    if (width < 1 || height < 1)
    {
        return NULL;
    }

    pstBMP = (DMF_BMP_S*)malloc(sizeof(DMF_BMP_S));
    if (pstBMP == NULL)
    {
        return NULL;
    }

    pstBMP->width = ALIGN_UP(width, 4);
    pstBMP->height = ALIGN_UP(height, 4);
    pstBMP->bgColor = bgColor;
    pstBMP->fgColor = fgColor;

    pstBMP->pData = (uint32_t *)malloc(pstBMP->width *
                    pstBMP->height * sizeof(uint32_t));
    if (NULL == pstBMP->pData)
    {
        goto END;
    }

    memset(pstBMP->pData, 0, pstBMP->width *
                    pstBMP->height * sizeof(uint32_t));

    return pstBMP;

END:
    if (pstBMP)
    {
        if (pstBMP->pData)
        {
            free(pstBMP->pData);
            pstBMP->pData = NULL;
        }

        free(pstBMP);
        pstBMP = NULL;
    }
    return NULL;
}

void _dmf_BMPDestory(DMF_BMP_S *pstBMP)
{
    if (pstBMP == NULL)
    {
        return;
    }

    if (pstBMP->pData)
    {
        free(pstBMP->pData);
        pstBMP->pData = NULL;
    }

    free(pstBMP);
    pstBMP = NULL;
}

void _dmf_BMPPoint(DMF_BMP_S *pstBMP, int x, int y, uint32_t color)
{
    if ((pstBMP == NULL) || (pstBMP->pData == NULL) ||
        (x < 0) || (y < 0) || (x > pstBMP->width) || (y > pstBMP->height))
    {
        return;
    }

    // printf("(%d,%d), color:%X\n", x, y, color);

    pstBMP->pData[y * pstBMP->width + x] = color;
}

#define RGB2PIXEL888(r,g,b)	        (((r) << 16) | ((g) << 8) | (b))
#define PIXEL888RED(pixelval)		(((pixelval) >> 16) & 0xff)
#define PIXEL888GREEN(pixelval)		(((pixelval) >> 8) & 0xff)
#define PIXEL888BLUE(pixelval)		((pixelval) & 0xff)


#define RGB2PIXEL555(r,g,b)	\
	((((r) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((b) & 0xf8) >> 3))
#define PIXEL555RED(pixelval)		(((pixelval) >> 10) & 0x1f)
#define PIXEL555GREEN(pixelval)		(((pixelval) >> 5) & 0x1f)
#define PIXEL555BLUE(pixelval)		((pixelval) & 0x1f)


#define RGB2PIXEL1555(r,g,b)	\
	((((b) & 0xf8) << 7) | (((g) & 0xf8) << 2) | (((r) & 0xf8) >> 3) | 0x8000)
#define PIXEL1555BLUE(pixelval)		(((pixelval) >> 10) & 0x1f)
#define PIXEL1555GREEN(pixelval)	(((pixelval) >> 5) & 0x1f)
#define PIXEL1555RED(pixelval)		((pixelval) & 0x1f)

int _dmf_BMPWrite(DMF_BMP_S *pstBMP, char *szPath)
{
    if ((pstBMP == NULL) || (szPath == NULL))
    {
        return -1;
    }

    unsigned char head[BMP_HEAD_LEN];
    int i = 0, j = 0;
    int fd = 0;
    unsigned long len = 0;

#if 0
    memset(head, 0, BMP_HEAD_LEN);
    fd = open(szPath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0)
    {
        return -1;
    }

    len = BMP_HEAD_LEN + 3 * pstBMP->width * pstBMP->height;
	head[0] = 'B';
	head[1] = 'M';
	head[2] = len & 0xff;
	head[3] = (len >> 8) & 0xff;
	head[4] = (len >> 16) & 0xff;
	head[5] = (len >> 24) & 0xff;
	head[10] = BMP_HEAD_LEN;
	head[14] = 40;
	head[18] = pstBMP->width & 0xff;
	head[19] = (pstBMP->width >> 8) & 0xff;
	head[20] = (pstBMP->width >> 16) & 0xff;
	head[22] = pstBMP->height & 0xff;
	head[23] = (pstBMP->height >> 8) & 0xff;
	head[24] = (pstBMP->height >> 16) & 0xff;
	head[26] = 1;
	head[28] = 24;
	head[34] = 16;
	head[36] = 0x13; // 2835 pixels/meter
	head[37] = 0x0b;
	head[42] = 0x13; // 2835 pixels/meter
	head[43] = 0x0b;

    //----------------------------------------
	// Write header.
	//
	if (BMP_HEAD_LEN != write (fd, head, BMP_HEAD_LEN))
    {
		close(fd);
		return -1;
	}

    //----------------------------------------
	// Write pixels.
	// Note that BMP has lower rows first.
	//
	for (j = pstBMP->height - 1; j >= 0; j--)
    {
		for (i = 0; i < pstBMP->width; i++)
        {
			unsigned char rgb[3];
			int ix = i + j * pstBMP->width;
			unsigned long pixel = pstBMP->pData[ix];
			rgb[0] = pixel & 0xff;
			rgb[1] = (pixel >> 8) & 0xff;
			rgb[2] = (pixel >> 16) & 0xff;
			if (3 != write(fd, rgb, 3))
            {
				close(fd);
				return -1;
			}
		}
	}

	close(fd);
#else
    int ix = 0;
    short color = 0;

    fd = open(szPath, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd < 0)
    {
        return -1;
    }

    for (j = 0; j < pstBMP->height; j++)
    // for (j = pstBMP->height - 1; j >= 0; j--)
    {
		for (i = 0; i < pstBMP->width; i++)
        {
            ix = i + j * pstBMP->width;
            color = RGB2PIXEL555(PIXEL888RED(pstBMP->pData[ix]), PIXEL888GREEN(pstBMP->pData[ix]),
                                    PIXEL888BLUE(pstBMP->pData[ix]));
			if (2 != write(fd, &color, 2))
            {
				close(fd);
				return -1;
			}
		}
	}

    close(fd);
#endif

    return 0;
}

int _dmf_BMPDumpBuf(DMF_BMP_S *pstBMP, unsigned char *pBuf)
{
    if ((pstBMP == NULL) || (pBuf == NULL))
    {
        return -1;
    }

    int ix = 0;
    short color = 0;
    int i = 0, j = 0;
    short *pTmp = (short *)pBuf;

    for (j = 0; j < pstBMP->height; j++)
    {
		for (i = 0; i < pstBMP->width; i++)
        {
            ix = i + j * pstBMP->width;
            color = RGB2PIXEL1555(PIXEL888RED(pstBMP->pData[ix]), PIXEL888GREEN(pstBMP->pData[ix]),
                                    PIXEL888BLUE(pstBMP->pData[ix]));

            *pTmp = color;
            pTmp ++;
		}
	}

    return 0;
}

void _dmf_FontDataToBMP(const uint8_t *pFontdata, int x, int y, int width,
                        int height, DMF_BMP_S *pstBMP)
{
    int i = 0, j = 0;
    int char_num;
	int char_bit;
	char bit;
    const uint8_t *pFontdataTemp = NULL;

    if ((pFontdata == NULL) || (pstBMP == NULL))
    {
        return;
    }

    if (pstBMP->pData == NULL)
    {
        return;
    }

#if 0
    for (i = 0; i < height; i ++) // µ¹Á¢µÄÎ»Í¼
#else
    for (i = height - 1; i >= 0; i--) // ÕýÁ¢µÄÎ»Í¼
    {
        pFontdataTemp = pFontdata + (width + 7) / 8 * i;

        for (j = 0; j < width; j ++)
        {
            char_num = j / 8;
		    char_bit = 7 - j % 8;
            bit = pFontdataTemp[char_num] & (1 << char_bit);

            if (bit)
            {
                _dmf_BMPPoint(pstBMP, x + j, y + i, pstBMP->fgColor);
            }
            else
            {
                _dmf_BMPPoint(pstBMP, x + j, y + i, pstBMP->bgColor);
            }
        }
    }
#endif
}

int DMF_LoadBitMapFile(int width, int height, const char *szFile)
{
    DMF_BitMapFile_S *pstDMFBitMapFile = &g_dmf_bitmapfile;
    int ret = 0;
    int fd = -1;
    unsigned char* pBitMapAddr = NULL;

    if (szFile == NULL)
    {
        ret = -1;
        goto END;
    }

    if ((pstDMFBitMapFile->fd > 0) &&
        (pstDMFBitMapFile->pBitMapAddr != NULL))
    {
        return 0;
    }

    fd = open(szFile, O_RDONLY);
    if (fd < 0)
    {
        printf("%s %d, open %s fail\n", __func__, __LINE__, szFile);
        ret = -1;
        goto END;
    }

    if (-1 == fstat(fd, &pstDMFBitMapFile->st))
    {
        ret = -1;
        goto END;
    }

    pBitMapAddr = (unsigned char *)mmap(NULL, (size_t)pstDMFBitMapFile->st.st_size,
                        PROT_READ, MAP_PRIVATE, fd, (off_t)0);
    if (pBitMapAddr == MAP_FAILED)
    {
        ret = -1;
        goto END;
    }

    snprintf(pstDMFBitMapFile->szFile, sizeof(pstDMFBitMapFile->szFile) - 1, "%s",
        szFile);
    pstDMFBitMapFile->fd            = fd;
    pstDMFBitMapFile->pBitMapAddr   = pBitMapAddr;
    pstDMFBitMapFile->width         = width;
    pstDMFBitMapFile->height        = height;

    return 0;

END:
    if (fd > 0)
        close(fd);

    return ret;
}

int DMF_SetBitMapAttr(DMF_BitMapAttr_S *pstDMFBitMapAttr)
{
    if (pstDMFBitMapAttr == NULL)
    {
        return -1;
    }

    memcpy(&g_dmf_bitmapattr, pstDMFBitMapAttr, sizeof(DMF_BitMapAttr_S));

    return 0;
}

int DMF_DumpToBMPFile(const char *string, char *szFile)
{
    DMF_BitMapFile_S *pstDMFBitMapFile = &g_dmf_bitmapfile;
    DMF_BitMapAttr_S *pstDMFBitMapAttr = &g_dmf_bitmapattr;
    int i = 0, j = 0;
    uint8_t gb2312buf[MAX_BUF_LEN * 2];
    uint8_t *pGb2312Line = NULL;
    int charTotalNum = 0, charNumPerLine[MAX_LINES];
    int charRemainNum = 0;
    int totalLines = 0;
    int gb2312Offset = 0, gb2312LineOffset = 0;
    int bmpWidth = 0, bmpHeight = 0;
    DMF_BMP_S *pstBMP = NULL;
    uint32_t fontoffset = 0;
    int fontWidth = 0, fontHeight = 0;
    int xpos = 0, ypos = 0;
    int fontTotalWidth = 0, fontTotalHeight = 0;
    uint8_t *fontAddr = NULL;

    if ((string == NULL) || (szFile == NULL))
    {
        return -1;
    }

    memset(gb2312buf, 0, MAX_BUF_LEN * 2);
    _dmf_CalcCharNumGetGb2312Code(string, &charTotalNum, gb2312buf, MAX_BUF_LEN * 2);

    totalLines = ALIGN_MULTI(charTotalNum, pstDMFBitMapAttr->charNumPerLine);
    totalLines = MIN(totalLines, MAX_LINES);

    memset(&charNumPerLine, 0, MAX_LINES);
    charRemainNum = charTotalNum;
    for (i = 0; i < totalLines; i ++)
    {
        charNumPerLine[i] = MIN(charRemainNum, pstDMFBitMapAttr->charNumPerLine);

        charRemainNum -= charNumPerLine[i];
    }

    _dmf_CalcBMPWH(charTotalNum, &bmpWidth, &bmpHeight, gb2312buf);

#if 0
    printf("string:\t%s\n", string);
    printf("charTotalNum:\t%d\n", charTotalNum);
    printf("charNumPerLine:\t");
    for (i = 0; i < totalLines; i ++)
    {
        printf("%d ", charNumPerLine[i]);
    }
    printf("\n");
    printf("totalLines:\t%d\n", totalLines);
    printf("bmpWidth:\t%d\n", bmpWidth);
    printf("bmpHeight:\t%d\n", bmpHeight);
#endif

    pstBMP = _dmf_BMPCreate(bmpWidth, bmpHeight,
                pstDMFBitMapAttr->bgColor, pstDMFBitMapAttr->fgColor);
    if (pstBMP == NULL)
    {
        return -1;
    }

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
                fontWidth = 8;
                fontHeight = 16;

                fontoffset = _dmf_AsciiToFontoffset(pGb2312Line[gb2312LineOffset], fontWidth, fontHeight);

                gb2312LineOffset ++;

                fontAddr = asciicode;
            }

            if (pstDMFBitMapAttr->verticalFlag == 0)
            {
                // ºáÅÅ
                xpos = pstDMFBitMapAttr->leftMargin + fontTotalWidth +
                        j * pstDMFBitMapAttr->lineSpace;
                ypos = pstDMFBitMapAttr->upMargin + i * pstDMFBitMapFile->height +
                        i * pstDMFBitMapAttr->lineSpace;

                fontTotalWidth += fontWidth;
            }
            else if (pstDMFBitMapAttr->verticalFlag == 1)
            {
                // ÊúÅÅ
                xpos = pstDMFBitMapAttr->leftMargin + i * pstDMFBitMapFile->width +
                        i * pstDMFBitMapAttr->lineSpace;
                ypos = pstDMFBitMapAttr->upMargin + fontTotalHeight +
                        j * pstDMFBitMapAttr->lineSpace;

                fontTotalHeight += fontHeight;
            }

            // printf("xpos:%d, ypos:%d, gb2312LineOffset:%d\n", xpos, ypos, gb2312LineOffset);
            _dmf_FontDataToBMP(fontAddr + fontoffset,
                            xpos, ypos, fontWidth, fontHeight,
                            pstBMP);
        }

        fontTotalWidth = 0;
        fontTotalHeight = 0;

        gb2312Offset += gb2312LineOffset;
    }

    if (0 != _dmf_BMPWrite(pstBMP, szFile))
    {
        return -1;
    }

    _dmf_BMPDestory(pstBMP);

    return 0;
}

int DMF_DumpToBMPBuf(const char *string, DMF_BitMapData_S *pstBitMapData)
{
    DMF_BitMapFile_S *pstDMFBitMapFile = &g_dmf_bitmapfile;
    DMF_BitMapAttr_S *pstDMFBitMapAttr = &g_dmf_bitmapattr;
    int i = 0, j = 0;
    uint8_t gb2312buf[MAX_BUF_LEN * 2];
    uint8_t *pGb2312Line = NULL;
    int charTotalNum = 0, charNumPerLine[MAX_LINES];
    int charRemainNum = 0;
    int totalLines = 0;
    int gb2312Offset = 0, gb2312LineOffset = 0;
    int bmpWidth = 0, bmpHeight = 0;
    DMF_BMP_S *pstBMP = NULL;
    uint32_t fontoffset = 0;
    int fontWidth = 0, fontHeight = 0;
    int xpos = 0, ypos = 0;
    int fontTotalWidth = 0, fontTotalHeight = 0;
    uint8_t *fontAddr = NULL;

    if ((string == NULL) || (pstBitMapData == NULL))
    {
        return -1;
    }

    memset(gb2312buf, 0, MAX_BUF_LEN * 2);
    _dmf_CalcCharNumGetGb2312Code(string, &charTotalNum, gb2312buf, MAX_BUF_LEN * 2);

    totalLines = ALIGN_MULTI(charTotalNum, pstDMFBitMapAttr->charNumPerLine);
    totalLines = MIN(totalLines, MAX_LINES);

    memset(&charNumPerLine, 0, MAX_LINES);
    charRemainNum = charTotalNum;
    for (i = 0; i < totalLines; i ++)
    {
        charNumPerLine[i] = MIN(charRemainNum, pstDMFBitMapAttr->charNumPerLine);

        charRemainNum -= charNumPerLine[i];
    }

    _dmf_CalcBMPWH(charTotalNum, &bmpWidth, &bmpHeight, gb2312buf);

#if 0
    printf("string:\t%s\n", string);
    printf("charTotalNum:\t%d\n", charTotalNum);
    printf("charNumPerLine:\t");
    for (i = 0; i < totalLines; i ++)
    {
        printf("%d ", charNumPerLine[i]);
    }
    printf("\n");
    printf("totalLines:\t%d\n", totalLines);
    printf("bmpWidth:\t%d\n", bmpWidth);
    printf("bmpHeight:\t%d\n", bmpHeight);
#endif

    pstBMP = _dmf_BMPCreate(bmpWidth, bmpHeight,
                pstDMFBitMapAttr->bgColor, pstDMFBitMapAttr->fgColor);
    if (pstBMP == NULL)
    {
        return -1;
    }

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
                fontWidth = 8;
                fontHeight = 16;

                fontoffset = _dmf_AsciiToFontoffset(pGb2312Line[gb2312LineOffset], fontWidth, fontHeight);

                gb2312LineOffset ++;

                fontAddr = asciicode;
            }

            if (pstDMFBitMapAttr->verticalFlag == 0)
            {
                // ºáÅÅ
                xpos = pstDMFBitMapAttr->leftMargin + fontTotalWidth +
                        j * pstDMFBitMapAttr->lineSpace;
                ypos = pstDMFBitMapAttr->upMargin + i * pstDMFBitMapFile->height +
                        i * pstDMFBitMapAttr->lineSpace;

                fontTotalWidth += fontWidth;
            }
            else if (pstDMFBitMapAttr->verticalFlag == 1)
            {
                // ÊúÅÅ
                xpos = pstDMFBitMapAttr->leftMargin + i * pstDMFBitMapFile->width +
                        i * pstDMFBitMapAttr->lineSpace;
                ypos = pstDMFBitMapAttr->upMargin + fontTotalHeight +
                        j * pstDMFBitMapAttr->lineSpace;

                fontTotalHeight += fontHeight;
            }

            // printf("xpos:%d, ypos:%d, gb2312LineOffset:%d\n", xpos, ypos, gb2312LineOffset);
            _dmf_FontDataToBMP(fontAddr + fontoffset,
                            xpos, ypos, fontWidth, fontHeight,
                            pstBMP);
        }

        fontTotalWidth = 0;
        fontTotalHeight = 0;

        gb2312Offset += gb2312LineOffset;
    }

    if (0 != _dmf_BMPDumpBuf(pstBMP, pstBitMapData->pBuf))
    {
        return -1;
    }

    pstBitMapData->width = pstBMP->width;
    pstBitMapData->height = pstBMP->height;

    _dmf_BMPDestory(pstBMP);

    return 0;
}

int DMF_CloseBitMapFile(void)
{
    DMF_BitMapFile_S *pstDMFBitMapFile = &g_dmf_bitmapfile;

    if ((pstDMFBitMapFile->fd < 0) ||
        (pstDMFBitMapFile->pBitMapAddr == NULL))
    {
        return 0;
    }

    munmap(pstDMFBitMapFile->pBitMapAddr, (size_t)pstDMFBitMapFile->st.st_size);
    pstDMFBitMapFile->pBitMapAddr = NULL;

    close(pstDMFBitMapFile->fd);
    pstDMFBitMapFile->fd = -1;

    return 0;
}

