#include "poll_t.h"
#include "sysutil.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>

void poll_add(poll_t *p, int fd);
void poll_del(poll_t *p, int i);



void poll_init(poll_t *p, int lfd, void(*hand)(int, char *))
{
	int i;
	for(i = 0; i != 1024; ++ i)
		p->_fds[i].fd = -1;
	p->_lfd = lfd;
	p->_fds[0].fd = lfd;
	p->_fds[0].events = POLLIN;

	p->_max = 0;
	p->_ret = 0;
	p->_callback = hand;
}


void poll_wait(poll_t *p)
{
	int ret;
	do
	{
		ret = poll(p->_fds, p->_max + 1, 5000);
	}while(ret == -1 && errno == EINTR);

	if(ret == -1)
		ERR_EXIT("poll");

	p->_ret = ret;
}


void poll_accept(poll_t *p)
{
	if(p->_fds[0].revents & POLLIN)
	{
		int pfd = accept(p->_lfd, NULL, NULL);
		if(pfd == -1)
			ERR_EXIT("accept");

		poll_add(p, pfd);
	}
}


void poll_data(poll_t *p)
{
	int i;
	char buf[1024];
	for(i = 1; i <= p->_max; ++ i)
	{
		int fd = p->_fds[i].fd;
		if(fd == -1)
			continue;
		if(p->_fds[i].revents & POLLIN)
		{
			int nread = recv_msg(fd, buf, sizeof buf);
			if(nread == -1)
				ERR_EXIT("readline");
			else if(nread == 0)
			{
				printf("client close...\n");
				poll_del(p, i);
				continue;
			}

			p->_callback(fd, buf);
		}
	}
}



void poll_add(poll_t *p, int fd)
{
	int i;
	for(i = 1; i != 1024; ++ i)
	{
		if(p->_fds[i].fd == -1)
		{
			p->_fds[i].fd = fd;
			p->_fds[i].events = POLLIN;
			if(i > p->_max)
				p->_max = i;

			break;
		}
	}

	if(i == 1024)
	{
		fprintf(stderr, "too mang clients!!\n");
		exit(EXIT_FAILURE);
	}
}

void poll_del(poll_t *p, int i)
{
	assert(i >= 1 && i < 1024);
	close(p->_fds[i].fd);
	p->_fds[i].fd = -1;
}
