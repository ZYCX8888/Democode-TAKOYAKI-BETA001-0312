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
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <sys/mman.h>
#include <string.h>

#include "st_usb_bi_dev.h"

#define DEVICE_NODE "/dev/usb_bi"

typedef struct Usb_Bi_Dev {
    int fd;
    int inited;
    mem_t *mem_in;
    mem_t *mem_out;
} Usb_Bi_Dev_t;


static Usb_Bi_Dev_t *dev;

static int Usb_Bi_Open()
{
    int fd;

    fd  = open(DEVICE_NODE, O_RDWR | O_NONBLOCK);
    if(fd == -1)
        return -EINVAL;

    return fd;
}

static void Usb_Bi_Close(int fd)
{
    if(!fd)
        return;

    close(fd);
}

static int Usb_Bi_RequestBuf(int fd, mem_t **mem,
                buf_type_e type, unsigned int count, unsigned int length, int flags)
{
    requeset_buf_t rb;
    mem_t *tmp;
    int i, ret;

    if(!fd || (flags && *mem))
        return -EINVAL;

    rb.type = type;
    rb.length = length;
    rb.count = flags ? count:0;
    ret = ioctl(fd, USB_NERVE_REQBUFS, &rb);
    if(ret)
        return ret;

    if(flags == 1) //reqbuf
    {
        buffer_t buf;
        void *addr;

        if(!count)
            return -EINVAL;

        tmp = malloc(sizeof(mem_t) + sizeof(Usb_Bi_Buf_t) * count);
        memset(tmp, 0x00, sizeof(mem_t) + sizeof(Usb_Bi_Buf_t) * count);
        *mem = tmp;
        tmp->size = count;

        for(i = 0; i < tmp->size; i++)
        {
            buf.index = i;
            buf.type = type;
            ret = ioctl(fd, USB_NERVE_QUERYBUF, &buf);
            if(ret)
                goto free_mem;

            addr = mmap(NULL,
                        buf.length,
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, buf.mem_offset);
            if(addr == (void*)-1){
                ret = (int)addr;
                goto free_mem;
             }

            tmp->b[i].buf = buf;
            tmp->b[i].start = addr;
            printf("index %d start 0x%x\n", tmp->b[i].buf.index, (unsigned int)tmp->b[i].start);
        }
    } else if(flags == 0){ //dereqbuf
        tmp = *mem;

        if(!tmp->size)
            return -EINVAL;

        for(i = 0; i < tmp->size; i++)
        {
            if(tmp->b[i].start)
                munmap(tmp->b[i].start, tmp->b[i].buf.length);
        }
        free(tmp);
    }

    return 0;

free_mem:
    Usb_Bi_RequestBuf(fd, mem, type, 0, 0, 0);
    return ret;
}

static int Usb_Bi_DequeueBuf(int fd)
{
    buffer_t buf;

    if(!fd)
        return -EINVAL;

    if(0 == ioctl(fd, USB_NERVE_DEQUEUE_BUF, &buf))
        return buf.index;

    return -EINVAL;
}

static int Usb_Bi_QueueBuf(int fd, int index, unsigned int size)
{
    buffer_t buf;

    if(!fd)
        return -EINVAL;

    buf.index = index;
    buf.bytesused = size;

    return ioctl(fd, USB_NERVE_QUEUE_BUF, &buf);
}

static int Usb_Bi_DequeueEvent(int fd, mem_t *mem)
{
    buffer_t buf;

    if(!fd || !mem)
        return -EINVAL;

    if(0 == ioctl(fd, USB_NERVE_DEQUEUE_EVENT, &buf)){
        if(mem->size < buf.index + 1)
            return -EINVAL;
        mem->b[buf.index].buf = buf;
        return buf.index;
    }
    return -EINVAL;
}

static int Usb_Bi_FinishEvent(int fd, int index)
{
    buffer_t buf;

    if(!fd)
        return -EINVAL;

    buf.index = index;
    if(0 == ioctl(fd, USB_NERVE_FINISH_EVENT, &buf))
        return buf.index;

    return -EINVAL;
}

int ST_Usb_Bi_Init()
{
    int ret;

    if(dev)
        return -EINVAL;

    dev = malloc(sizeof(*dev));
    if(!dev)
        perror("usb\n");
    memset(dev, 0x00, sizeof(*dev));

    dev->fd = Usb_Bi_Open();
    if(dev->fd < 0)
        goto free_dev;

    ret = Usb_Bi_RequestBuf(dev->fd, &dev->mem_in, TRANSFER_IN, 3, 40 * 1024, 1);
    if(ret)
        goto free_dev;

    ret = Usb_Bi_RequestBuf(dev->fd, &dev->mem_out, TRANSFER_OUT, 3, 400 * 1024, 1);
    if(ret)
        goto derequest;

    dev->inited = 1;
    return 0;

derequest:
    Usb_Bi_RequestBuf(dev->fd, &dev->mem_in, TRANSFER_IN, 0, 0, 0);
free_dev:
    free(dev);
    return -EINVAL;
}

int ST_Usb_Bi_Deinit()
{
    if(!dev || !dev->inited)
        return -EINVAL;

    Usb_Bi_RequestBuf(dev->fd, &dev->mem_in, TRANSFER_IN, 0, 0, 0);
    Usb_Bi_RequestBuf(dev->fd, &dev->mem_out, TRANSFER_OUT, 0, 0, 0);
    Usb_Bi_Close(dev->fd);
    free(dev);

    dev = NULL;
    return 0;
}

int ST_Usb_Bi_SendData(char *data, unsigned int size)
{
    unsigned int sentSize = 0, bytesused;
    int index;
    Usb_Bi_Buf_t *buf;

    if(!dev || !dev->inited)
        return -EINVAL;

    while(1) {
        index = Usb_Bi_DequeueBuf(dev->fd);
        if(index < 0) {
            usleep(1000);
            continue;
        }

        buf = &dev->mem_in->b[index];
        if(size - sentSize > buf->buf.length)
            bytesused = buf->buf.length;
        else
            bytesused = size - sentSize;

        memcpy(buf->start, data + sentSize, bytesused);
        if(Usb_Bi_QueueBuf(dev->fd, index, bytesused) < 0) {
            printf("%s Qbuf Failed\n", __func__);
            break;
        }
        sentSize += bytesused;

        if(sentSize == size)
            break;
    }
    return sentSize;
}

int ST_Usb_Bi_RecvData(char *data, unsigned int size)
{
    int ret, index;
    unsigned int RecvSize = 0;
    struct pollfd fds;
    Usb_Bi_Buf_t *buf;

    if(!dev || !dev->inited)
        return -EINVAL;

    while(1) {
        fds.fd = dev->fd;
        fds.events = POLLIN;

        ret = poll(&fds, 1 , 10 * 1000);
        if(ret == 0) {
            printf("poll timeout\n");
            continue;
        }

        index = Usb_Bi_DequeueEvent(dev->fd, dev->mem_out);
        if(index < 0)
            break;

        buf = &dev->mem_out->b[index];
        if(buf->buf.bytesused > size - RecvSize) {
            printf("%s OverFlow!!! bytesused %u rev %u \n",
                        __func__, buf->buf.bytesused, size - RecvSize);
            break;
        }
        memcpy(data + RecvSize, buf->start, buf->buf.bytesused);
        Usb_Bi_FinishEvent(dev->fd, index);
        RecvSize += buf->buf.bytesused;

        if(RecvSize == size)
            break;
    }
    return RecvSize;
}
