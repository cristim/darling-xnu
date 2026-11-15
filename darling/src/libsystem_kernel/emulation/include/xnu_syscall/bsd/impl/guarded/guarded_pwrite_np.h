#ifndef LINUX_GUARDED_PWRITE_H
#define LINUX_GUARDED_PWRITE_H

#include <stdint.h>

long sys_guarded_pwrite_np(int fd, const uint64_t* guardid, const void* buf, unsigned long nbyte, long long offset);

#endif // LINUX_GUARDED_PWRITE_H
