/*************************************************************************
	> File Name: k_min_heap.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Thu 09 Jun 2016 03:31:06 PM CST
 ************************************************************************/

#ifndef _K_MIN_HEAP_H
#define _K_MIN_HEAP_H
#include"k_event.h"
#include"k_evutil.h"
#include<stdlib.h>

//原来版本这里的命名有些不好这里改正一下
//原来结构体中//n是使用量  a是容量 想了解的可以看源码
typedef struct min_heap
{
    struct event** pevent;  //事件指针的数组
    unsigned int capacity;  //数组容量
    unsigned int size;     //已经使用的大小
} min_heap_t;

/*
C语言中使用static只是限制使用范围在本文件。
inline(不是C++的专利)这个关键词会给编译器这样一种建议：不要进行实际的函数调用
而只是将这段代码插入到进行调用的地方。对于比较短小的函数而言，
两种不同的处理方式对于频繁调用会带来比较大的性能差别。但是，
这个关键词对于编译器来说仅仅只是一种“hint”，
也即提示。编译器有可能会忽略它，并且即使在
没有加关键词inline的情况下也会对某些函数进行“inline”。
*/

static inline void           min_heap_ctor(min_heap_t* pmin_heap); //初始化小根堆管理结构体
static inline void           min_heap_dtor(min_heap_t* pmin_heap); //通过二叉堆管理结构体中释放小根堆节点
static inline void           min_heap_elem_init(struct event* e);//初始化时间在小根堆中的索引
static inline int            min_heap_elem_greater(struct event *a, struct event *b);
static inline int            min_heap_empty(min_heap_t* pmin_heap);
static inline unsigned       min_heap_size(min_heap_t* pmin_heap);
static inline struct event*  min_heap_top(min_heap_t* pmin_heap);
static inline int            min_heap_reserve(min_heap_t* pmin_heap, unsigned new_capacity);
static inline int            min_heap_push(min_heap_t* pmin_heap, struct event* e);
static inline struct event*  min_heap_pop(min_heap_t* pmin_heap);
static inline int            min_heap_erase(min_heap_t* pmin_heap, struct event* e);
static inline void           min_heap_shift_up_(min_heap_t* pmin_heap,unsigned int hole_index, struct event* e);
static inline void           min_heap_shift_down_(min_heap_t* pmin_heap, unsigned hole_index, struct event* e);




void min_heap_ctor(min_heap_t* pmin_heap) 
{ 
	pmin_heap->pevent = 0; 
	pmin_heap->capacity = 0; 
	pmin_heap->size = 0; 
}
void min_heap_dtor(min_heap_t* pmin_heap)
{ 
	if(pmin_heap->pevent) 
	{
	  free(pmin_heap->pevent);
	}
}
void min_heap_elem_init(struct event* e) 
{ 
	e->min_heap_idx = -1; 
}

int min_heap_reserve(min_heap_t* pmin_heap, unsigned new_capacity)
{
    if(pmin_heap->capacity < new_capacity)
    {
        struct event** pev;
        unsigned tmp =pmin_heap->capacity  ? pmin_heap->capacity  * 2 : 8; //容量如果不为0就增长两倍否则默认是8
        if(tmp < new_capacity) //如果原来的容量上边增长的还小于新的要申请的容量就直接申请新申请的大小
			{
				tmp = new_capacity;
			}
        if(!(pev = (struct event**)realloc(pmin_heap->pevent, tmp * (sizeof(*pev)) )))
		    {
                return -1;
		    }
        pmin_heap->pevent = pev;
        pmin_heap->capacity = tmp;
    }
    return 0;
}
unsigned min_heap_size(min_heap_t* pmin_heap)
{  
	return pmin_heap->size; 
}
int min_heap_push(min_heap_t* pmin_heap, struct event* e)
{
	//内存不够就重新分配
    if(min_heap_reserve(pmin_heap, pmin_heap->size + 1))
	   {
          return -1;
	   }
    min_heap_shift_up_(pmin_heap, pmin_heap->size++, e);
    return 0;
}

int min_heap_elem_greater(struct event *a, struct event *b)
{
    return evutil_timercmp(&a->ev_timeout, &b->ev_timeout, >);
}
void min_heap_shift_up_(min_heap_t* pmin_heap, unsigned int hole_index, struct event* e)
{
    unsigned parent = (hole_index - 1) / 2;
    while(hole_index && min_heap_elem_greater(pmin_heap->pevent[parent], e))
    {
        (pmin_heap->pevent[hole_index] = pmin_heap->pevent[parent])->min_heap_idx = hole_index;
        hole_index = parent;
        parent = (hole_index - 1) / 2;
    }
    (pmin_heap->pevent[hole_index] = e)->min_heap_idx = hole_index;
}

void min_heap_shift_down_(min_heap_t* pmin_heap, unsigned hole_index, struct event* e)
{
    unsigned min_child = 2 * (hole_index + 1);
    while(min_child <= pmin_heap->size)
	{
        min_child -= min_child == pmin_heap->size || min_heap_elem_greater(pmin_heap->pevent[min_child], pmin_heap->pevent[min_child - 1]);
        if(!(min_heap_elem_greater(e, pmin_heap->pevent[min_child])))
		  {
            break;
		  }
        (pmin_heap->pevent[hole_index] = pmin_heap->pevent[min_child])->min_heap_idx = hole_index;
        hole_index = min_child;
        min_child = 2 * (hole_index + 1);
	}
        min_heap_shift_up_(pmin_heap, hole_index,  e);
}


int min_heap_erase(min_heap_t* pmin_heap, struct event* e)
{
         if(((unsigned int)-1) != e->min_heap_idx)
         {
             struct event *last = pmin_heap->pevent[--pmin_heap->size];
             unsigned parent = (e->min_heap_idx - 1) / 2;
		    if (e->min_heap_idx > 0 && min_heap_elem_greater(pmin_heap->pevent[parent], last))
			   {
                  min_heap_shift_up_(pmin_heap, e->min_heap_idx, last);
			   }
           else
			 {
                  min_heap_shift_down_(pmin_heap, e->min_heap_idx, last);
			 }

             e->min_heap_idx = -1;
             return 0;
         }
    return -1;
}

int min_heap_empty(min_heap_t* pmin_heap)
{
	return 0u == pmin_heap->size; //无符号零是否恒等于小根堆已经使用的容量
}

struct event* min_heap_top(min_heap_t* pmin_heap)//返回小根堆的根节点,也就是最小的那一个
{ 
	return pmin_heap->size ? *pmin_heap->pevent : 0; 
	//return pmin_heap->size ? pmin_heap->pevent[0] : 0; 
}

#endif
