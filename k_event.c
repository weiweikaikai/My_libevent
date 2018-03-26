/*************************************************************************
	> File Name: k_event.c
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 14 May 2016 10:09:04 PM CST
 ************************************************************************/

#include<unistd.h>
#include<stdio.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include"k_event.h"
#include"k_evutil.h"
#include"k_event-internal.h"


extern const struct eventop selectops;
extern const struct eventop pollops;
extern const struct eventop epollops;


static const struct eventop *eventops[] = { //这里会根据不同平台使用宏选择不同的IO复用函数结构体，之间的先后排列顺序是根据具体测测试性能进行排列的
	&epollops,
	&pollops,
	&selectops,
	NULL
};


//Global state
struct event_base *current_base = NULL; //一个全局的记录当前struct event_base的结构体指针

extern struct event_base *evsignal_base; //用于信号使用的 struct event_base结构体指针

//static int use_monotonic;  //  Monotonic时间指示的是系统从boot后到现在所经过的时间，
                          //如果系统支持Monotonic时间就将全局变量use_monotonic设置为1

int (*event_sigcb)(void);		/* Signal callback when gotsig is set */
volatile sig_atomic_t event_gotsig;	/* Set in signal handler  是一个原子操作*/

static void	event_queue_insert(struct event_base *, struct event *, int);
static void	event_queue_remove(struct event_base *, struct event *, int);
static int	event_haveevents(struct event_base *);//是否有事件
static int	timeout_next(struct event_base *, struct timeval **);
static void	timeout_process(struct event_base *);
static void	event_process_active(struct event_base *);


int timeout_next(struct event_base *base, struct timeval **tv_p)
{
         struct timeval now;
	     struct event *ev;
	     struct timeval *tv = *tv_p;
		 if ((ev = min_heap_top(&base->timeheap)) == NULL)
		{
		/* if no time-based events are active wait for I/O */
		     *tv_p = NULL;
		     return (0);
	    }

	    if (gettimeofday(&now,0) == -1)
		{
		    return (-1);
		}

	    if (evutil_timercmp(&ev->ev_timeout, &now, <=)) //如果当前时刻已经大于等于事件超时时间
	    {
		       evutil_timerclear(tv);//已经超时就将事件的时间清空
		       return (0);
	    }

	     evutil_timersub(&ev->ev_timeout, &now, tv); //因为是单调时间将系统当前的时间与未来的超时时间相减从tv中保存返回

	       assert(tv->tv_sec >= 0);
	       assert(tv->tv_usec >= 0);

	       printf("timeout_next: in %ld seconds\n", tv->tv_sec);
	return (0);
}
int event_haveevents(struct event_base *base)
{
	return (base->event_count > 0);
}

void event_active(struct event *ev, int res, short ncalls)
{
	//不同类型的事件 只要处于活跃的状态中
    if (ev->ev_flags & EVLIST_ACTIVE) 
	{
		ev->ev_res |= res;
		return;
	}

    ev->ev_res = res;
	ev->ev_ncalls = ncalls;
	ev->ev_pncalls = NULL;
	event_queue_insert(ev->ev_base, ev, EVLIST_ACTIVE);   
}

 void event_process_active(struct event_base *base)
 {
     struct event *ev;
	struct event_list *activeq = NULL;
	int i;
	short ncalls;

 	for (i = 0; i < base->nactivequeues; ++i)
	 {
		if (TAILQ_FIRST(base->activequeues[i]) != NULL)
		{
			activeq = base->activequeues[i];
			break;
		}
	}
		assert(activeq != NULL);
		for (ev = TAILQ_FIRST(activeq); ev; ev = TAILQ_FIRST(activeq))
		 {
		      if (ev->ev_events & EV_PERSIST) //事件是一个永久事件，只是从队列中移除
			 {
			    event_queue_remove(base, ev, EVLIST_ACTIVE);
			 }
		      else //否则可能会被在IO复用监听中被移除
			 {
			    event_del(ev);
			 }
			 	/* Allows deletes to work */
		ncalls = ev->ev_ncalls;
		ev->ev_pncalls = &ncalls;
		while (ncalls)
			{
			    ncalls--;
			    ev->ev_ncalls = ncalls;
			    (*ev->ev_callback)((int)ev->ev_fd, ev->ev_res, ev->ev_arg);
			    if (event_gotsig || base->event_break) 
					{
			  	      ev->ev_pncalls = NULL;
				      return;
			        }
		    }
			ev->ev_pncalls = NULL;
	   	}
 }
