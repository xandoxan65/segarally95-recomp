/* Require a MAME srallycb dump before the host runs.
 * CRC32 values are the published srallycb checksums (IEEE). */
#include "model2_rom.h"
#include "model2_rom_dir.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
    const char *name;
    const char *alias;
    unsigned size;
    unsigned crc;
} rom_expect_t;

/* Files the host actually loads: program, main data, polygons, textures, 68k, samples. */
static const rom_expect_t k_roms[] = {
    { "epr-17888b.12", NULL, 524288u, 0x95bce0b9u },
    { "epr-17889b.13", NULL, 524288u, 0x395c425eu },
    { "mpr-17746.10", NULL, 2097152u, 0x8fe311f4u },
    { "mpr-17747.11", NULL, 2097152u, 0x543593fdu },
    { "mpr-17744.8", NULL, 2097152u, 0x71fed098u },
    { "mpr-17745.9", NULL, 2097152u, 0x8ecca705u },
    { "mpr-17884.6", NULL, 2097152u, 0x4cfc95e1u },
    { "mpr-17885.7", NULL, 2097152u, 0xa08d2467u },
    { "mpr-17748.16", NULL, 2097152u, 0x3148a2b2u },
    { "mpr-17750.20", NULL, 2097152u, 0x232aec29u },
    { "mpr-17749.17", NULL, 2097152u, 0x0838d184u },
    { "mpr-17751.21", NULL, 2097152u, 0xed87ac62u },
    { "mpr-17753.25", NULL, 2097152u, 0x6db0eb36u },
    { "mpr-17752.24", NULL, 2097152u, 0xd6aa86ceu },
    { "epr-17890a.30", "epr-17890.30", 262144u, 0x5bac3fa1u },
    { "mpr-17756.31", NULL, 2097152u, 0x7725f111u },
    { "mpr-17757.32", NULL, 2097152u, 0x1616e649u },
    { "mpr-17886.36", NULL, 2097152u, 0x54a72923u },
    { "mpr-17887.37", NULL, 2097152u, 0x38c31fddu },
};

static unsigned crc_table[256];
static int crc_ready;

static void crc_init(void)
{
    unsigned i, bit;

    for (i = 0; i < 256u; i++) {
        unsigned c = i;
        for (bit = 0; bit < 8u; bit++)
            c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        crc_table[i] = c;
    }
    crc_ready = 1;
}

static int file_crc(const char *path, unsigned *out_size, unsigned *out_crc)
{
    FILE *fp;
    unsigned char buf[65536];
    unsigned crc = 0xffffffffu;
    unsigned size = 0;
    size_t n;

    fp = fopen(path, "rb");
    if (!fp)
        return -1;
    if (!crc_ready)
        crc_init();
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        size_t i;
        for (i = 0; i < n; i++)
            crc = crc_table[(crc ^ buf[i]) & 0xffu] ^ (crc >> 8);
        size += (unsigned)n;
    }
    if (ferror(fp)) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    *out_size = size;
    *out_crc = crc ^ 0xffffffffu;
    return 0;
}

static int path_join(char *dst, size_t dst_sz, const char *dir, const char *name)
{
    int n = snprintf(dst, dst_sz, "%s/%s", dir, name);
    return (n < 0 || (size_t)n >= dst_sz) ? -1 : 0;
}

int model2_romset_verify(void)
{
    const char *dir = model2_resolve_rom_dir();
    struct stat st;
    unsigned i;
    int bad = 0;

    if (stat(dir, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr,
                "segamod2: ROM directory not found: %s\n"
                "  Place a MAME srallycb set there (see ROMS/README.md),\n"
                "  or set SEGAMOD2_ROM_DIR.\n",
                dir);
        return -1;
    }

    for (i = 0; i < sizeof(k_roms) / sizeof(k_roms[0]); i++) {
        const rom_expect_t *rom = &k_roms[i];
        char path[768];
        const char *used = rom->name;
        unsigned size = 0, crc = 0;

        if (path_join(path, sizeof(path), dir, rom->name) != 0) {
            fprintf(stderr, "segamod2: ROM path too long: %s/%s\n", dir, rom->name);
            bad = 1;
            continue;
        }
        if (file_crc(path, &size, &crc) != 0) {
            if (!rom->alias
                || path_join(path, sizeof(path), dir, rom->alias) != 0
                || file_crc(path, &size, &crc) != 0) {
                if (rom->alias)
                    fprintf(stderr, "segamod2: missing ROM %s (or %s) in %s\n",
                            rom->name, rom->alias, dir);
                else
                    fprintf(stderr, "segamod2: missing ROM %s in %s\n",
                            rom->name, dir);
                bad = 1;
                continue;
            }
            used = rom->alias;
        }

        if (size != rom->size || crc != rom->crc) {
            fprintf(stderr,
                    "segamod2: ROM checksum mismatch %s\n"
                    "  got size %u crc %08x, expected size %u crc %08x (MAME srallycb)\n",
                    used, size, crc, rom->size, rom->crc);
            bad = 1;
        }
    }

    if (bad) {
        fprintf(stderr, "segamod2: refusing to run without a valid srallycb ROM set\n");
        return -1;
    }
    return 0;
}
