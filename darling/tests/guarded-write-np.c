// SPDX-License-Identifier: GPL-3.0-or-later
// guarded_write_np, guarded_pwrite_np and guarded_writev_np write through a guarded descriptor when the guard
// matches and fail with the errno of bsd/kern/kern_guarded.c otherwise: EFAULT (guard unreadable), EBADF (no
// such descriptor, or none open for writing), EINVAL (not guarded), EPERM (a different guard). The prototypes
// are the PRIVATE ones of bsd/sys/guarded.h.
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/uio.h>
#include <unistd.h>

typedef uint64_t guardid_t;
extern int guarded_open_np(const char *path, const guardid_t *guard, unsigned guardflags, int flags, ...);
extern int guarded_close_np(int fd, const guardid_t *guard);
extern int change_fdguard_np(int fd, const guardid_t *guard, unsigned guardflags,
    const guardid_t *nguard, unsigned nguardflags, int *fdflagsp);
extern ssize_t guarded_write_np(int fd, const guardid_t *guard, const void *buf, size_t nbyte);
extern ssize_t guarded_pwrite_np(int fd, const guardid_t *guard, const void *buf, size_t nbyte, off_t offset);
extern ssize_t guarded_writev_np(int fd, const guardid_t *guard, const struct iovec *iovp, int iovcnt);

#define GUARD_CLOSE (1u << 0)
#define GUARD_DUP (1u << 1)
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); return 1; } } while (0)
#define CHECK_ERRNO(call, e) CHECK((call) == -1 && errno == (e))

/* the three calls with the same arguments: write "x" (or one iovec) at offset 0 with this fd and guard */
#define EACH_ERRNO(fd, guard, e) do { \
	struct iovec v_ = { "x", 1 }; \
	CHECK_ERRNO(guarded_write_np(fd, guard, "x", 1), e); \
	CHECK_ERRNO(guarded_pwrite_np(fd, guard, "x", 1, 0), e); \
	CHECK_ERRNO(guarded_writev_np(fd, guard, &v_, 1), e); \
} while (0)

int main(void) {
	const char *path = "/tmp/guarded-write-np.dat";
	guardid_t g = 0x1122334455667788ULL, bad = 0x99;
	int fl = 0;
	char buf[32];
	unlink(path);

	int fd = guarded_open_np(path, &g, GUARD_CLOSE | GUARD_DUP, O_RDWR | O_CREAT | O_CLOEXEC, 0600);
	CHECK(fd >= 0);
	int plain = open(path, O_RDWR);
	CHECK(plain >= 0);

	/* the write advances the offset, the pwrite does not, the writev advances it */
	CHECK(guarded_write_np(fd, &g, "abc", 3) == 3);
	CHECK(lseek(fd, 0, SEEK_CUR) == 3);
	CHECK(guarded_pwrite_np(fd, &g, "XYZ", 3, 10) == 3);
	CHECK(lseek(fd, 0, SEEK_CUR) == 3);
	struct iovec iov[2] = { { "de", 2 }, { "f", 1 } };
	CHECK(guarded_writev_np(fd, &g, iov, 2) == 3);
	CHECK(lseek(fd, 0, SEEK_CUR) == 6);
	memset(buf, 'q', sizeof buf);
	CHECK(pread(plain, buf, sizeof buf, 0) == 13);
	CHECK(memcmp(buf, "abcdef\0\0\0\0XYZ", 13) == 0);

	/* a different guard changes nothing */
	EACH_ERRNO(fd, &bad, EPERM);
	CHECK(lseek(fd, 0, SEEK_CUR) == 6);
	CHECK(pread(plain, buf, sizeof buf, 0) == 13 && memcmp(buf, "abcdef\0\0\0\0XYZ", 13) == 0);

	/* the guard must be readable */
	EACH_ERRNO(fd, NULL, EFAULT);
	long ps = sysconf(_SC_PAGESIZE);
	void *none = mmap(NULL, ps, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
	CHECK(none != MAP_FAILED);
	EACH_ERRNO(fd, (const guardid_t *)none, EFAULT);

	/* an unopened descriptor, an unguarded one, a guarded one opened read-only */
	EACH_ERRNO(900, &g, EBADF);
	EACH_ERRNO(plain, &g, EINVAL);
	int ro = guarded_open_np(path, &g, GUARD_DUP, O_RDONLY | O_CLOEXEC);
	CHECK(ro >= 0);
	EACH_ERRNO(ro, &g, EBADF);
	CHECK(guarded_close_np(ro, &g) == 0);
	CHECK(pread(plain, buf, sizeof buf, 0) == 13 && memcmp(buf, "abcdef\0\0\0\0XYZ", 13) == 0);

	/* sizes and counts XNU rejects */
	struct iovec one = { "x", 1 };
	CHECK_ERRNO(guarded_write_np(fd, &g, "x", (size_t)INT_MAX + 1), EINVAL);
	CHECK_ERRNO(guarded_pwrite_np(fd, &g, "x", (size_t)INT_MAX + 1, 0), EINVAL);
	CHECK_ERRNO(guarded_pwrite_np(fd, &g, "x", 1, -1), EINVAL);
	CHECK_ERRNO(guarded_writev_np(fd, &g, &one, 0), EINVAL);
	CHECK_ERRNO(guarded_writev_np(fd, &g, &one, 1025), EINVAL);

	/* a descriptor guarded after it was opened, here a pipe: no positioned writes */
	int p[2];
	CHECK(pipe(p) == 0);
	CHECK(change_fdguard_np(p[1], NULL, 0, &g, GUARD_DUP, &fl) == 0);
	CHECK(guarded_write_np(p[1], &g, "hi", 2) == 2);
	CHECK(guarded_writev_np(p[1], &g, &one, 1) == 1);
	CHECK(read(p[0], buf, sizeof buf) == 3 && memcmp(buf, "hix", 3) == 0);
	CHECK_ERRNO(guarded_pwrite_np(p[1], &g, "x", 1, 0), ESPIPE);
	CHECK(change_fdguard_np(p[1], &g, GUARD_DUP, NULL, 0, &fl) == 0);
	CHECK_ERRNO(guarded_write_np(p[1], &g, "x", 1), EINVAL);
	close(p[0]);
	close(p[1]);

	/* a closed descriptor is gone */
	CHECK(guarded_close_np(fd, &g) == 0);
	EACH_ERRNO(fd, &g, EBADF);

	close(plain);
	unlink(path);
	puts("PASS: guarded_write_np, guarded_pwrite_np and guarded_writev_np write when the guard matches and fail as XNU does");
	return 0;
}
