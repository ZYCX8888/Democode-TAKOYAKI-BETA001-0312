#ifndef _AT_CONFIG_H
#define _AT_CONFIG_H

#define    HAVE_SCHED_H        			1
#define    HAVE_PTHREADS				1
#define    HAVE_SYS_UIO_H				1
#define    HAVE_IOCTL					1
#define    HAVE_SYS_SOCKET_H        	1
#define    HAVE_LIBC_SYSTEM_PROPERTIES 	1

#define    HAVE_STRLCPY        		  	0
#define    HAVE_DIRENT_D_TYPE			0


/* Used to retry syscalls that can return EINTR. */
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(exp) ({         \
    typeof (exp) _rc;                      \
    do {                                   \
        _rc = (exp);                       \
    } while (_rc == -1 && errno == EINTR); \
    _rc; })
#endif

#define HERE printf("[%s]:%d\n",__FUNCTION__,__LINE__)

#endif
