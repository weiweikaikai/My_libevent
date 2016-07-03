/*************************************************************************
	> File Name: k_event-internal.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Thu 09 Jun 2016 03:18:11 PM CST
 ************************************************************************/

#ifndef _K_EVENT_INTERNAL_H
#define _K_EVENT_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif
//#include"k_config.h"
#include"k_min_heap.h"
#include"k_evsignal.h"


struct eventop  //事件操作结构体
{
	//IO复用名称
	const char *name; 
	
	//函数指针实现类似C++的多态
	void *(*init)(struct event_base *);
	int (*add)(void *, struct event *);
	int (*del)(void *, struct event *);
	int (*dispatch)(struct event_base *, void *, struct timeval *);
	void (*dealloc)(struct event_base *, void *);
	
	int need_reinit; //我们是否需要重新定义struct event_base 
};

//事件处理框架，其实就是Reactor的实例
 struct event_base
 {
	 	const struct eventop *evsel;  //事件操作结构体指针
		void *evbase;  /*你可以把evsel和evbase看作是类和静态函数的关系，
		                 比如添加事件时的调用行为：evsel->add(evbase, ev)，
		                 实际执行操作的是evbase实际上用C语言模拟实现了C++中传递this指针的效果，
		                 只是这里要传入一个eventops中的一个实例*/
		int event_count;  /* counts number of total events(事件总数) */  
        int event_count_active; /* counts number of active events (激活的事件总数)*/  
        int event_gotterm;  /* Set to terminate loop  (设置终止循环)*/  
        int event_break;  /* Set to terminate loop immediately (设置随时终止循环)*/  
        struct event_list **activequeues; /* active event management 
		                                  是一个二级指针，libevent支持事件优先级，
		                                  因此你可以把它看作是数组，
		                                  其中的元素activequeues[priority]是一个链表的管理结构体指针，
		                                  链表的每个节点指向一个优先级为priority的就绪事件event*/
	    int nactivequeues;             //活动队列个数
	    struct evsignal_info sig;     /* signal handling info 
	                                    用来管理信号的结构体对象 在Evsignal.h中*/

	     struct event_list eventqueue;  //事件队列使用宏实现的双向链表
	     struct timeval event_tv; //dispathc()函数返回的时间 也就是IO时间就绪的时间

	     struct min_heap timeheap;  //管理定时事件的小根堆对象

	     //struct timeval tv_cache;  //时间缓存
 };

#define	TAILQ_FIRST(head)		((head)->tqh_first)
#define	TAILQ_END(head)			NULL
#define	TAILQ_NEXT(elm, field)		((elm)->field.tqe_next)
#define	TAILQ_EMPTY(head)						\
	(TAILQ_FIRST(head) == TAILQ_END(head))


#ifdef __cplusplus
}
#endif

#endif
