#ifndef LIFT_LOG_H
#define LIFT_LOG_H

/* Diagnostic "lift:" traces. Off unless --log-lift or I960_LIFT_LOG=1.
 * One-line status (ROM load, boot, NVRAM) is off unless -v / I960_LIFT_VERBOSE=1.
 * Header-only so Model 2 libs can call it without linking the game host.
 * Failures stay on fprintf and are not gated.
 */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static inline __attribute__((unused)) int lift_log_enabled(void)
{
    static int on = -1;
    const char *env;

    if (on < 0) {
        env = getenv("I960_LIFT_LOG");
        on = (env && env[0] && env[0] != '0') ? 1 : 0;
    }
    return on;
}

static inline __attribute__((unused)) void lift_log(const char *fmt, ...)
{
    va_list ap;

    if (!fmt || !lift_log_enabled())
        return;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);
}

static inline __attribute__((unused)) int lift_verbose_enabled(void)
{
    static int on = -1;
    const char *env;

    if (on < 0) {
        env = getenv("I960_LIFT_VERBOSE");
        on = (env && env[0] && env[0] != '0') ? 1 : 0;
    }
    return on;
}

static inline __attribute__((unused)) void lift_status(const char *fmt, ...)
{
    va_list ap;

    if (!fmt || !lift_verbose_enabled())
        return;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fflush(stderr);
}

#endif
