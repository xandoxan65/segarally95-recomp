/* The object lists the race frame rebuilds each tick (0x1BBA0 calls obj_bind @ 0x23110, then list_flag_prep
 * @ 0x1C2A0): which pose blocks are objects (0x213980), their order along the course (0x20b0c0, 0x213b00,
 * 0x214180), the depth records the renderer walks (0x2139f0) and their level of detail.
 *
 * A solo race (0x20a560 < 0) publishes the local pose blocks (0x22C70); a linked race takes the comm board rows
 * (0x22E20, not reached in practice and so unchecked).
 *
 * Also unchecked by the practice drives: the comm board mirror in obj_publish (0x20a530 stays 0), any shift in the
 * depth sort (it compares against the previous record's own key), and list_flag_prep's level-of-detail path
 * (practice runs the 0x202230 == 1 branch). */
#include "race/race_flow.h"
#include "race/fp.h"
#include "sr_tgp.h"

#define RF_OBJ_RANK     0x20b0c0u /* object indices sorted by span, leader first */
#define RF_OBJ_SORTED   0x213b00u /* object addresses in that order */
#define RF_OBJ_PLACE    0x214180u /* place per object index */
#define RF_PEER_LIST    0x214130u /* zero-terminated pose block list */
#define RF_POSE_MIRROR  0x20a5b8u /* pose blocks copied out for the comm board */
#define RF_MIRROR_COUNT 0x20a5b4u
#define RF_DEPTH_FAR    0x2139d8u /* nonzero: every object in front is drawn */
#define POSE_SPAN       0x56u     /* s16 span index */

void sr_obj_bind(uint32_t net_timer) {
    if((int32_t)net_timer >= 0) sr_obj_list(net_timer);
    else sr_obj_publish();
}

/* Linked race (0x22E20): mirror up to four local pose blocks for the comm board, take the objects from our comm
 * row and from each peer row whose timer matches. Unchecked: practice never has 0x20a560 >= 0. */
void sr_obj_list(uint32_t net_timer) {
    uint32_t count = sr_ld32(RF_POSE_COUNT), i, scaled, list = RF_PEER_LIST;
    int32_t need;

    if(count > 4) count = 4;
    for(i = 0; (int32_t)i < (int32_t)count; i++) {
        const uint32_t src = RF_POSES + i * RF_POSE_SIZE, dst = RF_POSE_MIRROR + i * RF_POSE_SIZE;
        uint32_t k;
        for(k = 0; k < 0x40; k += 4) sr_st32(dst + k, sr_ld32(src + k));
        sr_st32(dst + 0x50, sr_ld32(src + 0x50));
        sr_st32(dst + 0x54, sr_ld32(src + 0x54));
    }
    sr_st32(RF_MIRROR_COUNT, count);

    scaled = sr_ld32(0x20a584) * RF_COMM_STRIDE;
    for(i = 0; (int32_t)i < (int32_t)count; i++) {
        if(i == 0) {
            sr_st32(list, 0x1a121e8u + scaled);
            list += 4;
        }
        sr_st32(RF_OBJ_LIST + i * 4, 0x1a121e8u + i * RF_POSE_SIZE + scaled);
    }
    sr_st32(RF_OBJ_COUNT, count);

    /* Peer rows here are 0x40 apart, as the lift computes them. */
    need = (int8_t)sr_ld8(0x20a581);
    for(i = 0; (int32_t)i < need; i++) {
        const uint32_t peer = 0x1a121e4u + i * 0x40u;
        if(sr_ld32(peer - 8) == net_timer && i != sr_ld32(0x20a584)) {
            const uint32_t flag_ea = 0x1a12238u + i * 0x40u, row_obj = 0x1a121e8u + i * 0x40u;
            const uint32_t n = sr_ld32(peer);
            uint32_t j;
            sr_st8(flag_ea, sr_ld8(flag_ea) | 0x80);
            sr_st32(list, row_obj);
            list += 4;
            for(j = 0; (int32_t)j < (int32_t)n; j++) {
                const uint32_t slot = sr_ld32(RF_OBJ_COUNT);
                sr_st32(RF_OBJ_COUNT, slot + 1);
                sr_st32(RF_OBJ_LIST + slot * 4, row_obj + j * RF_POSE_SIZE);
            }
        }
    }
    sr_st32(list, 0);
    sr_obj_sort_index();
    sr_obj_depth_pass(1);
}

