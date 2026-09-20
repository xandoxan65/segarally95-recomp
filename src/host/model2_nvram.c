/* Host NVRAM: persist backup SRAM @ 0x01D00000 + operator option bytes.
 *
 * File format (YAML, no external deps):
 *   version: 1
 *   settings: { country, advertise, ... }
 *   backup_sram_hex: "<8192 hex chars = 16 KiB>"
 *
 * Default path: I960_HOST_NVRAM or build/lift/nvram.yaml
 */

#include "model2_nvram.h"
#include "model2_rom.h"
#include "i960_mem.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NVRAM_VERSION 1

typedef struct {
    int loaded;
    int dirty;
    int have_settings;
    u8 country;      /* 0x202019 — 0=JPN 1=USA 2=EXPORT */
    u8 advertise;    /* 0x202018 */
    u8 flag_1a;      /* 0x20201a */
    u8 flag_1b;      /* 0x20201b */
    u8 flag_1c;      /* 0x20201c */
    u8 flag_1d;      /* 0x20201d */
    u8 flag_1e;      /* 0x20201e */
    u8 flag_1f;      /* 0x20201f */
    u8 option_flags; /* 0x202024 */
    u16 coin_unit;   /* 0x202028 */
    u16 coin_price;  /* 0x20202a */
    u16 coin_mech;   /* 0x20202c */
    u16 coin_limit;  /* 0x20202e */
    u16 mode_cksum;  /* 0x202010 */
    u16 mode_word;   /* 0x202012 — expect 36 when valid */
    u8 sram[MODEL2_BACKUP_SRAM_SIZE];
} model2_nvram_t;

static model2_nvram_t g_nvram;
static char g_nvram_path[512];
/* 0x202019: 0=japan, 1=us, 2=international. CLI default; overrides YAML. */
static u8 g_region = 2;

static const char *nvram_default_path(void)
{
    const char *env = getenv("I960_HOST_NVRAM");

    if (env && env[0] && env[0] != '0')
        return env;
    return "build/lift/nvram.yaml";
}

static void nvram_capture_from_host(void)
{
    const u8 *sram = model2_backup_sram_ptr();

    if (sram)
        memcpy(g_nvram.sram, sram, MODEL2_BACKUP_SRAM_SIZE);

    g_nvram.advertise = (u8)i960_ld_u8(I960_WORKRAM, 0x202018, 0);
    g_nvram.country = (u8)i960_ld_u8(I960_WORKRAM, 0x202019, 0);
    g_nvram.flag_1a = (u8)i960_ld_u8(I960_WORKRAM, 0x20201a, 0);
    g_nvram.flag_1b = (u8)i960_ld_u8(I960_WORKRAM, 0x20201b, 0);
    g_nvram.flag_1c = (u8)i960_ld_u8(I960_WORKRAM, 0x20201c, 0);
    g_nvram.flag_1d = (u8)i960_ld_u8(I960_WORKRAM, 0x20201d, 0);
    g_nvram.flag_1e = (u8)i960_ld_u8(I960_WORKRAM, 0x20201e, 0);
    g_nvram.flag_1f = (u8)i960_ld_u8(I960_WORKRAM, 0x20201f, 0);
    g_nvram.option_flags = (u8)i960_ld_u8(I960_WORKRAM, 0x202024, 0);
    g_nvram.coin_unit = (u16)i960_ld_u16(I960_WORKRAM, 0x202028, 0);
    g_nvram.coin_price = (u16)i960_ld_u16(I960_WORKRAM, 0x20202a, 0);
    g_nvram.coin_mech = (u16)i960_ld_u16(I960_WORKRAM, 0x20202c, 0);
    g_nvram.coin_limit = (u16)i960_ld_u16(I960_WORKRAM, 0x20202e, 0);
    g_nvram.mode_cksum = (u16)i960_ld_u16(I960_WORKRAM, 0x202010, 0);
    g_nvram.mode_word = (u16)i960_ld_u16(I960_WORKRAM, 0x202012, 0);
    g_nvram.have_settings = 1;
}

