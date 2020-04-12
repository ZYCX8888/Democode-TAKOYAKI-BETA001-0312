#ifndef __FILE_H__
#define __FILE_H__

#include "sys.h"

class File: public Sys
{
    public:
        File();
        virtual ~File();
    private:
        virtual void Init();
        virtual void Deinit();
        void BindBlock(stModInputInfo_t & stIn);
        void UnBindBlock(stModInputInfo_t & stIn);
        static void DataReceiver(void *pData, unsigned int dataSize, void *pUsrData);

        std::map<std::string, int> mapInputWrFile;
};
#endif

