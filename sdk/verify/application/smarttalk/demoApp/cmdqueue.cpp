#include "cmdqueue.h"
#include <pthread.h>

Msg::Msg (unsigned int value, const void *msg, unsigned int msg_len, unsigned int param)
{
    m_value = value;
    m_msg_len = 0;
    m_has_param = (param != 0);
    m_param = param;
    m_next = NULL;

    if (msg_len) {
        void *temp = malloc(msg_len);
        if (temp) {
            memcpy(temp, msg, msg_len);
            m_msg_len = msg_len;
        }else{
            printf("malloc failed  %s %d\n",__FILE__,__LINE__);
            temp = NULL;
        }
        m_msg = temp;
    } else {
        m_msg = msg;
    }
}

int Msg::init(unsigned int value, const void *msg, unsigned int msg_len, unsigned int param){
    m_value = value;
    m_msg_len = 0;
    m_has_param = (param != 0);
    m_param = param;
    m_next = NULL;

    if (msg_len) {
        void *temp = malloc(msg_len);
        if (temp) {
            memcpy(temp, msg, msg_len);
            m_msg_len = msg_len;
        }else{
            m_msg = NULL;
            m_msg_len = 0;
            printf("malloc failed  %s %d\n",__FILE__,__LINE__);
            return 0;
        }
        m_msg = temp;
    } else {
        m_msg = msg;
    }
    return 1;      
}

Msg::Msg (unsigned int value, unsigned int param)
{
    m_value = value;
    m_msg_len = 0;
    m_has_param = 1;
    m_param = param;
    m_next = NULL;
}

Msg::~Msg (void) 
{
    if (m_msg_len) {
        if(m_msg != NULL)
            free((void *)m_msg);
        m_msg = NULL;
        m_msg_len = 0;
    }
}

void  Msg::free_message(){
    if (m_msg_len) {
        if(m_msg != NULL){
            free((void *)m_msg);
        }
        m_msg = NULL;
        m_msg_len = 0;
    }
}

const void *Msg::get_message (unsigned int &len)
{
    len = m_msg_len;
    return (m_msg);
}

MsgQueue::MsgQueue(void)
{
    m_msg_queue = NULL;
    pthread_mutex_init(&m_cmdq_mutex, NULL);
}

MsgQueue::~MsgQueue (void) 
{
    Msg *p;
    pthread_mutex_lock(&m_cmdq_mutex);
    while (m_msg_queue != NULL) {
        p = m_msg_queue->get_next();
        m_msg_queue = (Msg *)malloc(sizeof(Msg));
        m_msg_queue = p;
    }
    pthread_mutex_unlock(&m_cmdq_mutex);
    pthread_mutex_destroy(&m_cmdq_mutex);
}

int MsgQueue::send_message (unsigned int msgval, 
                                const void *msg, 
                                unsigned int msg_len, 
                                sem_t *sem,
                                unsigned int param)
{
    Msg *newmsg = (Msg *)malloc(sizeof(Msg));
    if (newmsg == NULL){ 
        printf("MsgQueue::malloc failed\n");
        return (-1);
    }
    if(newmsg->init(msgval, msg, msg_len, param) == 0){
        free(newmsg);
        newmsg = NULL;
        return (-1);
    }

    return (send_message(newmsg, sem));
}

int MsgQueue::send_message(Msg *newmsg, sem_t *sem)
{
    pthread_mutex_lock(&m_cmdq_mutex);
    if (m_msg_queue == NULL) {
        m_msg_queue = newmsg;
    } else {
        Msg *p = m_msg_queue;
        while (p->get_next() != NULL) p = p->get_next();
        p->set_next(newmsg);
    }
    pthread_mutex_unlock(&m_cmdq_mutex);
    if (sem != NULL) {
        sem_post(sem);
    }
    return (0);
}

Msg *MsgQueue::get_message (void) 
{
    Msg *ret;

    if (m_msg_queue == NULL) 
        return(NULL);

    pthread_mutex_lock(&m_cmdq_mutex);
    if (m_msg_queue == NULL) 
        ret = NULL;
    else {
        ret = m_msg_queue;
        m_msg_queue = ret->get_next();
    }
    pthread_mutex_unlock(&m_cmdq_mutex);
    if (ret) {
        ret->set_next(NULL);
    }
    return (ret);
}

void MsgQueue::release(){
 	Msg* pMsg = NULL;
	while(1){
		pMsg = get_message();
		if(pMsg){
			pMsg->free_message();
			free(pMsg);
			pMsg = NULL;
		}else
			break;
	}
}

int cmd_parse_msg(Msg* pMsg, unsigned long* RMsg){
    int ret = 0;
    if(pMsg == NULL)
        return 0;
    if(pMsg->get_value() == MODULE_EXIT){
        pMsg->free_message();
        free(pMsg);
        pMsg = NULL;
        ret = -1;
    }else if(pMsg->get_value() == MODULE_MSG){
        unsigned int msglen = 0;
        const void* pt_msg = NULL;
        pt_msg = pMsg->get_message(msglen);
        if(pt_msg && msglen > 0){
            memcpy(RMsg,pt_msg,sizeof(unsigned long)*4);
            ret = 1;
        }
        pMsg->free_message();
        free(pMsg);
        pMsg = NULL;
    }else{
        pMsg->free_message();
        free(pMsg);
        pMsg = NULL;
    }
    return ret;
}

