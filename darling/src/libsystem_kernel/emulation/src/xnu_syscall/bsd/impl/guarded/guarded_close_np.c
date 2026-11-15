#include <darling/emulation/xnu_syscall/bsd/impl/guarded/guarded_close_np.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/guarded/guarded_fd.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>

long sys_guarded_close_np(int fd, const uint64_t* guardid)
{
	long error = guarded_fd_lookup(fd, guardid);
	if (error)
		return error;

	// sys_close also forgets the guard
	return sys_close(fd);
}
