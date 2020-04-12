#include <stdio.h>
#include "sys.h"

int main(int argc, char **argv)
{
    std::map<unsigned int, std::string> mapRootStr;
    char getC = 0;
    
    if (argc != 2)
    {
        printf("Usage: ./%s xxx_ini_path\n", argv[0]);

        return -1;
    }
    Sys::InitSys(argv[1]);
    do
    {
        printf("Press 'q' to exit!\n");
        getC = getchar();
    }while (getC != 'q');
    Sys::DeinitSys();

    return 0;
}
