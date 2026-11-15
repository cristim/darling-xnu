# guarded_write_np, guarded_pwrite_np and guarded_writev_np

Syscalls 485, 486 and 487 of `bsd/kern/syscalls.master`. Libraries that write through guarded descriptors
call them; without them every call failed with `ENOSYS` and the caller saw an I/O error (SQLite reports
`SQLITE_IOERR_WRITE`, 778, when its first write to a new database fails).

Each checks the guard like `guarded_close_np` (see `guarded-fd-guard.md`: the descriptor must have been guarded through `guarded_open_np`, `guarded_kqueue_np` or `change_fdguard_np`) and then writes like `write`,
`pwrite` and `writev`. XNU raises no guard exception for these calls, so none is raised here.

Compile `guarded-write-np.c` as a Darwin executable with the configured Darling SDK and libSystem and run
it in the matching private runtime:

```sh
darling shell /Volumes/SystemRoot/absolute/path/to/guarded-write-np
```

It prints `PASS: ...` or `FAIL line N: ...`. Before the change the first call prints
`Unimplemented syscall (485)` and fails.
