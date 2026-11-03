/* Deterministic transcendental functions for gameplay and the TGP HLE.
 *
 * Platform libm results differ in the last bit between macOS, glibc, MSVC and
 * newlib on the Dreamcast, and gameplay feeds angles back into the simulation.
 * These use only IEEE + - * / and sqrt in double, so every target that builds
 * them without FMA contraction (-ffp-contract=off) gets the same bits. The host
 * reference and dreamcast-native both call these. Kernels and constants are
 * fdlibm's (Sun Microsystems, freely distributable; see THIRD_PARTY notice in
 * docs/dreamcast-port.md). Accuracy is about 1 ulp for |x| < 2^19 * pi/2, the
 * range angles in this game stay in; reduction is two-step Cody-Waite.
 */
#ifndef M2_MATH_H
#define M2_MATH_H

#include <math.h>
#include <stdint.h>
#include <string.h>

/* isfinite() as a test of the exponent bits: the same answer for every value, without the library calls
 * (__unordsf2 / __unorddf2) SH-4 GCC makes for isfinite under IEEE comparisons, which cost the Dreamcast's race
 * tick about a tenth of its time. */
static inline int m2_isfinitef(float x)
{
    uint32_t u;
    memcpy(&u, &x, 4);
    return (u & 0x7f800000u) != 0x7f800000u;
}

static inline int m2_isfinite(double x)
{
    uint64_t u;
    memcpy(&u, &x, 8);
    return (u & 0x7ff0000000000000ull) != 0x7ff0000000000000ull;
}

static inline double m2__ksin(double x)
{
    const double S1 = -1.66666666666666324348e-01, S2 = 8.33333333332248946124e-03,
                 S3 = -1.98412698298579493134e-04, S4 = 2.75573137070700676789e-06,
                 S5 = -2.50507602534068634195e-08, S6 = 1.58969099521155010221e-10;
    double z = x * x;
    double v = z * x;
    double r = S2 + z * (S3 + z * (S4 + z * (S5 + z * S6)));
    return x + v * (S1 + z * r);
}

static inline double m2__kcos(double x)
{
    const double C1 = 4.16666666666666019037e-02, C2 = -1.38888888888741095749e-03,
                 C3 = 2.48015872894767294178e-05, C4 = -2.75573143513906633035e-07,
                 C5 = 2.08757232129817482790e-09, C6 = -1.13596475577881948265e-11;
    double z = x * x;
    double r = z * (C1 + z * (C2 + z * (C3 + z * (C4 + z * (C5 + z * C6)))));
    double hz = 0.5 * z;
    double w = 1.0 - hz;
    return w + (((1.0 - w) - hz) + z * r);
}

/* x = n*pi/2 + y with |y| <= pi/4; returns n mod 4. */
static inline int m2__rem_pio2(double x, double *y)
{
    const double invpio2 = 6.36619772367581382433e-01;
    const double pio2_1 = 1.57079632673412561417e+00, pio2_1t = 6.07710050650619224932e-11;
    const double pio2_2 = 6.07710050630396597660e-11, pio2_2t = 2.02226624879595063154e-21;
    double fn, r, w;
    long n;

    if (x > -7.85398163397448278999e-01 && x < 7.85398163397448278999e-01) {
        *y = x;
        return 0;
    }
    fn = x * invpio2;
    fn = fn < 0.0 ? -(double)(long)(0.5 - fn) : (double)(long)(fn + 0.5);
    n = (long)fn;
    r = x - fn * pio2_1;
    w = fn * pio2_1t;
    if (n > 32 || n < -32) {
        /* second step: pi/2 to 85 more bits */
        double t = r;
        w = fn * pio2_2;
        r = t - w;
        w = fn * pio2_2t - ((t - r) - w);
    }
    *y = r - w;
    return (int)(n & 3);
}

static inline double m2_sin(double x)
{
    double y;
    int n;

    if (x != x || x == INFINITY || x == -INFINITY)
        return x - x;
    n = m2__rem_pio2(x, &y);
    switch (n) {
    case 0: return m2__ksin(y);
    case 1: return m2__kcos(y);
    case 2: return -m2__ksin(y);
    default: return -m2__kcos(y);
    }
}

