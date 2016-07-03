/*************************************************************************
	> File Name: k_evutil.h
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Thu 09 Jun 2016 03:09:54 PM CST
 ************************************************************************/

#ifndef _K_EVUTIL_H
#define _K_EVUTIL_H

int evutil_socketpair(int family, int type, int protocol, int fd[2]);
int evutil_make_socket_nonblocking(int sock);
int evutil_make_socket_reuseable(int sock);

#define evutil_timeradd(tvp, uvp, vvp)/*最终函数返回的是将时间超时的未来时间点(这里是linux下使用的单调时间)存储在vvp中也就是event结构体中的ev_timeout中*/						\
	do {														\
		(vvp)->tv_sec = (tvp)->tv_sec + (uvp)->tv_sec; /*事件的超时时间秒数点等于现在时间秒数点加上超时时间秒数点*/			\
		(vvp)->tv_usec = (tvp)->tv_usec + (uvp)->tv_usec; /*事件的超时时间微妙数点等于现在时间微妙数点加上超时时间微妙数点*/	      \
		if ((vvp)->tv_usec >= 1000000) {	/* 如果微妙相加后产生微妙进位微微秒*/					\
			(vvp)->tv_sec++;									\
			(vvp)->tv_usec -= 1000000;							\
		}														\
	} while (0)

#define	evutil_timersub(tvp, uvp, vvp)						\
	do {													\
		(vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;		\
		(vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;	\
		if ((vvp)->tv_usec < 0) {							\
			(vvp)->tv_sec--;								\
			(vvp)->tv_usec += 1000000;						\
		}													\
	} while (0)


//秒相等比微妙是否相等否则比较秒是否相等
#define	evutil_timercmp(tvp, uvp, cmp)							\
	(((tvp)->tv_sec == (uvp)->tv_sec) ?							\
	 ((tvp)->tv_usec cmp (uvp)->tv_usec) :						\
	 ((tvp)->tv_sec cmp (uvp)->tv_sec))

#define evutil_timerclear(tvp) (tvp)->tv_sec = (tvp)->tv_usec = 0

#define EVUTIL_CLOSESOCKET(s) close(s)

#endif
