/* Race sub-slot 3 @ 0x1D160 — post-countdown race body.
 * After slot2 phase5 bumps 0x2020a8 to 6, table[3] runs here each frame.
 * Even first hit: 2020a8 += 1. Then: pose ring step, HUD helpers (1ebd0 still
 * call_rom), gate_walk, lap_tick, peer/checkpoint logic, and optional advances
 * to sub=8 (slot4) or sub=10 (slot5).
 * source: disasm/maincpu/maincpu_01d160_520.asm */
// @rom 0x1d160 +0x510 game_start_race_slot3

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdint.h>
#include <stdio.h>

void game_start_race_slot3(u32 arg0, u32 arg1, u32 arg2)
{
    u32 sub;
    u32 timer;
    u32 peers;
    u32 r4_hit;
    u32 g5_i;
    u32 g0_best;
    u32 g13_cap;
    u32 ac98;
    u32 c8;
    u32 c4;
    u32 pair_lo;
    u32 pair_hi;
    u32 span;
    u32 aea4;
    u32 aeac;
    u32 speed;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    /* @0x1D160: save g8 in r12 — host ignores; restored at ret. */
    sub = i960_ld_u8(I960_WORKRAM, 0x2020a8, 0);
    if ((sub & 1u) == 0u) {
        sub = i960_ld_u32(I960_WORKRAM, 0x2020a8, 0);
        i960_st_u32(I960_WORKRAM, 0x2020a8, 0, sub + 1u);
    }
    if (!logged) {
        fprintf(stderr, "lift: race_slot3 sub=%u\n",
                (unsigned)i960_ld_u32(I960_WORKRAM, 0x2020a8, 0));
        fflush(stderr);
        logged = 1;
    }

    game_start_race_pose_ring_step(0, 0, 0);
    i960_call_rom(0x1ebd0);
    g0 = i960_ld_u32(I960_WORKRAM, 0x2140c8, 0);
    if ((i32)g0 > 0)
        i960_call_rom(0x40820);
    game_start_race_gate_walk(0, 0, 0);
    game_start_race_lap_tick(0, 0, 0);

    timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
    r4_hit = 0;
    /* @0x1D1AC: timer >= 0 → skip peer block to 0x1D2FC. */
    if ((i32)timer < 0
        && i960_ld_u32(I960_WORKRAM, 0x20a758, 0) == 0u) {
        peers = (u32)(i32)(int8_t)i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
        g5_i = 0;
        if ((i32)peers >= 0) {
            /* peer count <= 0 as signed byte → skip scan (cmpibge 0,g1). */
            if ((i32)peers > 0) {
                u32 slot = 0x01a121dcu;
                u32 alt = 0x01a12348u;
                u32 alt_off = 0;
                u32 match = i960_ld_u32(I960_WORKRAM, 0x20a584, 0);
                u32 stride = 7u << 6;

                while ((i32)g5_i < (i32)peers) {
                    u32 peer_t = i960_ld_u32(I960_ABS, slot, 0);

                    if (peer_t == timer) {
                        u32 kind_ea = 0x01a12348u + alt_off;
                        u32 kind = i960_ld_u32(I960_ABS,
                                              i960_ld_u32(I960_ABS, kind_ea, 0),
                                              0);

                        if (kind == 1u) {
                            /* @0x1D3C8: clear a handler word; sticky r4. */
                            u32 idx = i960_ld_u32(
                                I960_ABS,
                                i960_ld_u32(I960_ABS, 0x01a1234cu + alt_off, 0),
                                0);
                            u32 h_ea = 0x20ab70u + (idx << 2);
                            u32 h = i960_ld_u32(I960_WORKRAM, h_ea, 0);

                            r4_hit = i960_ld_u32(I960_ABS, h, 0);
                            g14 = 0;
                            i960_st_u32(I960_ABS, h, 0, 0u);
                            break;
                        }
                        {
                            u32 k0 = i960_ld_u32(I960_ABS, alt, 0);

                            if (k0 == 2u && g5_i == match) {
                                /* @0x1D3B8 */
                                i960_st_u32(I960_WORKRAM, 0x20aea4, 0, 1u);
                                goto after_peer_scan;
                            }
                        }
                    }
                    g5_i++;
                    alt += stride;
                    alt_off += stride;
                    slot += stride;
                }
            }
        }

    after_peer_scan:
        if ((i32)r4_hit < 0) {
            u32 b0b0 = i960_ld_u32(I960_WORKRAM, 0x20b0b0, 0);

            i960_st_u32(I960_WORKRAM, 0x20b0b0, 0, b0b0 + r4_hit);
            g0 = 0x7fu;
            g1 = 0x7fu;
            tile_texture_descriptor_apply(0x7fu, 0x7fu, 0);
            g0 = 4;
            comm_palette_index_call(4);
        }
        if (r4_hit == 0u) {
            u32 rank = i960_ld_u32(I960_WORKRAM, 0x20a584, 0);
            u32 pcount = (u32)(i32)(int8_t)i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
            u32 scale = (rank << 3) - rank;
            u32 row = 0x01a1235cu + (scale << 6);
            u32 cap = i960_ld_u32(I960_ABS, row, 0);

            g0_best = 0;
            g5_i = 0;
            if ((i32)pcount > 0) {
                u32 slot = 0x01a121dcu;
                u32 off = 0;
                u32 stride = 7u << 6;

                while ((i32)g5_i < (i32)pcount) {
                    u32 peer_t = i960_ld_u32(I960_ABS, slot, 0);

                    if (peer_t == timer && g5_i != rank) {
                        u32 v = i960_ld_u32(
                            I960_ABS,
                            i960_ld_u32(I960_ABS, 0x01a1235cu + off, 0),
                            0);

                        if ((i32)v > (i32)g0_best)
                            g0_best = v;
                    }
                    g5_i++;
                    off += stride;
                    slot += stride;
                }
            }
            g13_cap = cap;
            if ((i32)g0_best > (i32)g13_cap) {
                u32 b0b0 = i960_ld_u32(I960_WORKRAM, 0x20b0b0, 0);
                u32 delta = (g0_best - g13_cap) >> 1;

                i960_st_u32(I960_WORKRAM, 0x20b0b0, 0, b0b0 + delta);
            }
        }
    }

    /* ——— checkpoint / progress @ 0x1D2FC ——— */
    ac98 = i960_ld_u32(I960_WORKRAM, 0x20ac98, 0);
    c8 = i960_ld_u32(I960_WORKRAM, 0x2140c8, 0);
    pair_lo = i960_ld_u32(I960_WORKRAM, 0x20aca0u + (ac98 << 3), 0);
    pair_hi = i960_ld_u32(I960_WORKRAM, 0x20aca0u + (ac98 << 3) + 4u, 0);

    if (pair_lo == c8) {
        if ((i32)pair_hi <= 0) {
            /* @0x1D3E8 path when r4==0 from ldl pair — pair_hi is r5. */
            u32 c0 = i960_ld_u32(I960_WORKRAM, 0x2020c0, 0);
            u32 lim = c0 * 0x12cu + 0x118u;

            c4 = i960_ld_u32(I960_WORKRAM, 0x2140c4, 0);
            if ((i32)c4 > (i32)lim) {
                game_start_race_hud_lap_time(0, 0, 0);
                i960_call_rom(0x1c940);
                ac98 = i960_ld_u32(I960_WORKRAM, 0x20ac98, 0);
                i960_st_u32(I960_WORKRAM, 0x20a760, 0, 1u);
                i960_st_u32(I960_WORKRAM, 0x20aea0, 0, 31u + 29u);
                timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
                i960_st_u32(I960_WORKRAM, 0x20ac98, 0, ac98 + 1u);
                i960_st_u32(I960_WORKRAM, 0x20a770, 0, ac98);
                i960_st_u32(I960_WORKRAM, 0x20a774, 0, ac98);
                if ((i32)timer > 0
                    || i960_ld_u32(I960_WORKRAM, 0x20a758, 0) != 0u) {
                    if ((i32)pair_hi >= 0) {
                        u32 b0b0 = i960_ld_u32(I960_WORKRAM, 0x20b0b0, 0);

                        i960_st_u32(I960_WORKRAM, 0x20b0b0, 0,
                                    b0b0 + pair_hi);
                        g0 = 0x7fu;
                        g1 = 0x7fu;
                        tile_texture_descriptor_apply(0x7fu, 0x7fu, 0);
                        g0 = 4;
                        comm_palette_index_call(4);
                    }
                }
            }
        } else {
            /* @0x1D31C: progress toward slot4 (sub=8). */
            u32 c0x = i960_ld_u32(I960_WORKRAM, 0x2020c4, 0);
            u32 lim = c0x * 0x12cu - 10u;

            c4 = i960_ld_u32(I960_WORKRAM, 0x2140c4, 0);
            if ((i32)c4 > (i32)lim) {
                g0 = i960_ld_u32(I960_WORKRAM, 0x20b0b0, 0);
                i960_st_u32(I960_WORKRAM, 0x2020a8, 0, 8u);
                game_start_race_aea8_set((u32)g0, 0, 0);
                {
                    u32 bc = i960_ld_u32(I960_WORKRAM, 0x2020bc, 0);
                    u32 course = i960_host_race_course_index();

                    i960_st_u32(I960_WORKRAM, 0x2020b0, 0, 1u);
                    g0 = 5;
                    i960_st_u32(I960_WORKRAM,
                                0x2021d0u + (course << 2), 0, bc);
                    i960_st_u32(I960_WORKRAM, 0x20afb0, 0, bc);
                    comm_palette_index_call(5);
                    if (i960_ld_u32(I960_WORKRAM, 0x202230, 0) == 1u) {
                        g0 = i960_ld_u32(I960_WORKRAM, 0x2020bc, 0);
                        i960_call_rom(0x18270);
                    }
                    g14 = 0;
                    i960_st_u32(I960_WORKRAM, 0x20a770, 0, 0u);
                    i960_st_u32(I960_WORKRAM, 0x20a760, 0, 2u);
                }
            }
        }
    } else {
        /* @0x1D4F4: pair mismatch — tick aea0 countdown. */
        if (i960_ld_u32(I960_WORKRAM, 0x20a760, 0) == 1u) {
            u32 aea0 = i960_ld_u32(I960_WORKRAM, 0x20aea0, 0) - 1u;

            i960_st_u32(I960_WORKRAM, 0x20aea0, 0, aea0);
            if (aea0 == (u32)-1) {
                g14 = 0;
                i960_st_u32(I960_WORKRAM, 0x20a760, 0, 0u);
                i960_st_u32(I960_WORKRAM, 0x20a770, 0, 0u);
            }
        }
    }

    /* ——— epilogue @ 0x1D52C ——— */
    span = i960_ld_u32(I960_WORKRAM, 0x20b0b0, 0);
    aea4 = i960_ld_u32(I960_WORKRAM, 0x20aea4, 0);
    if (span == 0u || aea4 != 0u) {
        aeac = i960_ld_u32(I960_WORKRAM, 0x20aeac, 0);
        if (aeac == 0u)
            i960_st_u32(I960_WORKRAM, 0x20aeac, 0, 0x12cu);
        /* g4=1 path */
        aeac = i960_ld_u32(I960_WORKRAM, 0x20aeac, 0);
        i960_st_u32(I960_WORKRAM, 0x214120, 0, 2u);
        aeac = aeac - 1u;
        i960_st_u32(I960_WORKRAM, 0x20aeac, 0, aeac);
        if (aeac == 0u) {
            speed = i960_ld_u32(I960_WORKRAM, 0x20b0b4, 0);
            speed &= 0x7fffffffu;
            {
                /* double 0x3fb7b425ed097b42 */
                double lim = 0.0;
                u32 lo = 0xed097b42u;
                u32 hi = 0x3fb7b425u;
                union {
                    u32 w[2];
                    double d;
                } u;

                u.w[0] = lo;
                u.w[1] = hi;
                lim = u.d;
                if (!(i960_u32_to_f64(speed) >= lim))
                    i960_st_u32(I960_WORKRAM, 0x2020a8, 0, 10u);
            }
        }
    } else {
        i960_st_u32(I960_WORKRAM, 0x214120, 0, 1u);
        if ((i32)span > 0x12c) {
            u32 rem = span % 60u;

            if (rem == 0u) {
                g0 = 1u << 7;
                g1 = 1u << 6;
                tile_texture_descriptor_apply((u32)g0, (u32)g1, 0);
            }
        }
    }

    g0 = 0;
    game_start_race_obj_scan(0, 0, 0);

    timer = i960_ld_u32(I960_WORKRAM, 0x20a560, 0);
    if ((i32)timer < 0) {
        if ((u32)g0 == 0u) {
            u32 flags = i960_ld_u32(I960_WORKRAM, 0x202008, 0);

            if ((flags & (1u << 5)) != 0u) {
                u8 b = i960_ld_u8(I960_WORKRAM, 0x20204c, 0);

                b = (u8)(b | 0x80u);
                i960_st_u8(I960_WORKRAM, 0x20204c, 0, b);
                goto after_04c;
            }
        }
        {
            u8 b = i960_ld_u8(I960_WORKRAM, 0x20204c, 0);

            i960_st_u8(I960_WORKRAM, 0x20204c, 0, (u8)(b & 0x7fu));
        }
    }
after_04c:
    i960_st_u32(I960_WORKRAM, 0x2020c8, 0, (u32)g0);
    {
        u8 b = i960_ld_u8(I960_WORKRAM, 0x20201d, 0);

        if (b > 1u) {
            g0 = i960_ld_u32(I960_WORKRAM, 0x213b94, 0);
            i960_call_rom(0x1ea40);
        }
    }
}
