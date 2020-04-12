#include "dla.h"
#include <stdio.h>

Dla::Dla()
{
    printf("func: %s\n", __FUNCTION__);
}
Dla::~Dla()
{
    printf("func: %s\n", __FUNCTION__);
}
void Dla::Init()
{
    printf("func: %s\n", __FUNCTION__);
    
}
void Dla::Deinit()
{
    printf("func: %s\n", __FUNCTION__);
}