void model2_nvram_apply_options(void)
{
    if (!g_nvram.loaded || !g_nvram.have_settings) {
        i960_st_u8(I960_WORKRAM, 0x202019, 0, g_region);
        return;
    }

    i960_st_u8(I960_WORKRAM, 0x202018, 0, g_nvram.advertise);
    i960_st_u8(I960_WORKRAM, 0x202019, 0, g_region);
    i960_st_u8(I960_WORKRAM, 0x20201a, 0, g_nvram.flag_1a);
    i960_st_u8(I960_WORKRAM, 0x20201b, 0, g_nvram.flag_1b);
    i960_st_u8(I960_WORKRAM, 0x20201c, 0, g_nvram.flag_1c);
    i960_st_u8(I960_WORKRAM, 0x20201d, 0, g_nvram.flag_1d);
    i960_st_u8(I960_WORKRAM, 0x20201e, 0, g_nvram.flag_1e);
    i960_st_u8(I960_WORKRAM, 0x20201f, 0, g_nvram.flag_1f);
    i960_st_u8(I960_WORKRAM, 0x202024, 0, g_nvram.option_flags);
    i960_st_u16(I960_WORKRAM, 0x202028, 0, g_nvram.coin_unit);
    i960_st_u16(I960_WORKRAM, 0x20202a, 0, g_nvram.coin_price);
    i960_st_u16(I960_WORKRAM, 0x20202c, 0, g_nvram.coin_mech);
    i960_st_u16(I960_WORKRAM, 0x20202e, 0, g_nvram.coin_limit);
    if (g_nvram.mode_word == 36u) {
        i960_st_u16(I960_WORKRAM, 0x202010, 0, g_nvram.mode_cksum);
        i960_st_u16(I960_WORKRAM, 0x202012, 0, g_nvram.mode_word);
    }
    g_nvram.country = g_region;
}

