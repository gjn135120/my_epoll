#include "epoll.h"
#include "sysutil.h"

void hand(int fd, char *buf, size_t len)
{
	printf("recv msg : %d\n", len);
	send_msg(fd, buf, len);
}

int main(int argc, const char *argv[])
{

	int lfd = tcp_server("192.168.150.128", 8899);

	epoll_t *p = (epoll_t *)malloc(sizeof(epoll_t));
	epoll_init(p, lfd, &hand);
	while(1)
	{
		epoll_loop(p);
		epoll_handle(p);
	}

	epoll_close(p);
	free(p);

	close(lfd);
	return 0;
}
