#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <time.h>

int clock_gettime(int clk_id, struct timespec *tp)
{
    static LARGE_INTEGER freq;
    static int init;
    LARGE_INTEGER now;
    unsigned long long ns;

    (void)clk_id;
    if (!tp)
        return -1;
    if (!init) {
        if (!QueryPerformanceFrequency(&freq) || freq.QuadPart == 0)
            return -1;
        init = 1;
    }
    QueryPerformanceCounter(&now);
    ns = (unsigned long long)now.QuadPart * 1000000000ull
        / (unsigned long long)freq.QuadPart;
    tp->tv_sec = (time_t)(ns / 1000000000ull);
    tp->tv_nsec = (long)(ns % 1000000000ull);
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    long long ns;
    HANDLE timer;
    LARGE_INTEGER due;

    if (!req)
        return -1;
    if (rem) {
        rem->tv_sec = 0;
        rem->tv_nsec = 0;
    }
    if (req->tv_sec < 0 || req->tv_nsec < 0)
        return -1;

    ns = (long long)req->tv_sec * 1000000000ll + (long long)req->tv_nsec;
    if (ns <= 0)
        return 0;

    /*
     * Sleep() alone is ~15.6 ms on Windows without a multimedia timer period.
     * WaitableTimer is relative in 100 ns units and wakes much closer to the
     * requested delay (important for the ~5.8 ms sound board chunk cadence).
     */
    timer = CreateWaitableTimerW(NULL, TRUE, NULL);
    if (!timer) {
        DWORD ms = (DWORD)((ns + 999999ll) / 1000000ll);
        if (ms == 0)
            ms = 1;
        Sleep(ms);
        return 0;
    }
    /* Negative = relative; 100 ns ticks. */
    due.QuadPart = -(ns / 100);
    if (due.QuadPart == 0)
        due.QuadPart = -1;
    if (!SetWaitableTimer(timer, &due, 0, NULL, NULL, FALSE)) {
        CloseHandle(timer);
        Sleep(1);
        return 0;
    }
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
    return 0;
}
