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

//#include "k_event-config.h"
#include"k_evutil.h"
#include"k_queue.h"


#include <sys/types.h>
#include <sys/time.h>


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

#define EVLIST_ALL	(0xf000 | 0x9f)

//事件类型
#define EV_TIMEOUT	0x01 //超时事件
#define EV_READ		0x02 //读事件
#define EV_WRITE	0x04 //写时间
#define EV_SIGNAL	0x08 //信号事件
#define EV_PERSIST	0x10	/* Persistant event  辅助选项 永久事件 不指定这个属性的话，回调函数被触发后事件会被删除*/ 

//#define TAILQ_ENTRY(type)						\
//struct {								\
//	struct type *tqe_next;	/* next element */			\
//	struct type **tqe_prev;	/* address of previous next element */	\
//}

TAILQ_HEAD (event_list, event); //双向链表的头节点

struct event_base; //struct event_base 定义一下

struct event {
	TAILQ_ENTRY (event) ev_next; //标识IO事件在双向链表(已注册事件链表)中的位置
	TAILQ_ENTRY (event) ev_active_next;//libevent将所有激活的事件放到双向链表active list中然后遍历链表进行调度
	TAILQ_ENTRY (event) ev_signal_next;//标识signal事件在双向链表(已注册事件链表)中的位置
	unsigned int min_heap_idx;	/* for managing timeouts */
    //对于定时事件 min_head_idx是小根堆中的索引，ev_timeout是事件超时值
	struct event_base *ev_base;

	int ev_fd;//对于IO事件绑定文件描述符，对于signal事件绑定的是信号
	short ev_events;//事件类型
	short ev_ncalls;//记录当事件就绪时候ev_callback被执行的次数 通常是1
	short *ev_pncalls;	/* Allows deletes in callback */

	struct timeval ev_timeout; //ev_timeout是事件超时值

	int ev_pri;		/* smaller numbers are higher priority  事件优先级*/

	void (*ev_callback)(int, short, void *arg);//函数指针，event的回调函数，被ev_base 也就是反应堆实例进行调用执行事件处理函数
	void *ev_arg;//设置event的回调函数时候传入的参数，void表明可以是任意类型的数据

	int ev_res;		/* result passed to event callback */  //返回的事件是什么状态，是超时还是其他
	int ev_flags;//用于标记当前event处于什么状态 比如：在time堆中， 在注册事件链表中，在激活链表中
};

#define EVENT_SIGNAL(ev)	(int)(ev)->ev_fd
#define EVENT_FD(ev)		(int)(ev)->ev_fd

struct event_base *event_base_new(void);

struct event_base *event_init(void);

int event_base_loop(struct event_base *, int);

int event_base_dispatch(struct event_base *);

void event_base_free(struct event_base *);

int event_base_set(struct event_base *, struct event *);

int	event_base_priority_init(struct event_base *, int);

int	event_priority_set(struct event *, int);

#define evtimer_set(ev, cb, arg)	event_set(ev, -1, 0, cb, arg)
#define signal_set(ev, x, cb, arg)	\
	event_set(ev, x, EV_SIGNAL|EV_PERSIST, cb, arg)
void event_set(struct event *, int, short, void (*)(int, short, void *), void *);
/*
event_loop() flags
*/
#define EVLOOP_ONCE	0x01	/**< Block at most once. */
#define EVLOOP_NONBLOCK	0x02	/**< Do not block. */
int event_loop(int);

#define evtimer_add(ev, tv)		event_add(ev, tv)
#define signal_add(ev, tv)		event_add(ev, tv)
int event_add(struct event *ev, const struct timeval *timeout);

#define evtimer_del(ev)			event_del(ev)
#define signal_del(ev)			event_del(ev)
int event_del(struct event *);

void event_active(struct event *, int, short);
int event_dispatch(void);
int event_pending(struct event *ev, short event, struct timeval *tv);

