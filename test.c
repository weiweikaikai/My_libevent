/*************************************************************************
	> File Name: test.c
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Thu 09 Jun 2016 03:00:38 PM CST
 ************************************************************************/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
//#include<event.h>
#include"k_event.h"
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>


void onTime(int sock,short event,void *arg)
{
       printf("hello libevent\n");  
      
        struct timeval tv;  
        tv.tv_sec = 10;  
        tv.tv_usec = 0;  

        // 重新添加定时事件（定时事件触发后默认自动删除）  
        event_add((struct event*)arg, &tv);   
}

//超时事件测试
void timer_test()
{
     // 初始化  
        event_init();  
       struct event evTime;  
      // 设置定时事件  
        evtimer_set(&evTime, onTime, &evTime);  
        struct timeval tv;  
        tv.tv_sec = 10;  
        tv.tv_usec = 0;  
      // 添加定时事件  
        event_add(&evTime, &tv);  
        // 事件调度循环  
       event_dispatch();  

}
  struct event_base* base;

  // 读事件回调函数  
    void onRead(int fd, short iEvent, void *arg)  
    {  
        int len;  
        char buf[1500];  
      
        len = recv(fd, buf, 1500, 0);  
        if (len <= 0)
		{  
            printf("Client Close");
            // 连接结束(=0)或连接错误(<0)，将事件删除并释放内存空间  
            struct event *pEvRead = (struct event*)arg;  
            event_del(pEvRead);  
            free(pEvRead);  
			pEvRead=NULL;
   
            close(fd);  
            return;  
        }  
      
        buf[len] = 0;  
        printf("Client Info: %s\n",buf);  
    }  


 // 连接请求事件回调函数  
    void onAccept(int fd, short iEvent, void *arg)  
    {  
        int connfd;  
        struct sockaddr_in cliAddr;  
      
        socklen_t size = sizeof(cliAddr);  
        connfd = accept(fd, (struct sockaddr*)&cliAddr, &size);  
           if(connfd < 0)
		{
		   return;  
		 }
		 printf("new client online\n");
        // 连接注册为新事件 (EV_PERSIST为事件触发后不默认删除)  
        struct event *pEvRead = (struct event*)malloc(sizeof(struct event));  
        event_set(pEvRead, connfd, EV_READ|EV_PERSIST, onRead, pEvRead); 
		event_base_set(base, pEvRead); 
        event_add(pEvRead, NULL);  
    } 


 
void io_test()
{
        struct sockaddr_in serverAddr;  
        memset(&serverAddr, 0, sizeof(serverAddr));    
        serverAddr.sin_family = AF_INET;    
        serverAddr.sin_addr.s_addr = inet_addr("121.42.180.114");      
        serverAddr.sin_port = htons(8001);  

		int  listenfd = socket(AF_INET, SOCK_STREAM, 0);    
        bind(listenfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));    
        listen(listenfd, 10);  

        base=event_init();
          
        struct event evListen;  
        // 设置事件  
        event_set(&evListen, listenfd, EV_READ|EV_PERSIST, onAccept, NULL);  

		  // 设置为base事件  
        event_base_set(base, &evListen); 
        // 添加事件  
        event_add(&evListen, NULL);  
        
        // 事件循环  
        event_dispatch();  
      
}

int main()
{
    //超时事件测试
   //timer_test();
   //IO事件测试
   io_test();

    return 0;
}

