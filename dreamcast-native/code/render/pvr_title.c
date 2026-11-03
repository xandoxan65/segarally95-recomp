/* The race's title card (tools/make_title_image.py): one 512 x 512 PAL8 texture, the card in its top left 496 x 384,
 * drawn over the whole screen in the opaque list for the card's frames before the countdown. The palette goes to the
 * PVR's last 256-entry bank (entries 768..1023, which the HUD's 16-entry banks 48..63 share: the HUD reloads its
 * colours after the card, sr_pvr_hud_reset_palette). */
#include "render/pvr_backend.h"
#include "render/ta.h"
#include "sr_alloc.h"
#include <dc/pvr.h>
#include <stdio.h>
#include <string.h>

#define BANK 3u

static pvr_poly_hdr_t s_hdr __attribute__((aligned(32)));
static uint16_t s_palette[256];
static uint32_t s_colours, s_w, s_h;
static int s_ready;

uint32_t sr_pvr_title_load(const uint8_t *data, size_t size) {
    uint16_t head[4];
    pvr_ptr_t vram;
    pvr_poly_cxt_t cxt;
    size_t texels;
    if(!data || size < 16 || memcmp(data, "SRTTL01", 8)) return 0;
    sr_copy(head, data + 8, sizeof(head));
    s_w = head[0], s_h = head[1], s_colours = head[3];
    texels = (size_t)s_w * s_h;
    if(s_colours > 256u || size < 16u + s_colours * 2u + texels) return 0;
    sr_copy(s_palette, data + 16, s_colours * 2u);
    vram = pvr_mem_malloc(texels);
    if(!vram) {
        printf("sr: no VRAM for the title card (free %lu KiB)\n", (unsigned long)(pvr_mem_available() >> 10));
        return 0;
    }
    pvr_txr_load(data + 16 + s_colours * 2u, vram, texels);
    pvr_poly_cxt_txr(&cxt, PVR_LIST_OP_POLY, PVR_TXRFMT_PAL8BPP | PVR_TXRFMT_8BPP_PAL(BANK) | PVR_TXRFMT_TWIDDLED, s_w,
                     s_h, vram, PVR_FILTER_NONE);
    cxt.gen.culling = PVR_CULLING_NONE;
    cxt.depth.comparison = PVR_DEPTHCMP_ALWAYS;
    cxt.txr.env = PVR_TXRENV_REPLACE;
    pvr_poly_compile(&s_hdr, &cxt);
    s_ready = 1;
    return head[2];
}

void sr_pvr_title_begin(void) {
    uint32_t i;
    if(!s_ready) return;
    for(i = 0; i < s_colours; i++) pvr_set_pal_entry(BANK * 256u + i, s_palette[i]);
}

void sr_pvr_title_draw(void) {
    const float u1 = 496.f / (float)s_w, v1 = 384.f / (float)s_h;
    const float xy[4][2] = {{0.f, 0.f}, {640.f, 0.f}, {0.f, 480.f}, {640.f, 480.f}};
    const float uv[4][2] = {{0.f, 0.f}, {u1, 0.f}, {0.f, v1}, {u1, v1}};
    uint32_t k;
    if(!s_ready) return;
    {
        pvr_poly_hdr_t *h = sr_ta_target();
        *h = s_hdr;
        sr_ta_commit(h);
    }
    for(k = 0; k < 4; k++) {
        pvr_vertex_t *v = sr_ta_target();
        v->flags = k == 3 ? PVR_CMD_VERTEX_EOL : PVR_CMD_VERTEX;
        v->x = xy[k][0];
        v->y = xy[k][1];
        v->z = 1.f;
        v->u = uv[k][0];
        v->v = uv[k][1];
        v->argb = 0xffffffffu;
        v->oargb = 0;
        sr_ta_commit(v);
    }
}
