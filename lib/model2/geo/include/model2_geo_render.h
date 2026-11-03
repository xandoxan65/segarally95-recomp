/* Engine API for the geo render worker (used by model2_geo.c facade).
 * External callers should use model2_geo.h. */
#ifndef MODEL2_GEO_RENDER_H
#define MODEL2_GEO_RENDER_H

#include "model2_geo.h"
#include "model2_geo_dl.h"

void model2_geo_render_bind_fifo(const model2_geo_fifo_ops_t *fifo);

int model2_geo_render_init(void);
void model2_geo_render_shutdown(void);
int model2_geo_render_decode_fifos(void);
void model2_geo_render_kick(void);
int model2_geo_render_decode_prg_file(const char *path, int require_end);

unsigned model2_geo_render_vertex_count(void);
unsigned model2_geo_render_triangle_count(void);
int model2_geo_render_projection(model2_geo_projection_t *out);
const float *model2_geo_render_positions(unsigned *out_n_floats);
const unsigned *model2_geo_render_indices(unsigned *out_n_indices);

int model2_geo_render_copy_draw_buffers(float **out_pos, unsigned *out_npos,
                                        unsigned **out_idx, unsigned *out_nidx);
int model2_geo_render_copy_draw_textured(float **out_xyzuv, unsigned *out_nverts,
                                         model2_geo_tri_mat_t **out_mats,
                                         unsigned *out_ntris);
int model2_geo_render_lock_draw_textured(const float **out_xyzuv, unsigned *out_nverts,
                                         const model2_geo_tri_mat_t **out_mats,
                                         unsigned *out_ntris);
void model2_geo_render_unlock_draw(void);
void model2_geo_render_clear_draw(void);

void model2_geo_render_texture_data(u32 address, u32 count, const u32 *data);
void model2_geo_render_polygon_data(u32 address, u32 count, const u32 *data);
int model2_geo_render_polygon_ram0_peek(u32 addr, u32 *out);

int model2_geo_render_dump_summary(const char *path);

/*
 * Capture hook (course export, src/host/lift_export_course.c): decode the latest
 * latched display frame synchronously, on the caller's thread, with its own
 * buffers and a copy of the GEO state seeded like the decode worker's — the
 * published mesh and the worker are not touched. `fn` sees every drawn object
 * (oba/obc, tpa/tha, mode, model-view matrix) and the frame's decoded mesh (view
 * space, focus applied, as the GL viewer draws it) while the polygon RAM lock is
 * held, so it may call model2_geo_dl_decode_object(ctx, ...). Returns 1 when a new
 * display frame (display_gen) was decoded, 0 when there is none, -1 on error.
 */
typedef void (*model2_geo_capture_fn)(void *user, model2_geo_dl_ctx_t *ctx,
                                      const model2_geo_state_t *frame_state,
                                      const model2_drawn_object_t *objs, unsigned n_objs,
                                      const model2_mesh_collector_t *frame_mesh);
int model2_geo_render_capture_display(model2_geo_capture_fn fn, void *user);
void model2_geo_render_draw_gl(void);

#endif
