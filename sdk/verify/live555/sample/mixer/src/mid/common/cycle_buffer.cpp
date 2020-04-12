/*
* cycle_buffer.cpp- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: XXXX <XXXX@sigmastar.com.cn>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#include "mid_common.h"
#include "cycle_buffer.h"


extern int gDebug_VideoStreamLog;
#define INFO(...) \
  do { \
      livecap_printf("\n");  \
      livecap_printf(__VA_ARGS__); \
      livecap_printf("\n"); \
  } while(0)

#define livecap_printf(fmt, arg...) \
    do { \
        if (1)  printf(fmt, ##arg); \
    } while (0);

//#define DELAY_SEND_TIMESTAMP_MSEC 100
#define DELAY_SEND_TIMESTAMP_MSEC 0

//struct Buffer_Item_Header
//{
//    int length;
//    unsigned int timestamp;
//};


cycle_buffer::cycle_buffer(unsigned int size /*= BLOCK_SIZE * MAX_BLOCK_COUNT*/, int maxBufCount)
{
    m_cb.begin = new unsigned  char[size];
    m_cb.size = size;
    m_cb.end = m_cb.begin + m_cb.size - 1;
    m_cb.rpos = m_cb.begin;
    m_cb.wpos = m_cb.begin;
    m_cb.readflag = 0x00;
    m_recordFile = NULL;
    //先缓冲1M数据才开始让player读
    //不在进行缓冲的
    setbufferingsize(0);
    pthread_mutex_init(&m_bufmux, NULL);
    pthread_mutex_init(&m_bufCountmux, NULL);
    pthread_mutex_init(&m_bufReset, NULL);

    m_flag_frame_size_notok = true;
    m_bufCount = 0;
    memset(m_name, 0, sizeof(m_name));
    memset(m_recordEnable_key, 0, sizeof(m_recordEnable_key));
    memset(m_recordDevice_key, 0, sizeof(m_recordDevice_key));
    m_recording = false;

    m_inRecording = false;
    m_outRecording = false;
    m_inFile = NULL;
    m_outFile = NULL;

    if(maxBufCount < 0)
    {
        m_maxBufCount = 10000;
    }
    else
    {
        m_maxBufCount = (unsigned int)maxBufCount;
    }
}

cycle_buffer::~cycle_buffer()
{
    printf("Enter %s line: %d\n", __FUNCTION__, __LINE__);
    delete []m_cb.begin;
    printf("Enter %s line: %d\n", __FUNCTION__, __LINE__);
    m_cb.begin = NULL;
    m_cb.rpos = NULL;
    m_cb.wpos = NULL;
    pthread_mutex_destroy(&m_bufmux);
    pthread_mutex_destroy(&m_bufCountmux);
    pthread_mutex_destroy(&m_bufReset);

    if(m_inFile)
    {
        fclose(m_inFile);
        m_inFile = NULL;
    }

    if(m_outFile)
    {
        fclose(m_outFile);
        m_outFile = NULL;
    }
}

/*! \fn                     cycle_buffer::read(）
 *  \brief                  从cycle_buffer读取len长度的数据，len<cycle_buffer中的实际字节时返回len,否则返回实际读取字节数
 *  \pre                    cycle_buffer不能为空
 *  \post                   返回实际读取的字节数
 *  \return                 成功返回实际读取的字节数,失败返回0
 */
int cycle_buffer::read(unsigned char* buffer, int len)
{
    if(m_flag_buffering_data_ok != 1)
    {
        //没有缓冲满，无法进行读
        return 0;
    }


    unsigned int valid_data_size = 0;
    unsigned int read_size = len;


    /* 修改获取相应的space 的信息,主要的原因就是有一个是需要空出来的*/
    unsigned char* pRPos = NULL;
    unsigned char* pWPos = NULL;
    pthread_mutex_lock(&m_bufReset);
    valid_data_size = m_cb.size - get_free_space(&pRPos, &pWPos) - 1;

    //todo test
    //printf("\n valid_data_size:%d,\n",valid_data_size);

    if(valid_data_size < (unsigned int)len)
    {
        //INFO("valid_data_size = %d, len = %d\n", valid_data_size, len);
        pthread_mutex_unlock(&m_bufReset);
        return 0;
    }


    if((m_cb.end - pRPos + 1) < len)
    {
        /*出现翻转的情况*/
        read_size = m_cb.end - pRPos + 1;

        if(NULL != buffer)
        {
            memcpy(buffer, pRPos, read_size);
            memcpy(buffer + read_size, m_cb.begin, len - read_size);
        }
    }
    else if(NULL != buffer)
    {
        memcpy(buffer, pRPos, read_size);
    }


    int offset = ((pRPos - m_cb.begin) + len) % m_cb.size;
    m_cb.rpos = m_cb.begin + offset;
    pthread_mutex_unlock(&m_bufReset);

    return len;
}

