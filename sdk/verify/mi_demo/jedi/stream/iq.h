#ifndef __IQ_H__
#define __IQ_H__

#include "sys.h"

class Iq: public Sys
{
    public:
        Iq();
        virtual ~Iq();
    private:
        virtual void Init();
        virtual void Deinit();
        virtual void Start();
        virtual void Stop();
        int IspWaitReadyTimeout(int time_ms);
        stModDesc_t stVpeDesc;
        int intbUseIq;
        unsigned int iqSrvVidWid;
        unsigned int iqSrvVidHei;
};
#endif

