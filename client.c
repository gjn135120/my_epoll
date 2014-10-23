#include "sysutil.h"
#include <sys/epoll.h>
void func(int sockfd);

int main(int argc, const char *argv[])
{
	int fd = tcp_client(0);

    connect_host(fd, "192.168.150.128", 9981);
    printf("%s\n", get_tcp_info(fd));
	func(fd);
	close(fd);
	return 0;
}


void func(int sockfd)
{
	char recvbuf[1024] = {0};
    char sendbuf[1024] = {0};

    while(1)
    {
        int efd = epoll_create(2);

        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = sockfd;
        if(epoll_ctl(efd, EPOLL_CTL_ADD, sockfd, &ev) == -1)
            ERR_EXIT("epoll_ctl");

        ev.events = EPOLLIN;
        ev.data.fd = STDIN_FILENO;
        if(epoll_ctl(efd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1)
            ERR_EXIT("epoll_ctl");

        struct epoll_event events[2];

        int ret = epoll_wait(efd, events, sizeof events, 5000);
        if(ret == -1)
        {
            if(errno == EINTR)
                continue;
            ERR_EXIT("epoll_wait");
        }
        if(ret == 0)
        {
            printf("timeout...\n");
            continue;
        }

        int i;
        for(i = 0; i < 2; ++ i)
        {
            if(events[i].events & EPOLLIN)
            {
                if(events[i].data.fd == sockfd)
                {
                    readline(sockfd, recvbuf, sizeof recvbuf);
                    printf("recv msg : %s", recvbuf);
                }

                if(events[i].data.fd == STDIN_FILENO)
                {
                    fgets(sendbuf, sizeof sendbuf, stdin);
                    writen(sockfd, sendbuf, strlen(sendbuf));
                }
            }
        }
        memset(sendbuf, 0, sizeof sendbuf);
        memset(recvbuf, 0, sizeof recvbuf);
    }
}
