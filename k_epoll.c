/*************************************************************************
	> File Name: k_epoll.c
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Fri 10 Jun 2016 08:41:18 PM CST
 ************************************************************************/

//#include "config.h"

#include <stdint.h>
#include <sys/types.h>
#include <sys/time.h>

#include <sys/epoll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "k_queue.h"
#include "k_event.h"
#include "k_event-internal.h"
#include "k_evsignal.h"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/epoll.h>


//epoll的读写事件 IO复用中只关心读写 网络库
struct evepoll {
	struct event *evread;
	struct event *evwrite;
};

//epoll结构体
struct epollop {
	struct evepoll *fds;
	int nfds; //监听的句柄数
	struct epoll_event *events; //epoll_init中会分配空间的
	int nevents;//活跃的事件数
	int epfd;//epoll定义产生的句柄
};

static void *epoll_init	(struct event_base *);
static int epoll_add	(void *, struct event *);
static int epoll_del	(void *, struct event *);
static int epoll_dispatch	(struct event_base *, void *, struct timeval *);
static void epoll_dealloc	(struct event_base *, void *);

const struct eventop epollops = {
	"epoll",
	epoll_init,
	epoll_add,
	epoll_del,
	epoll_dispatch,
	epoll_dealloc,
	1 /* need reinit */
};
#define FD_CLOSEONEXEC(x) do { \
        if (fcntl(x, F_SETFD, 1) == -1) \
                printf("fcntl(%d, F_SETFD)", x); \
} while(0)

#define MAX_EPOLL_TIMEOUT_MSEC (35*60*1000)

#define INITIAL_NFILES 32
#define INITIAL_NEVENTS 32
#define MAX_NEVENTS 4096
/*
如果 glibc 没有封装某个内核提供的系统调用时，
我就没办法通过上面的方法来调用该系统调用。
如我自己通过编译内核增加了一个系统调用，
这时 glibc 不可能有你新增系统调用的封装 API，
此时我们可以利用 glibc 提供的syscall 函数直接调用。
每个系统调用都有定义的系统调用号宏定义
*/

int epoll_create(int size)
{
	return (syscall(__NR_epoll_create, size));
}

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
	return (syscall(__NR_epoll_ctl, epfd, op, fd, event));
}

int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout)
{
	return (syscall(__NR_epoll_wait, epfd, events, maxevents, timeout));
}




static void *epoll_init(struct event_base *base);
static int epoll_add(void *arg, struct event *ev);
static int epoll_del(void *arg, struct event *ev);
static int epoll_dispatch(struct event_base *base, void *arg, struct timeval *tv);
static void epoll_dealloc(struct event_base *base, void *arg);
static int epoll_recalc(struct event_base *base, void *arg, int max);




static int epoll_recalc(struct event_base *base, void *arg, int max)
{
	struct epollop *epollop = arg;
	if(max >= epollop->nfds)
	{
	     struct evepoll *fds;
		 int nfds = epollop->nfds;
		 while(nfds <= max)
		 {
		    nfds <= 1; //增加为原来的2倍
		 }
         
		 fds = realloc(epollop->fds,nfds*sizeof(struct evepoll));
		 if (fds == NULL)
		{
			printf("realloc\n");
			return (-1);
		}
        epollop->fds = fds;
		memset(fds + epollop->nfds, 0,(nfds - epollop->nfds) * sizeof(struct evepoll));
		epollop->nfds = nfds; //重新增加了文件描述符的个数并更新
	}
  return 0;
}
static void *epoll_init(struct event_base *base)
{
   int epfd;
   struct epollop *epollop; 

   //如果有的系统设置了一个环境变量禁用了epoll就就直接返回(这里默认没有禁用)
  	if ((epfd = epoll_create(32000)) == -1) 
	{
		if (errno != ENOSYS)
		{
			printf("epoll_create Function not implemented\n");
		}
		return (NULL);
	}
    FD_CLOSEONEXEC(epfd);
	if (!(epollop =(struct epollop*)calloc(1, sizeof(struct epollop))))
	{
		return (NULL);
	}
	epollop->epfd = epfd;
	epollop->events = (struct epoll_event*)malloc(INITIAL_NEVENTS * sizeof(struct epoll_event));
	if (epollop->events == NULL)
	{
		free(epollop);
		return (NULL);
	}
    epollop->nevents = INITIAL_NEVENTS;
	epollop->fds = (struct evepoll*)calloc(INITIAL_NFILES, sizeof(struct evepoll));
	if (epollop->fds == NULL)
		{
		    free(epollop->events);
		    free(epollop);
		    return (NULL);
	    }
	  epollop->nfds = INITIAL_NFILES;

    	evsignal_init(base);
	  return (epollop);
}
static int epoll_add(void *arg, struct event *ev)
{
	struct epollop *epollop = arg;
	struct epoll_event epev = {0, {0}}; //事件类型和用户数据清空
	struct evepoll *evep;
	int fd, op, events;

	if (ev->ev_events & EV_SIGNAL) //如果是信号事件就使用信号事件的函数加入
		{
		  return (evsignal_add(ev));
		}
     fd = ev->ev_fd;
	 if(fd >= epollop->nfds)
	  {
	     //现在的文件描述符大于最大数量的文件描述符个数 重新分配
		 if(epoll_recalc(ev->ev_base,epollop,fd) == -1)
		  {
		   return (-1);
		 }
	  }
	  evep = &epollop->fds[fd];
	  op = EPOLL_CTL_ADD;
	  events =0;
	  if (evep->evread != NULL) //有数据进来就是可以读的时候
	 {
		events |= EPOLLIN;
		op = EPOLL_CTL_MOD;
	 } 
	 if (evep->evwrite != NULL)//要写的时候也就是数据要出去的时候
	{
		events |= EPOLLOUT;
		op = EPOLL_CTL_MOD;
	}
	if (ev->ev_events & EV_READ)
		events |= EPOLLIN;
	if (ev->ev_events & EV_WRITE)
		events |= EPOLLOUT;

    epev.data.fd = fd;
	epev.events = events;

	if (epoll_ctl(epollop->epfd, op, ev->ev_fd, &epev) == -1)
	{
	    return (-1);
	}
	/*跟新事件的负责*/
    if (ev->ev_events & EV_READ)
		evep->evread = ev;
	if (ev->ev_events & EV_WRITE)
		evep->evwrite = ev;

	return (0);
}
static int epoll_del(void *arg, struct event *ev)
{
    return (0);
}

