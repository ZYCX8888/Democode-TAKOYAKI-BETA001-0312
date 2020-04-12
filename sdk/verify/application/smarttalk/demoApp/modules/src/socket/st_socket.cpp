#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <linux/if.h>
#include <errno.h>

#include "st_common.h"
#include "st_socket.h"
#include "st_system.h"
#include "app_config.h"

#include "cmdqueue.h"

#define closesocket close
#define ioctlsocket ioctl
#define socketsend send
#define socketrecv recv

#define SOCKET_INVALID (-1)
SocketInfo_T g_stSocketInfo[SOCKET_MAX];
static MI_U8 u8TmpSendBuf[1600];

static MI_S32 g_ProcessRun = FALSE;
static MI_S32 g_ListenRun = FALSE;
static MI_S32 g_CallRun = FALSE;
static MI_S32 g_AnswerCallRun = FALSE;
static MI_S32 g_TcpRecvRun = FALSE;


static pthread_t tid_SocketProcess;
static pthread_t tid_SocketListen;
static pthread_t tid_SocketCall;
static pthread_t tid_SocketAnswerCall;
static pthread_t tid_SocketTcpRecv;

MsgQueue g_SocketProcess_Queue;
sem_t g_SocketProcess_Sem;

MsgQueue g_SocketCall_Queue;
sem_t g_SocketCall_Sem;
MsgQueue g_SocketAnswerCall_Queue;
sem_t g_SocketAnswerCall_Sem;

extern MsgQueue g_toAPP_msg_queue;
extern sem_t g_toAPP_sem;

static void SocketDebug(MI_S32 s32Num)
{
    if (SOCKET_DEBUG_FLAG)
    {
        printf("SocketDebug__(%d)__\n", s32Num);
    }
}

static MI_S32 GetIdleSocketIndex()
{
    MI_S32 i;
    for (i = 0; i < SOCKET_MAX; i++)
    {
        if (g_stSocketInfo[i].s32Socket == SOCKET_INVALID){
            return i;
        }
    }

    return -1;
}

static MI_S32 GetSocketByIp(unsigned long IPaddr)
{
    MI_S32 i;
    for (i = 0; i < SOCKET_MAX; i++)
    {
        if (g_stSocketInfo[i].u32Ipaddr == IPaddr)
        {
            if (g_stSocketInfo[i].s32Socket != SOCKET_INVALID)
            {
                return g_stSocketInfo[i].s32Socket;
            }
        }
    }

    return SOCKET_INVALID;
}

static void RemoveSocketByIp(unsigned long IPaddr)
{
    MI_S32 i;
    for (i = 0; i < SOCKET_MAX; i++)
    {
        if (g_stSocketInfo[i].u32Ipaddr == IPaddr)
        {
            if (g_stSocketInfo[i].s32Socket != SOCKET_INVALID)
            {
                g_stSocketInfo[i].s32Socket = -1;
                g_stSocketInfo[i].u32Ipaddr = 0xFFFFFFFF;
                g_stSocketInfo[i].u64SocketStartTime = 0;
            }
        }
    }

    return;
}

static MI_S32 SendBusyToRemote(MI_S32 s32Socket)
{
    MI_U8* u8PackBuf = (MI_U8*)malloc(sizeof(st_CmdPack_T));
    MI_U8* u8LocalID = NULL;
    MI_S32 s32len = sizeof(st_CmdPack_T);
    MI_U16 u16Cmd = 0x0;
    MI_S32 s32DevType = 0, s32MagicNum;
    ST_System_GetDeviceType(&s32DevType);
    u8LocalID = ST_System_GetLocalID();
    switch (s32DevType)
    {
        case E_ST_DEV_ROOM:
            u16Cmd = ROOM_BUSY;
            break;
        case E_ST_DEV_DOOR:
            u16Cmd = DOOR_BUSY;
            break;
        case E_ST_DEV_CENTER:
            u16Cmd = 0; //unkown
            break;
        default:
            ST_DBG("Unkown device type %d!\n", s32DevType);
    }
    u16Cmd = htons(u16Cmd);
    memset(u8PackBuf, 0, sizeof(st_CmdPack_T));
    s32MagicNum = HEADER_MAGIC_NUM;
    s32MagicNum = htonl(s32MagicNum);
    memcpy(u8PackBuf, &s32MagicNum, 4);
    memcpy(u8PackBuf+4, &s32len, 4);
    memcpy(u8PackBuf+8, &u16Cmd, 2);
    memcpy(u8PackBuf+10, &s32DevType, 2);
    memcpy(u8PackBuf+12, u8LocalID, CALL_ID_LEN);
    memset(u8PackBuf+20, '0', CALL_ID_LEN);
    socketsend(s32Socket, u8PackBuf, s32len, 0);
    if (u8PackBuf)
    {
        free(u8PackBuf);
        u8PackBuf = NULL;
    }
    closesocket(s32Socket);

    return 0;
}

