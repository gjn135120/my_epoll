#ifndef SYSUTIL_H
#define SYSUTIL_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <signal.h>
#define ERR_EXIT(m) \
		do { \
			perror(m);\
			exit(EXIT_FAILURE);\
		}while(0)

extern int h_errno;

typedef struct sockaddr SA;
typedef struct sockaddr_in SAI;

void nano_sleep(double val);

ssize_t readn(int fd, void *buf, size_t len);
ssize_t writen(int fd, const void *buf, size_t len);
ssize_t readline(int fd, void *buf, size_t len);

void send_int32(int fd, int32_t val);
int32_t recv_int32(int fd);

void set_reuseaddr(int fd, int val);
void set_reuseport(int fd, int val);
void set_tcpnodelay(int fd, int val);
void set_keppalive(int fd, int val);

int tcp_server(const char *host, uint16_t port);
int tcp_client(uint16_t port);
void connect_host(int fd, const char *host, uint16_t port);

const char *get_local_ip();

SAI get_peer_addr(int fd);
SAI get_local_addr(int fd);

const char *get_addr_ip(const SAI *addr);
uint16_t get_addr_port(const SAI *addr);
const char *get_tcp_info(int fd);

void send_msg(int fd, const void *buf, size_t len);
ssize_t recv_msg(int fd, void *buf, size_t bufsize);

#endif  /*SYSUTIL_H*/
