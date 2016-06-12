libevent:test.c k_epoll.c k_event.c  k_signal.c k_evutil.c k_poll.c k_select.c
	gcc -o $@ $^ -g -I./
.PHONY:clean
clean:
	rm -rf libevent




