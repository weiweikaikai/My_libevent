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

typedef struct min_heap
{
    struct event** p;  //事件指针的数组
    unsigned n, a; //n是容量  a是当前的使用量
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

static inline void           min_heap_ctor(min_heap_t* s);
static inline void           min_heap_dtor(min_heap_t* s);
static inline void           min_heap_elem_init(struct event* e);




void min_heap_ctor(min_heap_t* s) { s->p = 0; s->n = 0; s->a = 0; }
void min_heap_dtor(min_heap_t* s) { if(s->p) free(s->p); }
void min_heap_elem_init(struct event* e) { e->min_heap_idx = -1; }

#endif
