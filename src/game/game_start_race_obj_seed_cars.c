/* Practice/solo car seed @ 0x217E0 — course table @ 0x5DD190[course].
 *
 * Mode-3 practice path: phase_0 leaves timer = −1; cmpibge 0,timer at
 * 0x219DC branches to here when timer <= 0 (Intel/Ghidra: src1 >= src2).
 * The prior lift took 0x21918 instead, so 0x213b40 never received drive
 * nodes from this table — chase tracked a static cam-epilogue pose.
 *
 * Per entry (24 bytes, sentinel −1): alloc CGM node, copy pose words into
 * the private frame / node, link 0x213b40[i] at node+0x8c, call 0x2C850.
 *
 * source: disasm/maincpu/maincpu_0217e0_140.asm */
// @rom 0x217e0 +0x120 game_start_race_obj_seed_cars

#include "i960_lift.h"
#include "i960_mem.h"
#include "i960_host.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

void game_start_race_obj_seed_cars(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x80];
    u32 course;
    u32 table;
    u32 entry;
    static int logged;

    (void)arg0;
    (void)arg1;
    (void)arg2;

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    /* @0x217E0: lda 0x20(sp),sp — private frame. */
    sp = sp + 0x20u;

    if (!logged) {
        fprintf(stderr, "lift: race_obj_seed_cars (0x217e0)\n");
        fflush(stderr);
        logged = 1;
    }

    /* --practice: desert course table. 214354 may still be the car index (3). */
    if (i960_host_skip_practice())
        course = i960_host_race_course_index();
    else
        course = i960_ld_u32(I960_WORKRAM, 0x214354, 0);
    table = model2_workram_mirror_u32(0x5dd190u + (course << 2));
    i960_st_u32(I960_WORKRAM, 0x2021f0, 0, 0);
    i960_st_u32(I960_WORKRAM, 0x2139c8, 0, 1u);

    entry = i960_ld_u32(I960_ABS, table, 0);
    while (entry != (u32)-1) {
        u32 node;
        u32 slot;
        u32 pose;
        u32 payload; /* guest VA mirroring 0x40(fp) for 0x2C850 */
        u8 b;
        u32 w0, w1, w2, w3;
        u8 old_count_b;
        u8 fl;

        g0 = 0;
        cgm_scratch_pool_alloc_link(0, 0, 0);
        node = (u32)g0;
        if (node == 0u)
            goto next_entry;

        /*
         * Hardware keeps ldq at 0x40(fp) on the guest stack. Host fp is a
         * private buffer, so park the same 5 words at node+0x40 (CGM node is
         * 0x100 bytes; 0x2C850 reads g1 before writing those slots).
         */
        payload = node + 0x40u;
        (void)frame;

        w0 = i960_ld_u32(I960_ABS, table, 0);
        w1 = i960_ld_u32(I960_ABS, table, 4);
        w2 = i960_ld_u32(I960_ABS, table, 8);
        w3 = i960_ld_u32(I960_ABS, table, 12);
        w0 &= ~1u;
        i960_st_u32(I960_ABS, payload, 0, w0);
        i960_st_u32(I960_ABS, payload, 4, w1);
        i960_st_u32(I960_ABS, payload, 8, w2);
        i960_st_u32(I960_ABS, payload, 12, w3);
        /* table+16/+20 also live at 0x50(fp) on hardware; 0x2C850 uses g1 only. */
        i960_st_u32(I960_ABS, payload, 16, i960_ld_u32(I960_ABS, table + 16u, 0));
        i960_st_u32(I960_ABS, payload, 20, i960_ld_u32(I960_ABS, table + 20u, 0));

        /* @0x21858–0x21864: node flags &= 0xc3 | 4. */
        b = i960_ld_u8(I960_ABS, node, 0);
        b = (u8)((b & 0xc3u) | 4u);
        i960_st_u8(I960_ABS, node, 0, b);

        slot = i960_ld_u32(I960_WORKRAM, 0x2139c8, 0);
        /* @0x21870–0x21880: node+17 &= ~0x3 (subo 4,0 → mask 0xfffffffc). */
        b = i960_ld_u8(I960_ABS, node + 17u, 0);
        b = (u8)(b & (u8)~3u);
        i960_st_u8(I960_ABS, node + 17u, 0, b);

        pose = 0x00213b40u + slot * 0x58u;
        i960_st_u32(I960_ABS, node, 0x8c, pose);

        old_count_b = i960_ld_u8(I960_WORKRAM, 0x2139c8, 0);
        i960_st_u32(I960_WORKRAM, 0x2139c8, 0, slot + 1u);
        i960_st_u32(I960_WORKRAM, 0x2140d0u + (slot << 2), 0, node);

        /* @0x218A4–0x218C8: pose+0x51 = (flags & 0xc3) | ((old_count & 15) << 2). */
        fl = i960_ld_u8(I960_ABS, pose + 0x51u, 0);
        fl = (u8)((fl & 0xc3u) | (u8)(((old_count_b & 15u) << 2)));
        i960_st_u8(I960_ABS, pose + 0x51u, 0, fl);

        g0 = node;
        g1 = payload;
        game_start_race_obj_car_attach((u32)g0, (u32)g1, 0);

    next_entry:
        table += 24u;
        entry = i960_ld_u32(I960_ABS, table, 0);
    }

    {
        u32 n = i960_ld_u32(I960_WORKRAM, 0x2139c8, 0);

        i960_st_u32(I960_WORKRAM, 0x20caf0, 0, 0xffffffffu);
        i960_st_u32(I960_WORKRAM, 0x2140d0u + (n << 2), 0, 0);
        {
            u32 player = i960_ld_u32(I960_WORKRAM, 0x2140d4, 0);

            fprintf(stderr,
                    "lift: race_obj_seed_cars count=%u course=%u "
                    "pose50=%#x node=%#x n10=%#x\n",
                    (unsigned)n, (unsigned)course,
                    (unsigned)i960_ld_u8(I960_ABS, 0x00213b98u, 0x50),
                    (unsigned)player,
                    player ? (unsigned)i960_ld_u8(I960_ABS, player, 0x10) : 0u);
        }
        fflush(stderr);
    }

    fp = fp_save;
    sp = sp_save;
}
