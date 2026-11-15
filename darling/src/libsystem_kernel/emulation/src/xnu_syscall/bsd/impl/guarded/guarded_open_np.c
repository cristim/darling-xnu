#include <darling/emulation/xnu_syscall/bsd/impl/guarded/guarded_open_np.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/guarded/guard_args.h>
#include <darling/emulation/common/guarded/table.h>
#include <darling/emulation/xnu_syscall/bsd/impl/fcntl/open.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>

long sys_guarded_open_np(const char* filename, const uint64_t* guardid, unsigned int guardflags, int flags, unsigned int mode)
{
	long error = guard_args_validate(guardid, guardflags);
	if (error)
		return error;

	long fd = sys_open(filename, flags, mode);
	if (fd < 0)
		return fd;

	// remember the guard for guarded_close_np and the guarded writes
	uint64_t guard = *(const volatile uint64_t*) guardid;
	long recorded = guard ? guard_table_user_set(fd, guard, guardflags) : -EINVAL;
	if (recorded < 0) {
		sys_close_nocancel(fd);
		return recorded;
	}
	return fd;
}
