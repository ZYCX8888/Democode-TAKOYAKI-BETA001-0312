#ifndef __DIVP_H__
#define __DIVP_H__

#include "sys.h"

typedef struct stDivpInfo_s
{
    int intCropWidth;
    int intCropHeight;
    int intCropX;
    int intCropY;
}stDivpInfo_t;
typedef struct stDivpOutInfo_s
{
    int intPortId;
    int intDivpOutFmt;
    int intDivputWidth;
    int intDivpOutHeight;
}stDivpOutInfo_t;

class Divp: public Sys
{
    public:
        Divp();
        virtual ~Divp();
    private:
        virtual void Init();
        virtual void Deinit();
        stDivpInfo_t stDivpInfo;
        std::vector<stDivpOutInfo_t> vDivpOutInfo;
};
#endif

