/* Race object-slot bind @ 0x23110 — called from attract_hud_setup logo_path
 * and every game_start_race_frame with g0 = 0x20a560 (timer).
 *
 * Disasm @ 0x23110:
 *   cmpible 0,g0 → 0x22e20 when timer >= 0 (Intel: src1 <= src2)
 *   else → 0x22c70 when timer < 0
 *
 * Solo practice phase_0 leaves timer = −1, so 0x22c70 must run — it stores
 * 0x213b40 into 0x213980[]. The prior lift inverted cmpible and invented a
 * practice retarget of m2comm → 0x213b40 inside 0x22e20.
 *
 * source: disasm/maincpu/maincpu_023110_20.asm */
// @rom 0x23110 +0x14 game_start_race_obj_bind

#include "i960_lift.h"
#include "i960_fp.h"
#include "i960_mem.h"
#include "model2_rom.h"

#include "lift_syms.h"

#include <stdio.h>
#include <string.h>

void game_start_race_obj_list(u32 arg0, u32 arg1, u32 arg2);
void game_start_race_obj_publish(u32 arg0, u32 arg1, u32 arg2);
void game_start_race_obj_sort_index(u32 arg0, u32 arg1, u32 arg2);
void game_start_race_obj_depth_pass(u32 arg0, u32 arg1, u32 arg2);

void game_start_race_obj_bind(u32 arg0, u32 arg1, u32 arg2)
{
    static int logged;

    (void)arg1;
    (void)arg2;

    if (!logged) {
        fprintf(stderr, "lift: race_obj_bind timer=0x%x → %s\n",
                (unsigned)arg0,
                ((i32)arg0 >= 0) ? "list(0x22e20)" : "publish(0x22c70)");
        fflush(stderr);
        logged = 1;
    }

    /* @0x23110: cmpible 0,g0 → 0x22e20 when timer >= 0. */
    if ((i32)arg0 >= 0)
        game_start_race_obj_list(arg0, arg1, arg2);
    else
        game_start_race_obj_publish(arg0, arg1, arg2);
}

/* Object list publisher @ 0x22e20 — timer<=0 path. */
// @rom 0x22e20 +0x2f0 game_start_race_obj_list