/*! \fn                     cycle_buffer::write(）
 *  \brief                  向cycle_buffer写入len长度的数据，len<cycle_buffer中的实际空间返回len,否则返回实际写入字节数
 *  \pre                    cycle_buffer不能无空间
 *  \post                   返回实际写入的字节数，没空间写时sleep 500us，1s还无空间写时清空cycle_buffer
 *  \return                 成功返回实际读取的字节数,失败返回0
 */
int cycle_buffer::write(const unsigned char* buffer, int len)
{
    unsigned int free_space_size = 0;
    unsigned int writed_size = len;

    /* 修改获取相应的space 的信息*/
    unsigned char* pRPos = NULL;
    unsigned char* pWPos = NULL;
    free_space_size = get_free_space(&pRPos, &pWPos);

    if(free_space_size < (unsigned int)len)
    {
        printf("\n cycle_buffer::write:free_space_size < len free_space_size:%d,len:%d,\n", free_space_size, len);
        return 0;
    }

    if(m_flag_buffering_data_ok != 1)
    {
        //没有缓冲满，需要进行计数
        m_buffering_write_count += len;

        if(m_buffering_write_count > m_bufferingsize)
        {
            //已经缓冲满
            m_flag_buffering_data_ok = 1;
            //INFO( "\n circular_buffer_write buffering is ok,m_bufferingsize:%u,\n", m_buffering_write_count );
        }
    }


    if((m_cb.end - pWPos + 1) < len)
    {
        /*出现翻转的情况*/
        writed_size = m_cb.end - pWPos + 1;
        memcpy(pWPos, buffer, writed_size);
        memcpy(m_cb.begin, buffer + writed_size, len - writed_size);
    }
    else
    {
        memcpy(pWPos, buffer, writed_size);
    }


    int offset = ((pWPos - m_cb.begin) + len) % m_cb.size;
    m_cb.wpos = m_cb.begin + offset;


    return len;
}

void cycle_buffer::setbufferingsize(float bufferingsize)
{
    //bufferingsize单位为M
    //设置需要缓冲多少数据才能进行播放
    m_bufferingsize = bufferingsize * (1000 * 1000);
    //缓冲计数的标志
    m_buffering_write_count = 0;
    //是否缓冲满的标志，1表示ok，0表示nok
    m_flag_buffering_data_ok = 0;
}

void cycle_buffer::reset()
{
    pthread_mutex_lock(&m_bufReset);
    m_cb.rpos = m_cb.begin;
    m_cb.wpos = m_cb.begin;
    pthread_mutex_unlock(&m_bufReset);
	
    pthread_mutex_lock(&m_bufCountmux);
    m_bufCount = 0;
    pthread_mutex_unlock(&m_bufCountmux);
}

unsigned int cycle_buffer::get_valid_space()
{
    unsigned char* pRPos = NULL;
    unsigned char* pWPos = NULL;
    return (m_cb.size - get_free_space(&pRPos, &pWPos) - 1);
}

