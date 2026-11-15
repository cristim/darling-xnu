#ifndef LINUX_GUARDED_WRITEV_H
#define LINUX_GUARDED_WRITEV_H

#include <stdint.h>

struct iovec;

long sys_guarded_writev_np(int fd, const uint64_t* guardid, struct iovec* iovp, int iovcnt);

#endif // LINUX_GUARDED_WRITEV_H
