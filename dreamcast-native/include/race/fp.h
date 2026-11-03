/* Float rules of the reference, for ported gameplay (docs/dreamcast-port.md).
 *
 * The lifted code keeps float words as raw bits and computes in double. One op then a store to float equals the
 * IEEE single op (fadd/fsub/fmul/fdiv below). Where it chains several ops in double before one rounding, the port
 * computes in double too (D()/F()): products of two floats are exact in double, so float code would round
 * differently. Gameplay builds with -fno-fast-math -ffp-contract=off. */
#ifndef SR_RACE_FP_H
#define SR_RACE_FP_H
#include <stdint.h>
#include "sr_mem.h"

#ifdef _arch_dreamcast
/* The SH-4 runs with denormals flushed to zero (FPSCR.DN, as KOS sets it): a denormal operand reads as zero and a
 * result in the denormal range becomes zero. The i960 and the host keep them, and the game produces them (the
 * steering smoothing 0x2142c0 decays towards zero by x 0.1 a tick): with the wheel centred the Dreamcast went
 * apart from the reference some 40 ticks later. Here those cases take a software path: exact conversions, and
 * the float ops redone in double (products are exact there; sums and quotients that end up tiny are too). */
static inline int sr_fp_low(uint32_t bits) { return !(bits & 0x7f800000u); } /* zero or denormal */

static inline double D(uint32_t bits) {
    if(sr_fp_low(bits) && (bits & 0x7fffffu)) {
        const double m = (double)(int32_t)(bits & 0x7fffffu) * 0x1p-149;
        return (bits >> 31) ? -m : m;
    }
    return (double)sr_f(bits);
}

static inline uint32_t F(double d) {
    const double a = d < 0. ? -d : d;
    if(a < 0x1p-126 && a != 0.) {
        /* the denormal's mantissa, rounded to nearest even (2^23 is the smallest normal, encoded the same way) */
        const double m = (a * 0x1p149 + 0x1p52) - 0x1p52;
        return (d < 0. ? 0x80000000u : 0u) | (uint32_t)m;
    }
    return sr_b((float)d);
}

#define SR_FP_OP(name, op)                                                                                         \
    static inline uint32_t name(uint32_t a, uint32_t b) {                                                          \
        const uint32_t r = sr_b(sr_f(a) op sr_f(b));                                                               \
        return sr_fp_low(a) || sr_fp_low(b) || sr_fp_low(r) ? F(D(a) op D(b)) : r;                                \
    }
SR_FP_OP(fadd, +)
SR_FP_OP(fsub, -)
SR_FP_OP(fmul, *)
SR_FP_OP(fdiv, /)
#undef SR_FP_OP
#else
static inline double D(uint32_t bits) { return (double)sr_f(bits); }
static inline uint32_t F(double d) { return sr_b((float)d); }

static inline uint32_t fadd(uint32_t a, uint32_t b) { return sr_b(sr_f(a) + sr_f(b)); }
static inline uint32_t fsub(uint32_t a, uint32_t b) { return sr_b(sr_f(a) - sr_f(b)); }
static inline uint32_t fmul(uint32_t a, uint32_t b) { return sr_b(sr_f(a) * sr_f(b)); }
static inline uint32_t fdiv(uint32_t a, uint32_t b) { return sr_b(sr_f(a) / sr_f(b)); }
#endif
static inline uint32_t fneg(uint32_t a) { return a ^ 0x80000000u; }
static inline uint32_t fabs_bits(uint32_t a) { return a & 0x7fffffffu; }
static inline int finite_bits(uint32_t a) { return (a & 0x7f800000u) != 0x7f800000u; }

/* The i960 leaves the destination unchanged on an invalid operation; the lifted integrate keeps the first
 * operand when the double result is not finite, which with float inputs means when an input is not finite. */
static inline uint32_t gadd(uint32_t a, uint32_t b) { return finite_bits(a) && finite_bits(b) ? fadd(a, b) : a; }
static inline uint32_t gsub(uint32_t a, uint32_t b) { return finite_bits(a) && finite_bits(b) ? fsub(a, b) : a; }
static inline uint32_t gmul(uint32_t a, uint32_t b) { return finite_bits(a) && finite_bits(b) ? fmul(a, b) : a; }

/* Frame slots the original shares with its callers through the i960 frame pointer. */
static inline uint32_t frame_ld(const uint8_t *frame, uint32_t off) { uint32_t v; memcpy(&v, frame + off, 4); return v; }
static inline void frame_st(uint8_t *frame, uint32_t off, uint32_t v) { memcpy(frame + off, &v, 4); }

#endif