void timeout_process(struct event_base *base)
{
      struct timeval now;
	  struct event *ev;

	  if(min_heap_empty(&base->timeheap))//二叉堆是空的
	   {
		  return;
	   }
	   gettimeofday(&now,0);
	   while ((ev = min_heap_top(&base->timeheap)))
		  {
		     if(evutil_timercmp(&ev->ev_timeout, &now, >)) //如果还没有超时
			  {
			     break; //就直接退出
			  }
			  /* delete this event from the I/O queues */
		         event_del(ev);
				 event_active(ev, EV_TIMEOUT, 1);//将触发超时的时间重新插入活动队列
		   }
}

struct event_base* event_init(void)
{
	struct event_base *base = event_base_new();
	if (base != NULL)
	{
		current_base = base;
	}
	return (base);
}

struct event_base* event_base_new(void)
{
  int i;
  struct event_base *base;
  if((base = calloc(1,sizeof(struct event_base))) == NULL)
	{
       printf("event_base_new  err\n");
	   return NULL;
     }
	 //检查是否使用monotonic时间(单调时间）
	// detect_monotonic();
	// 获取系统时间放到event_tv中，我只实现linux版本不用使用包装的函数直接使用系统调用
     //gettime(base, &base->event_tv);
	 //此时struct base中的 struct timeval tv_cache 时间缓存就没有用了，因为默认linux使用单调时间
	 gettimeofday(&base->event_tv,0); //只获得当前时间，时区的参数为0
	 min_heap_ctor(&base->timeheap);//初始化事件超时小根堆
     TAILQ_INIT(&base->eventqueue);//初始化链表管理结构体
      
	  base->sig.ev_signal_pair[0] = -1; //初始化socketpair管道 用于在统一信号事件源时候作为通知机制
	  base->sig.ev_signal_pair[1] = -1;

	  base->evbase = NULL;
	  for (i = 0; eventops[i] && !base->evbase; i++) 
	{
		base->evsel = eventops[i]; //IO复用结构体指针赋值给base->evsel
		base->evbase = base->evsel->init(base);  //返回的是一个被转换为void*的具体的IO复用op
		printf("IO multiplexing function: %s\n",base->evsel->name);
	}
	if (base->evbase == NULL)
	{
		printf("no event mechanism available\n");
	}
	event_base_priority_init(base, 1);//分配一个活动时间队列
     
	return base;
}
 
 int event_base_priority_init(struct event_base *base, int npriorities)
 { 
	 	int i;
	if (base->event_count_active) //被激活的事件数
	 {
		return (-1);
	 }
	 if (npriorities == base->nactivequeues)
	 {
		return (0);
	 }
     if (base->nactivequeues) //队列的个数 释放原来的队列
	{
		for (i = 0; i < base->nactivequeues; ++i) 
		{
			free(base->activequeues[i]);
		}
		free(base->activequeues);
	}
	//释放完之后重新申请构建自己的队列
	base->nactivequeues = npriorities;
	base->activequeues = (struct event_list **)
	    calloc(base->nactivequeues, sizeof(struct event_list *));
	if(base->activequeues == NULL)
		{
	      printf("calloc error\n");
		  return -1;
	    }
	for(i = 0; i < base->nactivequeues; ++i) 
		{
		base->activequeues[i] = malloc(sizeof(struct event_list));
		if (base->activequeues[i] == NULL)
			{
			  printf("calloc error\n");
		      return -1;
			}
		TAILQ_INIT(base->activequeues[i]);//初始化链表管理结构体
	   }
        return 0;
 }