void game_start_race_obj_list(u32 arg0, u32 arg1, u32 arg2)
{
    uintptr_t fp_save = fp;
    uintptr_t sp_save = sp;
    u8 frame[0x100];
    u32 count;
    u32 i;
    u32 scaled;
    u32 base584;
    u32 need;
    u32 cursor;
    u32 list_ptr;
    u32 timer = arg0;
    static int logged;

    (void)arg1;
    (void)arg2;

    if (!logged) {
        fprintf(stderr, "lift: race_obj_list (publish 0x213980)\n");
        fflush(stderr);
        logged = 1;
    }

    memset(frame, 0, sizeof(frame));
    fp = (uintptr_t)frame;
    /* @0x22E20: lda 0xd0(sp),sp — private frame; save g8–g12 omitted (host). */
    sp = sp + 0xd0u;

    count = i960_ld_u32(I960_WORKRAM, 0x2139c8, 0);
    list_ptr = 0x00214130u;
    *(u32 *)(frame + 0x40) = list_ptr;
    if (count > 4u)
        count = 4u;
    *(u32 *)(frame + 0x50) = count;

    /*
     * @0x22E58–0x22F98: for each of count objects, copy 0x58-byte pose blocks
     * between 0x213b40… and 0x20a5b8… windows (ldq/stq pairs).
     */
    if ((i32)count > 0) {
        u32 src_b40 = 0x00213b40u;
        u32 src_b50 = 0x00213b50u;
        u32 src_b60 = 0x00213b60u;
        u32 src_b70 = 0x00213b70u;
        u32 src_b90 = 0x00213b90u;
        u32 dst_a5b8 = 0x0020a5b8u;
        u32 dst_a5c8 = 0x0020a5c8u;
        u32 dst_a5d8 = 0x0020a5d8u;
        u32 dst_a5e8 = 0x0020a5e8u;
        u32 dst_a608 = 0x0020a608u;
        u32 dst_a608_pre = dst_a608 - 16u;
        u32 end = dst_a608 + count * 0x58u;

        *(u32 *)(frame + 0x60) = dst_a608_pre;
        *(u32 *)(frame + 0x70) = src_b90 - 16u;
        *(u32 *)(frame + 0x80) = dst_a5e8;
        *(u32 *)(frame + 0x90) = src_b70;
        *(u32 *)(frame + 0xa0) = dst_a5d8;
        *(u32 *)(frame + 0xb0) = src_b60;
        *(u32 *)(frame + 0xc0) = end;

        while (dst_a608 < end) {
            u32 w0, w1, w2, w3;

            w0 = i960_ld_u32(I960_WORKRAM, src_b40, 0);
            w1 = i960_ld_u32(I960_WORKRAM, src_b40, 4);
            w2 = i960_ld_u32(I960_WORKRAM, src_b40, 8);
            w3 = i960_ld_u32(I960_WORKRAM, src_b40, 12);
            i960_st_u32(I960_WORKRAM, dst_a5b8, 0, w0);
            i960_st_u32(I960_WORKRAM, dst_a5b8, 4, w1);
            i960_st_u32(I960_WORKRAM, dst_a5b8, 8, w2);
            i960_st_u32(I960_WORKRAM, dst_a5b8, 12, w3);

            w0 = i960_ld_u32(I960_WORKRAM, src_b50, 0);
            w1 = i960_ld_u32(I960_WORKRAM, src_b50, 4);
            w2 = i960_ld_u32(I960_WORKRAM, src_b50, 8);
            w3 = i960_ld_u32(I960_WORKRAM, src_b50, 12);
            i960_st_u32(I960_WORKRAM, dst_a5c8, 0, w0);
            i960_st_u32(I960_WORKRAM, dst_a5c8, 4, w1);
            i960_st_u32(I960_WORKRAM, dst_a5c8, 8, w2);
            i960_st_u32(I960_WORKRAM, dst_a5c8, 12, w3);

            w0 = i960_ld_u32(I960_WORKRAM, src_b60, 0);
            w1 = i960_ld_u32(I960_WORKRAM, src_b60, 4);
            w2 = i960_ld_u32(I960_WORKRAM, src_b60, 8);
            w3 = i960_ld_u32(I960_WORKRAM, src_b60, 12);
            i960_st_u32(I960_WORKRAM, dst_a5d8, 0, w0);
            i960_st_u32(I960_WORKRAM, dst_a5d8, 4, w1);
            i960_st_u32(I960_WORKRAM, dst_a5d8, 8, w2);
            i960_st_u32(I960_WORKRAM, dst_a5d8, 12, w3);

            w0 = i960_ld_u32(I960_WORKRAM, src_b70, 0);
            w1 = i960_ld_u32(I960_WORKRAM, src_b70, 4);
            w2 = i960_ld_u32(I960_WORKRAM, src_b70, 8);
            w3 = i960_ld_u32(I960_WORKRAM, src_b70, 12);
            i960_st_u32(I960_WORKRAM, dst_a5e8, 0, w0);
            i960_st_u32(I960_WORKRAM, dst_a5e8, 4, w1);
            i960_st_u32(I960_WORKRAM, dst_a5e8, 8, w2);
            i960_st_u32(I960_WORKRAM, dst_a5e8, 12, w3);

            w0 = i960_ld_u32(I960_WORKRAM, src_b90, 0);
            w1 = i960_ld_u32(I960_WORKRAM, src_b90, 4);
            i960_st_u32(I960_WORKRAM, dst_a608, 0, w0);
            i960_st_u32(I960_WORKRAM, dst_a608, 4, w1);

            src_b40 += 0x58u;
            src_b50 += 0x58u;
            src_b60 += 0x58u;
            src_b70 += 0x58u;
            src_b90 += 0x58u;
            dst_a5b8 += 0x58u;
            dst_a5c8 += 0x58u;
            dst_a5d8 += 0x58u;
            dst_a5e8 += 0x58u;
            dst_a608 += 0x58u;
        }
    }

    /* @0x22F9C: st count → 0x20a5b4. */
    i960_st_u32(I960_WORKRAM, 0x20a5b4, 0, count);

    /*
     * @0x22FB4–0x23008: for i in [0,count): store object VA
     *   0x1a121e8 + scale(0x20a584) into 0x213980[i].
     * First slot also records the base into the 0x214130 list.
     * (timer >= 0 path only — linked / timer_arm; not solo practice −1.)
     */
    base584 = i960_ld_u32(I960_WORKRAM, 0x20a584, 0);
    scaled = ((base584 << 3) - base584) << 6;
    cursor = 0x01a121e8u;
    list_ptr = *(u32 *)(frame + 0x40);
    for (i = 0; (i32)i < (i32)count; i++) {
        u32 obj_va = 0x01a121e8u + scaled;

        if (i == 0u) {
            i960_st_u32(I960_WORKRAM, list_ptr, 0, obj_va);
            list_ptr += 4u;
            *(u32 *)(frame + 0x40) = list_ptr;
        }
        i960_st_u32(I960_WORKRAM, 0x213980u + i * 4u, 0, cursor + scaled);
        cursor += 0x58u;
    }

    {
        static int pose_logged;
        u32 obj0 = i960_ld_u32(I960_WORKRAM, 0x213980, 0);

        if (!pose_logged && obj0 != 0u) {
            fprintf(stderr,
                    "lift: race_obj_list pose obj=%#x xyz=(%.3g,%.3g,%.3g)\n",
                    obj0,
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, obj0, 0)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, obj0, 4)),
                    i960_u32_to_f64(i960_ld_u32(I960_ABS, obj0, 8)));
            fflush(stderr);
            pose_logged = 1;
        }
    }

    /* @0x2300C–0x2301C: 0x213978 = count. */
    i960_st_u32(I960_WORKRAM, 0x213978, 0, count);

    /*
     * @0x23024–0x230E4: for each of need(=0x20a581) peers, optionally append
     * more 0x1a121e8 rows into 0x213980 and the 0x214130 list when the slot
     * timer word matches incoming g0 (race timer).
     */
    need = (u32)(i32)(signed char)i960_ld_u8(I960_WORKRAM, 0x20a581, 0);
    if ((i32)need > 0) {
        u32 peer = 0x01a121e4u;
        u32 peer_base = peer - 8u;
        u32 bit7 = 1u << 7;
        u32 g2;

        for (g2 = 0; (i32)g2 < (i32)need; g2++) {
            u32 slot_tm = i960_ld_u32(I960_ABS, peer_base, 0);

            if (slot_tm == timer
                && g2 != i960_ld_u32(I960_WORKRAM, 0x20a584, 0)) {
                u32 flag_ea = 0x01a12238u + (peer - 0x01a121e4u);
                u8 fl = i960_ld_u8(I960_ABS, flag_ea, 0);
                u32 row0 = i960_ld_u32(I960_ABS, peer, 0);
                u32 row_obj = 0x01a121e8u + (peer - 0x01a121e4u);
                u32 n;
                u32 j;

                i960_st_u8(I960_ABS, flag_ea, 0, (u8)(fl | (u8)bit7));
                list_ptr = *(u32 *)(frame + 0x40);
                i960_st_u32(I960_WORKRAM, list_ptr, 0, row_obj);
                *(u32 *)(frame + 0x40) = list_ptr + 4u;

                n = i960_ld_u32(I960_ABS, peer, 0);
                (void)row0;
                for (j = 0; (i32)j < (i32)n; j++) {
                    u32 slot = i960_ld_u32(I960_WORKRAM, 0x213978, 0);
                    u32 obj_va = 0x01a121e8u + (peer - 0x01a121e4u);

                    i960_st_u32(I960_WORKRAM, 0x213978, 0, slot + 1u);
                    i960_st_u32(I960_WORKRAM, 0x213980u + slot * 4u, 0,
                                obj_va + j * 0x58u);
                }
            }
            peer += 0x40u; /* shlo 6,7 = 64 */
            peer_base += 0x40u;
        }
    }

    /* @0x230E8: terminate list with 0; sort/depth helpers. */
    list_ptr = *(u32 *)(frame + 0x40);
    i960_st_u32(I960_WORKRAM, list_ptr, 0, 0);
    game_start_race_obj_sort_index(0, 0, 0);
    g0 = 1;
    game_start_race_obj_depth_pass(1, 0, 0);

    fp = fp_save;
    sp = sp_save;
}
