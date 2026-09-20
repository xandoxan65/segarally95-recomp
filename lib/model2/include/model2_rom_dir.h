/* Resolve the srallyc-b dump directory for host loaders (sound, polygons, textures).
 * Explicit SEGAMOD2_ROM_DIR / DECOMP_ROM_DIR win. Otherwise prefer this game
 * tree (SEGAMOD2_ROOT/ROMS/srallyc-b or ./ROMS/srallyc-b) over the old
 * monorepo-relative ../ROMS/srallyc-b.
 */
#ifndef MODEL2_ROM_DIR_H
#define MODEL2_ROM_DIR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int model2_rom_dir_has_program(const char *dir)
{
    char path[768];
    struct stat st;

    if (!dir || !dir[0])
        return 0;
    snprintf(path, sizeof(path), "%s/epr-17888b.12", dir);
    return stat(path, &st) == 0 && S_ISREG(st.st_mode);
}

static const char *model2_resolve_rom_dir(void)
{
    static char joined[512];
    const char *env;
    const char *root;

    env = getenv("SEGAMOD2_ROM_DIR");
    if (env && env[0])
        return env;
    env = getenv("DECOMP_ROM_DIR");
    if (env && env[0])
        return env;

    root = getenv("SEGAMOD2_ROOT");
    if (root && root[0]) {
        snprintf(joined, sizeof(joined), "%s/ROMS/srallyc-b", root);
        if (model2_rom_dir_has_program(joined))
            return joined;
    }
    if (model2_rom_dir_has_program("ROMS/srallyc-b"))
        return "ROMS/srallyc-b";
    if (model2_rom_dir_has_program("../ROMS/srallyc-b"))
        return "../ROMS/srallyc-b";
    return "ROMS/srallyc-b";
}

#endif
