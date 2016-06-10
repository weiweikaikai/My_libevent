/*************************************************************************
	> File Name: k_evsignal.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Thu 09 Jun 2016 03:30:03 PM CST
 ************************************************************************/

#ifndef _K_EVSIGNAL_H
#define _K_EVSIGNAL_H
#include"k_event-internal.h"

typedef void (*ev_sighandler_t)(int);

struct evsignal_info {
	struct event ev_signal;
	int ev_signal_pair[2];
	int ev_signal_added;
	volatile sig_atomic_t evsignal_caught;
	struct event_list evsigevents[NSIG];
	sig_atomic_t evsigcaught[NSIG];
#ifdef HAVE_SIGACTION
	struct sigaction **sh_old;
#else
	ev_sighandler_t **sh_old;
#endif
	int sh_old_max;
};
int evsignal_init(struct event_base *);
void evsignal_process(struct event_base *);
int evsignal_add(struct event *);
int evsignal_del(struct event *);
void evsignal_dealloc(struct event_base *);

#endif
