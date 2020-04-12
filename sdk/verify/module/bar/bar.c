#include "mi_bar.h"
#include <stdio.h>

int main(){
    MI_S32 r = call_bar_function();
    printf("call_bar_function() => %d\n", r);
    return 0;
}
