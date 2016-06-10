/*************************************************************************
	> File Name: k_evsignal.c
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Fri 10 Jun 2016 08:25:53 PM CST
 ************************************************************************/

//#include "config.h"


#include <sys/types.h>
#include <sys/time.h>
#include <k_queue.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>

#include "k_event.h"
#include "k_event-internal.h"
#include "k_evsignal.h"
#include "k_evutil.h"


static void evsignal_cb(int fd, short what, void *arg)
{
	static char signals[1];
	ssize_t n;
	n = recv(fd, signals, sizeof(signals), 0);
	if (n == -1)
	{
		int err = errno;
		if (err == EAGAIN)
		{
			printf("recv \n");
			return;
		}
	}
}

#define FD_CLOSEONEXEC(x) do { \
        if (fcntl(x, F_SETFD, 1) == -1) \
                printf("fcntl(%d, F_SETFD)", x); \
} while (0)




int evsignal_init(struct event_base *base)
{
  int i;
  	if (evutil_socketpair(
		    AF_UNIX, SOCK_STREAM, 0, base->sig.ev_signal_pair) == -1)
	{
	    printf("evutil_socketpair errno\n");
		return -1;
	}
    FD_CLOSEONEXEC(base->sig.ev_signal_pair[0]);
	FD_CLOSEONEXEC(base->sig.ev_signal_pair[1]);
	base->sig.sh_old = NULL;
	base->sig.sh_old_max = 0;
	base->sig.evsignal_caught = 0;
	memset(&base->sig.evsigcaught, 0, sizeof(sig_atomic_t)*NSIG);
	/* initialize the queues for all events */
	for (i = 0; i < NSIG; ++i)
	{
		TAILQ_INIT(&base->sig.evsigevents[i]);
	}
        evutil_make_socket_nonblocking(base->sig.ev_signal_pair[0]);
        evutil_make_socket_nonblocking(base->sig.ev_signal_pair[1]);

	  event_set(&base->sig.ev_signal, base->sig.ev_signal_pair[1],
		EV_READ | EV_PERSIST, evsignal_cb, &base->sig.ev_signal);
	base->sig.ev_signal.ev_base = base;
	base->sig.ev_signal.ev_flags |= EVLIST_INTERNAL;

	return 0;
}