static int hex_nibble(int c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

static int parse_u32_value(const char *s, u32 *out)
{
    char *end = NULL;
    unsigned long v;

    while (*s && isspace((unsigned char)*s))
        s++;
    if (!*s)
        return -1;
    v = strtoul(s, &end, 0);
    if (end == s)
        return -1;
    *out = (u32)v;
    return 0;
}

static const char *country_name(u8 v)
{
    if (v == 1)
        return "us";
    if (v == 2)
        return "international";
    return "japan";
}

static int country_from_name(const char *s, u8 *out)
{
    char tok[32];
    size_t n = 0;

    while (*s && isspace((unsigned char)*s))
        s++;
    while (*s && !isspace((unsigned char)*s) && *s != '#' && n + 1 < sizeof(tok)) {
        char c = *s++;

        if (c >= 'A' && c <= 'Z')
            c = (char)(c - 'A' + 'a');
        tok[n++] = c;
    }
    tok[n] = '\0';
    if (n == 0)
        return -1;
    if (!strcmp(tok, "japan") || !strcmp(tok, "jpn") || !strcmp(tok, "jp")
        || !strcmp(tok, "0")) {
        *out = 0;
        return 0;
    }
    if (!strcmp(tok, "us") || !strcmp(tok, "usa") || !strcmp(tok, "1")) {
        *out = 1;
        return 0;
    }
    if (!strcmp(tok, "international") || !strcmp(tok, "export")
        || !strcmp(tok, "world") || !strcmp(tok, "2")) {
        *out = 2;
        return 0;
    }
    return -1;
}

int model2_nvram_set_region_name(const char *name)
{
    u8 country = 2;

    if (!name || country_from_name(name, &country) != 0)
        return -1;
    g_region = country;
    g_nvram.country = country;
    return 0;
}

int model2_nvram_load(const char *path)
{
    FILE *fp;
    char line[512];
    int in_hex = 0;
    u32 hex_off = 0;
    u8 *sram;

    if (!path || !*path)
        path = nvram_default_path();
    snprintf(g_nvram_path, sizeof(g_nvram_path), "%s", path);

    memset(&g_nvram, 0, sizeof(g_nvram));
    /* Sensible defaults matching game_option_flags_init / coin_option_defaults. */
    g_nvram.advertise = 1;
    g_nvram.country = g_region;
    g_nvram.flag_1a = 1;
    g_nvram.flag_1e = 1;
    g_nvram.flag_1f = 4;
    g_nvram.option_flags = 2;
    g_nvram.coin_unit = 1;
    g_nvram.coin_price = 2;
    g_nvram.coin_limit = 11;
    g_nvram.mode_word = 36;

    fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "lift: nvram — no file at %s (region %s)\n",
                path, country_name(g_region));
        fflush(stderr);
        return 1;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *p = line;
        char *colon;

        while (*p && isspace((unsigned char)*p))
            p++;
        if (*p == '#' || *p == '\0')
            continue;

        if (!strncmp(p, "backup_sram_hex:", 16)) {
            in_hex = 1;
            p += 16;
            while (*p && isspace((unsigned char)*p))
                p++;
            if (*p == '|' || *p == '>' || *p == '"')
                continue;
            /* inline hex on same line */
        }

        if (in_hex) {
            while (*p) {
                int hi, lo;

                while (*p && (isspace((unsigned char)*p) || *p == ':'))
                    p++;
                if (!*p || *p == '#')
                    break;
                /* skip offset prefixes like "0000:" */
                if (isxdigit((unsigned char)p[0]) && isxdigit((unsigned char)p[1])
                    && isxdigit((unsigned char)p[2]) && isxdigit((unsigned char)p[3])
                    && p[4] == ':') {
                    p += 5;
                    continue;
                }
                hi = hex_nibble((unsigned char)*p++);
                if (hi < 0)
                    break;
                while (*p && isspace((unsigned char)*p))
                    p++;
                lo = hex_nibble((unsigned char)*p++);
                if (lo < 0)
                    break;
                if (hex_off < MODEL2_BACKUP_SRAM_SIZE)
                    g_nvram.sram[hex_off++] = (u8)((hi << 4) | lo);
            }
            continue;
        }

        colon = strchr(p, ':');
        if (!colon)
            continue;
        *colon = '\0';
        {
            const char *key = p;
            const char *val = colon + 1;
            u32 num = 0;

            while (*val && isspace((unsigned char)*val))
                val++;
            if (!strcmp(key, "country")) {
                u8 country = g_region;

                if (country_from_name(val, &country) == 0)
                    g_nvram.country = country;
                g_nvram.have_settings = 1;
            } else if (!strcmp(key, "advertise") && parse_u32_value(val, &num) == 0) {
                g_nvram.advertise = (u8)num;
                g_nvram.have_settings = 1;
            } else if (!strcmp(key, "flag_1a") && parse_u32_value(val, &num) == 0) {
                g_nvram.flag_1a = (u8)num;
                g_nvram.have_settings = 1;
            } else if (!strcmp(key, "flag_1b") && parse_u32_value(val, &num) == 0) {
                g_nvram.flag_1b = (u8)num;
            } else if (!strcmp(key, "flag_1c") && parse_u32_value(val, &num) == 0) {
                g_nvram.flag_1c = (u8)num;
            } else if (!strcmp(key, "flag_1d") && parse_u32_value(val, &num) == 0) {
                g_nvram.flag_1d = (u8)num;
            } else if (!strcmp(key, "flag_1e") && parse_u32_value(val, &num) == 0) {
                g_nvram.flag_1e = (u8)num;
            } else if (!strcmp(key, "flag_1f") && parse_u32_value(val, &num) == 0) {
                g_nvram.flag_1f = (u8)num;
            } else if (!strcmp(key, "option_flags") && parse_u32_value(val, &num) == 0) {
                g_nvram.option_flags = (u8)num;
            } else if (!strcmp(key, "coin_unit") && parse_u32_value(val, &num) == 0) {
                g_nvram.coin_unit = (u16)num;
            } else if (!strcmp(key, "coin_price") && parse_u32_value(val, &num) == 0) {
                g_nvram.coin_price = (u16)num;
            } else if (!strcmp(key, "coin_mech") && parse_u32_value(val, &num) == 0) {
                g_nvram.coin_mech = (u16)num;
            } else if (!strcmp(key, "coin_limit") && parse_u32_value(val, &num) == 0) {
                g_nvram.coin_limit = (u16)num;
            } else if (!strcmp(key, "mode_cksum") && parse_u32_value(val, &num) == 0) {
                g_nvram.mode_cksum = (u16)num;
            } else if (!strcmp(key, "mode_word") && parse_u32_value(val, &num) == 0) {
                g_nvram.mode_word = (u16)num;
            }
        }
    }
    fclose(fp);

    sram = model2_backup_sram_ptr();
    if (sram && hex_off > 0)
        memcpy(sram, g_nvram.sram, MODEL2_BACKUP_SRAM_SIZE);
    else if (sram)
        memcpy(g_nvram.sram, sram, MODEL2_BACKUP_SRAM_SIZE);

    g_nvram.loaded = 1;
    g_nvram.dirty = 0;
    if (g_nvram.country != g_region) {
        fprintf(stderr, "lift: region %s overrides nvram country %s\n",
                country_name(g_region), country_name(g_nvram.country));
    }
    g_nvram.country = g_region;
    fprintf(stderr,
            "lift: nvram loaded from %s (country=%s, sram_bytes=%u)\n",
            path, country_name(g_nvram.country), (unsigned)hex_off);
    fflush(stderr);
    return 0;
}

