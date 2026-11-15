#include <darling/emulation/xnu_syscall/bsd/impl/guarded/guarded_pwrite_np.h>

#include <limits.h>
#include <sys/errno.h>

#include <darling/emulation/common/guarded/guarded_fd.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/pwrite.h>

// bsd/kern/kern_guarded.c, guarded_pwrite_np: a write at `offset` once the guard matches, without moving the
// file offset. pwrite64 refuses pipes, FIFOs, terminals and offset -1 itself, in Linux's order, not XNU's.
long sys_guarded_pwrite_np(int fd, const uint64_t* guardid, const void* buf, unsigned long nbyte, long long offset)
{
	long error = guarded_fd_lookup(fd, guardid);
	if (error)
		return error;

	// dofilewrite in bsd/kern/sys_generic.c
	if (nbyte > INT_MAX)
		return -EINVAL;

	return sys_pwrite_nocancel(fd, buf, nbyte, offset);
}
