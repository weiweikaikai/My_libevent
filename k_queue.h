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


/*每次都是尾插元素
新插入元素的next指针域总是为空
新插入元素的prev指针是个二级指针 
链表的管理节点的last指针也是一个二级指针，
链表管理节点的last指针(二级指针)已经被初始化为自己first指针的地址
将链表管理节点的last指针(二级指针)赋值给新插入的节点的prev指针
链表的last指针简引用就是最后一个节点的next指针，next指针指向新插入的元素
链表管理节点的last指针(二级指针)被重新定义为新节点的next指针的地址
*/
#define TAILQ_INSERT_TAIL(head, elm, field) do {			\
	(elm)->field.tqe_next = NULL;					\
	(elm)->field.tqe_prev = (head)->tqh_last;			\
	*(head)->tqh_last = (elm);					\
	(head)->tqh_last = &(elm)->field.tqe_next;			\
} while (0)


#define TAILQ_REMOVE(head, elm, field) do {				\
	if (((elm)->field.tqe_next) != NULL)				\
		(elm)->field.tqe_next->field.tqe_prev =			\
		    (elm)->field.tqe_prev;				\
	else								\
		(head)->tqh_last = (elm)->field.tqe_prev;		\
	*(elm)->field.tqe_prev = (elm)->field.tqe_next;			\
} while (0)
#endif
