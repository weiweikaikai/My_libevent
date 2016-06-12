/*************************************************************************
	> File Name: k_evsignal.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Thu 09 Jun 2016 03:30:03 PM CST
 ************************************************************************/

#ifndef _K_EVSIGNAL_H
#define _K_EVSIGNAL_H
#include"k_event-internal.h"

//typedef void (*ev_sighandler_t)(int);

struct evsignal_info {
	struct event ev_signal;
	int ev_signal_pair[2]; //用于通信的socketpair管道
	int ev_signal_added;//标志signal事件是否已经注册
	volatile sig_atomic_t evsignal_caught;//是否有信号发生的标记；是volatile类型，因为它会在另外的线程中被修改
	struct event_list evsigevents[NSIG]; //信号事件管理结构体数组
	sig_atomic_t evsigcaught[NSIG];//具体记录每个信号触发的次数，evsigcaught[signo]是记录信号signo被触发的次数
                                   //sig_atomic_t保证记录时候是原子性的
	struct sigaction **sh_old;    //记录原来signal处理函数指针，当信号注册的处理函数被清空，需要重新设置处理函数
	int sh_old_max;
};
int evsignal_init(struct event_base *);
void evsignal_process(struct event_base *);
int evsignal_add(struct event *);
int evsignal_del(struct event *);
void evsignal_dealloc(struct event_base *);

#endif
