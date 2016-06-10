/*************************************************************************
	> File Name: test.c
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Thu 09 Jun 2016 03:00:38 PM CST
 ************************************************************************/

#include<stdio.h>
#include<k_event.h>

//void ontime(int sock,short event,void *arg)
//{
//       printf("hello\n");  
//      
//        struct timeval tv;  
//        tv.tv_sec = 1;  
//        tv.tv_usec = 0;  
//        // 重新添加定时事件（定时事件触发后默认自动删除）  
//        event_add((struct event*)arg, &tv);   
//}


int main()
{
      // 初始化  
        event_init();  
      
//        struct event evTime;  
//        // 设置定时事件  
//        evtimer_set(&evTime, onTime, &evTime);  
//      
//        struct timeval tv;  
//        tv.tv_sec = 1;  
//        tv.tv_usec = 0;  
//        // 添加定时事件  
//        event_add(&evTime, &tv);  
//      
//        // 事件循环  
//        event_dispatch();  
//      

    return 0;
}

