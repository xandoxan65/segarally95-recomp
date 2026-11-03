/* The countdown's 3D digits: see include/render/countdown.h. */
#include "render/countdown.h"
#include "race/race_phases.h"

#define FOCUS 512.f

uint32_t sr_countdown_tris(const sr_course_t *course, float width, float height, sr_countdown_tri_t *out,
                           uint32_t max) {
    const sr_countdown_draw_t *d;
    const uint32_t n = sr_countdown_draws(&d);
    uint32_t i, m, t, k, count = 0;
    for(i = 0; i < n; i++)
        for(m = 0; m < course->n_meshes; m++) {
            const sr_course_mesh_t *mesh = &course->meshes[m];
            if(mesh->oba != d[i].oba) continue;
            for(t = 0; t < mesh->n_tris && count < max; t++) {
                sr_countdown_tri_t *o = &out[count];
                int behind = 0;
                o->texture = mesh->tris[t].texture;
                o->flags = mesh->tris[t].flags;
                for(k = 0; k < 3; k++) {
                    const sr_course_vertex_t *p = &mesh->tris[t].v[k];
                    const float *a = d[i].m;
                    const float x = a[0] * p->x + a[3] * p->y + a[6] * p->z + a[9];
                    const float y = a[1] * p->x + a[4] * p->y + a[7] * p->z + a[10];
                    const float z = a[2] * p->x + a[5] * p->y + a[8] * p->z + a[11];
                    if(z <= 0.f) {
                        behind = 1;
                        break;
                    }
                    o->x[k] = (248.f + FOCUS * x / z) * (width / 496.f);
                    o->y[k] = (192.f - FOCUS * y / z) * (height / 384.f);
                    o->invz[k] = 1.f / z;
                    o->u[k] = p->u;
                    o->v[k] = p->v;
                    o->argb[k] = p->argb | 0xff000000u;
                }
                if(!behind) count++;
            }
            break;
        }
    return count;
}
