/* Moving objects: see include/render/actors.h.
 *
 * Poses (fitted against the reference's own draws, docs/dreamcast-port.md): a car's pose block (0x213b40 + slot *
 * 0x58) holds the position at +0x00 and pitch, yaw, roll at +0x18/1c/20; the helicopter node (0x500200) holds them
 * at +0x18 and +0x24/28/2c. World = T(pos) Ry(-yaw) Rx(-pitch) Rz(-roll) x local. A wheel's local is its rest
 * offset with the suspension height (pose +0x30 + 4 * wheel) instead of its y, turned by the steering (pose +0x24,
 * front wheels) and the spin (pose +0x2c); the rotors turn by node +0x60 (main, about y) and +0x64 (tail, about x).
 * The road car's detail follows the arcade's distance bands: under 25 units the near set, to 400 the mid set. The
 * shadow (a flat plate under the car) is drawn at every distance, in the translucent pass, darkening the ground. */
#include "render/actors.h"
#include "race/particles.h"
#include "sr_alloc.h"
#include "sr_mem.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define POSE_PLAYER 0x213b40u
#define HELI_NODE   0x500200u


enum { PART_OTHER, PART_NEAR, PART_MID, PART_WHEEL_NEAR, PART_WHEEL_MID, PART_ROTOR_MAIN, PART_ROTOR_TAIL, PART_SHADOW };

static int part_of(uint32_t oba) {
    switch(oba) {
    case 0x80e8ddu: case 0x815430u: case 0x815574u: return PART_NEAR;
    case 0x814e11u: return PART_MID;
    case 0x815599u: return PART_SHADOW;
    case 0x8154b9u: return PART_WHEEL_NEAR;
    case 0x808596u: return PART_WHEEL_MID;
    case 0x82f4a3u: return PART_ROTOR_MAIN;
    case 0x82f4c8u: return PART_ROTOR_TAIL;
    default: return PART_OTHER;
    }
}

/* The shadow plate's outline: the convex hull of its vertices in its plane (x, z). The plate is a rounded car
 * footprint drawn as a few pieces, so its own boundary edges do not form one loop. */
static uint32_t plate_outline(const sr_course_mesh_t *m, float out[SR_SHADOW_OUTLINE_MAX][2]) {
    float p[256][2];
    uint32_t n = 0, i, j, h = 0, start = 0, cur;
    for(i = 0; i < m->n_tris; i++)
        for(j = 0; j < 3 && n < 256; j++) p[n][0] = m->tris[i].v[j].x, p[n][1] = m->tris[i].v[j].z, n++;
    if(n < 3) return 0;
    for(i = 1; i < n; i++)
        if(p[i][0] < p[start][0] || (p[i][0] == p[start][0] && p[i][1] < p[start][1])) start = i;
    cur = start;
    do { /* gift wrapping */
        uint32_t next = cur == 0 ? 1 : 0;
        if(h == SR_SHADOW_OUTLINE_MAX) return 0;
        out[h][0] = p[cur][0], out[h][1] = p[cur][1], h++;
        for(i = 0; i < n; i++) {
            const float cross = (p[next][0] - p[cur][0]) * (p[i][1] - p[cur][1]) -
                                (p[next][1] - p[cur][1]) * (p[i][0] - p[cur][0]);
            const float d_i = (p[i][0] - p[cur][0]) * (p[i][0] - p[cur][0]) + (p[i][1] - p[cur][1]) * (p[i][1] - p[cur][1]);
            const float d_n = (p[next][0] - p[cur][0]) * (p[next][0] - p[cur][0]) +
                              (p[next][1] - p[cur][1]) * (p[next][1] - p[cur][1]);
            if(cross < 0.f || (cross == 0.f && d_i > d_n)) next = i;
        }
        cur = next;
    } while(cur != start);
    return h >= 3 ? h : 0;
}

