# Guarded descriptors keep their guard

`guarded_open_np`, `guarded_kqueue_np` and `change_fdguard_np` record the guard of the descriptor they
guard. `guarded_close_np` checks it the way `fp_lookup_guarded` in `bsd/kern/kern_guarded.c` does: `EFAULT`
for an unreadable guard, `EBADF` for a descriptor that is not open, `EINVAL` for one that is not guarded,
`EPERM` for a different guard. The record is dropped when the descriptor is closed or replaced by `dup2`.

Compile `guarded-fd-guard.c` as a Darwin executable with the configured Darling SDK and libSystem and run
it in the matching private runtime:

```sh
darling shell /Volumes/SystemRoot/absolute/path/to/guarded-fd-guard
```

It prints `PASS: ...` or `FAIL line N: ...`. Before the change `guarded_close_np` with a different guard
closed the descriptor and returned 0 (first check). The host tests `change-fdguard-np.rb` and
`guarded-np-readonly-guard.rb` mock the guard table.

Not modelled: a plain `close`, `dup` or `write` on a descriptor whose guardflags forbid it raises a guard
exception in XNU and kills the process; here it succeeds. `change_fdguard_np` still does not compare the
old guard it is given with the recorded one.
