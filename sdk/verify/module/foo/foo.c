#include "mi_foo.h"
#include <stdio.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

const int wake_times = 5;

void *thread_wake(void *w){
    int i;
    for(i = 0; i < wake_times; ++i){
        foo_wake(1);
        sleep(1);
    }
    return w;
}

int main(int argc, const char *argv[]){
    int rval, a = 1, b = 2, i, j, fd[4];
    pthread_t tid;
    pthread_attr_t attr;
    if(argc == 3){
        a = atoi(argv[1]);
        b = atoi(argv[2]);
    }
    MI_S32 r = call_foo_function(a, b, &rval);
    printf("call_foo_function(%d, %d) => %d(%d)\n", a, b, rval, r);
    pthread_attr_init(&attr);
    pthread_create(&tid, &attr, thread_wake, NULL);
    r = foo_openpollfd(fd+0, 0);
    r = foo_openpollfd(fd+1, 1);
    r = foo_openpollfd(fd+2, 0);
    r = foo_openpollfd(fd+3, 1);
    for(i = 0; i < wake_times; ++i){
        struct pollfd pfd[4] = {
            {fd[0], POLLIN|POLLERR},
            {fd[1], POLLIN|POLLERR},
            {fd[2], POLLIN|POLLERR},
            {fd[3], POLLIN|POLLERR},
        };
        rval = poll(pfd, 4, -1);
        printf("poll return %d\n", rval);
        for(j = 0; j < 4; ++j){
            printf("ack %d %x\n", j, pfd[j].revents);
        }

        for(j = 0; j < rval; ++j){
            foo_ack(1);
        }
    }
    close(fd[0]);
    close(fd[1]);
    close(fd[2]);
    close(fd[3]);
    printf("test return\n");
    sleep(1);
    return 0;
}
