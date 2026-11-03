#ifndef SR_PREVIEW_H
#define SR_PREVIEW_H
#include "render/actors.h"
#include "render/course_render.h"
/* Render the course (and the actors, if not NULL) through view_matrix to a 640x480 PPM at path. */
int sr_preview_render(const sr_course_t *course, const sr_actors_t *actors, const float view_matrix[12],
                      const char *path);
/* The 2D layer's characters and palette (race pack / snapshot sections CHAR, PALR): with them and sr_tile_map the
 * previews draw the HUD over the 3D view. */
extern const uint8_t *sr_preview_chars, *sr_preview_palette;
#endif