//定义缓冲区
struct evbuffer 
{
   unsigned char *buffer;//现在的buffer指针
   unsigned char *orig_buffer;//原来的buffer指针
    size_t misalign;//buffer已经被使用的字节长度
	size_t totallen; //buffer总的字节长度
	size_t off; //buffer未被使用的字节长度

	void (*cb)(struct evbuffer *, size_t, size_t, void *);
	void *cbarg;
};
/* Just for error reporting - use other constants otherwise */
#define EVBUFFER_READ		0x01
#define EVBUFFER_WRITE		0x02
#define EVBUFFER_EOF		0x10
#define EVBUFFER_ERROR		0x20
#define EVBUFFER_TIMEOUT	0x40

struct bufferevent;
typedef void (*evbuffercb)(struct bufferevent *, void *);
typedef void (*everrorcb)(struct bufferevent *, short what, void *);

struct event_watermark 
{
	size_t low;
	size_t high;
};


//缓冲区管理器
struct bufferevent 
{
	struct event_base *ev_base;

	struct event ev_read;
	struct event ev_write;

	struct evbuffer *input;
	struct evbuffer *output;

	struct event_watermark wm_read;
	struct event_watermark wm_write;

	evbuffercb readcb;
	evbuffercb writecb;
	everrorcb errorcb;
	void *cbarg;

	int timeout_read;	/* 读事件的超时时间 */
	int timeout_write;	/* 写事件的超时时间 */

	short enabled;	/* 当前已启用的事件*/
};





#define EVBUFFER_LENGTH(x)	(x)->off
#define EVBUFFER_DATA(x)	(x)->buffer
#define EVBUFFER_INPUT(x)	(x)->input
#define EVBUFFER_OUTPUT(x)	(x)->output

//k_evbuffer.c中

struct bufferevent *bufferevent_new(int fd,
    evbuffercb readcb, evbuffercb writecb, everrorcb errorcb, void *cbarg);

int bufferevent_base_set(struct event_base *base, struct bufferevent *bufev);

int bufferevent_priority_set(struct bufferevent *bufev, int pri);

void bufferevent_free(struct bufferevent *bufev);

void bufferevent_setcb(struct bufferevent *bufev,
    evbuffercb readcb, evbuffercb writecb, everrorcb errorcb, void *cbarg);

void bufferevent_setfd(struct bufferevent *bufev, int fd);

int bufferevent_write(struct bufferevent *bufev,
    const void *data, size_t size);
int bufferevent_write_buffer(struct bufferevent *bufev, struct evbuffer *buf);

size_t bufferevent_read(struct bufferevent *bufev, void *data, size_t size);

int bufferevent_enable(struct bufferevent *bufev, short event);

int bufferevent_disable(struct bufferevent *bufev, short event);

void bufferevent_settimeout(struct bufferevent *bufev,int timeout_read, int timeout_write);

void bufferevent_setwatermark(struct bufferevent *bufev, short events,size_t lowmark, size_t highmark);

//evbuffer.c中
 struct evbuffer* evbuffer_new(void);
 
 void evbuffer_free(struct evbuffer *buffer);
 
 int evbuffer_add_buffer(struct evbuffer *outbuf, struct evbuffer *inbuf);
 
 int evbuffer_add(struct evbuffer *buf, const void *data, size_t datlen);
 
 int evbuffer_expand(struct evbuffer *buf, size_t datlen);
 
 void evbuffer_align(struct evbuffer *buf);
 
 int evbuffer_remove(struct evbuffer *buf, void *data, size_t datlen);
 
 void evbuffer_drain(struct evbuffer *buf, size_t len);
 
 char * evbuffer_readline(struct evbuffer *buffer);
 
 int evbuffer_read(struct evbuffer *buf, int fd, int howmuch);
 
 int evbuffer_write(struct evbuffer *buffer, int fd);


#ifdef __cplusplus
}
#endif
#endif
