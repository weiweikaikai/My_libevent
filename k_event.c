/*************************************************************************
	> File Name: k_event.c
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 14 May 2016 10:09:04 PM CST
 ************************************************************************/

#include<unistd.h>
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


static const struct eventop *eventops[] = {
	&epollops,
	//&pollops,
	//&selectops,
	NULL
};


//Global state
struct event_base *current_base = NULL; //一个全局的记录当前struct event_base的结构体指针

extern struct event_base *evsignal_base; //用于信号使用的 struct event_base结构体指针

//static int use_monotonic;  //  Monotonic时间指示的是系统从boot后到现在所经过的时间，
                          //如果系统支持Monotonic时间就将全局变量use_monotonic设置为1



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
      
	  base->sig.ev_signal_pair[0] = -1; //初始化socketpair管道
	  base->sig.ev_signal_pair[1] = -1;

	  base->evbase = NULL;
	  for (i = 0; eventops[i] && !base->evbase; i++) 
	{
		base->evsel = eventops[i]; //IO复用结构体指针赋值给base->evsel
		base->evbase = base->evsel->init(base);
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

	min_heap_elem_init(ev);

	/* by default, we put new events into the middle priority */
	if(current_base) //默认优先级插入是往中间插入
	{
		ev->ev_pri = current_base->nactivequeues/2;
	}
}