unsigned int cycle_buffer::get_free_space(unsigned char** pRPos, unsigned char** pWPos)
{
    //m_bufmux.lock();
    pthread_mutex_lock(&m_bufmux);
    int freespace = 0;

    if(m_cb.rpos == m_cb.wpos)
    {
        /* 重合，表示空的情况*/
        freespace = m_cb.size - 1;
    }
    else
    {
        /* m_cb.rpos，m_cb.wpos相差为一，表示满的情况*/
        freespace = ((m_cb.rpos + m_cb.size - m_cb.wpos) % m_cb.size) - 1;
    }

    /* m_cb.rpos 表示下一个读的位置*/
    /*m_cb.wpos 表示下一个写的位置需要空出一个写的位置*/

    *pRPos = m_cb.rpos;
    *pWPos = m_cb.wpos;
    //m_bufmux.unlock();
    pthread_mutex_unlock(&m_bufmux);

    return freespace;
}



unsigned int cycle_buffer:: getBufferCount()
{
    unsigned int count  = 0;
    pthread_mutex_lock(&m_bufCountmux);
    count = m_bufCount;
    pthread_mutex_unlock(&m_bufCountmux);

    return count;
}

unsigned int cycle_buffer:: getFreeBufferCount()
{
    unsigned int count  = 0;
    pthread_mutex_lock(&m_bufCountmux);
    count = m_maxBufCount - m_bufCount;
    pthread_mutex_unlock(&m_bufCountmux);

    return count;
}

void cycle_buffer::setBufferCount(unsigned int count)
{
    pthread_mutex_lock(&m_bufCountmux);
    m_bufCount = count;
    pthread_mutex_unlock(&m_bufCountmux);
}

void cycle_buffer::increaseCount()
{
    pthread_mutex_lock(&m_bufCountmux);
    m_bufCount++;
    pthread_mutex_unlock(&m_bufCountmux);
}

void cycle_buffer::decreaseCount()
{
    pthread_mutex_lock(&m_bufCountmux);

    if(m_bufCount > 0)
    {
        m_bufCount--;
    }

    pthread_mutex_unlock(&m_bufCountmux);
}



int cycle_buffer::getFrame(unsigned char *buffer, int len, struct timeval *pTimestamp)
{
    /* 修改获取相应的space 的信息*/
    unsigned char* pRPos = NULL;
    unsigned char* pWPos = NULL;
    unsigned int valid_data_size ;
    unsigned int read_size;
    valid_data_size = m_cb.size - get_free_space(&pRPos, &pWPos) - 1;
    unsigned int buffer_item_header_size = sizeof(Buffer_Item_Header);

    //printf("getFrame in");

    if(valid_data_size <= buffer_item_header_size)
    {
        return 0;
    }

    //1.先获取相应的header,已经进行判断的应该是可以读入的
    Buffer_Item_Header newbufferitem ;
    unsigned int buffersize = sizeof(Buffer_Item_Header);
    read((unsigned char*)&newbufferitem, buffersize);

    if(pTimestamp != NULL)
    {
        pTimestamp->tv_sec = newbufferitem.timestamp.tv_sec;
        pTimestamp->tv_usec = newbufferitem.timestamp.tv_usec;
    }

    read_size = newbufferitem.length;

    //printf("read_size = %d \n", newbufferitem.length);

    if(read_size > (unsigned int)len)
    {
        read(NULL, read_size);
    }
    else
    {
        read(buffer, read_size);
    }

    decreaseCount();


    //printf("getFrame out read_size  %d", read_size);
    return read_size;
}