int sr_actors_parse(sr_actors_t *actors, const sr_course_t *course, const uint8_t *data, size_t size) {
    uint32_t i;
    if(size < 12 || memcmp(data, "SRATT01", 8)) return -1;
    sr_copy(&actors->n, data + 8, 4);
    if(12 + (size_t)actors->n * sizeof(sr_attach_t) > size) return -1;
    actors->a = sr_alloc((actors->n ? actors->n : 1) * sizeof(sr_attach_t));
    if(!actors->a) return -1;
    sr_copy(actors->a, data + 12, actors->n * sizeof(sr_attach_t));
    actors->shadow_n = 0;
    actors->particle_mesh[0] = actors->particle_mesh[1] = course->n_meshes;
    for(i = 0; i < course->n_meshes; i++) {
        if(course->meshes[i].oba == SR_OBA_DUST) actors->particle_mesh[0] = i;
        if(course->meshes[i].oba == SR_OBA_DEBRIS) actors->particle_mesh[1] = i;
    }
    for(i = 0; i < actors->n; i++) {
        if(actors->a[i].mesh >= course->n_meshes) return -1;
        if(!actors->shadow_n && part_of(course->meshes[actors->a[i].mesh].oba) == PART_SHADOW)
            actors->shadow_n = plate_outline(&course->meshes[actors->a[i].mesh], actors->shadow_outline);
    }
    return 0;
}

/* Model 2 column layout from a row-major 3x3 and a translation. */
static void to_m2(float m[12], const float r[3][3], const float t[3]) {
    int row, col;
    for(col = 0; col < 3; col++)
        for(row = 0; row < 3; row++) m[col * 3 + row] = r[row][col];
    m[9] = t[0], m[10] = t[1], m[11] = t[2];
}

static void mul33(float out[3][3], const float a[3][3], const float b[3][3]) {
    float r[3][3];
    int i, j;
    for(i = 0; i < 3; i++)
        for(j = 0; j < 3; j++) r[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j];
    sr_copy(out, r, sizeof(r));
}

static void rot_x(float r[3][3], float a) {
    const float c = cosf(a), s = sinf(a);
    const float m[3][3] = {{1, 0, 0}, {0, c, -s}, {0, s, c}};
    sr_copy(r, m, sizeof(m));
}

static void rot_y(float r[3][3], float a) {
    const float c = cosf(a), s = sinf(a);
    const float m[3][3] = {{c, 0, s}, {0, 1, 0}, {-s, 0, c}};
    sr_copy(r, m, sizeof(m));
}

static void rot_z(float r[3][3], float a) {
    const float c = cosf(a), s = sinf(a);
    const float m[3][3] = {{c, -s, 0}, {s, c, 0}, {0, 0, 1}};
    sr_copy(r, m, sizeof(m));
}

/* T(pos) Ry(-yaw) Rx(-pitch) Rz(-roll) */
static void pose_matrix(float m[12], const float pos[3], float pitch, float yaw, float roll) {
    float r[3][3], t[3][3];
    rot_y(r, -yaw);
    rot_x(t, -pitch);
    mul33(r, r, t);
    rot_z(t, -roll);
    mul33(r, r, t);
    to_m2(m, r, pos);
}

static void xform_point(const float m[12], const float p[3], float out[3]);

static float ldf(uint32_t va) { return sr_f(sr_ld32(va)); }

/* The bumper camera sits in the player's car (0.75 above its pose): the car is not drawn then, nor its shadow
 * volume (the eye inside it). The behind view and the start's flying camera are outside it. */
static int eye_in_player_car(const float eye[3]) {
    const float dx = ldf(POSE_PLAYER) - eye[0], dy = ldf(POSE_PLAYER + 4) - eye[1], dz = ldf(POSE_PLAYER + 8) - eye[2];
    return dx * dx + dy * dy + dz * dz < 2.f * 2.f;
}

extern uint32_t sr_light_dir[3]; /* the GEO light vector the camera set (code/hooks/render_hooks.c), as bits */

/* the lit meshes' light this frame */
static void light_view_update(void) {
    int i;
    for(i = 0; i < 3; i++) sr_light_view[i] = sr_f(sr_light_dir[i]);
}

