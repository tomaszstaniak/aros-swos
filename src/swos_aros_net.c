/* Native AROS networking for the existing GGPO/ENet engine. Each calling
 * pthread owns its own bsdsocket.library base and socket table. Destruction
 * closes that task's outstanding sockets before releasing its library. */
#if defined(__AROS__) && defined(SWOS_NETPLAY)
#define SWOS_AROS_NET_IMPLEMENTATION
#include "swos_aros_net.h"
#include <pthread.h>
#include <proto/exec.h>
#include <proto/bsdsocket.h>
#include <bsdsocket/socketbasetags.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct net_context {
    struct Library *base;
    unsigned char live[FD_SETSIZE];
    int flags[FD_SETSIZE];
    char address[16];
};
/* SDK inline-call macros expand SocketBase at the actual call site. */
#define SocketBase (ctx->base)
static pthread_key_t context_key;
static pthread_once_t context_once = PTHREAD_ONCE_INIT;
static int context_error;
static void close_current_context(void);
static void destroy_context(void *p)
{
    struct net_context *ctx = p;
    if (!ctx) return;
    for (int i = 0; i < FD_SETSIZE; ++i)
        if (ctx->live[i]) CloseSocket(i);
    CloseLibrary(ctx->base);
    free(ctx);
}
static void create_key(void)
{
    context_error = pthread_key_create(&context_key, destroy_context);
    if (!context_error) atexit(close_current_context);
}
static void close_current_context(void)
{
    struct net_context *ctx = pthread_getspecific(context_key);
    pthread_setspecific(context_key, NULL);
    destroy_context(ctx);
}
static struct net_context *get_context(void)
{
    pthread_once(&context_once, create_key);
    if (context_error) { errno = ENOMEM; return NULL; }
    struct net_context *ctx = pthread_getspecific(context_key);
    if (ctx) return ctx;
    ctx = calloc(1, sizeof(*ctx));
    if (!ctx) return NULL;
    ctx->base = OpenLibrary((CONST_STRPTR)"bsdsocket.library", 4);
    if (!ctx->base) { free(ctx); errno = ENETDOWN; return NULL; }
    SocketBaseTags(SBTM_SETVAL(SBTC_ERRNOPTR(sizeof(errno))), (IPTR)&errno, TAG_DONE);
    if (pthread_setspecific(context_key, ctx)) {
        destroy_context(ctx); errno = ENOMEM; return NULL;
    }
    return ctx;
}
static int register_socket(struct net_context *ctx, int fd)
{
    if (fd >= FD_SETSIZE) { CloseSocket(fd); errno = EMFILE; return -1; }
    if (fd >= 0) { ctx->live[fd] = 1; ctx->flags[fd] = O_RDWR; }
    return fd;
}
#define CONTEXT(ret) struct net_context *ctx = get_context(); if (!ctx) return (ret)
#define SOCKET_CONTEXT(ret) CONTEXT(ret); if (s < 0 || s >= FD_SETSIZE || !ctx->live[s]) { errno = EBADF; return (ret); }
int swos_net_socket(int d, int t, int p)
{ CONTEXT(-1); return register_socket(ctx, socket(d, t, p)); }
int swos_net_bind(int s, const struct sockaddr *a, socklen_t n)
{ SOCKET_CONTEXT(-1); return bind(s, (struct sockaddr *)a, n); }
int swos_net_connect(int s, const struct sockaddr *a, socklen_t n)
{ SOCKET_CONTEXT(-1); return connect(s, (struct sockaddr *)a, n); }
int swos_net_listen(int s, int n)
{ SOCKET_CONTEXT(-1); return listen(s, n); }
int swos_net_accept(int s, struct sockaddr *a, socklen_t *n)
{ SOCKET_CONTEXT(-1); return register_socket(ctx, accept(s, a, n)); }
int swos_net_close(int s)
{
    SOCKET_CONTEXT(-1);
    int r = CloseSocket(s);
    if (!r) ctx->live[s] = 0;
    return r;
}
int swos_net_shutdown(int s, int how)
{ SOCKET_CONTEXT(-1); return shutdown(s, how); }
ssize_t swos_net_send(int s, const void *p, size_t n, int flags)
{ SOCKET_CONTEXT(-1); if (n > INT_MAX) { errno = EMSGSIZE; return -1; } return send(s, p, n, flags); }
ssize_t swos_net_recv(int s, void *p, size_t n, int flags)
{ SOCKET_CONTEXT(-1); if (n > INT_MAX) { errno = EMSGSIZE; return -1; } return recv(s, p, n, flags); }
ssize_t swos_net_sendto(int s, const void *p, size_t n, int flags, const struct sockaddr *a, socklen_t len)
{ SOCKET_CONTEXT(-1); if (n > INT_MAX) { errno = EMSGSIZE; return -1; } return sendto(s, p, n, flags, (struct sockaddr *)a, len); }
ssize_t swos_net_recvfrom(int s, void *p, size_t n, int flags, struct sockaddr *a, socklen_t *len)
{ SOCKET_CONTEXT(-1); if (n > INT_MAX) { errno = EMSGSIZE; return -1; } return recvfrom(s, p, n, flags, a, len); }
ssize_t swos_net_sendmsg(int s, const struct msghdr *p, int flags)
{ SOCKET_CONTEXT(-1); return sendmsg(s, (struct msghdr *)p, flags); }
ssize_t swos_net_recvmsg(int s, struct msghdr *p, int flags)
{ SOCKET_CONTEXT(-1); return recvmsg(s, p, flags); }
int swos_net_getsockname(int s, struct sockaddr *a, socklen_t *n)
{ SOCKET_CONTEXT(-1); return getsockname(s, a, n); }
int swos_net_getsockopt(int s, int l, int o, void *p, socklen_t *n)
{ SOCKET_CONTEXT(-1); return getsockopt(s, l, o, p, n); }
int swos_net_setsockopt(int s, int l, int o, const void *p, socklen_t n)
{ SOCKET_CONTEXT(-1); return setsockopt(s, l, o, (void *)p, n); }
int swos_net_select(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *t)
{
    CONTEXT(-1);
    if (n < 0 || n > FD_SETSIZE) { errno = EINVAL; return -1; }
    for (int i = 0; i < n; ++i)
        if (((r && FD_ISSET(i, r)) || (w && FD_ISSET(i, w)) || (e && FD_ISSET(i, e))) && !ctx->live[i]) {
            errno = EBADF; return -1;
        }
    return WaitSelect(n, r, w, e, t, NULL);
}
int swos_net_fcntl(int s, int cmd, ...)
{
    SOCKET_CONTEXT(-1);
    if (cmd == F_GETFL) return ctx->flags[s];
    if (cmd == F_SETFL) {
        va_list ap; va_start(ap, cmd); int flags = va_arg(ap, int); va_end(ap);
        unsigned long on = (flags & O_NONBLOCK) != 0;
        int r = IoctlSocket(s, FIONBIO, (char *)&on);
        if (r == 0) ctx->flags[s] = flags;
        return r;
    }
    errno = EINVAL; return -1;
}
int swos_net_ioctl(int s, unsigned long cmd, void *p)
{ SOCKET_CONTEXT(-1); return IoctlSocket(s, cmd, p); }
struct hostent *swos_net_gethostbyname(const char *s)
{ CONTEXT(NULL); return gethostbyname((char *)s); }
struct hostent *swos_net_gethostbyaddr(const void *p, socklen_t n, int family)
{ CONTEXT(NULL); return gethostbyaddr((char *)p, n, family); }
int swos_net_inet_pton(int family, const char *text, void *out)
{
    if (family != AF_INET) { errno = EAFNOSUPPORT; return -1; }
    unsigned char bytes[4];
    for (int i = 0; i < 4; ++i) {
        const char *start = text;
        unsigned n = 0;
        while (*text >= '0' && *text <= '9') {
            n = n * 10 + *text++ - '0';
            if (n > 255 || text - start > 3) return 0;
        }
        if (text == start || (text - start > 1 && *start == '0')) return 0;
        bytes[i] = n;
        if (i < 3) { if (*text++ != '.') return 0; }
        else if (*text) return 0;
    }
    memcpy(out, bytes, sizeof bytes); return 1;
}
const char *swos_net_inet_ntop(int family, const void *p, char *out, socklen_t cap)
{
    if (family != AF_INET) { errno = EAFNOSUPPORT; return NULL; }
    const unsigned char *b = p;
    char text[16];
    int n = snprintf(text, sizeof text, "%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
    if (cap <= (socklen_t)n) { errno = ENOSPC; return NULL; }
    memcpy(out, text, n + 1); return out;
}
char *swos_net_inet_ntoa(struct in_addr a)
{ CONTEXT(NULL); return (char *)swos_net_inet_ntop(AF_INET, &a, ctx->address, sizeof ctx->address); }
void swos_net_freeaddrinfo(struct addrinfo *p)
{
    while (p) { struct addrinfo *next = p->ai_next; free(p->ai_addr); free(p->ai_canonname); free(p); p = next; }
}
int swos_net_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **out)
{
    *out = NULL;
    if (hints && hints->ai_family != AF_INET && hints->ai_family != AF_UNSPEC) return EAI_FAMILY;
    if (hints && (hints->ai_flags & ~(AI_PASSIVE | AI_CANONNAME))) return EAI_BADFLAGS;
    char *end = NULL;
    unsigned long port = service ? strtoul(service, &end, 10) : 0;
    if (service && (!*service || *end || port > 65535)) return EAI_SERVICE;
    struct in_addr address;
    if (!node) address.s_addr = htonl(hints && (hints->ai_flags & AI_PASSIVE) ? INADDR_ANY : INADDR_LOOPBACK);
    else if (swos_net_inet_pton(AF_INET, node, &address) != 1) {
        struct hostent *h = swos_net_gethostbyname(node);
        if (!h || h->h_addrtype != AF_INET || h->h_length != sizeof address || !h->h_addr_list[0]) return EAI_NONAME;
        memcpy(&address, h->h_addr_list[0], sizeof address);
    }
    struct addrinfo *a = calloc(1, sizeof *a);
    if (!a) return EAI_MEMORY;
    struct sockaddr_in *sin = calloc(1, sizeof *sin);
    if (!sin) { free(a); return EAI_MEMORY; }
    sin->sin_len = sizeof *sin; sin->sin_family = AF_INET;
    sin->sin_port = htons(port); sin->sin_addr = address;
    a->ai_family = AF_INET; a->ai_socktype = hints ? hints->ai_socktype : SOCK_STREAM;
    a->ai_protocol = hints ? hints->ai_protocol : 0;
    a->ai_addrlen = sizeof *sin; a->ai_addr = (struct sockaddr *)sin;
    if (hints && (hints->ai_flags & AI_CANONNAME) && node) {
        a->ai_canonname = strdup(node);
        if (!a->ai_canonname) { swos_net_freeaddrinfo(a); return EAI_MEMORY; }
    }
    *out = a; return 0;
}
#endif