int event_priority_set(struct event *ev, int pri)
{
	if (ev->ev_flags & EVLIST_ACTIVE)
		return (-1);
	if (pri < 0 || pri >= ev->ev_base->nactivequeues)
		return (-1);

	ev->ev_pri = pri;

	return (0);
}


int event_base_set(struct event_base *base, struct event *ev)
{
	/* Only innocent events may be assigned to a different base */
	if (ev->ev_flags != EVLIST_INIT)
	{
		return (-1);
	}

	ev->ev_base = base;
	ev->ev_pri = base->nactivequeues/2;

	return (0);
}


void event_set(struct event *ev, int fd, short events,
	  void (*callback)(int, short, void *), void *arg)
{
	/* Take the current base - caller needs to set the real base later */
	ev->ev_base = current_base;

	ev->ev_callback = callback;
	ev->ev_arg = arg;
	ev->ev_fd = fd;
	ev->ev_events = events;
	ev->ev_res = 0;
	ev->ev_flags = EVLIST_INIT;
	ev->ev_ncalls = 0;
	ev->ev_pncalls = NULL;

	min_heap_elem_init(ev); //小堆

	/* by default, we put new events into the middle priority */
	if(current_base) //默认优先级插入是往中间插入
	{
		ev->ev_pri = current_base->nactivequeues/2;
	}
}
int event_add(struct event *ev, const struct timeval *timeout)
{
    struct event_base *base = ev->ev_base; //事件中的struct event_base 结构体也就是Reactor模式的主体
	const struct eventop *evsel = base->evsel; //事件操作结构体指针，内部全是函数指针
	void *evbase = base->evbase;//用于在函数指针中作为事件操作结构体指针进行传递
    int res=0;

	 assert(!(ev->ev_flags & ~EVLIST_ALL));
   
   //超时事件在二叉堆中的插入

   if(timeout != NULL && !(ev->ev_flags & EVLIST_TIMEOUT) ) //如果这个超时事件没有在二叉堆中就进行插入，否则不做任何事
   {
       if (min_heap_reserve(&base->timeheap,1 + min_heap_size(&base->timeheap)) == -1)
	      {//分配失败  ENOMEM == errno
	        return (-1);
		  }
   }
   //读事件写事件 信号事件没有插入队列中才准备插入
   if ((ev->ev_events & (EV_READ|EV_WRITE|EV_SIGNAL)) &&
	    !(ev->ev_flags & (EVLIST_INSERTED|EVLIST_ACTIVE)))
	{
         res = evsel->add(evbase, ev);//活跃的首先加入IO复用的管理中
		if (res != -1)
			{
			  event_queue_insert(base, ev, EVLIST_INSERTED);
			}
    }
	//只有在前边添加事件成功的时候再改变超时状态
    if(res != -1 && timeout != NULL)
	{
	  struct timeval now; 
	  //如果当前要添加的事件已经在超时队列中，删除原来的超时事件
	  if(ev->ev_flags & EVLIST_TIMEOUT)
		{
	      event_queue_remove(base, ev, EVLIST_TIMEOUT);
	    }
		//事件在活动队列中 而且时间超时返回了
       if ((ev->ev_flags & EVLIST_ACTIVE) && (ev->ev_res & EV_TIMEOUT)) 
		  {
		   //检查是否在一个循环中执行就是只执行了回调函数一次
		    if (ev->ev_ncalls && ev->ev_pncalls) 
			{
			    *ev->ev_pncalls = 0;//将执行函数次数的记录清零(ev->ev_ncalls清零)
			}
			event_queue_remove(base, ev, EVLIST_ACTIVE);//将事件从活跃队列中移除
		  }
         gettimeofday(&now,0);
		 evutil_timeradd(&now,timeout,&ev->ev_timeout);
		 event_queue_insert(base, ev, EVLIST_TIMEOUT);//将新的事件加入超时队列
	}
	return (res);
}