MI_S32 ProcessRecvAnalyzeCmd(MI_S32 s32Socket, unsigned long IPaddr)
{
    struct timeval timeout;
    MI_U32 u32Msg[4];
    MI_S32 s32TmpLen = 0;
    MI_S32 s32Ret = 0, s32HeaderMagicNum;
    MI_U16 u16RecvCmd = 0;
    fd_set readfd;
    MI_U8 *u8RecvBuf = NULL;
    FD_ZERO(&readfd);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    SocketDebug(101);
    u8RecvBuf = (MI_U8 *)malloc(1024);
    memset(u8RecvBuf, 0, 1024);
    if (!u8RecvBuf)
    {
        return -1;
    }
    s32TmpLen = socketrecv(s32Socket, (char*)u8RecvBuf, sizeof(st_CmdPack_T), 0);
    if (s32TmpLen <= 0)
    {
        SocketDebug(102);
        return -1;
    }
    else
    {
        SocketDebug(120);
        memcpy(&s32HeaderMagicNum, u8RecvBuf, 4);
        s32HeaderMagicNum = ntohl(s32HeaderMagicNum);
        if (HEADER_MAGIC_NUM == s32HeaderMagicNum)
        {
            MI_S32 s32PacketLen = *(MI_S32*)(u8RecvBuf+4);
            if (s32PacketLen == s32TmpLen)
            {
                u16RecvCmd = *(MI_U16*)(u8RecvBuf + 8);
                u16RecvCmd = ntohs(u16RecvCmd);
                SocketDebug(103);

                memset(u32Msg, 0, 16);
                u32Msg[0] = MSG_TYPE_SOCKET_RECV_NORMAL_PACK;
                u32Msg[1] = (unsigned long)u8RecvBuf; //free by caller
                u32Msg[2] = s32TmpLen;
                u32Msg[3] = IPaddr;
                g_SocketProcess_Queue.send_message(MODULE_MSG, (void*)u32Msg, sizeof(u32Msg), &g_SocketProcess_Sem);
            }
            else if (s32PacketLen > s32TmpLen)
            {
                MI_U8* u8TmpPack = (MI_U8*)malloc(s32PacketLen+1);
                if(NULL == u8TmpPack)
                {
                    SocketDebug(104);
                    return -1;
                }
                MI_S32 s32AlreadyRecvLen = s32TmpLen;
                memcpy(u8TmpPack, u8RecvBuf, s32TmpLen);
                free(u8RecvBuf);
                SocketDebug(105);
                FD_ZERO(&readfd);
                FD_SET(s32Socket, &readfd);
                while (1)
                {
                    s32Ret = select(s32Socket+1, &readfd, NULL, NULL, &timeout);
                    if (s32Ret < 0)
                    {
                        free(u8TmpPack);
                        SocketDebug(106);
                        return -1;
                    }
                    if (s32Ret == 0)
                    {
                        SocketDebug(107);
                        continue;
                    }
                    if (FD_ISSET(s32Socket,&readfd))
                    {
                        s32TmpLen = socketrecv(s32Socket,(char*)(u8TmpPack+s32AlreadyRecvLen),s32PacketLen-s32AlreadyRecvLen,0);
                        if(s32TmpLen <= 0)
                        {
                            if(errno == EWOULDBLOCK)
                            {
                                SocketDebug(108);
                                continue;
                            }
                            ST_ERR("Recv buffer error...\n");
                            free(u8TmpPack);
                            u8TmpPack = NULL;
                            break;
                        }
                        else if(s32TmpLen > 0)
                        {
                            s32AlreadyRecvLen += s32TmpLen;
                            if (s32AlreadyRecvLen == s32PacketLen)
                            {
                                memset(u32Msg, 0, 16);
                                u32Msg[0] = MSG_TYPE_SOCKET_RECV_EXT_PACK;
                                u32Msg[1] = (MI_U32)u8TmpPack; //free by caller
                                u32Msg[2] = s32AlreadyRecvLen;
                                u32Msg[3] = IPaddr;
                                g_SocketProcess_Queue.send_message(MODULE_MSG, (void*)u32Msg, sizeof(u32Msg), &g_SocketProcess_Sem);
                                SocketDebug(109);
                            }
                            else if (s32AlreadyRecvLen < s32PacketLen)
                            {
                                SocketDebug(110);
                                continue;
                            }
                        }
                    }
                }//while(1)
            }
        }
    }

    return 0;
}

static MI_S32 StartListen(){
    struct sockaddr_in serveraddr;
    MI_S32 connected_len = 0;
    unsigned long flag = 1;
    MI_S32 g_listensocket=-1;
    g_listensocket = socket( AF_INET ,SOCK_STREAM, 0);
    if(g_listensocket == -1){
        ST_ERR("create g_listensocket error 0x%x",errno);
        return  -1;
    }
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = INADDR_ANY;
    serveraddr.sin_port   = htons(LOCAL_LISTEN_PORT);
    if(!bind(g_listensocket, (struct sockaddr*)&serveraddr, sizeof(serveraddr))){
        listen(g_listensocket,5);
    } else{
        ST_ERR("bind g_listensocket error 0x%x",errno);
        closesocket(g_listensocket);
        g_listensocket = -1;
        return  -1;
    }
    ioctlsocket( g_listensocket,FIONBIO,(char*)&flag);

    return  g_listensocket;
}

void * ST_Socket_Listen_Task(void* args){
    struct sockaddr_in dstaddr;
    int listensocket = -1;
    socklen_t addlen = (socklen_t)sizeof(struct sockaddr_in);
    unsigned long msg[4];
    unsigned long t_count = 0;
    listensocket = StartListen();
    if(listensocket == -1){
        ST_ERR("socket_listen_task start error,reboot");
        system("reboot -f ");
        return NULL;
    }
    else
    {
        ST_DBG("Create listen socket(%d) success\n", listensocket);
    }

    while (g_ListenRun)
    {
        int tmp_sock = -1;
        memset(msg,0,sizeof(unsigned long)*4);
        //ST_DBG("socket_listen_task  wait accept ");
        if((tmp_sock = accept(listensocket, (struct sockaddr*)&dstaddr,&addlen)) != -1){
            int ret = 0;
            memset(msg, 0, 16);
            msg[0] = MSG_TYPE_SOCKET_RECV_CONNECT;
            msg[1] = tmp_sock;
            msg[2] = dstaddr.sin_addr.s_addr;
            msg[3] = 0;
            g_SocketProcess_Queue.send_message(MODULE_MSG,(void*)msg,sizeof(msg),&g_SocketProcess_Sem);
            ST_DBG("Accept a connect, create new socket(%d) !!\n", tmp_sock);
            t_count++;
        }
        else
        {
            ;//ST_DBG("accept ERROR 0x%x",errno);
        }
        usleep(100*1000); //avoid connect too fast
    }

    return NULL;
}

