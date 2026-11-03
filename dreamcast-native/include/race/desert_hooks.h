/* Drawing the desert helicopter and its dust (code/race/desert_node.c) hands to the renderer through these.
 * The diff build writes the Model 2 GEO ports as the lifted code does and calls the lifted pen push
 * (tests/diff/wrap_desert_node.c); the Dreamcast has stubs (code/hooks/desert_hooks.c) until the renderer draws
 * scenery objects. RAM the drawing keeps (the 0x20a290 cursor and its slot, the 0x20b940 catalog count) and the
 * TGP traffic (including the 0x05 matrix emit) stay in the native code. */
#ifndef SR_RACE_DESERT_HOOKS_H
#define SR_RACE_DESERT_HOOKS_H
#include <stdint.h>

/* GEO write pointer (read of 0x802008): the native code stores it in the 0x20a290 list slot of each object. */
uint32_t sr_hook_geo_list_ptr(void);
/* Start an object's GEO data: 0x801008 <- list_ptr + 0x34 (after the TGP 0x05 matrix emit). */
void sr_hook_geo_object_start(uint32_t list_ptr);
/* GEO bank word: 0x800010 + (0x202278 & 3) * 0x400 <- word (the original's link register, or 0). */
void sr_hook_geo_bank_word(uint32_t word);
/* Model catalog entry at `catalog` (main data ROM, 4 words) to the GEO program FIFO. The native code adds its
 * last word to 0x20b940 itself. */
void sr_hook_geo_catalog(uint32_t catalog);
/* geo_attract_pen_prg_push_b (0x3EF10): texture pen for the object at obj/angles (VAs) seen from the eye at
 * `eye` (0x20220c), table row `pen_mode`. Writes the texture staging 0x214380.. and its cursor 0x214374, which
 * only the renderer reads. */
void sr_hook_pen_push(uint32_t obj, uint32_t angles, uint32_t eye, uint32_t pen_mode);

#endif
