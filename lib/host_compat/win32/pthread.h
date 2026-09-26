/* Minimal pthread subset for MSVC / clang-cl (SRWLOCK + CONDITION_VARIABLE).
 * MinGW keeps the system <pthread.h>; this path is only on the include list
 * when CMake does not find a real pthread.h.
 */
#ifndef HOST_COMPAT_PTHREAD_H
#define HOST_COMPAT_PTHREAD_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef HANDLE pthread_t;
typedef SRWLOCK pthread_mutex_t;
typedef CONDITION_VARIABLE pthread_cond_t;
typedef void pthread_attr_t;
typedef void pthread_mutexattr_t;
typedef void pthread_condattr_t;

#define PTHREAD_MUTEX_INITIALIZER SRWLOCK_INIT
#define PTHREAD_COND_INITIALIZER CONDITION_VARIABLE_INIT

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start)(void *), void *arg);
int pthread_join(pthread_t thread, void **retval);

static inline int pthread_mutex_lock(pthread_mutex_t *m)
{
    AcquireSRWLockExclusive(m);
    return 0;
}

static inline int pthread_mutex_unlock(pthread_mutex_t *m)
{
    ReleaseSRWLockExclusive(m);
    return 0;
}

static inline int pthread_cond_wait(pthread_cond_t *c, pthread_mutex_t *m)
{
    if (!SleepConditionVariableSRW(c, m, INFINITE, 0))
        return -1;
    return 0;
}

static inline int pthread_cond_signal(pthread_cond_t *c)
{
    WakeConditionVariable(c);
    return 0;
}

static inline int pthread_cond_broadcast(pthread_cond_t *c)
{
    WakeAllConditionVariable(c);
    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /* HOST_COMPAT_PTHREAD_H */
