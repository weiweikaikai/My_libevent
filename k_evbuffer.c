/*************************************************************************
	> File Name: k_evbuffer.c
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Fri 17 Jun 2016 03:21:17 PM CST
 ************************************************************************/

#include <sys/types.h>
//#include "k_config.h"
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "k_evutil.h"
#include "k_event.h"

/*
*创建一个新的缓冲事件对象。
*每当读取新数据时调用读取回调函数。
*当输出缓冲区耗尽时调用写回调函数。
*错误回调被调用，在读写错误或EOF。
*读写回调可能无效。错误回调不是
*允许为空，并必须提供始终。
*/

static int bufferevent_add(struct event *ev, int timeout);
static void bufferevent_readcb(int fd, short event, void *arg);
static void bufferevent_writecb(int fd, short event, void *arg);
static void bufferevent_read_pressure_cb(struct evbuffer *, size_t, size_t, void *);



int bufferevent_add(struct event *ev, int timeout)
{
	struct timeval tv, *ptv = NULL;

	if (timeout)
	{
		evutil_timerclear(&tv);
		tv.tv_sec = timeout;
		ptv = &tv;
	}

	return (event_add(ev, ptv));
}
void bufferevent_read_pressure_cb(struct evbuffer *buf, size_t old, size_t now,void *arg)
{
   struct bufferevent *bufev = arg;
	/* 
	 * If we are below the watermark then reschedule reading if it's
	 * still enabled.
	 */
	if (bufev->wm_read.high == 0 || now < bufev->wm_read.high) 
	{
		evbuffer_setcb(buf, NULL, NULL);

		if (bufev->enabled & EV_READ)
			bufferevent_add(&bufev->ev_read, bufev->timeout_read);
	}

}

 void bufferevent_readcb(int fd, short event, void *arg)
{
	struct bufferevent *bufev = arg;
	int res = 0;
	short what = EVBUFFER_READ;
	size_t len;
	int howmuch = -1;

	if (event == EV_TIMEOUT)
	{
		what |= EVBUFFER_TIMEOUT;
		goto error;
	}

	/*
	 * If we have a high watermark configured then we don't want to
	 * read more data than would make us reach the watermark.
	 */
	if (bufev->wm_read.high != 0) 
	{
		howmuch = bufev->wm_read.high - EVBUFFER_LENGTH(bufev->input);
		/* we might have lowered the watermark, stop reading */
		if (howmuch <= 0) //数据读取完毕
			{
			struct evbuffer *buf = bufev->input;
			event_del(&bufev->ev_read);
			evbuffer_setcb(buf,
			    bufferevent_read_pressure_cb, bufev);
			return;
		}
	}

	res = evbuffer_read(bufev->input, fd, howmuch);
	if (res == -1) 
	{
		if (errno == EAGAIN || errno == EINTR)
			goto reschedule;
		/* error case */
		what |= EVBUFFER_ERROR;
	} else if (res == 0) 
	  {
		/* eof case */
		what |= EVBUFFER_EOF;
	  }

	if (res <= 0)
		goto error;

	bufferevent_add(&bufev->ev_read, bufev->timeout_read);

	/* See if this callbacks meets the water marks */
	len = EVBUFFER_LENGTH(bufev->input);
	if (bufev->wm_read.low != 0 && len < bufev->wm_read.low) //输入数据小于读取的最低水位直接返回
		return;
	if (bufev->wm_read.high != 0 && len >= bufev->wm_read.high)//输入数据大于最高水位
	{
		struct evbuffer *buf = bufev->input;
		event_del(&bufev->ev_read);

		/* Now schedule a callback for us when the buffer changes */
		evbuffer_setcb(buf, bufferevent_read_pressure_cb, bufev);
	}

	/* Invoke the user callback - must always be called last */
	if (bufev->readcb != NULL)
		(*bufev->readcb)(bufev, bufev->cbarg);
	return;

 reschedule:
	bufferevent_add(&bufev->ev_read, bufev->timeout_read);
	return;

 error:
	(*bufev->errorcb)(bufev, what, bufev->cbarg);
}

void bufferevent_writecb(int fd, short event, void *arg)
{
	struct bufferevent *bufev = arg;
	int res = 0;
	short what = EVBUFFER_WRITE;

	if (event == EV_TIMEOUT)
	{
		what |= EVBUFFER_TIMEOUT;
		goto error;
	}

	if (EVBUFFER_LENGTH(bufev->output)) 
	{
	    res = evbuffer_write(bufev->output, fd);
	    if (res == -1) 
		{
			goto reschedule;
	    } else if (res == 0) 
			{
		    /* eof case */
		    what |= EVBUFFER_EOF;
	       }
	    if (res <= 0)
		    goto error;
	}

	if (EVBUFFER_LENGTH(bufev->output) != 0)
		bufferevent_add(&bufev->ev_write, bufev->timeout_write);

	/*
	 * Invoke the user callback if our buffer is drained or below the
	 * low watermark.
	 */
	if (bufev->writecb != NULL && EVBUFFER_LENGTH(bufev->output) <= bufev->wm_write.low)
		(*bufev->writecb)(bufev, bufev->cbarg);//缓冲区中没有空间可写就返回
	return;

 reschedule:
	if (EVBUFFER_LENGTH(bufev->output) != 0)
		bufferevent_add(&bufev->ev_write, bufev->timeout_write);
	return;

 error:
	(*bufev->errorcb)(bufev, what, bufev->cbarg);

}
struct bufferevent* bufferevent_new(int fd, evbuffercb readcb, evbuffercb writecb,
    everrorcb errorcb, void *cbarg)
	{
	   struct bufferevent *bufev;
	if ((bufev = calloc(1, sizeof(struct bufferevent))) == NULL)
		{
		    return (NULL);
		}

	if ((bufev->input = evbuffer_new()) == NULL) 
	{
		free(bufev);
		return (NULL);
	}

	if ((bufev->output = evbuffer_new()) == NULL) 
	{
		evbuffer_free(bufev->input);
		free(bufev);
		return (NULL);
	}

	event_set(&bufev->ev_read, fd, EV_READ, bufferevent_readcb, bufev);
	event_set(&bufev->ev_write, fd, EV_WRITE, bufferevent_writecb, bufev);

	bufferevent_setcb(bufev, readcb, writecb, errorcb, cbarg);

	/*
	 * Set to EV_WRITE so that using bufferevent_write is going to
	 * trigger a callback.  Reading needs to be explicitly enabled
	 * because otherwise no data will be available.
	 */
	bufev->enabled = EV_WRITE;

	return (bufev);
	}