int event_del(struct event *ev)
{
	struct event_base *base;
	const struct eventop *evsel;
	void *evbase;
    if(ev->ev_base == NULL)
		{
		  return (-1);
		}
     base = ev->ev_base;
	 evsel = base->evsel;
	 evbase = base->evbase;

    assert(!(ev->ev_flags & ~EVLIST_ALL)); //有一个队列存在
     //看是否只是在一个循环中执行这个时间
	if (ev->ev_ncalls && ev->ev_pncalls) 
		{
		  *ev->ev_pncalls = 0;
	    }
	if (ev->ev_flags & EVLIST_TIMEOUT)
		{
	    	event_queue_remove(base, ev, EVLIST_TIMEOUT);
		}

	if (ev->ev_flags & EVLIST_ACTIVE)
		{
		    event_queue_remove(base, ev, EVLIST_ACTIVE);
		}
    if (ev->ev_flags & EVLIST_INSERTED)//如果要删除注册队列中的那就要在IO复用监听中也要删除这个事件
	    {
		     event_queue_remove(base, ev, EVLIST_INSERTED);
		     return (evsel->del(evbase, ev));
	    }    
          return (0);
}

void event_queue_insert(struct event_base *base, struct event *ev, int queue)
{
       if(ev->ev_flags & queue) //如果事件在规定的queue队列中
		 {
			 //活跃事件容易出现二次插入，就直接返回
            	if (queue & EVLIST_ACTIVE)
			       {
			        return;
				   }
		 }
		 //无论是那种事件，事件总数加1
        if (~ev->ev_flags & EVLIST_INTERNAL)
	   { 
	     	base->event_count++;
	   }
        ev->ev_flags |= queue;//当前时间被标记为queue所指的队列
       
	switch (queue)
		{
	case EVLIST_INSERTED:
		TAILQ_INSERT_TAIL(&base->eventqueue, ev, ev_next);
		break;
	case EVLIST_ACTIVE:
		base->event_count_active++;
		TAILQ_INSERT_TAIL(base->activequeues[ev->ev_pri],
		    ev,ev_active_next);
		break;
		case EVLIST_TIMEOUT: 
			{
		      min_heap_push(&base->timeheap, ev);
		      break;
	        }
	default:
		printf("unknown queue ");
		}

}
void event_queue_remove(struct event_base *base, struct event *ev, int queue)
{
        if (!(ev->ev_flags & queue))//不存在queue类型这种队列
	     {
		     printf("not exist this queue\n");   
			 return;
		 }
		 if (~ev->ev_flags & EVLIST_INTERNAL) //不管这个时间的类型是什么总的事件个数减1
	      {
		      base->event_count--;
		  }
		  ev->ev_flags &= ~queue;//将时间类型标志清零
          switch (queue)
			  {
	             case EVLIST_INSERTED:
		               TAILQ_REMOVE(&base->eventqueue, ev, ev_next);
		               break;
	             case EVLIST_ACTIVE:
		               base->event_count_active--;//活跃事件数减1
		               TAILQ_REMOVE(base->activequeues[ev->ev_pri],ev, ev_active_next);
		               break;
	             case EVLIST_TIMEOUT:
		               min_heap_erase(&base->timeheap, ev);
		               break;
	             default:
		            printf("unknown queue\n");
	}
  
}

int event_dispatch(void)
{
	return (event_loop(0));
}

int event_loop(int flags)
{
	return event_base_loop(current_base, flags);
}

