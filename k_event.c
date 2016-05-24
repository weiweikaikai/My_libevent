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

#include "event.h"
 

#ifdef HAVE_SELECT
extern const struct eventop selectops;
#endif
#ifdef HAVE_POLL
extern const struct eventop pollops;
#endif
#ifdef HAVE_EPOLL
extern const struct eventop epollops;
#endif

//Global state
struct event_base *current_base = NULL;
extern struct event_base *evsignal_base;
static int use_monotonic;

 
 struct event_base *
event_init(void)
{
	struct event_base *base = event_base_new();
	if (base != NULL)
		current_base = base;
	return (base);
}
struct event_base*
event_base_new(void)
{
  int i;
  struct event_base *base;
  if(base = calloc(1,)
}
 
 
int main()
{
    return 0;
}

