#ifndef _DARLING_EMULATION_COMMON_GUARDED_GUARDED_FD_H_
#define _DARLING_EMULATION_COMMON_GUARDED_GUARDED_FD_H_

#include <stdint.h>

// XNU's fp_lookup_guarded (bsd/kern/kern_guarded.c) as seen by the guarded_close_np and guarded_*write_np
// handlers: returns 0 if `fd` is guarded with the guard `guardid` points to, otherwise a negative BSD errno
// in the order XNU reports them: EFAULT (guard unreadable), EBADF (no such descriptor), EINVAL (the
// descriptor is not guarded), EPERM (a different guard; XNU raises no exception for this one).
long guarded_fd_lookup(int fd, const uint64_t* guardid);

#endif
