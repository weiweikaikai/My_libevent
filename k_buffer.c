/*************************************************************************
	> File Name: k_buffer.c
	> Author: wk
	> Mail: 18402927708@163.com
	> Created Time: Fri 17 Jun 2016 03:21:09 PM CST
 ************************************************************************/

#include <sys/types.h>
#include <sys/time.h>
#include <sys/ioctl.h>

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "k_event.h"
//#include "config.h"
#include "k_evutil.h"

#define SIZE_MAX ((size_t)-1)
#define EVBUFFER_MAX_READ	4096
#define SWAP(x,y) do { \
	(x)->buffer = (y)->buffer; \
	(x)->orig_buffer = (y)->orig_buffer; \
	(x)->misalign = (y)->misalign; \
	(x)->totallen = (y)->totallen; \
	(x)->off = (y)->off; \
} while (0)



struct evbuffer* evbuffer_new(void)
{
	struct evbuffer *buffer;
	buffer = calloc(1, sizeof(struct evbuffer));
	return (buffer);
}
void evbuffer_free(struct evbuffer *buffer)
{
	if (buffer->orig_buffer != NULL)
	{
		free(buffer->orig_buffer);
	}
	
	free(buffer);
}

int evbuffer_add_buffer(struct evbuffer *outbuf, struct evbuffer *inbuf)
{
	int res;

	/* Short cut for better performance */
	if (outbuf->off == 0) //缓冲区用完了
	{
		struct evbuffer tmp;
		size_t oldoff = inbuf->off;

		/* Swap them directly */
		SWAP(&tmp, outbuf);
		SWAP(outbuf, inbuf);
		SWAP(inbuf, &tmp);

		/* 
		 * 优化是由代价的 we need to notify the
		 * buffer if necessary of the changes. oldoff is the amount
		 * of data that we transfered from inbuf to outbuf
		 */
		if (inbuf->off != oldoff && inbuf->cb != NULL)
			(*inbuf->cb)(inbuf, oldoff, inbuf->off, inbuf->cbarg);
		if (oldoff && outbuf->cb != NULL)
			(*outbuf->cb)(outbuf, 0, oldoff, outbuf->cbarg);
		
		return (0);
	}

	res = evbuffer_add(outbuf, inbuf->buffer, inbuf->off);
	if (res == 0) 
	{
		/* We drain the input buffer on success */
		evbuffer_drain(inbuf, inbuf->off);
	}

	return (res);
}

int evbuffer_add(struct evbuffer *buf, const void *data, size_t datlen)
{
    size_t used = buf->misalign + buf->off;//已经使用的+空闲的
	size_t oldoff = buf->off;

	if (buf->totallen - used < datlen)  //总共剩下的少了就扩大
	{
		if (evbuffer_expand(buf, datlen) == -1)
			return (-1);
	}

	memcpy(buf->buffer + buf->off, data, datlen);
	buf->off += datlen;

	if (datlen && buf->cb != NULL)
		(*buf->cb)(buf, oldoff, buf->off, buf->cbarg);

	return (0);

}

void evbuffer_align(struct evbuffer *buf)
{
	memmove(buf->orig_buffer, buf->buffer, buf->off);
	buf->buffer = buf->orig_buffer;
	buf->misalign = 0;
}


int evbuffer_expand(struct evbuffer *buf, size_t datlen)
{
    size_t used = buf->misalign + buf->off;
	size_t need;

	assert(buf->totallen >= used);

	/* 如果我们能适应所有的数据，那么我们就不需要做任何事情了  */
	if (buf->totallen - used >= datlen)
		return (0);
	/* 如果我们溢出来才能适应这么多的数据，我们不能做任何事 */
	if (datlen > SIZE_MAX - buf->off)
		return (-1);

	/*如果剩余的空间满足就是用剩余的空间，此时就可以了 */
	if (buf->totallen - buf->off >= datlen)
	{
		evbuffer_align(buf);
	} 
	else 
	{
		void *newbuf;
		size_t length = buf->totallen;
		size_t need = buf->off + datlen;

		if (length < 256)
			length = 256;

		if (need < SIZE_MAX / 2)
		{
			while (length < need) 
			{
				length <<= 1;
			}
		} 
		else 
		{
			length = need;
		}

		if (buf->orig_buffer != buf->buffer)
			evbuffer_align(buf);
		if ((newbuf = realloc(buf->buffer, length)) == NULL)
			return (-1);

		buf->orig_buffer = buf->buffer = newbuf;
		buf->totallen = length;
	}

	return (0);
}

