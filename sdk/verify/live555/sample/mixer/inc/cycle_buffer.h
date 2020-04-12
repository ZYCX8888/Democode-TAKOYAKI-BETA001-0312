/*
* debug.h- Sigmastar
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

#ifndef __CYCLE_BUFFER_H_INCLUDED_
#define __CYCLE_BUFFER_H_INCLUDED_

#include <pthread.h>

/**
 * @class       cycle_buffer
 * @version     ver 1.0
 * @brief       实现循环缓冲区
 * @todo
 */
#define BLOCK_SIZE  188 * 7
#define MAX_BLOCK_COUNT 2000

struct Buffer_Item_Header
{
    int length;
    int frameType;
    struct timeval timestamp;
};

typedef struct iptv_circular_buffer
{
    unsigned char* begin;
    unsigned char* end;
    unsigned int size;
    unsigned char* rpos;
    unsigned char* wpos;
    unsigned int readflag    ;          /* 0表示可读，1表示无法读*/
} IPTV_CIRCULAR_BUFFER_T;

#define FRAME_SIZE 1000*1000 //500*1000

struct Buffer_Item
{
    int length;
    unsigned int timestamp;
    unsigned char buffer[FRAME_SIZE];
};

class cycle_buffer
{
public:
    cycle_buffer(unsigned int size = BLOCK_SIZE* MAX_BLOCK_COUNT, int maxBufCount = -1);
    ~cycle_buffer();

public:
    int read(unsigned char* buffer, int len);
    int write(const unsigned char* buffer, int len);
    void reset();
    void setbufferingsize(float bufferingsize);

    bool newwrite(const unsigned char* buffer, int len, unsigned int timestamp = 0, bool is_video = true);
    bool newread(Buffer_Item& bufferitem);

    int getFrame(unsigned char *buffer, int len, struct timeval *pTimestamp);
    int getFrame(unsigned char *buffer, int len, struct timeval *pTimestamp, int *frameType);

    unsigned int get_free_space(unsigned char** pRPos, unsigned char** pWPos);
    unsigned int get_valid_space();

    int writeFrame(unsigned char *buf, int len, struct timeval *timestamp);
    int writeFrame(unsigned char *buf, int len, struct timeval *timestamp, int frameType);

    unsigned int getBufferCount();

    unsigned int getFreeBufferCount();

    void setBufferCount(unsigned int count);

    void increaseCount();

    void decreaseCount();

    void setName(char* name);

    void setRecording(bool bIn, bool bOut);
    void setRecordingFileName(char *pInPath, char *pOutPath);
private:
    pthread_mutex_t m_bufmux;
    pthread_mutex_t m_bufCountmux;
    pthread_mutex_t m_bufReset;

    IPTV_CIRCULAR_BUFFER_T m_cb;
    unsigned int m_bufferingsize;
    int m_flag_buffering_data_ok;
    unsigned int m_buffering_write_count;

    bool m_flag_frame_size_notok;

    unsigned int m_maxBufCount;
    unsigned int m_bufCount;

    char    m_name[8];
    char    m_recordDevice_key[16];
    char    m_recordEnable_key[16];
    bool    m_recording;
    FILE*   m_recordFile;

    bool    m_inRecording;
    bool    m_outRecording;
    FILE*   m_inFile;
    FILE*   m_outFile;
    char    m_inFileName[128];
    char    m_outFileName[128];
};
#endif
