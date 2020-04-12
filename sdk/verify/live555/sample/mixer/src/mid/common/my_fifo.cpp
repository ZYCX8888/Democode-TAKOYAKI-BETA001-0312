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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "my_fifo.h"

struct _fifo_  streamManager;

int get_fifo_space(struct _fifo_ *ptr)
{
    assert(ptr);
    return (ptr->in - ptr->out);
}

static struct _fifo_ * fifo_init(unsigned char *buffer, \
        unsigned int size)
{
    struct _fifo_ *pfifo;

    if(size<=0 || (size & (size-1)) !=0x0 )
    {
        printf("size is not 2 aligned\n");
        return NULL;
    }
    pfifo = (struct _fifo_ *)malloc(sizeof(struct _fifo_));
    if(NULL == pfifo)
    {
        printf("can not malloc pfifo\n");
        return NULL;
    }

    pfifo->in = pfifo->out = 0x0;
    pfifo->size = size;
    pfifo->buffer = buffer;
    pthread_mutex_init(&pfifo->flock, NULL);

    return pfifo;
}

static unsigned int _fifo_put(struct _fifo_ *pfifo,\
        const unsigned char *buffer,\
        unsigned int len)
{
    unsigned int l;

    len = len < (pfifo->size - pfifo->in + pfifo->out) ? len :\
          (pfifo->size - pfifo->in + pfifo->out);

    l = len < (pfifo->size - (pfifo->in & (pfifo->size - 1))) ?\
        len : (pfifo->size - (pfifo->in & (pfifo->size -1)));

    memcpy(pfifo->buffer + (pfifo->in & (pfifo->size - 1)), \
            buffer,
            l);

    memcpy(pfifo->buffer, \
            buffer+ l , \
            (len-l));

    pfifo->in += len;

    return len;
}

static unsigned int _fifo_get(struct _fifo_ *pfifo,\
    unsigned char *buffer, \
    unsigned int len)
{
    unsigned int l;

    len = len < (pfifo->in - pfifo->out) ? \
        len : (pfifo->in - pfifo->out);
    l = len < (pfifo->size - (pfifo->out & (pfifo->size-1))) ?\
        len : (pfifo->size - (pfifo->out & (pfifo->size -1)));

    memcpy(buffer, \
        pfifo->buffer + (pfifo->out & (pfifo->size -1)), \
        l);

    memcpy(buffer + l, \
            pfifo->buffer,\
            (len - l));
    pfifo->out += len;

    return len;
}

struct _fifo_ * fifo_alloc(unsigned int count)
{
    unsigned char * buffer=NULL;
    struct _fifo_ *ptr=NULL;
    if(!count || ((count & (count-1)) != 0x0))
    {
        printf("count is not 2 aligned\n");
        return NULL;
    }
    buffer = (unsigned char*)malloc(count);
    if(NULL == buffer)
    {
        printf("can not malloc buffer\n");
        return NULL;
    }
    ptr = fifo_init(buffer, count);
    if(NULL == ptr)
    {
        free(buffer);
        return NULL;
    }

    return ptr;
}

int fifo_release(struct _fifo_ * tmp)
{

    if(NULL == tmp || NULL == tmp->buffer)
    {
        printf("in %s, can not release buf\n", __FUNCTION__);
        return -1;
    }

    free(tmp->buffer);
    return 0;
}

unsigned int fifo_put(struct _fifo_ *pfifo, \
                    const unsigned char *buffer, \
                    unsigned int len)
{
    unsigned int ret = 0x0;
    if(NULL == pfifo || NULL == buffer)
        return 0;

    pthread_mutex_lock(&pfifo->flock);
    ret = _fifo_put(pfifo, buffer, len);
    pthread_mutex_unlock(&pfifo->flock);
    return ret;
}

unsigned int fifo_get(struct _fifo_ *pfifo, \
                    unsigned char* buffer, \
                    unsigned int  len)
{
    unsigned int ret = 0x0;
    if(NULL == pfifo || NULL == buffer)
        return 0;

    pthread_mutex_lock(&pfifo->flock);
    ret = _fifo_get(pfifo, buffer, len);
    if(pfifo->in == pfifo->out)
        pfifo->in = pfifo->out = 0x0;
    pthread_mutex_unlock(&pfifo->flock);
    return ret;
}

