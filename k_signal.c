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

struct event_base *evsignal_base = NULL;
static void evsignal_handler(int sig);
static int _evsignal_set_handler(struct event_base *base,int evsignal, void (*handler)(int));
static int _evsignal_restore_handler(struct event_base *base, int evsignal);



int _evsignal_restore_handler(struct event_base *base, int evsignal)
{
	int ret = 0;
	struct evsignal_info *sig = &base->sig;
	struct sigaction *sh;

	/* restore previous handler  重新保存原来的句柄*/
	sh = sig->sh_old[evsignal];
	sig->sh_old[evsignal] = NULL;
	if (sigaction(evsignal, sh, NULL) == -1)
	{
		printf("sigaction\n");
		ret = -1;
	}
	free(sh);
	return ret;
}


/*
为evsignal来处理设置信号处理程序，使
当我们清除当前的一个时，我们可以还原原始的处理程序。 
*/
 int _evsignal_set_handler(struct event_base *base,int evsignal, void (*handler)(int))
{
       struct sigaction sa;
	   struct evsignal_info *sig = &base->sig;
	    void *p;
		if (evsignal >= sig->sh_old_max)
			{
		      int new_max = evsignal + 1;
			  //增加了信号量就要重新分配信号存储的空间
			  p = realloc(sig->sh_old, new_max * sizeof(*sig->sh_old));
			  if (p == NULL) 
				  {
		        	printf("realloc error\n");
			        return (-1);
	              }
		    //将重新分配的空间初始化为0
		       memset((char *)p + sig->sh_old_max * sizeof(*sig->sh_old),
		       0, (new_max - sig->sh_old_max) * sizeof(*sig->sh_old));
               sig->sh_old_max = new_max;
		       sig->sh_old = p;
			}
		   sig->sh_old[evsignal] = malloc(sizeof (*sig->sh_old[evsignal]));
	       if (sig->sh_old[evsignal] == NULL)
			 {
		        printf("malloc\n");
		        return (-1);
	         }
           //保存以前的处理程序和设置新的处理程序
		     memset(&sa, 0, sizeof(sa));
	         sa.sa_handler = handler;
	         sa.sa_flags |= SA_RESTART;
	         sigfillset(&sa.sa_mask);
			 if (sigaction(evsignal, &sa, sig->sh_old[evsignal]) == -1) 
				 {
	        	   printf("sigaction error\n");
		           free(sig->sh_old[evsignal]);
		           sig->sh_old[evsignal] = NULL;
		           return (-1);
	             }
		   return 0;
}

void evsignal_handler(int sig)
{
	int save_errno = errno;
	if (evsignal_base == NULL) 
	{
		printf("%s: received signal %d, but have no base configured",__func__, sig);
		return;
	}

	evsignal_base->sig.evsigcaught[sig]++;
	evsignal_base->sig.evsignal_caught = 1;
	/*唤醒我们的通知机制 */
	send(evsignal_base->sig.ev_signal_pair[0], "a", 1, 0);
	errno = save_errno;
}


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

int evsignal_add(struct event *ev)
{
     int evsignal;
	struct event_base *base = ev->ev_base;
	struct evsignal_info *sig = &ev->ev_base->sig;

	if (ev->ev_events & (EV_READ|EV_WRITE))
	{
        printf("this event is not signal event\n");
	    return -1;
	}
	evsignal = EVENT_SIGNAL(ev);
	assert(evsignal >= 0 && evsignal < NSIG);
	if (TAILQ_EMPTY(&sig->evsigevents[evsignal])) 
		{
		   if (_evsignal_set_handler(base, evsignal, evsignal_handler) == -1)
			{
		       return (-1);
            }
		    /* catch signals if they happen quickly */
	     	 evsignal_base = base;

		    if (!sig->ev_signal_added) //如果信号没有注册就注册 
		    {
			    if (event_add(&sig->ev_signal, NULL))
			     {
				    return (-1);
			     }
			     sig->ev_signal_added = 1;
		    }
		}
		 //多个事件可能听同一个信号，所以链表几点数组中每个元素底下都连接着等待该信号的事件
       TAILQ_INSERT_TAIL(&sig->evsigevents[evsignal], ev, ev_signal_next);
	   return 0;
}


void evsignal_process(struct event_base *base)
{
    struct evsignal_info *sig = &base->sig;
	struct event *ev, *next_ev;
	sig_atomic_t ncalls;//这个类型是原子操作
	int i;
	
	  base->sig.evsignal_caught = 0; //
	  for (i = 1; i < NSIG; ++i) 
	 {
		ncalls = sig->evsigcaught[i];//每个信号触发的次数
		if (ncalls == 0)
		{
			continue;
		}
		sig->evsigcaught[i] -= ncalls;

		for (ev = TAILQ_FIRST(&sig->evsigevents[i]);ev != NULL; ev = next_ev) 
		{
			next_ev = TAILQ_NEXT(ev, ev_signal_next);
			if (!(ev->ev_events & EV_PERSIST)) //不是永久事件就删除
				{
				   event_del(ev);
				}
			event_active(ev, EV_SIGNAL, ncalls); //插入活跃队列中
		}

	}
}

int evsignal_del(struct event *ev)
{
	struct event_base *base = ev->ev_base;
	struct evsignal_info *sig = &base->sig;
	int evsignal = EVENT_SIGNAL(ev);

	assert(evsignal >= 0 && evsignal < NSIG);

	/* multiple events may listen to the same signal */
	TAILQ_REMOVE(&sig->evsigevents[evsignal], ev, ev_signal_next);

	if (!TAILQ_EMPTY(&sig->evsigevents[evsignal]))
		return (0);

	printf("%s: %p: restoring signal handler", __func__, ev);

	return (_evsignal_restore_handler(ev->ev_base, EVENT_SIGNAL(ev)));
}


void evsignal_dealloc(struct event_base *base)
{
   int i = 0;
	if (base->sig.ev_signal_added) 
	{
		event_del(&base->sig.ev_signal);
		base->sig.ev_signal_added = 0;
	}
	for (i = 0; i < NSIG; ++i)
	{
		if (i < base->sig.sh_old_max && base->sig.sh_old[i] != NULL)
			_evsignal_restore_handler(base, i); //还原到信号原来的属性
	}

	if (base->sig.ev_signal_pair[0] != -1) 
	{
		EVUTIL_CLOSESOCKET(base->sig.ev_signal_pair[0]);
		base->sig.ev_signal_pair[0] = -1;
	}
	if (base->sig.ev_signal_pair[1] != -1) {
		EVUTIL_CLOSESOCKET(base->sig.ev_signal_pair[1]);
		base->sig.ev_signal_pair[1] = -1;
	}
	base->sig.sh_old_max = 0;

	/* per index frees are handled in evsig_del() */
	if (base->sig.sh_old) {
		free(base->sig.sh_old);
		base->sig.sh_old = NULL;
	}
}