static void ST_Socket_ProcessRecv()
{
    struct timeval timeout;
    fd_set readfd;
    MI_S32 max_fd = -1, i, s32Ret = -1;
    FD_ZERO(&readfd);
    for (i = 0; i < SOCKET_MAX; i++)
    {
        if (-1 != g_stSocketInfo[i].s32Socket)
        {
            FD_SET(g_stSocketInfo[i].s32Socket, &readfd);
            if (g_stSocketInfo[i].s32Socket > max_fd)
            {
                max_fd = g_stSocketInfo[i].s32Socket;
                //ST_DBG("Add Link socket(%d) to select.\n", g_stSocketInfo[i].s32Socket);
            }
        }
    }
    if (max_fd == -1)
    {
        SocketDebug(10);
        return;
    }
    timeout.tv_sec = 0;
    timeout.tv_usec = 100000;
    s32Ret = select(max_fd+1, &readfd, NULL, NULL, &timeout);
    if(s32Ret <= 0)
    {
        SocketDebug(11);
        return;
    }
    SocketDebug(12);
    for (i = 0; i < SOCKET_MAX; i++)
    {
        if (-1 != g_stSocketInfo[i].s32Socket)
        {
            if (FD_ISSET(g_stSocketInfo[i].s32Socket, &readfd))
            {
                SocketDebug(13);
                ProcessRecvAnalyzeCmd(g_stSocketInfo[i].s32Socket, g_stSocketInfo[i].u32Ipaddr);
            }
        }
    }
}

MI_U16 ST_SocketRecvCmd(MI_S32 s32Socket)
{
    struct timeval timeout;
    fd_set readfd;
    MI_S32 max_fd = -1, s32Ret = -1, s32TmpLen;
    MI_S32 s32HeaderMagicNum;
    MI_U16 u16RecvCmd;
    FD_ZERO(&readfd);
    FD_SET(s32Socket, &readfd);
    MI_U8 *u8RecvBuf;
    max_fd = s32Socket;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    s32Ret = select(max_fd+1, &readfd, NULL, NULL, &timeout);
    if(s32Ret <= 0)
    {
        SocketDebug(416);
        return 0;
    }
    else
    {
        SocketDebug(417);
        if (FD_ISSET(s32Socket, &readfd))
        {
            SocketDebug(13);
            u8RecvBuf = (MI_U8 *)malloc(1024);
            memset(u8RecvBuf, 0, 1024);
            if (!u8RecvBuf)
            {
                return -1;
            }
            s32TmpLen = 0;
            s32TmpLen = socketrecv(s32Socket, (char*)u8RecvBuf, sizeof(st_CmdPack_T), 0);
            if (s32TmpLen == sizeof(st_CmdPack_T))
            {
                memcpy(&s32HeaderMagicNum, u8RecvBuf, 4);
                s32HeaderMagicNum = ntohl(s32HeaderMagicNum);
                if (HEADER_MAGIC_NUM == s32HeaderMagicNum)
                {
                    MI_S32 s32PacketLen = *(MI_S32*)(u8RecvBuf+4);
                    if (s32PacketLen == s32TmpLen)
                    {
                        u16RecvCmd = *(MI_U16*)(u8RecvBuf + 8);
                        u16RecvCmd = ntohs(u16RecvCmd);
                        return u16RecvCmd;
                    }
                }
            }
        }
    }

    return 0;
}

MI_S32 ST_SocketSem_timeoutWait(sem_t *sem, MI_S32 s32Sec, MI_S32 s32NanoSec)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
    {
        printf("CLOCK_MONOTONIC\n");
        return -1;
    }
    ts.tv_sec += s32Sec;
    ts.tv_nsec += s32NanoSec;

    #define NSECTOSEC    1000000000
    ts.tv_sec += ts.tv_nsec/NSECTOSEC; //Nanoseconds [0 .. 999999999]
    ts.tv_nsec = ts.tv_nsec%NSECTOSEC;

    return sem_timedwait(sem, &ts);
}

