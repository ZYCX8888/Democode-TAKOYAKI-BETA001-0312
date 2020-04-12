#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>       /* time */

#define S8      char
#define U8      unsigned char
#define S32     long
#define U32     unsigned long

#define TRUE            1
#define FALSE           0

#define MIN(a,b)    ((a < b) ? a : b)

typedef enum {
    E_CODEC_NONE,
    E_CODEC_H264,
    E_CODEC_H265,
    E_CODEC_MAX
} E_CODEC_TYPE;

typedef enum {
    E_DBG_LEVEL_NONE,
    E_DBG_LEVEL_ERR,
    E_DBG_LEVEL_INFO,
    E_DBG_LEVEL_DBG,
} E_DEBUG_LEVEL;

#define _DEBUG
#ifdef _DEBUG
#define ASCII_COLOR_RED     "\033[1;31m"
#define ASCII_COLOR_WHITE   "\033[1;37m"
#define ASCII_COLOR_YELLOW  "\033[1;33m"
#define ASCII_COLOR_BLUE    "\033[1;36m"
#define ASCII_COLOR_GREEN   "\033[1;32m"
#define ASCII_COLOR_END     "\033[0m"

#define FUNC_DBG(fmt, args...) \
        ({if(_gDbgLevel>=E_DBG_LEVEL_DBG){do{printf(ASCII_COLOR_WHITE"%s[L%d] ",__FUNCTION__,__LINE__);printf(fmt, ##args);printf(ASCII_COLOR_END);}while(0);}})
        //({if(_gDbgLevel>=E_DBG_LEVEL_DBG){do{printf("%s[L%d] ",__FUNCTION__,__LINE__);printf(fmt, ##args);}while(0);}})
#define FUNC_MSG(fmt, args...)  \
  ({if(_gDbgLevel>=E_DBG_LEVEL_INFO){do{printf(ASCII_COLOR_GREEN"%s[L%d] ",__FUNCTION__,__LINE__);printf(fmt, ##args);printf(ASCII_COLOR_END);}while(0);}})
#define FUNC_ERR(fmt, args...)  \
  ({if(_gDbgLevel>=E_DBG_LEVEL_ERR){do{printf(ASCII_COLOR_RED"%s[L%d] ",__FUNCTION__,__LINE__);printf(fmt, ##args);printf(ASCII_COLOR_END);}while(0);}})
  //({if(_gDbgLevel>=E_VDEC_EX_DBG_LEVEL_ERR){do{printf(ASCII_COLOR_WHITE"%s[%d] ",__FUNCTION__,__LINE__);printf(fmt, ##args);printf(ASCII_COLOR_END);}while(0);}})

#else
#define FUNC_DBG(fmt, args...)
#define FUNC_MSG(fmt, args...)
#define FUNC_ERR(fmt, args...)
#endif

#define NUM_HEADER_MESS_BYTES   2
#define NUM_DATA_MESS_BYTES     10

//-====================================================================
E_DEBUG_LEVEL _gDbgLevel = E_DBG_LEVEL_DBG;
int _gnHeaderMess = NUM_HEADER_MESS_BYTES;
int _gnDataMess   = NUM_DATA_MESS_BYTES;
int _gbMessBit    = TRUE;

int _gbRandInited = FALSE;


//-====================================================================

void display_help(void)
{
    printf("\n");
    printf("Usage: es_tool [-h] [-i input_file] [-o output_file] [-c h264|h265] [-d dbg_level]\n"
           "               [-m mess_header_cnt] [-n mess_data_cnt]\n");
    printf("\th: display help\n");
    printf("\ti: set input file path\n");
    printf("\to: set output file path\n");
    printf("\tc: set codec type, if not specify codec type, each nal will be one es pack\n");
    printf("\td: set debug_level: 0(None),1(Error),2(Info),3(Debug)\n");
    printf("\tm: set num of header bytes to messup. default: 2\n");
    printf("\tn: set num of data bytes to messup. default: 10\n");
}

char *find_start_code(char *input_buffer, int input_size, E_CODEC_TYPE codec_type)
{
    char *start_code_ptr = input_buffer;
    char return_next_nal = 0;

    start_code_ptr += 4;

    while (start_code_ptr - input_buffer < input_size)
    {
        if (start_code_ptr[0] == 0x00 &&
            start_code_ptr[1] == 0x00 &&
            start_code_ptr[2] == 0x00 &&
            start_code_ptr[3] == 0x01)
        {
            return start_code_ptr;
        }
        else
            start_code_ptr++;
    }

    return NULL;
}


void mess_nalu(char tag, U8 *pu8Buf, int mess_cnt, int dataSize, int offset)
{
    int i, byte, bit;
    U8 mask, value;

    for (i = 0; i < mess_cnt; i++)
    {
         byte = (rand() % (dataSize -offset)) + offset;
         if (_gbMessBit)
         {
             bit = rand() % 8;
             mask = 1 << bit;
             value = (pu8Buf[byte] & ~mask) | (~(pu8Buf[byte] & mask) & mask);
         }
         else
         {
             value = rand() % 255;
         }
         printf("[0x%x][%d] (0x%X:%d,0x%02X-->0x%02X)\n", (int)tag, i, byte, bit, pu8Buf[byte], value);
         pu8Buf[byte] = value;
     }
}

//================================================================================
#define DEMO_VESFILE_FRAME_HEADER_LEN              (16)
#define MS_GETU32VALUE(pu8Data, index)             (pu8Data[index]<<24)|(pu8Data[index+1]<<16)|(pu8Data[index+2]<<8)|(pu8Data[index+3])
#define MS_SETU32VALUE(value, pu8Data, index)      {pu8Data[index]   = (value & 0xFF000000) >>24; \
                                                    pu8Data[index+1] = (value & 0x00FF0000) >>16; \
                                                    pu8Data[index+2] = (value & 0x0000FF00) >>8; \
                                                    pu8Data[index+3] = (value & 0x000000FF); }

void messup_packet(E_CODEC_TYPE eCodecType, U8 *pu8Buf, S32 dataSize, U32 *pOutSize)
{
    U32 u32HeaderSize, u32SliceDataSize;
    int i, byte;
    U8  value;

    /* initialize random seed: */
    if (!_gbRandInited)
    {
        srand (time(NULL));
        _gbRandInited = TRUE;
    }

   // (H264) 0x67: SPS, 0x68: PPS, 0x06: SEI, 0x65: IDR, 0x21: P frame
    if (E_CODEC_H264 == eCodecType)
    {
        switch (pu8Buf[4])
        {
            case 0x67:    /* SPS */
            case 0x68:    /* PPS */
                mess_nalu(pu8Buf[4], pu8Buf, _gnHeaderMess, dataSize, 5);
                break;

            case 0x65:  /* IDR */
                u32HeaderSize = MIN(dataSize, 20);
                mess_nalu(pu8Buf[4], pu8Buf, _gnHeaderMess, u32HeaderSize, 5);

                if (dataSize > u32HeaderSize)
                {
                    mess_nalu(pu8Buf[4], pu8Buf, _gnDataMess, dataSize, u32HeaderSize);
                }
                break;
            case 0x61:
                mess_nalu(pu8Buf[4], pu8Buf, _gnDataMess, dataSize, 5);
                break;
            default:
                break;
        }
    }
    // (H265) 0x40: VPS 0x42: SPS, 0x44: PPS, 0x4e: SEI, 0x26:IDR, 0x02: P frame
    else if (E_CODEC_H265 == eCodecType)
    {
        switch (pu8Buf[4])
        {
            case 0x40:   /* VPS */
            case 0x42:   /* SPS */
            case 0x44:   /* PPS */
                mess_nalu(pu8Buf[4], pu8Buf, _gnHeaderMess, dataSize, 6);
                break;

            case 0x26:   /* IDR */
                u32HeaderSize = MIN(dataSize, 20);
                mess_nalu(pu8Buf[4], pu8Buf, _gnHeaderMess, u32HeaderSize, 6);

                if (dataSize > u32HeaderSize)
                {
                    mess_nalu(pu8Buf[4], pu8Buf, _gnDataMess, dataSize, u32HeaderSize);
                }
                break;
            case 0x02:   /* P feame */
                mess_nalu(pu8Buf[4], pu8Buf, _gnDataMess, dataSize, 5);
                break;
            case 0x4e:   /* SEI */
            default:
                break;
        }
    }

    *pOutSize = dataSize;
}


int messup_es_stream(E_CODEC_TYPE eCodecType, int input_fd, int out_fd)
{
    U8 *pu8Buf= NULL;
    S32 s32ReadLen;
    U32 u32EsSize, u32OutEsSize = 0;
    U8 sFrameHeader[DEMO_VESFILE_FRAME_HEADER_LEN] = {0};
    U8 *ptr, *pu8StartCode;
    U32 u32PacketSize, u32OutSize;
    int len;

    while(DEMO_VESFILE_FRAME_HEADER_LEN == read(input_fd, sFrameHeader, DEMO_VESFILE_FRAME_HEADER_LEN))
    {
        u32EsSize = MS_GETU32VALUE(sFrameHeader, 4);
        pu8Buf = (U8*)malloc(u32EsSize);
        if(pu8Buf == NULL)
        {
            FUNC_ERR("Error: failed to malloc(%d) for reading VES file!\n", (int)u32EsSize);
            return -1;
        }

        s32ReadLen = read(input_fd, pu8Buf, u32EsSize);
        if(s32ReadLen <= 0)
        {
            FUNC_MSG("read end of file!\n");
            return -1;
        }

        ptr = pu8Buf;
        u32OutEsSize = 0;
        while (ptr - pu8Buf < u32EsSize)
        {
            if (pu8StartCode = find_start_code(ptr, (u32EsSize - (ptr - pu8Buf)), eCodecType))
                u32PacketSize = pu8StartCode - ptr;
            else
                u32PacketSize = u32EsSize - (ptr - pu8Buf);

            messup_packet(eCodecType, ptr, u32PacketSize, &u32OutSize);
            printf("messup_packet[0x%x]: p=%p,%p sz=%x(%x)\n", ptr[4], pu8Buf, ptr, (int)u32PacketSize, (int)u32OutSize);
            u32OutEsSize += u32OutSize;
            ptr += u32PacketSize;
        }

        if (u32OutEsSize < s32ReadLen)
        {
            MS_SETU32VALUE(u32OutEsSize, sFrameHeader, 4);
        }
        if ((len = write(out_fd, sFrameHeader, DEMO_VESFILE_FRAME_HEADER_LEN)) != DEMO_VESFILE_FRAME_HEADER_LEN)
        {
            FUNC_ERR("write packet header fail(written=%d)!\n", len);
            return -1;
        }

        if ((len = write(out_fd, pu8Buf, u32OutEsSize)) != u32OutEsSize)
        {
            FUNC_ERR("write packet fail(towrite=0x%x, written=0x%x)!\n", (int)u32OutEsSize, len);
            return -1;
        }
        free(pu8Buf);
     }

    return 0;
}

int main(int argc, char *argv[])
{
    int c, input_fd=-1, out_fd=-1;
    char *input_file_path=NULL, *output_file_path=NULL;
    E_CODEC_TYPE codec_type=E_CODEC_NONE;

    while ((c = getopt (argc, argv, "hi:o:c:d:m:n:")) != -1)
    {
        switch (c)
        {
            case 'i':
                input_file_path = optarg;
                break;
            case 'o':
                output_file_path = optarg;
                break;
            case 'c':
                if (!strncmp(optarg, "h264", sizeof("h264")))
                {
                    codec_type = E_CODEC_H264;
                    //printf("Codec is H.264\n");
                }
                else if (!strncmp(optarg, "h265", sizeof("h265")))
                {
                    codec_type = E_CODEC_H265;
                    //printf("Codec is H.265\n");
                }
                break;
            case 'd':
                _gDbgLevel = atoi(optarg);
                break;
            case 'm':
                _gnHeaderMess = atoi(optarg);
                 break;
            case 'n':
                _gnDataMess = atoi(optarg);
                 break;
            case 'h':
            default:
                display_help();
                return -1;
        }
    }

    if (input_file_path == NULL)
    {
        FUNC_ERR("Please specify input file name with -i\n");
        return -1;
    }

    if (output_file_path == NULL)
    {
        FUNC_ERR("Please specify output file name with -o\n");
        return -1;
    }


    input_fd = open(input_file_path, O_RDONLY, 0);
    if (0 > input_fd)
    {
        FUNC_ERR("can't open file:%s error\n",input_file_path);
        return -1;
    }
    out_fd = open(output_file_path, O_WRONLY | O_CREAT, 0644);
    if (0 > out_fd)
    {
        FUNC_ERR("can't open file:%s error\n",output_file_path);
        close(input_fd);
        return -1;
    }

    messup_es_stream(codec_type, input_fd, out_fd);

ES_EXIT:
    if (input_fd >= 0)
        close(input_fd);
    if (out_fd >= 0)
        close(out_fd);

	return 0;
}
