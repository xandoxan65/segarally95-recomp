/* Host harness: feed RE-backed catalog / placement rows into geo FIFOs. */

#include "placement_catalog_feed.h"
#include "i960_lift.h"
#include "lift_syms.h"
#include "model2_memory.h"
#include "model2_rom.h"
#include "i960_mem.h"

#include <stdlib.h>
#include <string.h>

/* Race draw list @ maincpu ROM 0x34E88 (catalog 96–107, wheels 132–143, …). */
static const u16 race_draw_catalog[] = {
    96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107,
    132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143,
};

static void geo_bank_clear(void)
{
    u32 bank = i960_ld_u32(I960_WORKRAM, 0x202278, 0) & 3u;

    i960_mmio_write_u32(0x800010u + (bank << 10), 0);
}

/*
 * Feeder ABI (disasm @ 0x23CCC):
 *   arg0[+4] = catalog index
 *   arg1     = object pose (xyz @ 0/4/8, angles @ 0x18/0x1c/0x20)
 *   arg2     = remaining budget
 */
void placement_catalog_push(u32 catalog_index)
{
    u32 rec[2];
    u32 obj[0x24 / 4];

    memset(rec, 0, sizeof(rec));
    memset(obj, 0, sizeof(obj));
    rec[1] = catalog_index;
    placement_geo_feeder(rec, obj, 0x100u);
}

void placement_catalog_push_race_slot(u32 slot)
{
    u32 n = (u32)(sizeof(race_draw_catalog) / sizeof(race_draw_catalog[0]));

    if (slot >= n)
        slot %= n;
    placement_catalog_push(race_draw_catalog[slot]);
}

/*
 * Placement stream @ 0x02867C20 is stride-4 (tpa,tha,oba,obc) — same shape as a
 * catalog row, but not indexed through 0x02864B40. Push the typed record and a
 * minimal copro pose so prg decode sees a matrix snapshot.
 */
void placement_catalog_push_placement_index(u32 placement_index)
{
    u32 base;
    u32 w0, w1, w2, w3;

    if (placement_index > 781u)
        placement_index = 781u;

    base = PLACEMENT_STREAM_VADDR + (placement_index << 4);
    w0 = i960_ld_u32(I960_ROM, base, 0);
    w1 = i960_ld_u32(I960_ROM, base, 4);
    w2 = i960_ld_u32(I960_ROM, base, 8);
    w3 = i960_ld_u32(I960_ROM, base, 12);

    geo_bank_clear();
    i960_mmio_write_u32(GEO_PRG_FIFO, w0);
    i960_mmio_write_u32(GEO_PRG_FIFO, w1);
    i960_mmio_write_u32(GEO_PRG_FIFO, w2);
    i960_mmio_write_u32(GEO_PRG_FIFO, w3);

    i960_mmio_write_u32(COPRO_FIFO, 0x10802121u);
    i960_mmio_write_u32(COPRO_FIFO, 0x10002020u);
    i960_mmio_write_u32(COPRO_FIFO, 0x13802727u);
    i960_mmio_write_u32(COPRO_FIFO, 0);
    i960_mmio_write_u32(COPRO_FIFO, 0);
    i960_mmio_write_u32(COPRO_FIFO, 0);
    i960_mmio_write_u32(COPRO_FIFO, 0x02800505u);
}

void placement_catalog_feed_track_batch(u32 start, u32 count)
{
    u32 i;

    if (start < PLACEMENT_TRACK_DESERT_START)
        start = PLACEMENT_TRACK_DESERT_START;
    for (i = 0; i < count; i++) {
        u32 pi = start + i;
        if (pi >= PLACEMENT_TRACK_DESERT_START + PLACEMENT_TRACK_DESERT_COUNT)
            break;
        placement_catalog_push_placement_index(pi);
    }
}

int placement_catalog_feed_source(void)
{
    const char *s = getenv("I960_HOST_PLACEMENT_SOURCE");

    if (s && strcmp(s, "track") == 0)
        return 1;
    return 0;
}

void placement_catalog_feed_race_batch(u32 start, u32 count)
{
    u32 i;
    u32 n = (u32)(sizeof(race_draw_catalog) / sizeof(race_draw_catalog[0]));

    if (start >= n)
        return;
    if (start + count > n)
        count = n - start;
    for (i = 0; i < count; i++)
        placement_catalog_push(race_draw_catalog[start + i]);
}

unsigned placement_catalog_feed_env_batch(void)
{
    const char *s = getenv("I960_HOST_PLACEMENT_BATCH");
    unsigned n = 4;

    if (s && *s)
        n = (unsigned)strtoul(s, NULL, 0);
    if (n > 12)
        n = 12;
    return n;
}