void sr_actors_render(const sr_actors_t *actors, const sr_course_t *course, const float view_matrix[12],
                      const sr_view_t *view, sr_render_stats_t *stats) {
    const float eye[3] = {ldf(0x20220cu), ldf(0x202210u), ldf(0x202214u)};
    uint32_t i;
    light_view_update();
    for(i = 0; i < actors->n; i++) {
        const sr_attach_t *at = &actors->a[i];
        const int part = part_of(course->meshes[at->mesh].oba);
        float pose[12], local[12], world[12];
        if(at->pose_va == POSE_PLAYER && eye_in_player_car(eye)) continue;
        /* the shadows alone in the translucent pass, everything else in the others */
        if((part == PART_SHADOW) != (view->list == SR_LIST_TRANSLUCENT)) continue;
        if(at->pose_va == HELI_NODE) {
            const float pos[3] = {ldf(HELI_NODE + 0x18), ldf(HELI_NODE + 0x1c), ldf(HELI_NODE + 0x20)};
            pose_matrix(pose, pos, ldf(HELI_NODE + 0x24), ldf(HELI_NODE + 0x28), ldf(HELI_NODE + 0x2c));
            sr_copy(local, at->local, sizeof(local));
            if(part == PART_ROTOR_MAIN || part == PART_ROTOR_TAIL) {
                float r[3][3];
                const float t[3] = {at->local[9], at->local[10], at->local[11]};
                if(part == PART_ROTOR_MAIN) rot_y(r, -ldf(HELI_NODE + 0x60));
                else rot_x(r, -ldf(HELI_NODE + 0x64));
                to_m2(local, r, t);
            }
        } else {
            const uint32_t pb = at->pose_va;
            const float pos[3] = {ldf(pb), ldf(pb + 4), ldf(pb + 8)};
            const float dx = pos[0] - eye[0], dy = pos[1] - eye[1], dz = pos[2] - eye[2];
            const float d2 = dx * dx + dy * dy + dz * dz;
            const int near = d2 < 25.f * 25.f;
            if(d2 > 400.f * 400.f) continue;
            if((part == PART_NEAR || part == PART_WHEEL_NEAR) && !near) continue;
            if((part == PART_MID || part == PART_WHEEL_MID) && near) continue;
            pose_matrix(pose, pos, ldf(pb + 0x18), ldf(pb + 0x1c), ldf(pb + 0x20));
            sr_copy(local, at->local, sizeof(local));
            if(part == PART_WHEEL_NEAR || part == PART_WHEEL_MID) {
                /* wheel index from the rest offset: front (z > 0) 0/1, rear 2/3; left (x < 0) first */
                const int w = (at->local[11] > 0.f ? 0 : 2) + (at->local[9] > 0.f ? 1 : 0);
                const float t[3] = {at->local[9], ldf(pb + 0x30 + 4u * (uint32_t)w) - 0.09f, at->local[11]};
                float r[3][3], s[3][3];
                rot_y(r, w < 2 ? -ldf(pb + 0x24) : 0.f);
                rot_x(s, ldf(pb + 0x2c));
                mul33(r, r, s);
                to_m2(local, r, t);
            }
        }
        sr_m2_compose(world, pose, local);
        sr_mesh_render(course, at->mesh, view_matrix, world, view, stats);
    }
}

#define SHADOW_REACH 1.f /* up and down from the posed plate, world units */

static void xform_point(const float m[12], const float p[3], float out[3]) {
    out[0] = m[0] * p[0] + m[3] * p[1] + m[6] * p[2] + m[9];
    out[1] = m[1] * p[0] + m[4] * p[1] + m[7] * p[2] + m[10];
    out[2] = m[2] * p[0] + m[5] * p[1] + m[8] * p[2] + m[11];
}

