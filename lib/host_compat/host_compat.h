/* Force-included on Windows (see CMakeLists.txt). No-op on POSIX. */
#ifndef HOST_COMPAT_H
#define HOST_COMPAT_H

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <sys/stat.h>
#include <time.h>

#ifndef S_ISREG
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif

#if defined(_MSC_VER)
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#endif

#if defined(_MSC_VER) && !defined(__clang__)
#ifndef __attribute__
#define __attribute__(x)
#endif
#endif

#ifndef HOST_COMPAT_HAVE_CLOCK_GETTIME
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif
int clock_gettime(int clk_id, struct timespec *tp);
int nanosleep(const struct timespec *req, struct timespec *rem);
#endif

#endif /* _WIN32 */

#endif /* HOST_COMPAT_H */
