#ifndef __VENC_H__
#define __VENC_H__

#include "sys.h"

typedef struct stVencInfo_s
{
    int intWidth;
    int intHeight;
    int intBitRate;
    int intEncodeType;
}stVencInfo_t;
class Venc: public Sys
{
    public:
        Venc();
        virtual ~Venc();
    private:
        virtual void Init();
        virtual void Deinit();
        stVencInfo_t stVencInfo;
};
#endif