uint32_t sr_actors_shadow_volumes(const sr_actors_t *actors, const sr_course_t *course, const float view_matrix[12],
                                  const sr_view_t *view, void (*tri)(void *user, const float v[3][3], int last),
                                  void *user) {
    const float eye[3] = {ldf(0x20220cu), ldf(0x202210u), ldf(0x202214u)};
    const uint32_t n = actors->shadow_n;
    uint32_t i, volumes = 0;
    if(n < 3) return 0;
    for(i = 0; i < actors->n; i++) {
        const sr_attach_t *at = &actors->a[i];
        const uint32_t pb = at->pose_va;
        float pose[12], world[12], scr[2][SR_SHADOW_OUTLINE_MAX][3];
        uint32_t k, side, out_l = 1, out_r = 1, out_t = 1, out_b = 1, behind = 0, t, total;
        if(part_of(course->meshes[at->mesh].oba) != PART_SHADOW || pb == HELI_NODE) continue;
        if(pb == POSE_PLAYER && eye_in_player_car(eye)) continue;
        {
            const float pos[3] = {ldf(pb), ldf(pb + 4), ldf(pb + 8)};
            const float dx = pos[0] - eye[0], dy = pos[1] - eye[1], dz = pos[2] - eye[2];
            if(dx * dx + dy * dy + dz * dz > 400.f * 400.f) continue;
            pose_matrix(pose, pos, ldf(pb + 0x18), ldf(pb + 0x1c), ldf(pb + 0x20));
        }
        sr_m2_compose(world, pose, at->local);
        for(k = 0; k < n; k++) {
            const float local[3] = {actors->shadow_outline[k][0], 0.f, actors->shadow_outline[k][1]};
            float w[3];
            xform_point(world, local, w);
            for(side = 0; side < 2; side++) {
                const float p[3] = {w[0], w[1] + (side ? -SHADOW_REACH : SHADOW_REACH), w[2]};
                float v[3];
                xform_point(view_matrix, p, v);
                if(v[2] < view->znear) behind = 1;
                else {
                    const float iz = 1.f / v[2];
                    scr[side][k][0] = view->cx + v[0] * view->focal_x * iz;
                    scr[side][k][1] = view->cy - v[1] * view->focal_y * iz;
                    scr[side][k][2] = iz;
                    out_l &= scr[side][k][0] < 0.f;
                    out_r &= scr[side][k][0] > view->width;
                    out_t &= scr[side][k][1] < 0.f;
                    out_b &= scr[side][k][1] > view->height;
                }
            }
        }
        if(behind || out_l || out_r || out_t || out_b) continue;
        /* caps (fans) and walls: 4n - 4 triangles, the last one closing the volume */
        total = 4 * n - 4;
        t = 0;
        for(k = 1; k + 1 < n; k++)
            for(side = 0; side < 2; side++) {
                const float v[3][3] = {{scr[side][0][0], scr[side][0][1], scr[side][0][2]},
                                       {scr[side][k][0], scr[side][k][1], scr[side][k][2]},
                                       {scr[side][k + 1][0], scr[side][k + 1][1], scr[side][k + 1][2]}};
                tri(user, v, ++t == total);
            }
        for(k = 0; k < n; k++) {
            const uint32_t k2 = (k + 1) % n;
            const float a[3][3] = {{scr[0][k][0], scr[0][k][1], scr[0][k][2]},
                                   {scr[0][k2][0], scr[0][k2][1], scr[0][k2][2]},
                                   {scr[1][k2][0], scr[1][k2][1], scr[1][k2][2]}};
            const float b[3][3] = {{scr[0][k][0], scr[0][k][1], scr[0][k][2]},
                                   {scr[1][k2][0], scr[1][k2][1], scr[1][k2][2]},
                                   {scr[1][k][0], scr[1][k][1], scr[1][k][2]}};
            tri(user, a, ++t == total);
            tri(user, b, ++t == total);
        }
        volumes++;
    }
    return volumes;
}

