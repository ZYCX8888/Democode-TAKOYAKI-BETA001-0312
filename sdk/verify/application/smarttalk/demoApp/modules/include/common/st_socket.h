#ifndef _ST_SOCKET_H_
#define _ST_SOCKET_H_

#include "st_common.h"

#define SOCKET_MAX 8
#define RTP_HEAD_SIZE  12

#ifdef  __cplusplus
extern "C"
{
#endif

typedef struct _RTP_header 
{
    /* byte 0 */
#if (BYTE_ORDER == LITTLE_ENDIAN)
    unsigned char csrc_len:4;
    unsigned char extension:1;
    unsigned char padding:1;
    unsigned char version:2;
#elif (BYTE_ORDER == BIG_ENDIAN)
    unsigned char version:2;
    unsigned char padding:1;
    unsigned char extension:1;
    unsigned char csrc_len:4;
#else
#error Neither big nor little
#endif
    /* byte 1 */
#if (BYTE_ORDER == LITTLE_ENDIAN)
    unsigned char payload:7;
    unsigned char marker:1;
#elif (BYTE_ORDER == BIG_ENDIAN)
    unsigned char marker:1;
    unsigned char payload:7;
#endif
    unsigned short seq_no;
    unsigned int timestamp;
    unsigned int ssrc;
} RTP_header; //12 Byte
typedef struct SocketInfo_s
{
    MI_S32 s32Socket;
    MI_U32 u32Ipaddr;//hex
    MI_U64 u64SocketStartTime;
} SocketInfo_T;

MI_S32 ST_Socket_CmdProcessInit();
MI_S32 ST_Socket_CmdProcessDeInit();

MI_S32 ST_CreateVideoRecvSocket();
MI_S32 ST_Socket_UdpSend(MI_S32 s32Socket, unsigned long DstIP, MI_S32 s32DstPort,
    MI_U8 *u8SlicePack, MI_S32 s32SlicePackLen);
MI_S32 ST_Socket_PackNalu_UdpSend(MI_S32 s32UdpSocket, MI_U8 *NaluBuf, MI_S32 s32BufLen,
    unsigned long DstIP, MI_S32 s32DstPort);
MI_S32 ST_CreateSendVideoSocket();

MI_S32 ST_CreateSendAudioSocket();
MI_S32 ST_CreateAudioRecvSocket();
#ifdef  __cplusplus
}
#endif

#endif //_ST_SOCKET_H_