int event_base_loop(struct event_base *base, int flags)
{
	
	const struct eventop *evsel = base->evsel;
	void *evbase = base->evbase;

	struct timeval tv;
	struct timeval *tv_p;
	int res=0;
	if (base->sig.ev_signal_added)
	{
		evsignal_base = base;
	}
	while(1)
	{
	   	if (base->event_gotterm)//如果设置了终止
		{
			base->event_gotterm = 0;
			break;
		}
		if (base->event_break) 
		{
			base->event_break = 0;
			break;
		}
		/* You cannot use this interface for multi-threaded apps */
		while (event_gotsig) //如果event_gotsig被设置就说明已经设置了信号的回调函数
		{
			event_gotsig = 0;
			if (event_sigcb) 
			{
				res = (*event_sigcb)();
				if (res == -1)
				{
					errno = EINTR; //如果被中断就直接返回
					return (-1);
				}
			}
		}
	      //因为使用了单调时间这里就不用再核对时间的正确性了
		  tv_p = &tv;
      if (!base->event_count_active && !(flags & EVLOOP_NONBLOCK)) //如果当前时间没有事件处于激活状态且不是非阻塞的
		  {
		    	timeout_next(base, &tv_p);//就获下一个时间超时时间
		  }
	  else
		{
		  /* 
			if we have active events, we just poll new event without waiting.
		  */
	      evutil_timerclear(&tv);  
	    }
			/* If we have no events, we just exit */
		if (!event_haveevents(base))
		{
			printf("no events registered.\n");
			return (1);
		}
		/* update last old time */
		gettimeofday(&base->event_tv,0);

		res = evsel->dispatch(base, evbase, tv_p);
        if (res == -1)
		{
			return (-1);
		}
		timeout_process(base); //检测时间堆，将触发超时的时间重新插入活动队列
		if (base->event_count_active)
		{
			event_process_active(base);//根据优先级处理活动队列中的事件
		   if (!base->event_count_active && (flags & EVLOOP_ONCE))
			  {
		        break;	
			  }
				
		} 
		else if (flags & EVLOOP_NONBLOCK)
		{
			break;
		}
	}

    return 0;
}

int event_pending(struct event *ev, short event, struct timeval *tv)
{
	struct timeval	now, res;
	int flags = 0;

	if (ev->ev_flags & EVLIST_INSERTED)
		flags |= (ev->ev_events & (EV_READ|EV_WRITE|EV_SIGNAL));
	if (ev->ev_flags & EVLIST_ACTIVE)
		flags |= ev->ev_res;
	if (ev->ev_flags & EVLIST_TIMEOUT)
		flags |= EV_TIMEOUT;

	event &= (EV_TIMEOUT|EV_READ|EV_WRITE|EV_SIGNAL);

	/* 看看是否有一个超时，有就需要报告 */
	if (tv != NULL && (flags & event & EV_TIMEOUT)) 
	{
		gettimeofday(&now,NULL);
		evutil_timersub(&ev->ev_timeout, &now, &res);
		/* 正确映射到真实地址 */
		gettimeofday(&now, NULL);
		evutil_timeradd(&now, &res, tv);
	}

	return (flags & event);
}

void event_base_free(struct event_base *base)
{
     int i, n_deleted=0;
	 struct event *ev;

	if (base == NULL && current_base)
		base = current_base;
	if (base == current_base)
		current_base = NULL;

	/* 首先检查内部事件 */
	assert(base);
	/* 删除所有非内部事件. */
	for (ev = TAILQ_FIRST(&base->eventqueue); ev; ) 
	{
		struct event *next = TAILQ_NEXT(ev, ev_next);
		if (!(ev->ev_flags & EVLIST_INTERNAL)) 
		{
			event_del(ev);
			++n_deleted;
		}
		ev = next;
	}
	while ((ev = min_heap_top(&base->timeheap)) != NULL) 
	{
		event_del(ev);
		++n_deleted;
	}

	for (i = 0; i < base->nactivequeues; ++i)
	{
		for (ev = TAILQ_FIRST(base->activequeues[i]); ev; ) 
		{
			struct event *next = TAILQ_NEXT(ev, ev_active_next);
			if (!(ev->ev_flags & EVLIST_INTERNAL))
			{
				event_del(ev);
				++n_deleted;
			}
			ev = next;
		}
	}

	if (n_deleted)
		printf("%s: %d events were still set in base",__func__, n_deleted);

	if (base->evsel->dealloc != NULL)
		base->evsel->dealloc(base, base->evbase);

	for (i = 0; i < base->nactivequeues; ++i)
		assert(TAILQ_EMPTY(base->activequeues[i]));

	assert(min_heap_empty(&base->timeheap));
	min_heap_dtor(&base->timeheap);

	for (i = 0; i < base->nactivequeues; ++i)
	{
		free(base->activequeues[i]);
	}
	free(base->activequeues);

	assert(TAILQ_EMPTY(&base->eventqueue));

	free(base);
}

