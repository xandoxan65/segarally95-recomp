#ifndef MODEL2_NVRAM_H
#define MODEL2_NVRAM_H

#include "i960_lift.h"

/* Load YAML NVRAM into backup SRAM + pending operator settings. Returns 0 ok,
 * 1 if file missing (defaults kept), -1 on parse/IO error. */
int model2_nvram_load(const char *path);

/* Capture current backup SRAM + workram options and write YAML. */
int model2_nvram_save(const char *path);

/* Re-apply loaded settings into workram (after option init / seed). */
void model2_nvram_apply_options(void);

void model2_nvram_mark_dirty(void);
void model2_nvram_sync_if_dirty(void);
int model2_nvram_loaded(void);
const char *model2_nvram_path(void);

#endif
