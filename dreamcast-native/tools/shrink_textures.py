#!/usr/bin/env python3
"""Fit a v2 course pack's textures into a VRAM budget by halving the largest ones (2x2 box filter, cut-out texels
kept when any of the four is opaque gets the average of the opaque ones), until the total fits. UVs are in texture
units, so they stay valid. A stopgap until the paletted / VQ textures.

  shrink_textures.py IN.course OUT.course [--budget-kib 3584] [--min 16]
"""
import argparse, struct, sys
sys.path.insert(0, __file__.rsplit('/', 1)[0])


def twiddle(x, y, w, h):
    s = min(w, h)
    block = x // s if w > h else y // s
    x %= s; y %= s
    i = 0; bit = 0
    while (1 << bit) < s:
        i |= ((y >> bit) & 1) << (2 * bit)
        i |= ((x >> bit) & 1) << (2 * bit + 1)
        bit += 1
    return block * s * s + i


def halve(w, h, texels):
    nw, nh = max(w // 2, 1), max(h // 2, 1)
    out = [0] * (nw * nh)
    for y in range(nh):
        for x in range(nw):
            px = [texels[twiddle(min(2 * x + dx, w - 1), min(2 * y + dy, h - 1), w, h)] for dx in (0, 1) for dy in (0, 1)]
            op = [p for p in px if p & 0x8000]
            if not op:
                out[twiddle(x, y, nw, nh)] = 0
                continue
            r = sum((p >> 10) & 31 for p in op) // len(op)
            g = sum((p >> 5) & 31 for p in op) // len(op)
            b = sum(p & 31 for p in op) // len(op)
            out[twiddle(x, y, nw, nh)] = 0x8000 | (r << 10) | (g << 5) | b
    return nw, nh, out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('src'); ap.add_argument('dst')
    ap.add_argument('--budget-kib', type=int, default=3584)
    ap.add_argument('--min', type=int, default=16)
    a = ap.parse_args()
    d = open(a.src, 'rb').read()
    assert d[:7] == b'SRCRS02'
    nt, nm, ni = struct.unpack('<III', d[8:20])
    off = 20; tex = []
    for _ in range(nt):
        w, h, fl = struct.unpack('<HHI', d[off:off + 8])
        t = list(struct.unpack('<%dH' % (w * h), d[off + 8:off + 8 + w * h * 2]))
        off += 8 + w * h * 2; off = (off + 3) & ~3
        tex.append([w, h, fl, t])
    rest = d[off:]
    total = lambda: sum(t[0] * t[1] * 2 for t in tex)
    before = total()
    while total() > a.budget_kib * 1024:
        i = max(range(nt), key=lambda k: tex[k][0] * tex[k][1] if min(tex[k][0], tex[k][1]) > a.min else -1)
        if min(tex[i][0], tex[i][1]) <= a.min: break
        tex[i][0], tex[i][1], tex[i][3] = halve(tex[i][0], tex[i][1], tex[i][3])
    out = [d[:20]]
    for w, h, fl, t in tex:
        blob = struct.pack('<HHI', w, h, fl) + struct.pack('<%dH' % len(t), *t)
        blob += b'\0' * ((-len(blob)) % 4)
        out.append(blob)
    out.append(rest)
    open(a.dst, 'wb').write(b''.join(out))
    print(f'textures {before >> 10} KiB -> {total() >> 10} KiB')


if __name__ == '__main__':
    main()