void *ST_Socket_MainProcess(void *args)
{
    MI_S32 s32Ret = 0;
    Msg* pMsg = NULL;
    unsigned long recvmsg[4];
    unsigned long sendmsg[4];

    while (g_ProcessRun)
    {
        memset(recvmsg,0,sizeof(recvmsg));
        //ST_SocketSem_timeoutWait(&g_SocketProcess_Sem, 0, 1000*1000*1000);
        sem_wait(&g_SocketProcess_Sem);
        pMsg = g_SocketProcess_Queue.get_message();
        if (pMsg)
        {
            s32Ret = cmd_parse_msg(pMsg, recvmsg);
            if(s32Ret < 0)
                break;
            if(s32Ret == 0)
                continue;
            switch (recvmsg[0])
            {
                case MSG_TYPE_SOCKET_RECV_CONNECT:
                {
                    MI_S32 s32Index = -1;
                    MI_S32 s32Flag = TRUE;
                    MI_S32 s32SockBufLen = 16*1024;
                    s32Index = GetIdleSocketIndex();
                    if (-1 != s32Index)
                    {
                        g_stSocketInfo[s32Index].s32Socket = recvmsg[1];
                        g_stSocketInfo[s32Index].u32Ipaddr = recvmsg[2];
                        g_stSocketInfo[s32Index].u64SocketStartTime = ST_Sys_GetCurTimeU64();
                        ioctlsocket(g_stSocketInfo[s32Index].s32Socket, FIONBIO, (char*)&s32Flag);
                        setsockopt(g_stSocketInfo[s32Index].s32Socket, SOL_SOCKET, SO_SNDBUF, (char*)&s32SockBufLen, sizeof(MI_S32));
                        setsockopt(g_stSocketInfo[s32Index].s32Socket, SOL_SOCKET, SO_RCVBUF, (char*)&s32SockBufLen, sizeof(MI_S32));
                        ST_DBG("MSG_TYPE_SOCKET_RECV_CONNECT...\n");
                    }
                    else
                    {
                        SendBusyToRemote(recvmsg[1]);
                    }
                    break;
                }
                case MSG_TYPE_SOCKET_SEND_PACKET:
                {
                    break;
                }
                case MSG_TYPE_SOCKET_RECV_NORMAL_PACK:
                {
                    ST_DBG("MSG_TYPE_SOCKET_RECV_NORMAL_PACK.....packlen = %d\n", recvmsg[2]);
                    MI_U16 u16Cmd;
                    u16Cmd = *(MI_U16*)((char *)recvmsg[1] + 8);
                    switch (ntohs(u16Cmd))
                    {
                        case ROOM_STARTMONT:
                            memset(sendmsg, 0x0, 16);
                            sendmsg[0] = MSG_TYPE_RECV_MONITOR_CMD;
                            sendmsg[1] = ROOM_STARTMONT;
                            sendmsg[2] = 1;
                            sendmsg[3] = recvmsg[3];
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toAPP_sem);
                            break;
                        case ROOM_STOPMONT:
                            memset(sendmsg, 0x0, 16);
                            sendmsg[0] = MSG_TYPE_RECV_MONITOR_CMD;
                            sendmsg[1] = ROOM_STOPMONT;
                            sendmsg[2] = 0;
                            sendmsg[3] = recvmsg[3];
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toAPP_sem);
                            ST_DBG("Recv stop monitor\n");
                            break;
                        case ROOM_IDLE:
                            memset(sendmsg, 0x0, 16);
                            sendmsg[0] = MSG_TYPE_RECV_IDLE_CMD;
                            sendmsg[1] = ROOM_IDLE;
                            sendmsg[2] = 0;
                            sendmsg[3] = recvmsg[3];
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toAPP_sem);
                            break;
                        case ROOM_CALLROOM:
                        case DOOR_CALLROOM:
                            ST_DBG("Net Recv stop callroom\n");
                            memset(sendmsg, 0x0, 16);
                            sendmsg[0] = MSG_TYPE_RECV_REMOTE_CALL_ROOM;
                            sendmsg[1] = ntohs(u16Cmd);
                            sendmsg[2] = 0;
                            sendmsg[3] = recvmsg[3];
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toAPP_sem);
                            break;
                        case ROOM_HANGUP:
                            ST_DBG("Net Recv Hangup callroom\n");
                            memset(sendmsg, 0x0, 16);
                            sendmsg[0] = MSG_NET_TALK_CALLER_HANGUP;
                            sendmsg[1] = ROOM_HANGUP;
                            sendmsg[2] = 0;
                            sendmsg[3] = recvmsg[3];
                            g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toAPP_sem);
                            break;
                        default:
                            break;
                    }
                    break;
                }
                case MSG_TYPE_SOCKET_RECV_EXT_PACK:
                {
                    break;
                }
                default:
                    break;
            }
        }
        ST_Socket_ProcessRecv();
    }

    return NULL;
}

MI_S32 ST_Socket_TcpPacksend(MI_S32 s32Socket, const char *data, int len)
{
    MI_S32 i = 0;
    MI_S32 s32Ret = 0;
    MI_S32 result;
    MI_S32 count;
    MI_S32 mmsgid;
    struct timeval timeout;
    fd_set writefd;
    unsigned long maxfd;

    timeout.tv_sec = 0;
    timeout.tv_usec = 300*1000;

    if (s32Socket > 0)
    {
        FD_ZERO(&writefd);
        FD_SET(s32Socket, &writefd);
        maxfd = (s32Socket+1)>maxfd?(s32Socket+1):maxfd;
        result = select(maxfd, NULL, &writefd, NULL, &timeout);
        if (result)
        {
            if (FD_ISSET(s32Socket, &writefd))
            {
                SocketDebug(405);
                s32Ret = socketsend(s32Socket, data, len, 0);
            }
        }
        count++;
        if (s32Ret == len)
        {
            SocketDebug(406);
            ST_DBG("send cmd success\n");
            return 0;
        }
        else
        {
            SocketDebug(407);
            return -1;
        }
    }
    else
        printf("tcp_send sock < 0\n");

    return -1;
}

static void ST_Socket_SendCmdPack(MI_S32 s32Socket, MI_U16 u16Cmd)
{
    st_CmdPack_T stCmdPack;
    stCmdPack.s16DeviceType = E_ST_DEV_ROOM;
    stCmdPack.u16Cmd = htons(u16Cmd);
    stCmdPack.s32PackLen = sizeof(st_CmdPack_T);
    stCmdPack.u32HeaderMagicNum = htonl(HEADER_MAGIC_NUM);
    memcpy(stCmdPack.u8SrcID, "01010101", 8);
    memcpy(stCmdPack.u8DstID, "01010100", 8);
    if (s32Socket > 0)
    {
        if (0 != ST_Socket_TcpPacksend(s32Socket, (const char*)&stCmdPack, sizeof(st_CmdPack_T)))
        {
            ST_DBG("send cmd pack fail cmd(0x%x)\n", u16Cmd);
        }
        SocketDebug(430);
    }
    else
    {
        SocketDebug(431);
    }
}

