/*************************************************************************
	> File Name: k_select.c
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Sat 11 Jun 2016 01:00:36 AM CST
 ************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include "k_queue.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

#include "k_event.h"
#include "k_evutil.h"
#include "k_event-internal.h"
#include "k_evsignal.h"

#define        howmany(x, y)   (((x)+((y)-1))/(y))



struct selectop {
	int event_fds;		/* Highest fd in fd set */
	int event_fdsz;
	fd_set *event_readset_in;
	fd_set *event_writeset_in;
	fd_set *event_readset_out;
	fd_set *event_writeset_out;
	struct event **event_r_by_fd;
	struct event **event_w_by_fd;
};

static void *select_init	(struct event_base *);
static int select_add		(void *, struct event *);
static int select_del		(void *, struct event *);
static int select_dispatch	(struct event_base *, void *, struct timeval *);
static void select_dealloc     (struct event_base *, void *);

const struct eventop selectops = {
	"select",
	select_init,
	select_add,
	select_del,
	select_dispatch,
	select_dealloc,
	0
};
