/*************************************************************************
	> File Name: k_poll.c
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 11 Jun 2016 01:06:25 AM CST
 ************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include "k_queue.h"
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "k_event.h"
#include "k_event-internal.h"
#include "k_evsignal.h"

struct pollop {
	int event_count;		/* Highest number alloc */
	int nfds;                       /* Size of event_* */
	int fd_count;                   /* Size of idxplus1_by_fd */
	struct pollfd *event_set;
	struct event **event_r_back;
	struct event **event_w_back;
	int *idxplus1_by_fd; /* Index into event_set by fd; we add 1 so
			      * that 0 (which is easy to memset) can mean
			      * "no entry." */
};

static void *poll_init	(struct event_base *);
static int poll_add		(void *, struct event *);
static int poll_del		(void *, struct event *);
static int poll_dispatch	(struct event_base *, void *, struct timeval *);
static void poll_dealloc	(struct event_base *, void *);

const struct eventop pollops = {
	"poll",
	poll_init,
	poll_add,
	poll_del,
	poll_dispatch,
	poll_dealloc,
    0
};