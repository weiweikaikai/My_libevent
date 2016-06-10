/*************************************************************************
	> File Name: k_queue.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Fri 10 Jun 2016 07:48:30 PM CST
 ************************************************************************/

#ifndef _K_QUEUE_H
#define _K_QUEUE_H


//使用宏定义构建结构体用于进行双向链表的管理(算是管理节点了)
#define TAILQ_HEAD(name, type)						\
struct name {								\
	struct type *tqh_first;	/*链表第一个节点的地址*/			\
	struct type **tqh_last;	/*链表最后一个节点的next指针的地址 也就是 *tqh_last = next  */		\
}

//和前面的TAILQ_HEAD不同，这里的结构体并没有name.即没有结构体名。  
//所以该结构体只能作为一个匿名结构体。所以，它一般都是另外一个结构体  
//或者共用体的成员 

//双向链表节点
#define TAILQ_ENTRY(type)						\
struct {								\
	struct type *tqe_next;	/*下一个节点的地址*/			\
	struct type **tqe_prev;	/*上一个节点的next指针的地址 其实就是 *tqe_prev就是自己节点本身的地址 */	\
}

//初始化链表管理结构体
#define	TAILQ_INIT(head) do {						\
	(head)->tqh_first = NULL;					\
	(head)->tqh_last = &(head)->tqh_first;				\
} while (0)


#endif
