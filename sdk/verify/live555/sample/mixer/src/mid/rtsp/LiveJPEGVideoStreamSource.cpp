/*
* LiveJPEGVideoStreamSource.cpp- Sigmastar
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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include "assert.h"
#include "LiveJPEGVideoStreamSource.hh"

enum
{
    START_MARKER = 0xFF,
    SOI_MARKER   = 0xD8,
    JFIF_MARKER  = 0xE0,
    CMT_MARKER   = 0xFE,
    DQT_MARKER   = 0xDB,
    SOF_MARKER   = 0xC0,
    DHT_MARKER   = 0xC4,
    SOS_MARKER   = 0xDA,
    EOI_MARKER   = 0xD9,
    DRI_MARKER   = 0xDD
};

extern void __attribute__ ((noinline)) memcpy_neon_pld(void *dest, const void *src, size_t n);

LiveJPEGVideoStreamSource*
LiveJPEGVideoStreamSource::createNew(UsageEnvironment& env, FramedSource* source)
{
    return new LiveJPEGVideoStreamSource(env, source);
}

LiveJPEGVideoStreamSource::LiveJPEGVideoStreamSource(UsageEnvironment& env, FramedSource* source)
    : JPEGVideoSource(env), fSource(source), fType(0), fWidth(0), fHeight(0), fPrecision(0), fQtableLength(0),
    fWidthPixels(0), fHeightPixels(0)
{
}

LiveJPEGVideoStreamSource::~LiveJPEGVideoStreamSource()
{
    Medium::close(fSource);
}

u_int8_t const* LiveJPEGVideoStreamSource::quantizationTables(u_int8_t& precision, u_int16_t& length)
{
    precision = fPrecision;
    length = fQtableLength;
    return fQuantizationTable;
}

void LiveJPEGVideoStreamSource::doGetNextFrame()
{
    if(fSource)
    {
        fSource->getNextFrame(fTo, fMaxSize,
                              afterGettingFrame, this,
                              FramedSource::handleClosure, this);
    }
}

void LiveJPEGVideoStreamSource::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
        struct timeval presentationTime, unsigned durationInMicroseconds)
{
    LiveJPEGVideoStreamSource* source = (LiveJPEGVideoStreamSource*) clientData;
    source->afterGettingFrame1(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

void LiveJPEGVideoStreamSource::afterGettingFrame1(unsigned frameSize, unsigned numTruncatedBytes,
        struct timeval presentationTime, unsigned durationInMicroseconds)
{
    int soi = 0;
    int sos = 0;
    int dataLen = 0;

    unsigned char* pFrame = NULL;
    unsigned char* pFrameEnd = NULL;

    do
    {
        fFrameSize = 0;
        fNumTruncatedBytes = 0;

        if (0 >= frameSize)
        {
            break;
        }

        pFrame = fTo;
        pFrameEnd = fTo + frameSize - 1;

        if(START_MARKER != *(pFrameEnd - 1) || EOI_MARKER != *pFrameEnd)
        {
            // EOI
          //  envir() << "LiveJPEGVideoStreamSource::afterGettingFrame1: JPEG has no EOI_MARKER\n";
            break;
        }

        pFrameEnd--;
        sos = 0;

        while(pFrame < pFrameEnd && 0 == sos)
        {
            if(START_MARKER != *pFrame)
            {
                printf("LiveJPEGVideoStreamSource::afterGettingFrame1: No START_MARKER\n");
                break;
            }

            pFrame++;

            switch(*pFrame)
            {
                case SOI_MARKER: // SOI
                    {
                        pFrame++;
                        soi = 1;
                        fQtableLength = 0;
                        //envir() << "Start of Image\n";
                        break;
                    }

                case JFIF_MARKER: //APP0 (JFIF segment marker)
                case 0xE1:
                case 0xE2:
                case 0xE3:
                case 0xE4:
                case 0xE5:
                case 0xE6:
                case 0xE7:
                case 0xE8:
                case 0xE9:
                case 0xEA:
                case 0xEB:
                case 0xEC:
                case 0xED:
                case 0xEE:
                case 0xEF:
                    {
                        int appLength = 0;

                        //envir() << "APP" << (*pFrame & 0x0F);

                        pFrame++;

                        appLength = (pFrame[0] << 8) | pFrame[1];

                        //envir() << ", segmentLength = " << segmentLength << "\n";
                        pFrame += appLength;

                        break;
                    }

                case DQT_MARKER: //DQT (Define Quantization Table)
                    {
                        int i = 0, j = 0;
                        int DQTLength = 0;

                        unsigned short DQTNum = 0; //0: DQT Y, 1: DQT Cb/Cr
                        unsigned short DQTPrecision = 0;

                        pFrame++;
                        DQTLength = (pFrame[0] << 8) | pFrame[1];

                        assert(67 == DQTLength);

                        DQTNum = pFrame[2] & 0x0F;
                        DQTPrecision = pFrame[2] >> 4;

                        assert(0 == DQTPrecision);

                        j = fQtableLength;

                        if(0 == DQTPrecision)
                        {
                            fPrecision &= ~(0x1 << DQTNum);
                            fQtableLength += 64;
                        }
                        else if(1 == DQTPrecision)
                        {
                            fPrecision |= (0x1 << DQTNum);
                            fQtableLength += (64 * 2);
                        }

                        for(i = 3; j < fQtableLength; j++)
                        {
                            fQuantizationTable[j] = pFrame[i++];

                        }

                        pFrame += DQTLength;

#if 0
                        envir() << "Define Quantization Table: " << DQTNum << "\n";
                        envir() << "fQtableLength = " << fQtableLength << "\n";
                        envir() << "fPrecision = " << fPrecision << "\n";
#endif

                        break;
                    }

                case SOF_MARKER: //SOF0 (Start Of Frame 0)
                    {
                        int SOFLength = 0;

                        unsigned int i = 2;

                        u_int8_t dataPrecision = 0;
                        u_int8_t numOfComponents = 0;
                        u_int8_t componentId[3] = {0};
                        u_int8_t samplingFactor[3] = {0};
                        u_int8_t QtableNum[3] = {0};

                        //envir() << "Start Of Frame 0\n";

                        pFrame++;
                        SOFLength = (pFrame[0] << 8) | pFrame[1];

                        dataPrecision = pFrame[i++];

                        assert(8 == dataPrecision);

                        fHeightPixels = (pFrame[i] << 8) | (pFrame[i + 1]);
                        fHeight = (pFrame[i] << 5) | (pFrame[i + 1] >> 3);
                        i += 2;
                        fWidthPixels = (pFrame[i] << 8) | (pFrame[i + 1]);
                        fWidth = (pFrame[i] << 5) | (pFrame[i + 1] >> 3);
                        i += 2;
                        if(fWidthPixels>2040||fHeightPixels>2040)
                        {
                            fWidth = 0;
                            fHeight = 0;
                        }

                        numOfComponents = pFrame[i++];

                        assert(3 == numOfComponents);  //YUV

                        for(int j = 0; j < numOfComponents; j++)
                        {
                            componentId[j] = pFrame[i++];
                            samplingFactor[j] = pFrame[i++];
                            QtableNum[j] = pFrame[i++];
                        }

                        if((componentId[0] == 1 && samplingFactor[0] == 0x21 && QtableNum[0] == 0) &&
                                (componentId[1] == 2 && samplingFactor[1] == 0x11 && QtableNum[1] == 1) &&
                                (componentId[2] == 3 && samplingFactor[2] == 0x11 && QtableNum[2] == 1))
                        {
                            fType = 0;  // YUV 4:2:2
                        }
                        else if((componentId[0] == 1 && samplingFactor[0] == 0x22 && QtableNum[0] == 0) &&
                                (componentId[1] == 2 && samplingFactor[1] == 0x11 && QtableNum[1] == 1) &&
                                (componentId[2] == 3 && samplingFactor[2] == 0x11 && QtableNum[2] == 1))
                        {
                            fType = 1;  // YUV 4:2:0
                        }

                        //assert(SOFLength == i);

                        pFrame += SOFLength;
                        break;
                    }

                case DHT_MARKER: //DHT( Define Huffman Table)
                    {
                        // DHT Y-DC diff, Y-AC-Coef, Cb/Cr-DC diff, Cb/Cr-AC-Coef
                        int DCTLength = 0;

                        pFrame++;
                        DCTLength = (pFrame[0] << 8) | pFrame[1];
                        pFrame += DCTLength;

                        break;
                    }

                case SOS_MARKER: //SOS (Start Of Scan)
                    {
                        int SOSLength = 0;
                        //envir() << "Start Of Scan\n";

                        pFrame++;
                        SOSLength = (pFrame[0] << 8) | pFrame[1];
                        pFrame += SOSLength;

                        //envir() << "SOSLength = " << SOSLength << "\n";

                        sos = 1;

                        break;
                    }

                case EOI_MARKER: //EOI

                    envir() << "End of Image\n";
                    break;
                default:
                    break;
            }

            if(0 == soi)
            {
                envir() << "LiveJPEGVideoStreamSource::afterGettingFrame1: JPEG has no SOI_MARKER\n";
                break;
            }
        }

        if(0 == sos)
        {
            envir() << "LiveJPEGVideoStreamSource::afterGettingFrame1: JPEG has no SOS_MARKER\n";
            break;
        }

        fFrameSize = pFrameEnd - pFrame;

        dataLen = fFrameSize & ~(0x3f);
        memcpy_neon_pld(fTo, pFrame, dataLen);

        pFrame += dataLen;
        memcpy(fTo + dataLen, pFrame, fFrameSize - dataLen);

        fNumTruncatedBytes = numTruncatedBytes;
        fPresentationTime = presentationTime;
        fDurationInMicroseconds = durationInMicroseconds;

    }
    while(0);

    afterGetting(this);
}
