#include "epoll.h"
#include "sysutil.h"

void epoll_add(epoll_t *p, int fd);
void epoll_del(epoll_t *p, int fd);
void epoll_accept(epoll_t *p);
void epoll_data(epoll_t *p, int fd);


void epoll_init(epoll_t *p, int lfd, callback_t callback)
{
	if((p->_efd = epoll_create(1024)) == -1)
		ERR_EXIT("epoll_create");

	p->_lfd = lfd;
	memset(&p->_events, 0, sizeof(p->_events));
	p->_callback = callback;

	epoll_add(p, lfd);
}

void epoll_close(epoll_t *p)
{
	close(p->_efd);
}


void epoll_loop(epoll_t *p)
{
	int ret;
	while(1)
	{
		ret = epoll_wait(p->_efd, p->_events, sizeof(p->_events), 5000);
		if(ret == -1)
		{
			if(errno == EINTR)
				continue;
			ERR_EXIT("epoll_wait");
		}
		else if(ret == 0)
		{
			printf("timeout...\n");
			continue;
		}

		break;
	}
	p->_ret = ret;
}


void epoll_handle(epoll_t *p)
{
	int i;
	for(i = 0; i < p->_ret; ++ i)
	{
		if(p->_events[i].events & EPOLLIN)
		{
			int fd = p->_events[i].data.fd;
			if(fd == p->_lfd)
				epoll_accept(p);
			else
				epoll_data(p, fd);
		}
	}
}

void epoll_accept(epoll_t *p)
{
	int pfd = accept(p->_lfd, NULL, NULL);
	if(pfd == -1)
		ERR_EXIT("accept");
	epoll_add(p, pfd);
	printf("%s\n", get_tcp_info(pfd));
}


void epoll_data(epoll_t *p, int fd)
{
	char buf[1024] = {0};
	size_t ret = recv_msg(fd, buf, sizeof buf);
	if(ret == 0)
	{
		printf("client close...\n");
		epoll_del(p, fd);
		close(fd);
		return;
	}

	p->_callback(fd, buf, ret);
}


void epoll_add(epoll_t *p, int fd)
{
	struct epoll_event ev;
	ev.data.fd = fd;
	ev.events = EPOLLIN;
	if(epoll_ctl(p->_efd, EPOLL_CTL_ADD, fd, &ev) == -1)
		ERR_EXIT("EPOLL_CTL_ADD");
}

void epoll_del(epoll_t *p, int fd)
{
	struct epoll_event ev;
	ev.data.fd = fd;
	if(epoll_ctl(p->_efd, EPOLL_CTL_DEL, fd, &ev) == -1)
		ERR_EXIT("EPOLL_CTL_DEL");
}
