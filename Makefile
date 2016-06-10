libevent:test.c k_epoll.c k_event.c  k_signal.c k_evutil.c 
	gcc -o $@ $^ -g -I./
.PHONY:clean
clean:
	rm -rf libevent




