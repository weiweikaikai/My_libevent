/*************************************************************************
	> File Name: k_event.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 14 May 2016 09:50:13 PM CST
 ************************************************************************/

#ifndef _K_EVENT_H
#define _K_EVENT_H

#ifdef __cplusplus
extern "C"{
#endif

#include "k_event-config.h"
#include"k_evutil.h"


#ifdef _EVENT_HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef _EVENT_HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

/*
用于标记当前event处于什么状态 
比如：在time堆中， 
在注册事件链表中，在激活链表中
*/
#define EVLIST_TIMEOUT 0x01 // event在time堆中  
#define EVLIST_INSERTED 0x02 // event在已注册事件链表中  
#define EVLIST_SIGNAL 0x04 // 未见使用  
#define EVLIST_ACTIVE 0x08 // event在激活链表中  
#define EVLIST_INTERNAL 0x10 // 内部使用标记  
#define EVLIST_INIT     0x80 // event已被初始化 


struct event_base; //struct event_base 定义一下

struct event {
	TAILQ_ENTRY (event) ev_next;
	TAILQ_ENTRY (event) ev_active_next;
	TAILQ_ENTRY (event) ev_signal_next;
	unsigned int min_heap_idx;	/* for managing timeouts */

	struct event_base *ev_base;

	int ev_fd;
	short ev_events;
	short ev_ncalls;
	short *ev_pncalls;	/* Allows deletes in callback */

	struct timeval ev_timeout;

	int ev_pri;		/* smaller numbers are higher priority */

	void (*ev_callback)(int, short, void *arg);
	void *ev_arg;

	int ev_res;		/* result passed to event callback */
	int ev_flags;
};

#define EVENT_SIGNAL(ev)	(int)(ev)->ev_fd
#define EVENT_FD(ev)		(int)(ev)->ev_fd

struct event_base *event_base_new(void);
struct event_base *event_init(void);
int event_dispatch(void);
int event_base_dispatch(struct event_base *);
void event_base_free(struct event_base *);

int event_base_set(struct event_base *, struct event *);
#ifdef __cplusplus
}
#endif
#endif
