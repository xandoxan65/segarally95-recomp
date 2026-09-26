#include "pthread.h"

#include <stdlib.h>

typedef struct {
    void *(*start)(void *);
    void *arg;
} host_pthread_thunk_t;

static DWORD WINAPI host_pthread_thunk(LPVOID param)
{
    host_pthread_thunk_t *t = (host_pthread_thunk_t *)param;
    void *(*start)(void *) = t->start;
    void *arg = t->arg;

    free(t);
    start(arg);
    return 0;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start)(void *), void *arg)
{
    host_pthread_thunk_t *t;
    HANDLE h;

    (void)attr;
    if (!thread || !start)
        return -1;
    t = (host_pthread_thunk_t *)malloc(sizeof(*t));
    if (!t)
        return -1;
    t->start = start;
    t->arg = arg;
    h = CreateThread(NULL, 0, host_pthread_thunk, t, 0, NULL);
    if (!h) {
        free(t);
        return -1;
    }
    *thread = h;
    return 0;
}

int pthread_join(pthread_t thread, void **retval)
{
    if (retval)
        *retval = NULL;
    if (!thread)
        return -1;
    if (WaitForSingleObject(thread, INFINITE) != WAIT_OBJECT_0)
        return -1;
    CloseHandle(thread);
    return 0;
}
