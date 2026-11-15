#ifndef LINUX_GUARDED_WRITE_H
#define LINUX_GUARDED_WRITE_H

#include <stdint.h>

long sys_guarded_write_np(int fd, const uint64_t* guardid, const void* buf, unsigned long nbyte);

#endif // LINUX_GUARDED_WRITE_H