static int epoll_dispatch(struct event_base *base, void *arg, struct timeval *tv)
{
    struct epollop *epollop = arg;
	struct epoll_event *events = epollop->events;
	struct evepoll *evep;
	int i, res, timeout = -1;

   if (tv != NULL)
	{
		timeout = tv->tv_sec * 1000 + (tv->tv_usec + 999) / 1000;
		if (timeout > MAX_EPOLL_TIMEOUT_MSEC)
	      {
		    /* Linux kernels can wait forever if the timeout is too big;
		    * see comment on MAX_EPOLL_TIMEOUT_MSEC. 
		    内核会一直等下去，但是我们不希望epoll永远等下去*/
		     timeout = MAX_EPOLL_TIMEOUT_MSEC;
	      }
	}
	
	res = epoll_wait(epollop->epfd, events, epollop->nevents, timeout);
	  if(res == -1) 
		{
		  if (errno != EINTR) 
			{
			  printf("epoll_wait\n");
			  return (-1);
		    }
			evsignal_process(base);//EINTR信号触发
		    return (0);
	    }
		else if (base->sig.evsignal_caught) //此事件的信号被注册
	    {
		   evsignal_process(base);
	    }
		for (i = 0; i < res; i++) 
		{
		    int what = events[i].events;
		     struct event *evread = NULL;
			 struct event *evwrite = NULL;
		     int fd = events[i].data.fd;

		      if (fd < 0 || fd >= epollop->nfds)
			     continue;
		      evep = &epollop->fds[fd];

		    if (what & (EPOLLHUP|EPOLLERR))
				{
			      evread = evep->evread;
			      evwrite = evep->evwrite;
		        } 
			else
				{
			      if (what & EPOLLIN) 
					 {
				       evread = evep->evread;
			         }
			        if (what & EPOLLOUT)
					{
				      evwrite = evep->evwrite;
			        }
		        }
			if (!(evread || evwrite))
			   {
		       	 continue;
			   }

		if (evread != NULL)
			event_active(evread, EV_READ, 1);
		if (evwrite != NULL)
			event_active(evwrite, EV_WRITE, 1);
	  }
	  	if (res == epollop->nevents && epollop->nevents < MAX_NEVENTS)
		   {
		    /* We used all of the event space this time.  We should
		       be ready for more events next time. 
		      我们使用了全部的空间,我们要准备好更多空间来容量更多的事件*/
		      int new_nevents = epollop->nevents * 2;
		      struct epoll_event *new_events;

		      new_events = realloc(epollop->events, new_nevents * sizeof(struct epoll_event));
		      if (new_events)
		       {
			      epollop->events = new_events;
			      epollop->nevents = new_nevents;
		      }
	     }

        return 0;
}
static void epoll_dealloc(struct event_base *base, void *arg)
{
	struct epollop *epollop = arg;

	evsignal_dealloc(base);
	if (epollop->fds)
		free(epollop->fds);
	if (epollop->events)
		free(epollop->events);
	if (epollop->epfd >= 0)
		close(epollop->epfd);

	memset(epollop, 0, sizeof(struct epollop));
	free(epollop);
}