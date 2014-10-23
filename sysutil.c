#include "sysutil.h"


ssize_t recv_peek(int fd, void *buf, size_t len);
//信号处理函数
void handle_sigpipe()
{
	if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		ERR_EXIT("signal");
}

//精确睡眠
void nano_sleep(double val)
{
	struct timespec tv;
	tv.tv_sec = val;
	tv.tv_nsec = (val - tv.tv_sec) * 1000 * 1000 * 1000;

	int ret;
	do
	{
		ret = nanosleep(&tv, &tv);
	}while(ret == -1 && errno == EINTR);
}

//发送和接受一个长度
void send_int32(int fd, int32_t val)
{
	int32_t tmp = htonl(val);
	if(writen(fd, &tmp, sizeof(int32_t)) != sizeof(int32_t))
		ERR_EXIT("send_int32");
}

int32_t recv_int32(int fd)
{
	int32_t tmp;
	int nread = readn(fd, &tmp, sizeof(int32_t));
	if(nread == 0)
		return 0;
	else if(nread != sizeof(int32_t))
		ERR_EXIT("recv_int32");

	return ntohl(tmp);
}

//读写的相关函数
ssize_t readn(int fd, void *buf, size_t len)
{
	size_t left = len;
	ssize_t nread;
	char *p = (char *)buf;

	while(left > 0)
	{
		nread = read(fd, p, left);
		if(nread == -1)
		{
			if(errno == EINTR)
				continue;
			return -1;
		}
		else if(nread == 0)
			break;

		left -= nread;
		p += nread;
	}

	return len - left;
}

ssize_t writen(int fd, const void *buf, size_t len)
{
	size_t left = len;
	ssize_t nwrite;
	const char *p =(const char *)buf;
	while(left > 0)
	{
		nwrite = write(fd, p, left);
		if(nwrite <= 0)
		{
			if(nwrite == -1 && errno == EINTR)
				continue;
			return -1;
		}

		left -= nwrite;
		p += nwrite;
	}

	return len;
}


ssize_t recv_peek(int fd, void *buf, size_t len)
{
	int nread;
	do
	{
		nread = recv(fd, buf, len, MSG_PEEK);
	}while(nread == -1 && errno == EINTR);

	return nread;
}

ssize_t readline(int fd, void *buf, size_t len)
{
	size_t left = len - 1;
	char *p = (char *)buf;
	size_t sum = 0;

	ssize_t nread;
	while(left > 0)
	{
		nread = recv_peek(fd, p, left);
		if(nread <= 0)
			return nread;

		int i;
		for(i = 0; i < nread; ++ i)
		{
			if(p[i] == '\n')
			{
				size_t size = i + 1;
				if(readn(fd, p, size) != size)
					return -1;

				p += size;
				sum += size;
				*p = 0;

				return sum;
			}

		}
		if(readn(fd, p, nread) != nread)
			return -1;

		p += nread;
		sum += nread;
		left -= nread;
	}

	*p = 0;
	return len - 1;
}

//这是一个reanline性能较慢的版本
ssize_t readline_slow(int fd, void *buf, size_t len)
{
    char *p = buf;  //记录缓冲区当前位置
    ssize_t nread;
    size_t left = len - 1;  //留一个位置给 '\0'
    char c;
    while(left > 0)
    {
        if((nread = read(fd, &c, 1)) < 0)
        {
            if(errno == EINTR)
                continue;
            return -1;
        }else if(nread == 0) // EOF
        {
            break;
        }

        //普通字符
        *p++ = c;
        left--;

        if(c == '\n')
            break;
    }
    *p = '\0';
    return (len - left - 1);
}

//TCP套接字的一些设置
void set_reuseaddr(int fd, int val)
{
	int on = (val != 0) ? 1 : 0;
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0)
		ERR_EXIT("set_reuseaddr");
}

void set_reuseport(int fd, int val)
{
#ifdef SO_REUSEPORT
	int on = (val != 0) ? 1 : 0;
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on)) < 0)
		ERR_EXIT("set_reuseport");
#else
	fprintf(stderr, "SO_REUSEPORT is not supported!!\n", );
#endif
}

void set_tcpnodelay(int fd, int val)
{
	int on = (val != 0) ? 1 : 0;
	if(setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) == -1)
		ERR_EXIT("set_tcpnodelay");
}

void set_keepalive(int fd, int val)
{
	int on = (val != 0) ? 1 : 0;
	if(setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on)) < 0)
		ERR_EXIT("set_keppalive");
}

