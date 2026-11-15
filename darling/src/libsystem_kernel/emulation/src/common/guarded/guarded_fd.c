#include <darling/emulation/common/guarded/guarded_fd.h>

#include <sys/errno.h>
#include <sys/fcntl.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/guarded/guard_args.h>
#include <darling/emulation/common/guarded/table.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/fcntl.h>

long guarded_fd_lookup(int fd, const uint64_t* guardid)
{
	long error = guard_args_readable(guardid);
	if (error)
		return error;

	error = guard_table_user_check(fd, *(const volatile uint64_t*) guardid);
	if (error != -ENOENT)
		return error;

	// no recorded guard: either the descriptor is not open or it is not guarded
	error = sys_fcntl_nocancel(fd, F_GETFD, 0);
	return error < 0 ? error : -EINVAL;
}
