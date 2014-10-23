#ifndef POLL_H
#define POLL_H
#include <poll.h>

#define ERR_EXIT(m) \
		do { \
			perror(m);\
			exit(EXIT_FAILURE);\
		}while(0)

typedef struct
{
	struct pollfd _fds[1024];
	int _lfd;
	int _max;
	int _ret;
	void (*_callback) (int, char *);
}poll_t;

void poll_init(poll_t *p, int lfd, void(*hand)(int, char *));
void poll_wait(poll_t *p);
void poll_accept(poll_t *p);
void poll_data(poll_t *p);


#endif  /*POLL_H*/