/* Solo race (0x22C70): every local pose block is an object. */
void sr_obj_publish(void) {
    const uint32_t count = sr_ld32(RF_POSE_COUNT);
    uint32_t i;

    for(i = 0; i < count; i++) sr_st32(RF_OBJ_LIST + i * 4, RF_POSES + i * RF_POSE_SIZE);
    sr_st32(RF_OBJ_COUNT, count);
    sr_st32(RF_PEER_LIST, RF_POSES);
    sr_st32(RF_PEER_LIST + 4, 0);
    sr_obj_sort_index();
    sr_obj_depth_pass(1);

    if(sr_ld32(0x20a530) != 0) {
        /* Mirror the own block, and up to three from the depth list, for the comm board. */
        uint32_t k, n = 1;
        for(k = 0; k < RF_POSE_SIZE; k += 4) sr_st32(RF_POSE_MIRROR + k, sr_ld32(RF_POSES + k));
        if(sr_ld32(RF_DEPTH_LIST + 8) != 0) {
            uint32_t list = RF_DEPTH_LIST + 8, out = RF_POSE_MIRROR + RF_POSE_SIZE;
            while(n <= 3) {
                const uint32_t peer = sr_ld32(list);
                if(peer == 0) break;
                for(k = 0; k < RF_POSE_SIZE; k += 4) sr_st32(out + k, sr_ld32(peer + k));
                n++;
                list += 16;
                out += RF_POSE_SIZE;
            }
        }
        sr_st32(RF_MIRROR_COUNT, n);
    }
}

/* Order the objects by span (0x231F0): identity ranks, sort, then the sorted addresses and each object's place. */
void sr_obj_sort_index(void) {
    uint32_t count = sr_ld32(RF_OBJ_COUNT), i;

    if((int32_t)count > 0)
        for(i = 0; i < count; i++) sr_st32(RF_OBJ_RANK + i * 4, i);
    sr_obj_rank_fill();
    count = sr_ld32(RF_OBJ_COUNT);
    if((int32_t)count <= 0) return;
    for(i = 0; i < count; i++) {
        const uint32_t rank = sr_ld32(RF_OBJ_RANK + i * 4);
        sr_st32(RF_OBJ_PLACE + (rank << 2), i);
        sr_st32(RF_OBJ_SORTED + i * 4, sr_ld32(RF_OBJ_LIST + (rank << 2)));
    }
}

static int32_t obj_span(uint32_t ix) { return (int16_t)sr_ld16(sr_ld32(RF_OBJ_LIST + (ix << 2)) + POSE_SPAN); }

/* Insertion sort of the ranks by descending span (0x23130). */
void sr_obj_rank_fill(void) {
    const int32_t count = (int32_t)sr_ld32(RF_OBJ_COUNT);
    int32_t i, j;

    for(i = 1; i < count; i++) {
        const uint32_t ins = sr_ld32(RF_OBJ_RANK + (uint32_t)i * 4);
        const int32_t span = obj_span(ins);
        for(j = i - 1; j >= 0 && obj_span(sr_ld32(RF_OBJ_RANK + (uint32_t)j * 4)) < span; j--)
            sr_st32(RF_OBJ_RANK + (uint32_t)(j + 1) * 4, sr_ld32(RF_OBJ_RANK + (uint32_t)j * 4));
        sr_st32(RF_OBJ_RANK + (uint32_t)(j + 1) * 4, ins);
    }
}

#define F_MINUS2 0xc0000000u
#define F_3      0x40400000u
#define F_4      0x40800000u
#define F_8      0x41000000u
#define F_MINUS3 0xc0400000u

/* Depth records for objects start.. (0x23280): transform each position into view space (TGP 0x2c), record its
 * length (TGP 0x58), and flag it: bit 0 drawn (in front, or near the axis), +2 near (z < 8; the length is tripled
 * behind z < -3). Then the records are insertion-sorted as the original does it. */
