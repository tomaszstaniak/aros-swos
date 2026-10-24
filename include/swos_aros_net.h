#ifndef SWOS_AROS_NET_H
#define SWOS_AROS_NET_H
#ifdef __AROS__
/* Scoped to network translation units: AROS socket descriptors are not
 * posixc file descriptors. Never apply these mappings to general file I/O. */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
/* Newer SDK pthread headers wrap socket calls as cancellation points. Include
 * them before our mappings so later C++ headers cannot restore SocketBase calls. */
#include <pthread.h>
#ifndef PF_INET
#define PF_INET AF_INET
#endif
#ifdef __cplusplus
extern "C" {
#endif
int swos_net_socket(int, int, int);
int swos_net_bind(int, const struct sockaddr *, socklen_t);
int swos_net_connect(int, const struct sockaddr *, socklen_t);
int swos_net_listen(int, int);
int swos_net_accept(int, struct sockaddr *, socklen_t *);
int swos_net_close(int);
int swos_net_shutdown(int, int);
ssize_t swos_net_send(int, const void *, size_t, int);
ssize_t swos_net_recv(int, void *, size_t, int);
ssize_t swos_net_sendto(int, const void *, size_t, int, const struct sockaddr *, socklen_t);
ssize_t swos_net_recvfrom(int, void *, size_t, int, struct sockaddr *, socklen_t *);
ssize_t swos_net_sendmsg(int, const struct msghdr *, int);
ssize_t swos_net_recvmsg(int, struct msghdr *, int);
int swos_net_getsockname(int, struct sockaddr *, socklen_t *);
int swos_net_getsockopt(int, int, int, void *, socklen_t *);
int swos_net_setsockopt(int, int, int, const void *, socklen_t);
int swos_net_select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
int swos_net_fcntl(int, int, ...);
int swos_net_ioctl(int, unsigned long, void *);
struct hostent *swos_net_gethostbyname(const char *);
struct hostent *swos_net_gethostbyaddr(const void *, socklen_t, int);
int swos_net_getaddrinfo(const char *, const char *, const struct addrinfo *, struct addrinfo **);
void swos_net_freeaddrinfo(struct addrinfo *);
int swos_net_inet_pton(int, const char *, void *);
const char *swos_net_inet_ntop(int, const void *, char *, socklen_t);
char *swos_net_inet_ntoa(struct in_addr);
#ifdef __cplusplus
}
#endif
#ifndef SWOS_AROS_NET_IMPLEMENTATION
#undef socket
#define socket swos_net_socket
#undef bind
#define bind swos_net_bind
#undef connect
#define connect swos_net_connect
#undef listen
#define listen swos_net_listen
#undef accept
#define accept swos_net_accept
#undef close
#define close swos_net_close
#undef shutdown
#define shutdown swos_net_shutdown
#undef send
#define send swos_net_send
#undef recv
#define recv swos_net_recv
#undef sendto
#define sendto swos_net_sendto
#undef recvfrom
#define recvfrom swos_net_recvfrom
#undef sendmsg
#define sendmsg swos_net_sendmsg
#undef recvmsg
#define recvmsg swos_net_recvmsg
#undef getsockname
#define getsockname swos_net_getsockname
#undef getsockopt
#define getsockopt swos_net_getsockopt
#undef setsockopt
#define setsockopt swos_net_setsockopt
#undef select
#define select swos_net_select
#undef fcntl
#define fcntl swos_net_fcntl
#undef ioctl
#define ioctl swos_net_ioctl
#undef gethostbyname
#define gethostbyname swos_net_gethostbyname
#undef gethostbyaddr
#define gethostbyaddr swos_net_gethostbyaddr
#undef getaddrinfo
#define getaddrinfo swos_net_getaddrinfo
#undef freeaddrinfo
#define freeaddrinfo swos_net_freeaddrinfo
#undef inet_pton
#define inet_pton swos_net_inet_pton
#undef inet_ntop
#define inet_ntop swos_net_inet_ntop
#undef inet_ntoa
#define inet_ntoa swos_net_inet_ntoa
#endif
#endif
#endif