static inline double m2_cos(double x)
{
    double y;
    int n;

    if (x != x || x == INFINITY || x == -INFINITY)
        return x - x;
    n = m2__rem_pio2(x, &y);
    switch (n) {
    case 0: return m2__kcos(y);
    case 1: return -m2__ksin(y);
    case 2: return -m2__kcos(y);
    default: return m2__ksin(y);
    }
}

static inline double m2_atan(double x)
{
    static const double atanhi[4] = {
        4.63647609000806093515e-01, 7.85398163397448278999e-01,
        9.82793723247329054082e-01, 1.57079632679489655800e+00,
    };
    static const double atanlo[4] = {
        2.26987774529616870924e-17, 3.06161699786838301793e-17,
        1.39033110312309984516e-17, 6.12323399573676603587e-17,
    };
    static const double aT[11] = {
        3.33333333333329318027e-01, -1.99999999998764832476e-01,
        1.42857142725034663711e-01, -1.11111104054623557880e-01,
        9.09088713343650656196e-02, -7.69187620504482999495e-02,
        6.66107313738753120669e-02, -5.83357013379057348645e-02,
        4.97687799461593236017e-02, -3.65315727442169155270e-02,
        1.62858201153657823623e-02,
    };
    double ax, z, w, s1, s2;
    int id;
    int neg = x < 0.0;

    if (x != x)
        return x;
    ax = neg ? -x : x;
    if (ax >= 7.3786976294838206464e19) { /* 2^66 */
        z = atanhi[3] + atanlo[3];
        return neg ? -z : z;
    }
    if (ax < 0.4375) {
        if (ax < 3.7252902984e-09) /* 2^-28 */
            return x;
        id = -1;
    } else if (ax < 1.1875) {
        if (ax < 0.6875) {
            id = 0;
            ax = (2.0 * ax - 1.0) / (2.0 + ax);
        } else {
            id = 1;
            ax = (ax - 1.0) / (ax + 1.0);
        }
    } else if (ax < 2.4375) {
        id = 2;
        ax = (ax - 1.5) / (1.0 + 1.5 * ax);
    } else {
        id = 3;
        ax = -1.0 / ax;
    }
    z = ax * ax;
    w = z * z;
    s1 = z * (aT[0] + w * (aT[2] + w * (aT[4] + w * (aT[6] + w * (aT[8] + w * aT[10])))));
    s2 = w * (aT[1] + w * (aT[3] + w * (aT[5] + w * (aT[7] + w * aT[9]))));
    if (id < 0)
        z = ax - ax * (s1 + s2);
    else
        z = atanhi[id] - ((ax * (s1 + s2) - atanlo[id]) - ax);
    return neg ? -z : z;
}

static inline double m2_atan2(double y, double x)
{
    const double pi = 3.1415926535897931160e+00, pi_lo = 1.2246467991473531772e-16;
    const double pio2 = 1.5707963267948965580e+00;
    double z;

    if (x != x || y != y)
        return x + y;
    if (x == 1.0)
        return m2_atan(y);
    if (y == 0.0) {
        if (x > 0.0 || (x == 0.0 && !signbit(x)))
            return y;
        return signbit(y) ? -pi : pi;
    }
    if (x == 0.0)
        return y < 0.0 ? -pio2 : pio2;
    if (x == INFINITY || x == -INFINITY || y == INFINITY || y == -INFINITY)
        return atan2(y, x); /* never reached by gameplay; exact cases only */
    z = m2_atan(fabs(y / x));
    if (x > 0.0)
        return y < 0.0 ? -z : z;
    z = pi - (z - pi_lo);
    return y < 0.0 ? -z : z;
}

static inline double m2_asin(double x)
{
    if (x != x || x > 1.0 || x < -1.0)
        return (x - x) / (x - x);
    return m2_atan2(x, sqrt((1.0 - x) * (1.0 + x)));
}

static inline float m2_sinf(float x) { return (float)m2_sin((double)x); }
static inline float m2_cosf(float x) { return (float)m2_cos((double)x); }
static inline float m2_atan2f(float y, float x) { return (float)m2_atan2((double)y, (double)x); }

#endif