void sr_obj_depth_pass(uint32_t start) {
    uint32_t rec = RF_DEPTH_LIST, i, ea;
    int32_t idx;
    const int32_t count = (int32_t)sr_ld32(RF_OBJ_COUNT);

    for(i = 0, ea = 0x213af8u; i < 18; i++, ea -= 16) sr_st32(ea, 0);
    sr_st32(RF_DEPTH_LIST - 4, 0);
    if((int32_t)start >= count) return;

    for(idx = (int32_t)start; idx < count; idx++, rec += 16) {
        const uint32_t obj = sr_ld32(RF_OBJ_LIST + ((uint32_t)idx << 2));
        uint32_t pos[3], view[3], in[6], len;
        float z;
        if(obj == 0) continue;
        pos[0] = sr_ld32(obj);
        pos[1] = sr_ld32(obj + 4);
        pos[2] = sr_ld32(obj + 8);
        sr_tgp_cmd(0x16002c2cu, pos, 3, view, 3);
        sr_st32(rec + 4, 0);
        sr_st32(rec + 8, obj);
        in[0] = in[1] = in[2] = 0;
        in[3] = view[0];
        in[4] = view[1];
        in[5] = view[2];
        sr_tgp_cmd(0x16002c2cu << 1, in, 6, &len, 1);
        z = sr_f(view[2]);
        sr_st32(rec, 0);
        if(!(z <= sr_f(F_MINUS2))) {
            uint32_t drawn;
            if(z >= sr_f(F_3)) drawn = 1;
            else if(sr_ld32(RF_DEPTH_FAR) != 0) drawn = 1;
            else drawn = !(sr_f(view[0] & 0x7fffffffu) >= sr_f(F_4));
            if(drawn) {
                sr_st32(rec + 12, len);
                sr_st32(rec, 1);
            }
        }
        if(z < sr_f(F_8)) {
            if(z < sr_f(F_MINUS3)) sr_st32(rec + 12, fmul(len, F_3));
            sr_st32(rec, sr_ld32(rec) + 2);
        }
    }
    sr_st32(rec + 8, 0);

    /* The sort key is the previous record's depth word (cur - 4), as the lift reads it. */
    if(rec != RF_DEPTH_LIST) {
        uint32_t cur;
        for(cur = RF_DEPTH_LIST + 16; cur < rec; cur += 16) {
            const uint32_t key = sr_ld32(cur - 4);
            const uint32_t w0 = sr_ld32(cur), w1 = sr_ld32(cur + 4), w2 = sr_ld32(cur + 8), w3 = sr_ld32(cur + 12);
            uint32_t dst = cur, src = cur - 16;
            while(src >= RF_DEPTH_LIST && sr_f(sr_ld32(src + 12)) > sr_f(key)) {
                uint32_t k;
                for(k = 0; k < 16; k += 4) sr_st32(dst + k, sr_ld32(src + k));
                dst = src;
                if(src < RF_DEPTH_LIST + 16) break;
                src -= 16;
            }
            sr_st32(dst, w0);
            sr_st32(dst + 4, w1);
            sr_st32(dst + 8, w2);
            sr_st32(dst + 12, w3);
        }
    }
}

#define LOD_COUNTS     0x5bb200u /* per budget row: objects allowed at each of four levels (ROM mirror) */
#define LOD_DEPTHS     0x5bb290u /* depth limits between the levels */
#define OBJ_LIST_FLAGS 0x213be9u /* pose byte +0x51 of blocks 1.. */

/* Level of detail per depth record (0x1C2A0): level = how many depth limits the record is past, moved further out
 * while that level's quota (from the budget row) is used up; stored in object byte +0x51 bits 6..7. Each record's
 * mode word becomes 1 for a level 0..2 object with flag bit 7 at +0x50, else `link` (the original stores its g14,
 * the caller's return address). The championship (0x202230 == 1) clears the levels and sets every mode to 1. */
void sr_list_flag_prep(uint32_t budget, uint32_t link) {
    uint32_t quota[4], level, row, rec, rec_mode;
    int32_t b;

    if(sr_ld32(0x202230) == 1) {
        const uint32_t n = sr_ld32(RF_POSE_COUNT);
        uint32_t k, ea = OBJ_LIST_FLAGS;
        if(n > 1)
            for(k = 1; k < n; k++, ea += RF_POSE_SIZE) sr_st8(ea, sr_ld8(ea) & 0x3f);
        if(sr_ld32(RF_DEPTH_LIST + 8) != 0) {
            uint32_t next = RF_DEPTH_LIST + 8, mode = RF_DEPTH_LIST + 4, more;
            do {
                next += 16;
                more = sr_ld32(next);
                sr_st32(mode, 1);
                mode += 16;
            } while(more != 0);
        }
        return;
    }

    b = (((int32_t)budget / 10000) - 21) >> 1;
    if(b < 0) b = 0;
    else if(b > 7) b = 8;
    for(row = 0; row < 4; row++) quota[row] = sr_ld32(LOD_COUNTS + ((uint32_t)b << 4) + row * 4);

    if(sr_ld32(RF_DEPTH_LIST + 8) == 0) return;
    /* level indexes quota[] and the depth limits; the original keeps it three ways (g1, g2, g3). The last quota is
     * 99 and the last limit FLT_MAX, so it stays below 4. */
    level = 0;
    rec = RF_DEPTH_LIST + 8;
    rec_mode = RF_DEPTH_LIST + 4;
    for(;;) {
        const uint32_t obj = sr_ld32(rec);
        uint32_t flags_ea;
        if(sr_f(sr_ld32(rec + 4)) > sr_f(sr_ld32(LOD_DEPTHS + level * 4))) {
            const uint32_t depth = sr_ld32(rec + 4);
            do {
                level++;
            } while(sr_f(depth) > sr_f(sr_ld32(LOD_DEPTHS + level * 4)));
        }
        while(quota[level] == 0) level++;
        flags_ea = obj + 0x51;
        quota[level]--;
        sr_st8(flags_ea, (sr_ld8(flags_ea) & 0x3f) | (level << 6));
        if((int32_t)level <= 2 && (sr_ld8(sr_ld32(rec) + 0x50) & 0x80) != 0) sr_st32(rec_mode, 1);
        else sr_st32(rec_mode, link);
        rec += 16;
        rec_mode += 16;
        if(sr_ld32(rec) == 0) break;
    }
}
