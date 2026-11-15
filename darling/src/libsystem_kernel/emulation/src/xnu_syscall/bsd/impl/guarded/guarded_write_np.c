#include <darling/emulation/xnu_syscall/bsd/impl/guarded/guarded_write_np.h>

#include <limits.h>
#include <sys/errno.h>

#include <darling/emulation/common/guarded/guarded_fd.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/write.h>

// bsd/kern/kern_guarded.c, guarded_write_np: a write at the current offset once the guard matches. Not a
// cancellation point. A descriptor opened without write access fails with EBADF in the write itself.
long sys_guarded_write_np(int fd, const uint64_t* guardid, const void* buf, unsigned long nbyte)
{
	long error = guarded_fd_lookup(fd, guardid);
	if (error)
		return error;

	// dofilewrite in bsd/kern/sys_generic.c
	if (nbyte > INT_MAX)
		return -EINVAL;

	return sys_write_nocancel(fd, buf, nbyte);
}