int model2_nvram_save(const char *path)
{
    FILE *fp;
    u32 i;

    if (!path || !*path)
        path = g_nvram_path[0] ? g_nvram_path : nvram_default_path();

    nvram_capture_from_host();

    fp = fopen(path, "w");
    if (!fp) {
        fprintf(stderr, "lift: nvram save failed: %s\n", path);
        return -1;
    }

    fprintf(fp,
            "# Sega Rally (srallyc) host NVRAM — backup SRAM + operator options\n"
            "# country: international | japan | us  (workram 0x202019; CLI --region overrides)\n"
            "version: %d\n"
            "settings:\n"
            "  country: %s\n"
            "  advertise: %u\n"
            "  flag_1a: %u\n"
            "  flag_1b: %u\n"
            "  flag_1c: %u\n"
            "  flag_1d: %u\n"
            "  flag_1e: %u\n"
            "  flag_1f: %u\n"
            "  option_flags: %u\n"
            "  coin_unit: %u\n"
            "  coin_price: %u\n"
            "  coin_mech: %u\n"
            "  coin_limit: %u\n"
            "  mode_cksum: 0x%04x\n"
            "  mode_word: %u\n"
            "backup_sram_base: 0x%08x\n"
            "backup_sram_size: 0x%04x\n"
            "backup_sram_hex: |\n",
            NVRAM_VERSION,
            country_name(g_nvram.country),
            (unsigned)g_nvram.advertise,
            (unsigned)g_nvram.flag_1a,
            (unsigned)g_nvram.flag_1b,
            (unsigned)g_nvram.flag_1c,
            (unsigned)g_nvram.flag_1d,
            (unsigned)g_nvram.flag_1e,
            (unsigned)g_nvram.flag_1f,
            (unsigned)g_nvram.option_flags,
            (unsigned)g_nvram.coin_unit,
            (unsigned)g_nvram.coin_price,
            (unsigned)g_nvram.coin_mech,
            (unsigned)g_nvram.coin_limit,
            (unsigned)g_nvram.mode_cksum,
            (unsigned)g_nvram.mode_word,
            MODEL2_BACKUP_SRAM_BASE,
            MODEL2_BACKUP_SRAM_SIZE);

    for (i = 0; i < MODEL2_BACKUP_SRAM_SIZE; i++) {
        if ((i & 15u) == 0)
            fprintf(fp, "  %04x:", i);
        fprintf(fp, " %02x", g_nvram.sram[i]);
        if ((i & 15u) == 15u)
            fputc('\n', fp);
    }

    fclose(fp);
    g_nvram.loaded = 1;
    g_nvram.have_settings = 1;
    g_nvram.dirty = 0;
    snprintf(g_nvram_path, sizeof(g_nvram_path), "%s", path);
    fprintf(stderr, "lift: nvram saved → %s (country=%s)\n", path,
            country_name(g_nvram.country));
    return 0;
}

void model2_nvram_mark_dirty(void)
{
    g_nvram.dirty = 1;
}

int model2_nvram_loaded(void)
{
    return g_nvram.loaded;
}

const char *model2_nvram_path(void)
{
    return g_nvram_path[0] ? g_nvram_path : nvram_default_path();
}

void model2_nvram_sync_if_dirty(void)
{
    const u8 *sram;

    if (!g_nvram.loaded && !g_nvram.dirty)
        return;
    sram = model2_backup_sram_ptr();
    if (sram && memcmp(sram, g_nvram.sram, MODEL2_BACKUP_SRAM_SIZE) != 0)
        g_nvram.dirty = 1;
    if (g_nvram.dirty)
        (void)model2_nvram_save(NULL);
}
