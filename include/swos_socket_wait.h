#ifndef SWOS_SOCKET_WAIT_H
#define SWOS_SOCKET_WAIT_H
/* The game's network callers wait on one socket only. AROS has select(),
 * whereas the selected v11 SDK has no poll.h. Keep the original poll path
 * on other targets. Positive = ready (including errors), 0 = timeout. */
enum { SWOS_WAIT_READ = 1, SWOS_WAIT_WRITE = 2 };
#if defined(__AROS__) || defined(SWOS_WAIT_SELECT_TEST)
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>
#include "swos_aros_net.h"
static inline int swos_socket_wait(int fd, int events, int ms)
{
    if (fd < 0 || fd >= FD_SETSIZE || ms < 0) {
        errno = EINVAL;
        return -1;
    }
    fd_set rd, wr, ex;
    FD_ZERO(&rd); FD_ZERO(&wr); FD_ZERO(&ex);
    if (events & SWOS_WAIT_READ) FD_SET(fd, &rd);
    if (events & SWOS_WAIT_WRITE) FD_SET(fd, &wr);
    FD_SET(fd, &ex);
    struct timeval timeout;
    timeout.tv_sec = ms / 1000;
    timeout.tv_usec = (ms % 1000) * 1000;
    return select(fd + 1, &rd, &wr, &ex, &timeout);
}
#else
#include <poll.h>
static inline int swos_socket_wait(int fd, int events, int ms)
{
    struct pollfd p = {fd, 0, 0};
    if (events & SWOS_WAIT_READ) p.events |= POLLIN;
    if (events & SWOS_WAIT_WRITE) p.events |= POLLOUT;
    return poll(&p, 1, ms);
}
#endif
#endif
