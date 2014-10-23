#ifndef EPOLL_H
#define EPOLL_H
#include <sys/epoll.h>
#define ERR_EXIT(m) \
		do { \
			perror(m);\
			exit(EXIT_FAILURE);\
		}while(0)

typedef void (*callback_t)(int, char *, size_t );

typedef struct
{
	int _efd;
	int _lfd;
	struct epoll_event _events[1024];
	int _ret;
	callback_t _callback;
}epoll_t;

void epoll_init(epoll_t *p, int lfd, callback_t callback);
void epoll_loop(epoll_t *p);
void epoll_handle(epoll_t *p);
void epoll_close(epoll_t *p);
#endif  /*EPOLL_H*/
