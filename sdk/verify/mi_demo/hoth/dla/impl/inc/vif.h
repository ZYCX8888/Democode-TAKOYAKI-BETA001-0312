#ifndef __VIF_H__
#define __VIF_H__

#include "sys.h"

typedef struct stVifInfo_s
{
    int intSensorId;
    int intSensorRes;
    int intHdrType;
}stVifInfo_t;

class Vif: public Sys
{
    public:
        Vif();
        virtual ~Vif();
    private:
        virtual void Init();
        virtual void Deinit();
        stVifInfo_t stVifInfo;
};
#endif


