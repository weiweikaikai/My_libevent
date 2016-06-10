/*************************************************************************
	> File Name: k_evutil.c
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Thu 09 Jun 2016 03:10:03 PM CST
 ************************************************************************/


//#include "config.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#include "k_queue.h"
#include "k_event.h"
#include "k_event-internal.h"
#include "k_evutil.h"


int evutil_socketpair(int family, int type, int protocol, int fd[2])
{
	//只在linux下使用所以直接用linux的系统调用就可以了
    	return socketpair(family, type, protocol, fd);
}
int evutil_make_socket_nonblocking(int fd)
{
        int flags;
		if ((flags = fcntl(fd, F_GETFL, NULL)) < 0) {
			printf("fcntl(%d, F_GETFL)", fd);
			return -1;
		}
		if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
			printf("fcntl(%d, F_SETFL)", fd);
			return -1;
		}
		return 0;
}
