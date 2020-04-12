#ifndef __VPE_H__
#define __VPE_H__

#include "sys.h"

typedef struct stVpeInfo_s
{
    int intHdrType;
    int intbUseSnrFmt;
    int intSensorId;
    int intInputFmt;
    int intRotation;
    int int3dNrLevel;
    int intRunningMode;
}stVpeInfo_t;

typedef struct stVpeOutInfo_s
{
    int intPortId;
    int intVpeOutFmt;
    int intVpeOutWidth;
    int intVpeOutHeight;
}stVpeOutInfo_t;

class Vpe: public Sys
{
    public:
        Vpe();
        virtual ~Vpe();
    private:
        virtual void Init();
        virtual void Deinit();
        void Start();
        void Stop();

        stVpeInfo_t stVpeInfo;
        std::vector<stVpeOutInfo_t> vVpeOutInfo;
};
#endif