void bufferevent_setcb(struct bufferevent *bufev,
    evbuffercb readcb, evbuffercb writecb, everrorcb errorcb, void *cbarg)
{
	bufev->readcb = readcb;
	bufev->writecb = writecb;
	bufev->errorcb = errorcb;

	bufev->cbarg = cbarg;
}
int bufferevent_enable(struct bufferevent *bufev, short event)
{
	if (event & EV_READ)
	{
		if (bufferevent_add(&bufev->ev_read, bufev->timeout_read) == -1)
			return (-1);
	}
	if (event & EV_WRITE)
	{
		if (bufferevent_add(&bufev->ev_write, bufev->timeout_write) == -1)
			return (-1);
	}

	bufev->enabled |= event;
	return (0);
}
int bufferevent_disable(struct bufferevent *bufev, short event)
{
	if (event & EV_READ) 
	{
		if (event_del(&bufev->ev_read) == -1)
			return (-1);
	}
	if (event & EV_WRITE) 
	{
		if (event_del(&bufev->ev_write) == -1)
			return (-1);
	}

	bufev->enabled &= ~event;
	return (0);
}

size_t bufferevent_read(struct bufferevent *bufev, void *data, size_t size)
{
   struct evbuffer *buf = bufev->input;

	if (buf->off < size)
		size = buf->off;
	/* 将数据拷贝到用户空间*/
	memcpy(data, buf->buffer, size);
	if (size)
	{
	   evbuffer_drain(buf, size);
	}
	return (size);
}

int bufferevent_write(struct bufferevent *bufev, const void *data, size_t size)
{
	int res;
	res = evbuffer_add(bufev->output, data, size);

	if (res == -1)
		return (res);

	/* If everything is okay, we need to schedule a write */
	if (size > 0 && (bufev->enabled & EV_WRITE))
		bufferevent_add(&bufev->ev_write, bufev->timeout_write);

	return (res);
}
int bufferevent_write_buffer(struct bufferevent *bufev, struct evbuffer *buf)
{
	int res;
	res = bufferevent_write(bufev, buf->buffer, buf->off);
	if (res != -1)
		evbuffer_drain(buf, buf->off);

	return (res);
}

void bufferevent_free(struct bufferevent *bufev)
{
	event_del(&bufev->ev_read);
	event_del(&bufev->ev_write);

	evbuffer_free(bufev->input);
	evbuffer_free(bufev->output);

	free(bufev);
}

void bufferevent_setfd(struct bufferevent *bufev, int fd)
{
	event_del(&bufev->ev_read);
	event_del(&bufev->ev_write);

	event_set(&bufev->ev_read, fd, EV_READ, bufferevent_readcb, bufev);
	event_set(&bufev->ev_write, fd, EV_WRITE, bufferevent_writecb, bufev);
	if (bufev->ev_base != NULL) 
	{
		event_base_set(bufev->ev_base, &bufev->ev_read);
		event_base_set(bufev->ev_base, &bufev->ev_write);
	}

	/*可能必须手动触发事件注册 n */
}

//手动设置事件的优先级
int bufferevent_priority_set(struct bufferevent *bufev, int priority)
{
	if (event_priority_set(&bufev->ev_read, priority) == -1) 
		return (-1);
	if (event_priority_set(&bufev->ev_write, priority) == -1)
		return (-1);

	return (0);
}

/*
 * 设置缓冲事件的读写超时时间.
 */
void bufferevent_settimeout(struct bufferevent *bufev,
    int timeout_read, int timeout_write)
{
	bufev->timeout_read = timeout_read;
	bufev->timeout_write = timeout_write;

	if (event_pending(&bufev->ev_read, EV_READ, NULL))
		bufferevent_add(&bufev->ev_read, timeout_read);
	if (event_pending(&bufev->ev_write, EV_WRITE, NULL))
		bufferevent_add(&bufev->ev_write, timeout_write);
}
void bufferevent_setwatermark(struct bufferevent *bufev, short events,
    size_t lowmark, size_t highmark)
{
	if (events & EV_READ)
	{
		bufev->wm_read.low = lowmark;
		bufev->wm_read.high = highmark;
	}

	if (events & EV_WRITE) 
	{
		bufev->wm_write.low = lowmark;
		bufev->wm_write.high = highmark;
	}

	/* If the watermarks changed then see if we should call read again */
	bufferevent_read_pressure_cb(bufev->input,
	    0, EVBUFFER_LENGTH(bufev->input), bufev);
}