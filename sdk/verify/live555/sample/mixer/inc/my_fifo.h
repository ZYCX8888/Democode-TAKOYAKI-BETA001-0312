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
#ifndef _MY_FIFO_H_
#define _MY_FIFO_H_


#include <pthread.h>

struct _fifo_{
    volatile unsigned int in;
    volatile unsigned int out;
    unsigned int size;
    unsigned char * buffer;
    pthread_mutex_t flock;
};

struct _fifo_ * fifo_alloc(unsigned int count);
int fifo_release(struct _fifo_ * tmp);
unsigned int fifo_put(struct _fifo_ *pfifo, \
                    const unsigned char *buffer, \
                    unsigned int len);
unsigned int fifo_get(struct _fifo_ *pfifo, \
                    unsigned char* buffer, \
                    unsigned int  len);
#endif