//服务器与客户端套接字的获得与连接
int tcp_server(const char *host, uint16_t port)
{
	//处理sigpipe信号
	handle_sigpipe();

	int lfd = socket(PF_INET, SOCK_STREAM, 0);
	if(lfd == -1)
		ERR_EXIT("socket lfd");

	set_reuseaddr(lfd, 1);
	set_reuseport(lfd, 1);
	set_tcpnodelay(lfd, 0);
	set_keepalive(lfd, 0);

	SAI addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if(host == NULL)
		addr.sin_addr.s_addr = INADDR_ANY;
	else
	{
		if(inet_aton(host, &addr.sin_addr) == 0)
		{
			struct hostent *p = gethostbyname(host);
			if(p == NULL)
				ERR_EXIT("gethostbyname");
			addr.sin_addr = *(struct in_addr *)p->h_addr;
		}
	}

	if(bind(lfd, (SA *)&addr, sizeof addr) == -1)
		ERR_EXIT("bind lfd");

	if(listen(lfd, SOMAXCONN) == -1)
		ERR_EXIT("listen");

	return lfd;
}

int tcp_client(uint16_t port)
{
	int pfd = socket(PF_INET, SOCK_STREAM, 0);
	if(pfd == -1)
		ERR_EXIT("socket pfd");

    set_reuseaddr(pfd, 1);
    set_reuseport(pfd, 1);
    set_keepalive(pfd, 0);
    set_tcpnodelay(pfd, 0);

	if(port == 0)
		return pfd;

	SAI addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(get_local_ip());

	if(bind(pfd, (SA *)&addr, sizeof addr) == -1)
		ERR_EXIT("bind pfd");

	return pfd;
}

void connect_host(int fd, const char *host, uint16_t port)
{
	if(host == NULL)
	{
		fprintf(stderr, "host can not be NULL\n");
		exit(EXIT_FAILURE);
	}

	SAI addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);

	if(inet_aton(host, &addr.sin_addr) == 0)
	{
		struct hostent *p = gethostbyname(host);
		if(p == NULL)
			ERR_EXIT("gethostbyname");

		addr.sin_addr = *(struct in_addr *)p->h_addr;
	}

	if(connect(fd, (SA *)&addr, sizeof addr) == -1)
		ERR_EXIT("connect");
}


//获取点分十进制
const char *get_local_ip()
{
	static char ip[16] = {0};

	int fd;
	if((fd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
		ERR_EXIT("socket");

	struct ifreq req;
	bzero(&req, sizeof(struct ifreq));
	strcpy(req.ifr_name, "eth3");
	if(ioctl(fd, SIOCGIFADDR, &req) == -1)
		ERR_EXIT("iotcl");

	SAI *host = (SAI *)&req.ifr_addr;
	strcpy(ip, inet_ntoa(host->sin_addr));
	close(fd);

	return ip;
}

//获取服务器和客户端的addr
SAI get_peer_addr(int fd)
{
	SAI addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	socklen_t len = sizeof addr;
	if(getpeername(fd, (SA *)&addr, &len) == -1)
		ERR_EXIT("getpeername");

	return addr;
}

SAI get_local_addr(int fd)
{
	SAI addr;
	memset(&addr, 0, sizeof addr);
	addr.sin_family = AF_INET;
	socklen_t len = sizeof addr;
	if(getsockname(fd, (SA *)&addr, &len) == -1)
		ERR_EXIT("getsockname");

	return addr;
}

//从addr中获取相应的ip和port信息
const char *get_addr_ip(const SAI *addr)
{
	return inet_ntoa(addr->sin_addr);
}

uint16_t get_addr_port(const SAI *addr)
{
	return ntohs(addr->sin_port);
}

static __thread char info[100] = {0};
const char *get_tcp_info(int fd)
{

	SAI sfd = get_local_addr(fd);
	SAI pfd = get_peer_addr(fd);

	snprintf(info, sizeof(info), "%s : %d >> %s : %d",
	         get_addr_ip(&sfd),
	         get_addr_port(&sfd),
	         get_addr_ip(&pfd),
	         get_addr_port(&pfd));

	return info;
}

//将先发长度再发信息的信息发送机制封装
void send_msg(int fd, const void *buf, size_t len)
{
	send_int32(fd, len);
	if(writen(fd, buf, len) != len)
		ERR_EXIT("send_msg");
}

ssize_t recv_msg(int fd, void *buf, size_t bufsize)
{
	int32_t len = recv_int32(fd);
	if(len == 0)
		return 0;
	else if(len > (int32_t)bufsize)
	{
		fprintf(stderr, "bufsize is not enough\n");
		exit(EXIT_FAILURE);
	}

	ssize_t nread = readn(fd, buf, len);
	if(nread == -1)
		ERR_EXIT("recv_msg");
	else if((size_t)nread < len)
		return 0;

	return len;
}