int cycle_buffer::getFrame(unsigned char *buffer, int len, struct timeval *pTimestamp, int *frameType)
{
    /* 修改获取相应的space 的信息*/
    unsigned char* pRPos = NULL;
    unsigned char* pWPos = NULL;
    unsigned int valid_data_size ;
    unsigned int read_size;
    valid_data_size = m_cb.size - get_free_space(&pRPos, &pWPos) - 1;
    unsigned int buffer_item_header_size = sizeof(Buffer_Item_Header);

    //printf("getFrame in");

    if(valid_data_size <= buffer_item_header_size)
    {
        return 0;
    }

    //1.先获取相应的header,已经进行判断的应该是可以读入的
    Buffer_Item_Header newbufferitem ;
    unsigned int buffersize = sizeof(Buffer_Item_Header);
    read((unsigned char*)&newbufferitem, buffersize);

    if(pTimestamp != NULL)
    {
        pTimestamp->tv_sec = newbufferitem.timestamp.tv_sec;
        pTimestamp->tv_usec = newbufferitem.timestamp.tv_usec;
    }

    read_size = newbufferitem.length;

    if(frameType != NULL)
    {
        *frameType = newbufferitem.frameType;
    }

    //printf("read_size = %d \n", newbufferitem.length);

    if(read_size > (unsigned int)len)
    {
        if(gDebug_VideoStreamLog)
        {
            printf("%s: read_size > (unsigned int)len\n", __func__);
        }

        read(NULL, read_size);
    }
    else
    {
        read(buffer, read_size);
    }

    decreaseCount();

    if(m_outRecording)
    {
        if(NULL == m_outFile)
        {
            m_outFile = fopen(m_outFileName, "ab+");

            if(NULL != m_outFile)
            {
                printf("start to record on recordFile:%s\n", m_outFileName);
            }
            else
            {
                printf("failed to open recordFile %s\n", m_outFileName);
            }
        }

        if(m_outFile)
        {
            fwrite(buffer, 1, read_size, m_outFile);
        }
    }

    //printf("getFrame out read_size  %d", read_size);
    return read_size;
}
void cycle_buffer::setName(char* name)
{
    strncpy(m_name, name, 7);

    if(strlen(m_name) > 0)
    {
        sprintf(m_recordEnable_key, "mxr.r.%s", m_name);
        sprintf(m_recordDevice_key, "mxr.d.%s", m_name);
    }

}

int cycle_buffer::writeFrame(unsigned char *buf, int len, struct timeval *timestamp)
{
    if(buf == NULL || len <= 0)
    {
        return 0;
    }

    unsigned char* pRPos = NULL;
    unsigned char* pWPos = NULL;
    unsigned int free_space_size = 0;

    char recordEnable[2] = {0};

    if(mixer_property_get(m_recordEnable_key, recordEnable, NULL) && '1' == recordEnable[0])
    {
        if(!m_recording)
        {
            char recordFileName[128] = {0};

            if(mixer_property_get(m_recordDevice_key, recordFileName, NULL))
            {

                m_recordFile = fopen(recordFileName, "ab+");

                if(NULL != m_recordFile)
                {
                    m_recording = true;
                    printf("start to record %s on recordFile:%s\n", m_name, recordFileName);
                }
                else
                {
                    printf("failed to open recordFile %s\n", recordFileName);
                }

            }
            else
            {
                printf("failed to get recordDevice property for %s\n", m_recordDevice_key);
            }
        }
    }

    if(m_recording)
    {
        if((!mixer_property_get(m_recordEnable_key, recordEnable, NULL)) || ('0' == recordEnable[0]))
        {
            m_recording = false;

            if(NULL != m_recordFile)
            {
                printf("stop recording %s\n", m_name);
                fclose(m_recordFile);
                m_recordFile = NULL;
            }

        }
        else
        {
            fwrite(buf, 1, len, m_recordFile);
        }
    }

    if(getBufferCount() >= m_maxBufCount)
    {
        if(gDebug_VideoStreamLog)
        {
            printf("\nwriteFrame thrown length = %d, maxCount = %d", len, m_maxBufCount);
        }


    }

    free_space_size = get_free_space(&pRPos, &pWPos);

    //printf("writePacket: free_space_size = %d\n", free_space_size);

    Buffer_Item_Header pnewbufferitem;
    //pnewbufferitem.timestamp = timestamp;
    //gettimeofday(&pnewbufferitem.timestamp , 0);
    pnewbufferitem.timestamp.tv_sec = timestamp->tv_sec;
    pnewbufferitem.timestamp.tv_usec = timestamp->tv_usec;
    pnewbufferitem.length = len;
    //printf("write len = %d \n", pnewbufferitem.length);

    if(0 == pnewbufferitem.length || free_space_size < pnewbufferitem.length + sizeof(Buffer_Item_Header))
    {
        printf("\n%s fail, freespacesize:%d need size = %d", __func__, free_space_size,  pnewbufferitem.length + sizeof(Buffer_Item_Header));
        return -1;

    }

    write((unsigned char*)&pnewbufferitem, sizeof(Buffer_Item_Header));
    write(buf, len);

    increaseCount();

    return pnewbufferitem.length;
}