void * ST_Socket_Local_Call_Task(void* args)
{
    MI_S32 s32Ret = -1;
    MI_BOOL bFlag = TRUE;
    MI_U8 *pu8TmpPack = NULL;
    MI_U16 u16RecvCmd;
    Msg* pMsg = NULL;
    unsigned long recvmsg[4];
    unsigned long sendmsg[4];
    struct sockaddr_in server_addr;
    MI_S32 s32SocketCall = -1;
    unsigned long CurServerIP;

    SocketDebug(397);
    MI_S32 s32ConnectCnt = 0;
    s32SocketCall = -1;
    s32SocketCall = socket(AF_INET, SOCK_STREAM, 0);
    ioctlsocket(s32SocketCall, FIONBIO, (char *)&bFlag);

    while (g_CallRun)
    {
        memset(recvmsg,0,sizeof(recvmsg));
        sem_wait(&g_SocketCall_Sem);
        SocketDebug(398);
        pMsg = g_SocketCall_Queue.get_message();
        if(pMsg)
        {
            SocketDebug(399);
            s32Ret = cmd_parse_msg(pMsg, recvmsg);
            if(s32Ret < 0)
                break;
            if(s32Ret == 0)
                continue;
            switch (recvmsg[0])
            {
                case MSG_TYPE_SOCKET_CONNECT_REMOTE:
                {
                    s32ConnectCnt = 0;
reconnect:
                    server_addr.sin_family = AF_INET;
                    server_addr.sin_port = htons(LOCAL_LISTEN_PORT); //recv cmd port
                    server_addr.sin_addr.s_addr = recvmsg[1];
                    CurServerIP = recvmsg[1];
                    s32Ret = connect(s32SocketCall, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));
                    SocketDebug(400);
                    if (0 == s32Ret)
                    {
                        SocketDebug(401);
                        ST_DBG("Connect remote success IPaddr(0x%x)(%s).\n", recvmsg[1], inet_ntoa(server_addr.sin_addr));
                    }
                    else
                    {
                        SocketDebug(402);
                        ST_ERR("Connect remote fail IPaddr(0x%x)(%s)  errorCode=0x%x!\n",
                            recvmsg[1], inet_ntoa(server_addr.sin_addr), s32Ret);
                        if (s32ConnectCnt++ < 4)
                        {
                            ST_DBG("goto connect server!\n");
                            usleep(10*1000);
                            goto reconnect;
                        }
                    }
                    break;
                }
                case MSG_TYPE_SOCKET_SEND_LOCAL_CALL_CMD:
                {
                    if (SOCKET_INVALID != s32SocketCall)
                    {
                        ST_Socket_SendCmdPack(s32SocketCall, recvmsg[2]);
                        if ((ROOM_STOPMONT == recvmsg[2]) || (ROOM_HANGUP == recvmsg[2]))
                        {
                            //usleep(100*1000);
                            //closesocket(s32SocketCall);
                            //s32SocketCall = -1;
                        }
                        ST_DBG("MSG_TYPE_SOCKET_SEND_LOCAL_CALL_CMD...socket(%d).cmd(0x%x).\n", s32SocketCall, recvmsg[2]);
                        u16RecvCmd = ST_SocketRecvCmd(s32SocketCall); //Wait idle/busy
                        if (0 != u16RecvCmd)
                        {
                            switch (u16RecvCmd)
                            {
                                case ROOM_MONTACK:
                                {
                                    memset(sendmsg, 0x0, 16);
                                    sendmsg[0] = MSG_NET_RECV_ROOM_MONTACK_CMD;
                                    sendmsg[1] = ROOM_MONTACK;
                                    sendmsg[2] = CurServerIP;
                                    sendmsg[3] = s32SocketCall;
                                    g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toAPP_sem);
                                    continue;
                                }
                                case ROOM_IDLE:
                                {
                                    memset(sendmsg, 0x0, 16);
                                    sendmsg[0] = MSG_TYPE_RECV_IDLE_CMD;
                                    sendmsg[1] = ROOM_IDLE;
                                    sendmsg[2] = CurServerIP;
                                    sendmsg[3] = s32SocketCall;
                                    g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toAPP_sem);
                                    if (ROOM_IDLE == u16RecvCmd)
                                    {
                                        MI_S32 s32WaitHoldOnCnt = 0;
                                        MI_S32 s32WaitHangUp = 0;
                                        MI_S32 s32HoldFlag = 0;
try_wait_holdon:
                                        u16RecvCmd = ST_SocketRecvCmd(s32SocketCall); //Wait idle/busy
                                        if (0 == u16RecvCmd)
                                        {
                                            printf("11111111111 %d %d\n", s32WaitHoldOnCnt, s32WaitHangUp);
                                            ST_SocketSem_timeoutWait(&g_SocketCall_Sem, 0, 10*1000*1000);
                                            pMsg = g_SocketCall_Queue.get_message();
                                            if(pMsg)
                                            {
                                                s32Ret = cmd_parse_msg(pMsg, recvmsg);
                                                if(s32Ret < 0)
                                                    break;
                                                if(s32Ret == 0)
                                                    continue;
                                                switch (recvmsg[0])
                                                {
                                                    case MSG_TYPE_SOCKET_SEND_LOCAL_CALL_CMD:
                                                    {
                                                        printf("11111111111222222222\n");
                                                        ST_Socket_SendCmdPack(s32SocketCall, ROOM_HANGUP);
                                                        //usleep(100*1000);
                                                        //closesocket(s32SocketCall);
                                                        //s32SocketCall = -1;
                                                        continue;
                                                    }
                                                    default:
                                                        break;
                                                }
                                            }
                                            if (0 == s32HoldFlag)
                                            {
                                                if (s32WaitHoldOnCnt++ < 15)
                                                {
                                                    goto try_wait_holdon;
                                                }
                                                else
                                                {
                                                    ST_Socket_SendCmdPack(s32SocketCall, ROOM_HANGUP);
                                                    memset(sendmsg, 0x0, 16);
                                                    sendmsg[0] = MSG_TIMEOUT_NO_HOLDON_HANGUP;
                                                    sendmsg[1] = 0;
                                                    sendmsg[2] = 0;
                                                    sendmsg[3] = 0;
                                                    g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toAPP_sem);
                                                }
                                            }
                                            else
                                            {
                                                if (s32WaitHangUp++ < 60)
                                                {
                                                    goto try_wait_holdon;
                                                }
                                                else
                                                {
                                                    ST_Socket_SendCmdPack(s32SocketCall, ROOM_HANGUP);
                                                    memset(sendmsg, 0x0, 16);
                                                    sendmsg[0] = MSG_TIMEOUT_HOLDON_HANGUP;
                                                    sendmsg[1] = 0;
                                                    sendmsg[2] = 0;
                                                    sendmsg[3] = 0;
                                                    g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toAPP_sem);
                                                }
                                            }
                                        }
                                        else
                                        {
                                            printf("2222222222222\n");
                                            switch (u16RecvCmd)
                                            {
                                                case ROOM_HOLDON:
                                                    printf("3333333333333333\n");
                                                    memset(sendmsg, 0x0, 16);
                                                    sendmsg[0] = MSG_NET_TALK_CALLED_HOLDON;
                                                    sendmsg[1] = ROOM_HOLDON;
                                                    sendmsg[2] = CurServerIP;
                                                    sendmsg[3] = s32SocketCall;
                                                    g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toAPP_sem);
                                                    printf("Need wait hangup");
                                                    s32HoldFlag = 1;
                                                    goto try_wait_holdon;
                                                case ROOM_HANGUP:
                                                    printf("44444444444444\n");
                                                    memset(sendmsg, 0x0, 16);
                                                    sendmsg[0] = MSG_NET_TALK_CALLED_HANGUP;
                                                    sendmsg[1] = ROOM_HANGUP;
                                                    sendmsg[2] = CurServerIP;
                                                    sendmsg[3] = s32SocketCall;
                                                    g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toAPP_sem);
                                                    break;
                                                default:
                                                    break;
                                            }
                                        }
                                    }
                                    break;
                                }
                                case DOOR_IDLE:
                                {
                                    ;
                                    break;
                                }
                                case ROOM_BUSY:
                                {
                                    memset(sendmsg, 0x0, 16);
                                    sendmsg[0] = MSG_TYPE_RECV_ROOM_BUSY;
                                    sendmsg[1] = ROOM_BUSY;
                                    sendmsg[2] = CurServerIP;
                                    sendmsg[3] = s32SocketCall;
                                    g_toAPP_msg_queue.send_message(MODULE_MSG,(void*)sendmsg,sizeof(sendmsg),&g_toAPP_sem);
                                    break;
                                }
                                default:
                                    break;
                            }
                        }

                    }
                    SocketDebug(408);
                    break;
                }
                default:
                    break;
            }
        }
    }

    return NULL;
}

