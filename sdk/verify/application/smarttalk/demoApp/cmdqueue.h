#ifndef __CMD_QUEUE_H__
#define __CMD_QUEUE_H__ 

#include <stdio.h>
#include <pthread.h>
#include <malloc.h>
#include <semaphore.h>
#include <string.h>

#define MODULE_MSG      0x01
#define MODULE_EXIT     0x02

//toUI
#define MSG_TYPE_SMARTMIC_START 0x100
#define MSG_TYPE_SMARTMIC_STOP 0x101
#define MSG_TYPE_SMARTMIC_MACTCH 0x102

#define MSG_TYPE_FACEREGISTER_CTRL  0x103
#define MSG_TYPE_DISP_DETECT_FACE   0x104
#define MSG_TYPE_REMOTE_CALL_ME     0x105
#define MSG_UI_CALLER_HANGUP        0x106
#define MSG_UI_CALLED_HANGUP        0x107
#define MSG_UI_CREATE_MONITOR_WIN   0x108


//toAPP
#define MSG_TYPE_CONFIRM_REGSITER_FACE  0x200
#define MSG_TYPE_START_SEND_VIDEO       0x201
#define MSG_TYPE_STOP_SEND_VIDEO        0x202
#define MSG_TYPE_CREATE_XMLCFG          0x203
#define MSG_TYPE_PRASE_XMLCFG           0x204

#define MSG_TYPE_LOCAL_VIDEO_DISP       0x205
#define MSG_TYPE_LOCAL_START_MONITOR    0x206
#define MSG_TYPE_LOCAL_STOP_MONITOR     0x207

#define MSG_TYPE_RECV_MONITOR_CMD       0x208 //msg[1] == cmd  msg[2] == Start/Stop msg[3] == RemoteIPaddr
#define MSG_TYPE_RECV_IDLE_CMD          0x209 //msg[1] == cmd  msg[2] == ServerIp reserve msg[3] == sendsocket

#define MSG_TYPE_LOCAL_CALL_ROOM        0x20A //msg[1] == roomid msg[2]=msg[3]=reserve
#define MSG_TYPE_RECV_REMOTE_CALL_ROOM  0x20B //msg[1] == cmd  msg[2] == reserve msg[3] == RemoteIPaddr
#define MSG_TYPE_RECV_ROOM_BUSY         0x20C 

#define MSG_TYPE_TALK_CALLED_HOLDON     0x20D
#define MSG_TYPE_TALK_CALLED_HANGUP     0x20E
#define MSG_TYPE_TALK_CALLER_HANGUP     0x20F

#define MSG_NET_RECV_ROOM_MONTACK_CMD   0x212 //msg[1] == cmd  msg[2] == ServerIp reserve msg[3] == sendsocket

#define MSG_NET_TALK_CALLED_HOLDON      0x213
#define MSG_NET_TALK_CALLED_HANGUP      0x214
#define MSG_NET_TALK_CALLER_HANGUP      0x215
#define MSG_TIMEOUT_NO_HOLDON_HANGUP    0x216 //not recv remote holdon
#define MSG_TIMEOUT_HOLDON_HANGUP       0x217 //talking timeout

//SocketProcess
#define MSG_TYPE_SOCKET_RECV_CONNECT 0x300
#define MSG_TYPE_SOCKET_SEND_PACKET 0x301
#define MSG_TYPE_SOCKET_REMOTE_HANGUP 0x302
#define MSG_TYPE_SOCKET_TIMEOUT_HANGUP 0x303
#define MSG_TYPE_SOCKET_RECV_NORMAL_PACK 0x304
#define MSG_TYPE_SOCKET_RECV_EXT_PACK 0x305

//Local call
#define MSG_TYPE_SOCKET_CONNECT_REMOTE 0x400
#define MSG_TYPE_SOCKET_SEND_LOCAL_CALL_CMD 0x401
#define MSG_TYPE_SOCKET_ANSWER_REMOTE_CMD 0x402


#ifdef  __cplusplus
extern "C"
{
#endif

class Msg {
public:
        Msg(unsigned int value, const void *msg = NULL, unsigned int msg_len = 0,
                unsigned int param = 0);
        Msg(unsigned int value, unsigned int param);
        ~Msg(void);
        int init(unsigned int value, const void *msg = NULL, unsigned int msg_len = 0,
                unsigned int param = 0);
        const void *get_message(unsigned int &len);
        void free_message();
        Msg *get_next(void) { return m_next; };
        void set_next (Msg *next) { m_next = next; };
        unsigned int get_value(void) { return m_value;};
        int has_param(void) { return m_has_param; };
        unsigned int get_param (void) { return m_param; };
private:
        Msg *m_next;
        unsigned int m_value;
        int m_has_param;
        unsigned int m_param;
        unsigned int m_msg_len;
        const void *m_msg;
};

class MsgQueue {
public:
        MsgQueue(void);
        ~MsgQueue(void);
        int send_message(unsigned int msgval,
                const void *msg = NULL,
                unsigned int msg_len = 0,
                sem_t *sem = NULL,
                unsigned int param = 0);
        Msg *get_message(void);
        void release();
private:
        int send_message(Msg *msg, sem_t *sem);
        Msg *m_msg_queue;
        pthread_mutex_t m_cmdq_mutex;
};

int cmd_parse_msg(Msg* pMsg, unsigned long* RMsg);
#ifdef  __cplusplus
}
#endif

#endif //__CMD_QUEUE_H__
