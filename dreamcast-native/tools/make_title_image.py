#!/usr/bin/env python3
"""The race's title card ("1995 SEGA RALLY / DESERT / START!" over the course picture) as one paletted image for
the Dreamcast (pc/<course>.title, read by code/main.c).

The card is the practice scene 4 (attract_hud_setup 0x1B940, absolute ticks 26..146): two CGM pictures in the 2D
layer, the course picture on map 2 (category 0) and the title on map 0 (category 1) over it, no 3D, static until
the race frame starts at tick 147 (fading in from white). So it is composed here from the reference's 2D state in
the middle of the scene (a snapshot's TILE, CHAR, PALR sections) exactly as the host draws it: maps 3..0 category 0,
then 3..0 category 1, pen 0 see-through, black kept (the reference punches black only over a 3D view), white where
no map has a pixel. The colours it uses (the picture's sepia is its own palette) go into one 256-entry palette.

  make_title_image.py SNAPSHOT OUT.title [--frames 121]

OUT: "SRTTL01\\0", u16 width, u16 height (512 x 512, the card in the top left 496 x 384), u16 frames to show,
u16 colours, colours x u16 ARGB1555, then width x height palette indices in the PVR's twiddled order.
"""
import argparse, struct, sys

W, H = 496, 384
TEX = 512


def twiddle(x, y, size):
    i = 0
    bit = 0
    while (1 << bit) < size:
        i |= ((y >> bit) & 1) << (2 * bit)
        i |= ((x >> bit) & 1) << (2 * bit + 1)
        bit += 1
    return i


def sections(path):
    d = open(path, 'rb').read()
    if d[:8] != b'SRSNAP01':
        sys.exit(f'{path}: not a segamod2 snapshot')
    o, out = 8, {}
    while o < len(d):
        tag = d[o:o + 4]
        n = struct.unpack_from('<I', d, o + 4)[0]
        out[tag] = d[o + 8:o + 8 + n]
        o += 8 + n
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('snapshot')
    ap.add_argument('out')
    ap.add_argument('--frames', type=int, default=121)
    a = ap.parse_args()
    s = sections(a.snapshot)
    tile, chars, pal = s[b'TILE'], s[b'CHAR'], s[b'PALR']

    def cell(m, x, y):
        return struct.unpack_from('<H', tile, 2 * (m * 0x1000 + y * 64 + x))[0]

    def pixel(code, x, y):
        row = chars[(code & 0x3fff) * 32 + y * 4:]
        word = row[(x >> 2) * 2] | row[(x >> 2) * 2 + 1] << 8
        return (word >> (12 - 4 * (x & 3))) & 15

    white = 0xffff
    img = [white] * (W * H)
    for cat in (0, 1):
        for m in (3, 2, 1, 0):
            for cy in range(H // 8):
                for cx in range(W // 8):
                    c = cell(m, cx, cy)
                    if not c or (c >> 15) != cat:
                        continue
                    line = (c >> 7) & 0xff
                    for y in range(8):
                        for x in range(8):
                            pen = pixel(c, x, y)
                            if pen:
                                v = struct.unpack_from('<H', pal, 2 * ((line * 16 + pen) & 2047))[0]
                                # RGB555 (red in bits 0-4) -> ARGB1555 (red in bits 10-14)
                                img[(cy * 8 + y) * W + cx * 8 + x] = 0x8000 | (v & 31) << 10 | (v & 0x3e0) | (v >> 10 & 31)
    colours = sorted(set(img))
    if len(colours) > 256:
        sys.exit(f'{len(colours)} colours: more than one PAL8 palette')
    index = {c: i for i, c in enumerate(colours)}
    texels = bytearray(TEX * TEX)
    for y in range(H):
        for x in range(W):
            texels[twiddle(x, y, TEX)] = index[img[y * W + x]]
    with open(a.out, 'wb') as f:
        f.write(b'SRTTL01\0' + struct.pack('<4H', TEX, TEX, a.frames, len(colours)))
        f.write(struct.pack('<%dH' % len(colours), *colours))
        f.write(texels)
    print(f'{a.out}: {len(colours)} colours, {a.frames} frames')


if __name__ == '__main__':
    main()
