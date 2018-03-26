/*************************************************************************
	> File Name: test.cpp
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Tue 10 May 2016 10:15:37 PM CST
 ************************************************************************/

#include<iostream>
#include<sys/signal.h>
#include<event.h>
using namespace std;




void timeout_cb(int fd,short event,void *argc)
{
  cout<<"timeout\n";
  struct timeval tv={1,0}; 
event_add((struct event*)argc,&tv);

}

int main()
{
      event_init();
      struct timeval tv={1,0};
      struct event ev;
      evtimer_set(&ev,timeout_cb,&ev);
      event_add(&ev,&tv);
      event_dispatch();



    return 0;
}

