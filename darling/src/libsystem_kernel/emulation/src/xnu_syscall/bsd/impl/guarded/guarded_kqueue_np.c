#include <darling/emulation/xnu_syscall/bsd/impl/guarded/guarded_kqueue_np.h>

#include <sys/errno.h>

#include <darling/emulation/common/base.h>
#include <darling/emulation/conversion/errno.h>
#include <darling/emulation/linux_premigration/linux-syscalls/linux.h>
#include <darling/emulation/common/guarded/guard_args.h>
#include <darling/emulation/common/guarded/table.h>
#include <darling/emulation/xnu_syscall/bsd/impl/kqueue/kqueue.h>
#include <darling/emulation/xnu_syscall/bsd/impl/unistd/close.h>

long sys_guarded_kqueue_np(const uint64_t* guardid, unsigned guardflags)
{
	long error = guard_args_validate(guardid, guardflags);
	if (error)
		return error;

	long kq = sys_kqueue();
	if (kq < 0)
		return kq;

	uint64_t guard = *(const volatile uint64_t*) guardid;
	long recorded = guard ? guard_table_user_set(kq, guard, guardflags) : -EINVAL;
	if (recorded < 0) {
		sys_close_nocancel(kq);
		return recorded;
	}
	return kq;
}
