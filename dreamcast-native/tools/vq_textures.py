#!/usr/bin/env python3
"""Store a v2 course pack's larger textures in the PVR's VQ form (SR_TEX_VQ, include/course_pack.h): 256 codebook
entries of 2x2 texels per texture, one byte per 2x2 block. About 8x smaller than ARGB1555, so the whole desert fits
in VRAM at full resolution.

The codebook is a k-means of the texture's 2x2 blocks (weighted by how often each distinct block occurs). The
cut-out bit is weighted far above any colour difference, so a block's opaque / cut-out pattern is kept exactly and
punch-through edges do not move. Textures under --min-texels stay ARGB1555 (the 2 KiB codebook would not pay), and
so do the textures of the meshes named by --raw-oba: the particles' single-texel grains would become 2x2 blobs.

  vq_textures.py IN.course OUT.course [--min-texels 4096] [--iters 12] [--raw-oba 0x82c95b,0x82c96c]
"""
import argparse, struct, sys
import numpy as np

ALPHA_WEIGHT = 100.0  # per texel; a colour channel spans 0..31


def to_vectors(blocks):
    """(n, 4) ARGB1555 -> (n, 16) float: r, g, b, weighted alpha per texel; cut-out texels' colour is 0."""
    a = (blocks >> 15) & 1
    r = ((blocks >> 10) & 31) * a
    g = ((blocks >> 5) & 31) * a
    b = (blocks & 31) * a
    return np.stack([r, g, b, a * ALPHA_WEIGHT], axis=2).reshape(len(blocks), 16).astype(np.float32)


def from_vectors(v):
    """(k, 16) float -> (k, 4) ARGB1555."""
    v = v.reshape(-1, 4, 4)
    a = (v[:, :, 3] >= ALPHA_WEIGHT / 2).astype(np.uint16)
    c = np.clip(np.rint(v[:, :, :3]), 0, 31).astype(np.uint16) * a[:, :, None]
    return (a << 15) | (c[:, :, 0] << 10) | (c[:, :, 1] << 5) | c[:, :, 2]


def nearest(x, c):
    """Index of the nearest row of c for each row of x, in slices to bound memory."""
    out = np.empty(len(x), np.int64)
    cn = (c * c).sum(1)
    for s in range(0, len(x), 16384):
        xs = x[s:s + 16384]
        out[s:s + 16384] = np.argmin(cn[None, :] - 2.0 * xs @ c.T, axis=1)
    return out


def kmeans(x, w, k, iters, rng):
    # k-means++ seeding on the distinct blocks, weighted by count
    c = np.empty((k, x.shape[1]), np.float32)
    c[0] = x[rng.choice(len(x), p=w / w.sum())]
    d = ((x - c[0]) ** 2).sum(1)
    for i in range(1, k):
        p = d * w
        c[i] = x[rng.choice(len(x), p=p / p.sum())] if p.sum() > 0 else x[rng.integers(len(x))]
        d = np.minimum(d, ((x - c[i]) ** 2).sum(1))
    for _ in range(iters):
        lab = nearest(x, c)
        sw = np.bincount(lab, weights=w, minlength=k)
        for j in range(x.shape[1]):
            s = np.bincount(lab, weights=w * x[:, j], minlength=k)
            c[:, j] = np.where(sw > 0, s / np.maximum(sw, 1e-9), c[:, j])
    return c


def encode(w, h, texels, iters, rng):
    """Twiddled ARGB1555 -> (codebook bytes, index bytes). Twiddled order keeps each 2x2 block's texels together."""
    blocks = np.asarray(texels, np.uint16).reshape(-1, 4)
    uniq, inv, cnt = np.unique(blocks, axis=0, return_inverse=True, return_counts=True)
    inv = inv.reshape(-1)
    if len(uniq) <= 256:
        book = np.zeros((256, 4), np.uint16)
        book[:len(uniq)] = uniq
        idx = inv
    else:
        x = to_vectors(uniq)
        c = kmeans(x, cnt.astype(np.float64), 256, iters, rng)
        book = from_vectors(c)
        idx = nearest(x, to_vectors(book))[inv]  # against the codebook as stored
    return book.astype('<u2').tobytes(), idx.astype(np.uint8).tobytes()


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('src'); ap.add_argument('dst')
    ap.add_argument('--min-texels', type=int, default=4096)
    ap.add_argument('--iters', type=int, default=12)
    ap.add_argument('--raw-oba', default='0x82c95b,0x82c96c', help='meshes whose textures stay ARGB1555')
    a = ap.parse_args()
    rng = np.random.default_rng(1)
    d = open(a.src, 'rb').read()
    assert d[:7] == b'SRCRS02'
    nt, nm = struct.unpack('<II', d[8:16])
    raw_obas = {int(x, 0) for x in a.raw_oba.split(',') if x.strip()}
    off = 20
    raw_tex = set()
    o = off
    for i in range(nt):  # skip the textures to reach the meshes
        w, h, fl = struct.unpack('<HHI', d[o:o + 8])
        o += 8 + w * h * 2; o = (o + 3) & ~3
    for i in range(nm):
        oba, obc, n = struct.unpack('<III', d[o:o + 12])
        o += 28
        if oba in raw_obas:
            for t in range(n):
                raw_tex.add(struct.unpack_from('<H', d, o + 76 * t)[0])
        o += 76 * n
    out = [d[:20]]
    before = after = n_vq = 0
    for i in range(nt):
        w, h, fl = struct.unpack('<HHI', d[off:off + 8])
        assert not fl & 16, 'already VQ'
        raw = d[off + 8:off + 8 + w * h * 2]
        off += 8 + w * h * 2; off = (off + 3) & ~3
        before += len(raw)
        if w * h >= a.min_texels and i not in raw_tex:
            book, idx = encode(w, h, np.frombuffer(raw, '<u2'), a.iters, rng)
            blob = struct.pack('<HHI', w, h, fl | 16) + book + idx
            n_vq += 1
        else:
            blob = struct.pack('<HHI', w, h, fl) + raw
        after += len(blob) - 8
        blob += b'\0' * ((-len(blob)) % 4)
        out.append(blob)
        print(f'\r{i + 1}/{nt}', end='', file=sys.stderr)
    out.append(d[off:])
    open(a.dst, 'wb').write(b''.join(out))
    print(f'\ntextures {before >> 10} KiB -> {after >> 10} KiB ({n_vq} of {nt} VQ)', file=sys.stderr)


if __name__ == '__main__':
    main()
