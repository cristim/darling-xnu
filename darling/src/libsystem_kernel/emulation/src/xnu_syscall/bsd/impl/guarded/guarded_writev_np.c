#include <darling/emulation/xnu_syscall/bsd/impl/guarded/guarded_writev_np.h>

#include <sys/errno.h>

#include <darling/emulation/common/guarded/guarded_fd.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/writev.h>

// UIO_MAXIOV in bsd/sys/uio.h
#define GUARDED_WRITEV_MAXIOV 1024

// bsd/kern/kern_guarded.c, guarded_writev_np: a writev once the guard matches. Not a cancellation point.
// XNU checks the count first, then reads the iovecs and only then the guard; here the guard is checked
// before writev reads the iovecs, so a call with an unreadable iovec array and a wrong guard reports the
// guard instead of EFAULT.
long sys_guarded_writev_np(int fd, const uint64_t* guardid, struct iovec* iovp, int iovcnt)
{
	if (iovcnt <= 0 || iovcnt > GUARDED_WRITEV_MAXIOV)
		return -EINVAL;

	long error = guarded_fd_lookup(fd, guardid);
	if (error)
		return error;

	return sys_writev_nocancel(fd, iovp, iovcnt);
}
