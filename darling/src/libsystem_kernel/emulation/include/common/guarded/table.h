#ifndef _DARLING_EMULATION_LINUX_GUARDED_TABLE_H_
#define _DARLING_EMULATION_LINUX_GUARDED_TABLE_H_

#include <stdint.h>

typedef enum guard_flags {
	guard_flag_none = 0,
	guard_flag_prevent_close = 1U << 0,
	guard_flag_close_on_fork = 1U << 1,
} __attribute__((flag_enum)) guard_flags_t;

typedef void (*guard_entry_close_f)(int fd);

typedef struct guard_entry_options {
	guard_entry_close_f close;
} guard_entry_options_t;

// TODO: expand this code to support different types of guards (e.g. close, dup, socket IPC, etc.).
//       right now, it's only checked when closing descriptors.

/**
 * @retval `0` on success
 * @retval `-EEXIST` if the entry already exists
 * @retval `-ENOMEM` if no space available for new entry
 */
int guard_table_add(int fd, guard_flags_t flags, guard_entry_options_t* options);

/**
 * @retval `0` on success
 * @retval `-ENOENT` if the entry doesn't exist
 */
int guard_table_modify(int fd, guard_flags_t flags, guard_entry_options_t* options);

/**
 * @retval `0` on success
 * @retval `-ENOENT` if the entry doesn't exist
 */
int guard_table_remove(int fd);

/**
 * @returns `0` if FD is unguarded for all the specified guard flags, 1 otherwise (if it is guarded for at least one).
 *           This includes the case when the FD isn't present in the table at all (it is simply treated as unguarded).
 */
int guard_table_check(int fd, guard_flags_t flags);

// XNU-style guards on caller descriptors (guarded_open_np, guarded_kqueue_np, change_fdguard_np): the guard
// value the caller chose and its guardflags. They live in the same table as the internal guards above but never
// set a guard_flags_t flag, so they do not change what guard_table_check() reports. Callers include
// <darling/emulation/common/guarded/guard_args.h> for the DARLING_GUARDFLAG_* values.

/**
 * Records `guard` and `attrs` as the guard of `fd`, replacing the guard already recorded for it.
 * @retval `0` on success
 * @retval `-ENOMEM` if no space available for a new entry
 */
int guard_table_user_set(int fd, uint64_t guard, unsigned int attrs);

/**
 * Forgets the guard of `fd`. A descriptor without a recorded guard is left alone.
 */
void guard_table_user_clear(int fd);

/**
 * @retval `0` if `fd` has a recorded guard equal to `guard`
 * @retval `-EPERM` if it has a different one
 * @retval `-ENOENT` if it has none
 */
int guard_table_user_check(int fd, uint64_t guard);

void guard_table_postfork_child(void);

#endif // _DARLING_EMULATION_LINUX_GUARDED_TABLE_H_