int cycle_buffer::writeFrame(unsigned char *buf, int len, struct timeval *timestamp, int frameType)
{
    if(buf == NULL || len <= 0)
    {
        return 0;
    }

    unsigned char* pRPos = NULL;
    unsigned char* pWPos = NULL;
    unsigned int free_space_size = 0;

    char recordEnable[2] = {0};

    if(mixer_property_get(m_recordEnable_key, recordEnable, NULL) && '1' == recordEnable[0])
    {
        if(!m_recording)
        {
            char recordFileName[128] = {0};

            if(mixer_property_get(m_recordDevice_key, recordFileName, NULL))
            {

                m_recordFile = fopen(recordFileName, "ab+");

                if(NULL != m_recordFile)
                {
                    m_recording = true;
                    printf("start to record %s on recordFile:%s\n", m_name, recordFileName);
                }
                else
                {
                    printf("failed to open recordFile %s\n", recordFileName);
                }

            }
            else
            {
                printf("failed to get recordDevice property for %s\n", m_recordDevice_key);
            }
        }
    }

    if(m_recording)
    {
        if((!mixer_property_get(m_recordEnable_key, recordEnable, NULL)) || ('0' == recordEnable[0]))
        {
            m_recording = false;

            if(NULL != m_recordFile)
            {
                printf("stop recording %s\n", m_name);
                fclose(m_recordFile);
                m_recordFile = NULL;
            }

        }
        else
        {
            fwrite(buf, 1, len, m_recordFile);
        }
    }

    if(getBufferCount() >= m_maxBufCount)
    {
        if(gDebug_VideoStreamLog)
        {
            printf("\n%s drop frame length = %d, over maxCount = %d", __func__, len, m_maxBufCount);
        }

    }

    free_space_size = get_free_space(&pRPos, &pWPos);

    //printf("writePacket: free_space_size = %d\n", free_space_size);

    Buffer_Item_Header pnewbufferitem;
    //pnewbufferitem.timestamp = timestamp;
    //gettimeofday(&pnewbufferitem.timestamp , 0);
    pnewbufferitem.timestamp.tv_sec = timestamp->tv_sec;
    pnewbufferitem.timestamp.tv_usec = timestamp->tv_usec;
    pnewbufferitem.length = len;
    pnewbufferitem.frameType = frameType;
    //printf("write len = %d \n", pnewbufferitem.length);

    if(0 == pnewbufferitem.length || free_space_size < pnewbufferitem.length + sizeof(Buffer_Item_Header))
    {
        if(gDebug_VideoStreamLog)
        {
            printf("%s fail, freespacesize:%d need size = %d cycle buf size:%d\n", __func__, free_space_size,  pnewbufferitem.length + sizeof(Buffer_Item_Header), m_cb.size);
        }

        return -1;
    }

    write((unsigned char*)&pnewbufferitem, sizeof(Buffer_Item_Header));
    write(buf, len);
    increaseCount();

    if(m_inRecording)
    {
        if(NULL == m_inFile)
        {
            m_inFile = fopen(m_inFileName, "ab+");

            if(NULL != m_inFile)
            {
                printf("start to record on recordFile:%s\n", m_inFileName);
            }
            else
            {
                printf("failed to open recordFile %s\n", m_inFileName);
            }
        }

        if(m_inFile)
        {
            fwrite(buf, 1, len, m_inFile);
        }
    }

    return pnewbufferitem.length;
}

void cycle_buffer::setRecording(bool bIn, bool bOut)
{
    m_inRecording = bIn;
    m_outRecording = bOut;
    printf("%s: m_inRecording = %d, m_outRecording = %d", __func__, m_inRecording, m_outRecording);
}

void cycle_buffer::setRecordingFileName(char *pInPath, char *pOutPath)
{
    if(NULL != pInPath)
    {
        strcpy(m_inFileName, pInPath);
    }
    else
    {
        m_inFileName[0] = '\0';
    }

    if(NULL != pOutPath)
    {
        strcpy(m_outFileName, pOutPath);
    }
    else
    {
        m_outFileName[0] = '\0';
    }
}
