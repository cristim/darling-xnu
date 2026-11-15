// SPDX-License-Identifier: GPL-3.0-or-later
// A guarded descriptor remembers its guard: guarded_close_np refuses a different guard (EPERM) and an
// unguarded descriptor (EINVAL), and the guard does not outlive the descriptor. Errors as in
// bsd/kern/kern_guarded.c (fp_lookup_guarded); the prototypes are the PRIVATE ones of bsd/sys/guarded.h.
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

typedef uint64_t guardid_t;
extern int guarded_open_np(const char *path, const guardid_t *guard, unsigned guardflags, int flags, ...);
extern int guarded_kqueue_np(const guardid_t *guard, unsigned guardflags);
extern int guarded_close_np(int fd, const guardid_t *guard);
extern int change_fdguard_np(int fd, const guardid_t *guard, unsigned guardflags,
    const guardid_t *nguard, unsigned nguardflags, int *fdflagsp);

#define GUARD_CLOSE (1u << 0)
#define GUARD_DUP (1u << 1)
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); return 1; } } while (0)
#define CHECK_ERRNO(call, e) CHECK((call) == -1 && errno == (e))
#define OPEN_FLAGS (O_RDWR | O_CREAT | O_CLOEXEC)

static int is_open(int fd) { return fcntl(fd, F_GETFD) != -1; }

int main(void) {
	const char *path = "/tmp/guarded-fd-guard.dat";
	guardid_t g = 0x1122334455667788ULL, bad = 0x99, g2 = 0x5566;
	int fl = 0;

	/* the descriptor keeps its guard; a different guard and no guard are refused and leave it open */
	int fd = guarded_open_np(path, &g, GUARD_CLOSE | GUARD_DUP, OPEN_FLAGS, 0600);
	CHECK(fd >= 0);
	CHECK_ERRNO(guarded_close_np(fd, &bad), EPERM);
	CHECK(is_open(fd));
	CHECK_ERRNO(guarded_close_np(fd, NULL), EFAULT);
	CHECK(is_open(fd));
	CHECK(guarded_close_np(fd, &g) == 0);
	CHECK(!is_open(fd));
	CHECK_ERRNO(guarded_close_np(fd, &g), EBADF);

	/* an unguarded descriptor is EINVAL, an unopened one EBADF */
	int plain = open(path, O_RDWR);
	CHECK(plain >= 0);
	CHECK_ERRNO(guarded_close_np(plain, &g), EINVAL);
	CHECK(is_open(plain));
	CHECK_ERRNO(guarded_close_np(900, &g), EBADF);
	CHECK_ERRNO(guarded_close_np(-1, &g), EBADF);

	/* the guard does not survive a plain close: the next descriptor with this number is unguarded */
	close(plain);
	fd = guarded_open_np(path, &g, GUARD_DUP, OPEN_FLAGS, 0600);
	CHECK(fd >= 0);
	close(fd);
	plain = open(path, O_RDWR);
	CHECK(plain == fd);
	CHECK_ERRNO(guarded_close_np(plain, &g), EINVAL);

	/* nor a dup2 over it */
	fd = guarded_open_np(path, &g, GUARD_DUP, OPEN_FLAGS, 0600);
	CHECK(fd >= 0);
	CHECK(dup2(plain, fd) == fd);
	CHECK_ERRNO(guarded_close_np(fd, &g), EINVAL);
	close(fd);
	close(plain);

	/* change_fdguard_np guards, re-guards and unguards */
	int a = open(path, O_RDWR), b = open(path, O_RDWR);
	CHECK(a >= 0 && b >= 0);
	CHECK(change_fdguard_np(a, NULL, 0, &g, GUARD_DUP, &fl) == 0);
	CHECK_ERRNO(guarded_close_np(a, &bad), EPERM);
	CHECK(change_fdguard_np(a, &g, GUARD_DUP, &g2, GUARD_DUP, &fl) == 0);
	CHECK_ERRNO(guarded_close_np(a, &g), EPERM);
	CHECK(guarded_close_np(a, &g2) == 0);
	CHECK(change_fdguard_np(b, NULL, 0, &g, GUARD_DUP, &fl) == 0);
	CHECK(change_fdguard_np(b, &g, GUARD_DUP, NULL, 0, &fl) == 0);
	CHECK_ERRNO(guarded_close_np(b, &g), EINVAL);
	close(b);

	/* a guarded kqueue */
	int kq = guarded_kqueue_np(&g, GUARD_DUP);
	CHECK(kq >= 0);
	CHECK_ERRNO(guarded_close_np(kq, &bad), EPERM);
	CHECK(guarded_close_np(kq, &g) == 0);

	unlink(path);
	puts("PASS: guarded descriptors keep their guard until they are closed");
	return 0;
}
