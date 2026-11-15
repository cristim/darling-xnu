#ifndef _DARLING_EMULATION_COMMON_GUARDED_GUARD_ARGS_H_
#define _DARLING_EMULATION_COMMON_GUARDED_GUARD_ARGS_H_

#include <stdint.h>
#include <sys/errno.h>

// the guardflags XNU accepts in guarded_open_np and guarded_kqueue_np (bsd/sys/guarded.h)
#define DARLING_GUARDFLAG_CLOSE      (1u << 0)
#define DARLING_GUARDFLAG_DUP        (1u << 1)
#define DARLING_GUARDFLAG_SOCKET_IPC (1u << 2)
#define DARLING_GUARDFLAG_FILEPORT   (1u << 3)
#define DARLING_GUARDFLAG_WRITE      (1u << 4)
#define DARLING_GUARDFLAG_ALL        (DARLING_GUARDFLAG_CLOSE | DARLING_GUARDFLAG_DUP | \
	DARLING_GUARDFLAG_SOCKET_IPC | DARLING_GUARDFLAG_FILEPORT | DARLING_GUARDFLAG_WRITE)

// FUTEX_WAIT | FUTEX_PRIVATE_FLAG
#define DARLING_FUTEX_WAIT_PRIVATE 128

// Returns 0 if the guard can be read and -EFAULT if it cannot (NULL included): readability stands in for XNU's
// copyin, probed with a zero-timeout futex wait on each aligned half (no side effect, allowed by container seccomp);
// a misaligned guard skips the probe. Callers include <darling/emulation/common/base.h> and the linux syscall
// numbers first.
static inline long guard_args_readable(const uint64_t* guardid)
{
	if (!guardid)
		return -EFAULT;
	if (((uintptr_t) guardid & 3) == 0)
	{
		const uint32_t* halves = (const uint32_t*) guardid;
		for (int i = 0; i < 2; ++i)
		{
			long timeout[2] = { 0, 0 };
			if (LINUX_SYSCALL(__NR_futex, halves + i, DARLING_FUTEX_WAIT_PRIVATE, 0, timeout) == -EFAULT)
				return -EFAULT;
		}
	}
	return 0;
}

// Returns 0 or a negative BSD errno, in the order XNU checks: guardflags must include GUARD_DUP and
// no unknown bit (EINVAL), the guard must be readable (EFAULT) and must not be zero (EINVAL).
// The guard is only read.
static inline long guard_args_validate(const uint64_t* guardid, unsigned int guardflags)
{
	if ((guardflags & DARLING_GUARDFLAG_DUP) == 0 || (guardflags & ~DARLING_GUARDFLAG_ALL) != 0)
		return -EINVAL;
	long error = guard_args_readable(guardid);
	if (error)
		return error;
	if (*(const volatile uint64_t*) guardid == 0)
		return -EINVAL;
	return 0;
}

#endif