uint32_t sr_particles_quads(const sr_actors_t *actors, const sr_course_t *course, const sr_view_t *view,
                            sr_particle_quad_t *out, uint32_t max) {
    const sr_particle_draw_t *d;
    const uint32_t n = sr_particles_draws(&d);
    float tgp_view[12];
    uint32_t i, k, count = 0;
    sr_particles_view(tgp_view);
    light_view_update();
    for(i = 0; i < n && count < max; i++) {
        const uint32_t mesh = actors->particle_mesh[d[i].oba == SR_OBA_DEBRIS ? 1 : 0];
        const sr_course_tri_t *t;
        const sr_course_vertex_t *corner[4];
        float centre[3];
        int behind = 0;
        if(mesh >= course->n_meshes || course->meshes[mesh].n_tris < 2) continue;
        t = course->meshes[mesh].tris;
        /* the quad's two triangles (a,b,c),(a,c,d) as the strip b,a,c,d */
        corner[0] = &t[0].v[1], corner[1] = &t[0].v[0], corner[2] = &t[0].v[2], corner[3] = &t[1].v[2];
        xform_point(tgp_view, d[i].pos, centre);
        out[count].texture = t[0].texture;
        out[count].argb = 0xffffffffu;
        if(course->meshes[mesh].light && sr_light_tables) {
            /* lit as the GEO lights it: the quad's normal through the TGP's diag(1.5, size, 1), unnormalised */
            const sr_tri_light_t *l = course->meshes[mesh].light;
            const float nv[3] = {l->n[0] * 1.5f, l->n[1] * d[i].size, l->n[2]};
            const float c = sr_light_factor(l, nv, centre) * 255.f;
            const uint32_t g = c >= 255.f ? 255u : (uint32_t)c;
            out[count].argb = 0xff000000u | g << 16 | g << 8 | g;
        }
        for(k = 0; k < 4; k++) {
            const float x = centre[0] + corner[k]->x * 1.5f, y = centre[1] + corner[k]->y * d[i].size;
            const float z = centre[2] + corner[k]->z;
            sr_particle_vertex_t *v = &out[count].v[k];
            if(z < view->znear) {
                behind = 1;
                break;
            }
            v->invz = 1.f / z;
            v->x = view->cx + x * view->focal_x * v->invz;
            v->y = view->cy - y * view->focal_y * v->invz;
            v->u = corner[k]->u;
            v->v = corner[k]->v;
        }
        if(!behind) count++;
    }
    return count;
}

int sr_actors_light_load(sr_course_t *course, const uint8_t *data, size_t size) {
    uint32_t n_tab, n_mesh, i, m, t;
    int lit = 0;
    size_t off = 12;
    float(*tables)[17];
    if(size < 12 || memcmp(data, "SRLIT01", 8)) return -1;
    sr_copy(&n_tab, data + 8, 4);
    if(off + (size_t)n_tab * 68u + 4u > size) return -1;
    tables = sr_alloc((n_tab ? n_tab : 1) * sizeof(*tables));
    if(!tables) return -1;
    sr_copy(tables, data + off, n_tab * 68u);
    off += n_tab * 68u;
    sr_copy(&n_mesh, data + off, 4);
    off += 4;
    for(i = 0; i < n_mesh; i++) {
        uint32_t oba, n;
        sr_tri_light_t *light;
        sr_course_mesh_t *mesh = NULL;
        if(off + 8 > size) return -1;
        sr_copy(&oba, data + off, 4);
        sr_copy(&n, data + off + 4, 4);
        off += 8;
        if(off + (size_t)n * 16u > size) return -1;
        for(m = 0; m < course->n_meshes; m++)
            if(course->meshes[m].oba == oba && course->meshes[m].n_tris == n) mesh = &course->meshes[m];
        if(mesh && (light = sr_alloc(n * sizeof(*light))) != NULL) {
            for(t = 0; t < n; t++) {
                const uint8_t *r = data + off + t * 16u;
                const sr_course_vertex_t *v = mesh->tris[t].v;
                const float ax = v[1].x - v[0].x, ay = v[1].y - v[0].y, az = v[1].z - v[0].z;
                const float bx = v[2].x - v[0].x, by = v[2].y - v[0].y, bz = v[2].z - v[0].z;
                const float nx = ay * bz - az * by, ny = az * bx - ax * bz, nz = ax * by - ay * bx;
                const float len = sqrtf(nx * nx + ny * ny + nz * nz);
                uint16_t table;
                light[t].n[0] = len > 0.f ? nx / len : 0.f;
                light[t].n[1] = len > 0.f ? ny / len : 0.f;
                light[t].n[2] = len > 0.f ? nz / len : 0.f;
                light[t].d = light[t].n[0] * v[0].x + light[t].n[1] * v[0].y + light[t].n[2] * v[0].z;
                sr_copy(&light[t].ambient, r, 4);
                sr_copy(&light[t].diffuse, r + 4, 4);
                sr_copy(&light[t].spec_scale, r + 8, 4);
                light[t].spec_ctrl = r[12];
                light[t].spec_mode = r[13];
                sr_copy(&table, r + 14, 2);
                light[t].table = table < n_tab ? table : 0;
            }
            mesh->light = light;
            lit++;
        }
        off += (size_t)n * 16u;
    }
    sr_light_tables = (const float(*)[17])tables;
    return lit;
}