void * ST_Socket_Answer_Call_Task(void* args)
{
    MI_S32 s32Ret = -1;
    Msg* pMsg = NULL;
    unsigned long recvmsg[4];

    SocketDebug(397);

    while (g_AnswerCallRun)
    {
        memset(recvmsg,0,sizeof(recvmsg));
        sem_wait(&g_SocketAnswerCall_Sem);
        SocketDebug(411);
        pMsg = g_SocketAnswerCall_Queue.get_message();
        if(pMsg)
        {
            SocketDebug(412);
            s32Ret = cmd_parse_msg(pMsg, recvmsg);
            if(s32Ret < 0)
                break;
            if(s32Ret == 0)
                continue;
            switch (recvmsg[0])
            {
                case MSG_TYPE_SOCKET_ANSWER_REMOTE_CMD:
                {
                    ST_DBG("MSG_TYPE_SOCKET_ANSWER_REMOTE_CMD..cmd(0x%x).\n", recvmsg[2]);
                    MI_S32 s32TmpSocket = SOCKET_INVALID;
                    s32TmpSocket = GetSocketByIp(recvmsg[1]);
                    SocketDebug(413);
                    if (SOCKET_INVALID != s32TmpSocket)
                    {
                        ST_Socket_SendCmdPack(s32TmpSocket, recvmsg[2]);
                        if ((ROOM_STOPMONT == recvmsg[2]) || (ROOM_HANGUP == recvmsg[2]))
                        {
                            //closesocket(s32TmpSocket);
                            //usleep(100*1000);
                            //RemoveSocketByIp(recvmsg[1]);
                        }
                    }
                }
                default:
                    break;
            }
        }
    }

    return NULL;
}

void * ST_Socket_TcpRecv_Task(void* args)
{
    MI_S32 s32Ret = -1;
    Msg* pMsg = NULL;
    unsigned long recvmsg[4];

    while (g_TcpRecvRun)
    {
        usleep(10*1000);
        ST_Socket_ProcessRecv();
    }

    return NULL;
}

//tcp socket transfer cmd
MI_S32 ST_Socket_CmdProcessInit()
{
    MI_S32 i;
    for (i = 0; i < SOCKET_MAX; i++)
    {
        g_stSocketInfo[i].s32Socket = -1;
        g_stSocketInfo[i].u32Ipaddr = 0xFFFFFFFF;
        g_stSocketInfo[i].u64SocketStartTime = 0;
    }
    g_ProcessRun = TRUE;
    g_ListenRun = TRUE;
    g_CallRun = TRUE;
    g_AnswerCallRun = TRUE;
    g_TcpRecvRun = TRUE;
    pthread_create(&tid_SocketListen, NULL, ST_Socket_Listen_Task, NULL); //Create task to listen socket port
    pthread_create(&tid_SocketProcess, NULL, ST_Socket_MainProcess, NULL); //Create task to listen socket port
    pthread_create(&tid_SocketCall, NULL, ST_Socket_Local_Call_Task, NULL); //Create task to listen socket port
    pthread_create(&tid_SocketAnswerCall, NULL, ST_Socket_Answer_Call_Task, NULL); //Create task to listen socket port
    pthread_create(&tid_SocketTcpRecv, NULL, ST_Socket_TcpRecv_Task, NULL); //Create task to listen socket port

    return MI_SUCCESS;
}