//从事件缓冲区中读取数据并读取所读取的字节数
int evbuffer_remove(struct evbuffer *buf, void *data, size_t datlen)
{
	size_t nread = datlen;
	if (nread >= buf->off)
		nread = buf->off;

	memcpy(data, buf->buffer, nread);
	evbuffer_drain(buf, nread);
	
	return (nread);
}

void evbuffer_drain(struct evbuffer *buf, size_t len)
{
	size_t oldoff = buf->off;
	if (len >= buf->off)
	{
		buf->off = 0;
		buf->buffer = buf->orig_buffer;
		buf->misalign = 0;
		goto done;
	}

	buf->buffer += len;
	buf->misalign += len;

	buf->off -= len;

 done:
	/* Tell someone about changes in this buffer */
	if (buf->off != oldoff && buf->cb != NULL)
		(*buf->cb)(buf, oldoff, buf->off, buf->cbarg);
}

char * evbuffer_readline(struct evbuffer *buffer)
{
	u_char *data = EVBUFFER_DATA(buffer);
	size_t len = EVBUFFER_LENGTH(buffer);
	char *line;
	unsigned int i;

	for (i = 0; i < len; i++)
	{
		if (data[i] == '\r' || data[i] == '\n')
			break;
	}

	if (i == len)
		return (NULL);

	if ((line = malloc(i + 1)) == NULL) 
	{
		fprintf(stderr, "%s: out of memory\n", __func__);
		return (NULL);
	}

	memcpy(line, data, i);
	line[i] = '\0';

	/* Some protocols terminate a line with '\r\n', so check for that, too.*/
	if ( i < len - 1 ) //说明遇到第一个\r的时候break的
	{
		char fch = data[i], sch = data[i+1];

		/* Drain one more character if needed 如果需要消耗一个更多的字符*/
		if ( (sch == '\r' || sch == '\n') && sch != fch )
			i += 1;
	}

	evbuffer_drain(buffer, i + 1);

	return (line);
}

int evbuffer_read(struct evbuffer *buf, int fd, int howmuch)
{
	unsigned char *p;
	size_t oldoff = buf->off;
	int n = EVBUFFER_MAX_READ;

	if (ioctl(fd, FIONREAD, &n) == -1 || n <= 0)// 获取接收缓存区中的字节数
	{
		n = EVBUFFER_MAX_READ;
	} 
	else if (n > EVBUFFER_MAX_READ && n > howmuch) 
	{/*大量的数据可能会被读，在这之前我们为了不浪费空间应该人为的限制一下*/
		if ((size_t)n > buf->totallen << 2)
		{
			n = buf->totallen << 2;
		}
		if (n < EVBUFFER_MAX_READ)
		{
			n = EVBUFFER_MAX_READ;
		}
	}	
	if (howmuch < 0 || howmuch > n)
		howmuch = n;

	/* If we don't have FIONREAD, we might waste some space here
	   如果没有FIONREAD 我们就会扩容到 EVBUFFER_MAX_READ 大小 所以我们会浪费空间*/
	if (evbuffer_expand(buf, howmuch) == -1)
		return (-1);

	/* We can append new data at this point */
	p = buf->buffer + buf->off;
	n = recv(fd, p, howmuch, 0);
	if (n == -1)
		return (-1);
	if (n == 0)
		return (0);

	buf->off += n; //空闲空间增加了

	/* Tell someone about changes in this buffer */
	if (buf->off != oldoff && buf->cb != NULL)
		(*buf->cb)(buf, oldoff, buf->off, buf->cbarg);

	return (n);
}

int evbuffer_write(struct evbuffer *buffer, int fd)
{
	int n;
	n = send(fd, buffer->buffer, buffer->off, 0);

	if (n == -1)
		return (-1);
	if (n == 0)
		return (0);
	evbuffer_drain(buffer, n);

	return (n);
}

void evbuffer_setcb(struct evbuffer *buffer,
    void (*cb)(struct evbuffer *, size_t, size_t, void *),
    void *cbarg)
{
	buffer->cb = cb;
	buffer->cbarg = cbarg;
}
