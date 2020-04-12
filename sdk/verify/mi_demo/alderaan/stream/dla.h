#ifndef __DLA_H__
#define __DLA_H__

#include "sys.h"

class Dla: public Sys
{
    public:
        Dla();
        virtual ~Dla();
    private:
        virtual void Init();
        virtual void Deinit();
};
#endif