MI_S32 ST_Socket_CmdProcessDeInit()
{
    MI_S32 i;
    for (i = 0; i < SOCKET_MAX; i++)
    {
        g_stSocketInfo[i].s32Socket = -1;
        g_stSocketInfo[i].u32Ipaddr = 0xFFFFFFFF;
        g_stSocketInfo[i].u64SocketStartTime = 0;
    }
    g_ProcessRun = FALSE;
    g_ListenRun = FALSE;
    g_CallRun = FALSE;
    g_AnswerCallRun = FALSE;
    g_TcpRecvRun = FALSE;
    pthread_join(tid_SocketListen, NULL);
    pthread_join(tid_SocketProcess, NULL);
    pthread_join(tid_SocketCall, NULL);
    pthread_join(tid_SocketAnswerCall, NULL);
    pthread_join(tid_SocketTcpRecv, NULL);

    return MI_SUCCESS;
}

MI_S32 ST_CreateVideoRecvSocket()
{
    struct sockaddr_in s_addr;
    MI_S32 s32Len = 0, s32CfgBufLen;
    MI_S32 s32Socket, s32Ret;
    MI_S32 s32Reuse = TRUE;

    MI_S32 s32Ttl;

    if ((s32Socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        ST_ERR("Create video recv socket fail !!!\r");
        return -1;
    }
    else
    {
        ST_DBG("Create video recv socket(%d) success.\n", s32Socket);
    }
    memset(&s_addr, 0, sizeof(struct sockaddr_in));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(RECV_VIDEO_PORT);
    s_addr.sin_addr.s_addr = INADDR_ANY;

    s32CfgBufLen = 128*1024;
    setsockopt(s32Socket, SOL_SOCKET, SO_SNDBUF, (char*)&s32CfgBufLen, sizeof((char*)&s32CfgBufLen));
    setsockopt(s32Socket, SOL_SOCKET, SO_RCVBUF, (char*)&s32CfgBufLen, sizeof((char*)&s32CfgBufLen));
    setsockopt(s32Socket, SOL_SOCKET, SO_REUSEADDR, &s32Reuse, sizeof(s32Reuse));

    s32Ttl = 5;
    s32Ret = setsockopt(s32Socket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&s32Ttl, sizeof(s32Ttl));

    if ((bind(s32Socket, (struct sockaddr *) &s_addr, sizeof(s_addr))) == -1)
    {
        ST_ERR("bind error m_Socket = %d error =%x\n", s32Socket, errno);
        return -1;
    }
    else
    {
        ST_DBG("bind address to video recv socket(%d).\n\r", s32Socket);
    }

    return s32Socket;
}

MI_S32 ST_Socket_UdpSend(MI_S32 s32Socket, unsigned long DstIP, MI_S32 s32DstPort,
    MI_U8 *u8SlicePack, MI_S32 s32SlicePackLen)
{
    struct sockaddr_in DstSockaddr;
    MI_S32 s32rSendLen;

    if (s32Socket < 0)
    {
        ST_ERR("s32Socket(%d) is invalid!!!\n", s32Socket);
        return -1;
    }
    SocketDebug(508);

    DstSockaddr.sin_family = AF_INET;
    DstSockaddr.sin_port = htons(s32DstPort);
    DstSockaddr.sin_addr.s_addr = DstIP;//inet_addr("172.19.24.171"); //forexample ip= 0x0101A8C0;

    if ((0x0 == DstIP) || (0xFFFFFFFF == DstIP))
    {
        ST_ERR("Dst ip is invalid!!!\n");
        return -1;
    }
    SocketDebug(509);
    //printf("socket(%d)...ip(0x%x)...port(%d)\n", s32Socket, DstIP, s32DstPort);
    s32rSendLen = sendto(s32Socket, u8SlicePack, s32SlicePackLen, 0, (struct sockaddr*)&DstSockaddr, sizeof(struct sockaddr));

    return s32rSendLen;
}

MI_S32 ST_Socket_PackNalu_UdpSend(MI_S32 s32UdpSocket, MI_U8 *NaluBuf, MI_S32 s32BufLen,
    unsigned long DstIP, MI_S32 s32DstPort)
{
    static MI_U32 u32NowTimeStamp = 3000;
    static MI_S32 s32FrameNo = 0;
    MI_S32 s32SlicePackNum;
    MI_U8 *pu8TempHeader;
    MI_S32 j, s32SendRet = 0, s32SliceLen = 0;
    RTP_header video_rtp_header;
    //RTP_header *pvideo_rtp_header = (RTP_header *)&video_rtp_header;
    video_rtp_header.timestamp = ((u32NowTimeStamp & 0x000000FF) << 24) + ((u32NowTimeStamp & 0x0000FF00)<< 8) +((u32NowTimeStamp  & 0x00FF0000) >> 8) + ((u32NowTimeStamp & 0xFF000000) >> 24);
    u32NowTimeStamp += 3000;
    SocketDebug(505);
    if ((s32BufLen % UDP_MAX_PACKSIZE) == 0)
        s32SlicePackNum = s32BufLen/UDP_MAX_PACKSIZE;
    else
        s32SlicePackNum = s32BufLen/UDP_MAX_PACKSIZE+1;
    //DstIP = inet_addr("172.19.24.221");

    for (j = 1; j <= s32SlicePackNum; j++)
    {
        s32FrameNo++;
        video_rtp_header.seq_no = ((s32FrameNo & 0xff00) >> 8) + ((s32FrameNo & 0x00ff) << 8);

        if (j == s32SlicePackNum)
        {
            pu8TempHeader = (MI_U8 *)&video_rtp_header;
            *pu8TempHeader = 0x80;
            pu8TempHeader++;
            *pu8TempHeader = 0xe6; //payload 102 H264 + Last slice
            memset(u8TmpSendBuf, 0x0, 1600);
            memcpy(u8TmpSendBuf, &video_rtp_header, RTP_HEAD_SIZE);
            memcpy(u8TmpSendBuf + RTP_HEAD_SIZE, NaluBuf + (j-1)*UDP_MAX_PACKSIZE, (s32BufLen-(j-1)*UDP_MAX_PACKSIZE));
            s32SliceLen = RTP_HEAD_SIZE + (s32BufLen-(j-1)*UDP_MAX_PACKSIZE);
            s32SendRet = ST_Socket_UdpSend(s32UdpSocket, DstIP, RECV_VIDEO_PORT, u8TmpSendBuf, s32SliceLen);
            if (s32SendRet != s32SliceLen)
            {
                ST_ERR("Send video slice fail (%d) != (%d)...\n", s32SliceLen, s32SendRet);
            }
            SocketDebug(506);
        }
        else
        {
            pu8TempHeader = (MI_U8 *)&video_rtp_header;
            *pu8TempHeader = 0x80;
            pu8TempHeader++;
            *pu8TempHeader = 0x66; //102 H264 + Not last slice
            memcpy(u8TmpSendBuf, &video_rtp_header, RTP_HEAD_SIZE);
            memcpy(u8TmpSendBuf +RTP_HEAD_SIZE, NaluBuf + (j-1)*UDP_MAX_PACKSIZE, UDP_MAX_PACKSIZE);
            s32SliceLen = RTP_HEAD_SIZE + UDP_MAX_PACKSIZE;
            s32SendRet = ST_Socket_UdpSend(s32UdpSocket, DstIP, RECV_VIDEO_PORT, u8TmpSendBuf, s32SliceLen);
            if (s32SendRet != s32SliceLen)
            {
                ST_ERR("Send video slice fail (%d) != (%d)...\n", s32SliceLen, s32SendRet);
            }
            SocketDebug(507);
        }
    }

    return MI_SUCCESS;
}

MI_S32 ST_CreateSendVideoSocket()
{
    MI_S32 s32Socket, s32CfgBufLen;
    MI_S32 s32Reuse, s32Ttl, s32Ret;
    struct sockaddr_in local_addr;
    s32Socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (s32Socket<0)
    {
        ST_ERR("socket error: ");
        return -1;
    }
    else
    {
        ST_DBG("Create send video socket(%d) success \n", s32Socket);
    }
    SocketDebug(500);
    #if 0
    memset(&local_addr, 0, sizeof(struct sockaddr_in));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(SEND_VIDEO_PORT);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    s32CfgBufLen = 128 * 1024;
    setsockopt(s32Socket, SOL_SOCKET, SO_SNDBUF, (char*)&s32CfgBufLen, sizeof((char*)&s32CfgBufLen));
    setsockopt(s32Socket, SOL_SOCKET, SO_RCVBUF, (char*)&s32CfgBufLen, sizeof((char*)&s32CfgBufLen));
    SocketDebug(501);

    s32Reuse = TRUE;
    s32Ret = setsockopt(s32Socket, SOL_SOCKET, SO_REUSEADDR, &s32Reuse, sizeof(s32Reuse));
    s32Ttl = 5;
    s32Ret = setsockopt(s32Socket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&s32Ttl, sizeof(s32Ttl));
    SocketDebug(502);
#endif
    return s32Socket;
}


MI_S32 ST_CreateAudioRecvSocket()
{
    struct sockaddr_in s_addr;
    MI_S32 s32Len = 0, s32CfgBufLen;
    MI_S32 s32Socket, s32Ret;
    MI_S32 s32Reuse = TRUE;

    MI_S32 s32Ttl;

    if ((s32Socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
    {
        ST_ERR("Create audio recv socket fail !!!\r");
        return -1;
    }
    else
    {
        ST_DBG("Create audio recv socket(%d) success.\n", s32Socket);
    }
    memset(&s_addr, 0, sizeof(struct sockaddr_in));
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(RECV_AUDIO_PORT);
    s_addr.sin_addr.s_addr = INADDR_ANY;

    s32CfgBufLen = 128*1024;
    setsockopt(s32Socket, SOL_SOCKET, SO_SNDBUF, (char*)&s32CfgBufLen, sizeof((char*)&s32CfgBufLen));
    setsockopt(s32Socket, SOL_SOCKET, SO_RCVBUF, (char*)&s32CfgBufLen, sizeof((char*)&s32CfgBufLen));
    setsockopt(s32Socket, SOL_SOCKET, SO_REUSEADDR, &s32Reuse, sizeof(s32Reuse));

    s32Ttl = 5;
    s32Ret = setsockopt(s32Socket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&s32Ttl, sizeof(s32Ttl));
    if ((bind(s32Socket, (struct sockaddr *) &s_addr, sizeof(s_addr))) == -1)
    {
        ST_ERR("bind error m_Socket = %d error =%x\n", s32Socket, errno);
        return -1;
    }
    else
    {
        ST_DBG("bind address to audio recv socket(%d).\n\r", s32Socket);
    }

    return s32Socket;
}

MI_S32 ST_CreateSendAudioSocket()
{
    MI_S32 s32Socket, s32CfgBufLen;
    MI_S32 s32Reuse, s32Ttl, s32Ret;
    s32Socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (s32Socket<0)
    {
        ST_ERR("socket error: ");
        return -1;
    }
    else
    {
        ST_DBG("Create send audio socket(%d) success \n", s32Socket);
    }
    SocketDebug(500);

    s32CfgBufLen = 128 * 1024;
    setsockopt(s32Socket, SOL_SOCKET, SO_SNDBUF, (char*)&s32CfgBufLen, sizeof((char*)&s32CfgBufLen));
    setsockopt(s32Socket, SOL_SOCKET, SO_RCVBUF, (char*)&s32CfgBufLen, sizeof((char*)&s32CfgBufLen));
    SocketDebug(501);

    s32Reuse = TRUE;
    s32Ret = setsockopt(s32Socket, SOL_SOCKET, SO_REUSEADDR, &s32Reuse, sizeof(s32Reuse));
    s32Ttl = 5;
    s32Ret = setsockopt(s32Socket, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&s32Ttl, sizeof(s32Ttl));
    SocketDebug(502);

    return s32Socket;
}

