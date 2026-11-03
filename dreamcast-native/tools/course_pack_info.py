#!/usr/bin/env python3
"""Inspect, validate, merge and draw course packs, version 2 (dreamcast-native/include/course_pack.h).

  course_pack_info.py build/course/desert.course                     counts, bounds, triangles, textures, layers
  course_pack_info.py build/course/desert.course --meshes            plus one line per mesh
  course_pack_info.py build/course/desert.course --textures          plus one line per texture (biggest first)
  course_pack_info.py build/course/desert.course --png top.png       top-down view of the static instances, each
                                                                     triangle in its texture's mean colour (or its
                                                                     flat colour); SVG when PIL is missing
  course_pack_info.py build/course/desert.course --sheet tex.png     contact sheet of the first --sheet-count (64)
                                                                     textures, untwiddled, cut-out texels as a
                                                                     checkerboard
  course_pack_info.py --merge out.course a.course b.course ...       union of several captures: textures deduped by
                                                                     content, meshes with the same oba/obc and
                                                                     geometry shared (first pack's texturing), static instances at the same
                                                                     matrix kept once, one dynamic instance per
                                                                     mesh without a static one

  course_pack_info.py PACK --attach OUT.attach --dyn DUMP             fit the moving objects (cars, wheels,
                                                                     helicopter) to their poses from a capture dump
                                                                     (segamod2 I960_HOST_EXPORT_DYNDUMP) and write the
                                                                     attachment table (format and Pose under ATTACH)

World axes: x and z span the ground, y is height, +y up (the latched view maps world +y to view +y, which the host
GL viewer draws screen-up). The top-down view looks along y, x to the right and z up, and leaves out instances whose
bounding sphere is larger than --max-radius (the ground/sky planes) and meshes named by --skip (e.g. --skip 0x7c11:
polygon RAM slot 0, a sky-palette wrapped-texture plane drawn at scale 40 below the infield terrain).
"""
import argparse
import hashlib
import math
import struct
import sys

MAGIC = b"SRCRS02\0"
SR_INST_STATIC = 1
SR_NO_TEXTURE = 0xFFFF
SR_TEX_REPEAT_U, SR_TEX_REPEAT_V, SR_TEX_MIRROR_U, SR_TEX_MIRROR_V = 1, 2, 4, 8
SR_TRI_CUTOUT = 1
TRI_SIZE = 4 + 3 * 24


def twiddle_index(x, y, w, h):
    """dreamcast-native/include/render/twiddle.h sr_twiddle_index."""
    s = min(w, h)
    block = x // s if w > h else y // s
    x %= s
    y %= s
    i = 0
    bit = 0
    while (1 << bit) < s:
        i |= ((y >> bit) & 1) << (2 * bit)
        i |= ((x >> bit) & 1) << (2 * bit + 1)
        bit += 1
    return block * s * s + i


class Texture:
    __slots__ = ("w", "h", "flags", "texels")  # texels: bytes, twiddled ARGB1555

    def __init__(self, w, h, flags, texels):
        self.w, self.h, self.flags, self.texels = w, h, flags, texels

    @property
    def nbytes(self):
        return len(self.texels)

    def linear(self):
        """Row-major list of ARGB1555 values (v = 0 first row)."""
        tw = struct.unpack("<%dH" % (self.w * self.h), self.texels)
        return [tw[twiddle_index(x, y, self.w, self.h)] for y in range(self.h) for x in range(self.w)]

    def mean_rgb(self):
        r = g = b = n = 0
        for t in struct.unpack("<%dH" % (self.w * self.h), self.texels):
            if t & 0x8000:
                r += (t >> 10) & 31
                g += (t >> 5) & 31
                b += t & 31
                n += 1
        if not n:
            return (0, 0, 0)
        return (r * 255 // (31 * n), g * 255 // (31 * n), b * 255 // (31 * n))


class Mesh:
    __slots__ = ("oba", "obc", "bounds", "tris")  # tris: bytes, n * TRI_SIZE

    def __init__(self, oba, obc, bounds, tris):
        self.oba, self.obc, self.bounds, self.tris = oba, obc, bounds, tris

    @property
    def n_tris(self):
        return len(self.tris) // TRI_SIZE

    def triangles(self):
        """(texture, flags, [(x, y, z, u, v, argb)] * 3)"""
        for o in range(0, len(self.tris), TRI_SIZE):
            tex, fl = struct.unpack_from("<HH", self.tris, o)
            vs = [struct.unpack_from("<5fI", self.tris, o + 4 + 24 * k) for k in range(3)]
            yield tex, fl, vs


class Instance:
    __slots__ = ("mesh", "flags", "m")

    def __init__(self, mesh, flags, m):
        self.mesh, self.flags, self.m = mesh, flags, m


def load(path):
    data = open(path, "rb").read()
    if data[:8] != MAGIC:
        raise SystemExit(f"{path}: bad magic {data[:8]!r} (expected {MAGIC!r})")
    n_tex, n_meshes, n_inst = struct.unpack_from("<III", data, 8)
    off = 20
    texs = []
    for _ in range(n_tex):
        w, h, flags = struct.unpack_from("<HHI", data, off)
        off += 8
        if w & (w - 1) or h & (h - 1) or not (8 <= w <= 1024 and 8 <= h <= 1024):
            raise SystemExit(f"{path}: texture {len(texs)} bad size {w}x{h}")
        texs.append(Texture(w, h, flags, data[off:off + 2 * w * h]))
        off += 2 * w * h
    meshes = []
    for _ in range(n_meshes):
        oba, obc, n_tris = struct.unpack_from("<III", data, off)
        bounds = struct.unpack_from("<4f", data, off + 12)
        off += 28
        size = n_tris * TRI_SIZE
        if off + size > len(data):
            raise SystemExit(f"{path}: truncated mesh data")
        meshes.append(Mesh(oba, obc, bounds, data[off:off + size]))
        off += size
    insts = []
    for _ in range(n_inst):
        mesh, flags = struct.unpack_from("<II", data, off)
        m = struct.unpack_from("<12f", data, off + 8)
        off += 56
        if mesh >= n_meshes:
            raise SystemExit(f"{path}: instance mesh index {mesh} out of range")
        insts.append(Instance(mesh, flags, m))
    if off != len(data):
        raise SystemExit(f"{path}: {len(data) - off} trailing bytes")
    return texs, meshes, insts


def save(path, texs, meshes, insts):
    out = [MAGIC, struct.pack("<III", len(texs), len(meshes), len(insts))]
    for t in texs:
        out.append(struct.pack("<HHI", t.w, t.h, t.flags))
        out.append(t.texels)
    for me in meshes:
        out.append(struct.pack("<III4f", me.oba, me.obc, me.n_tris, *me.bounds))
        out.append(me.tris)
    for it in insts:
        out.append(struct.pack("<II12f", it.mesh, it.flags, *it.m))
    with open(path, "wb") as f:
        f.write(b"".join(out))


def xform(m, x, y, z):
    return (x * m[0] + y * m[3] + z * m[6] + m[9],
            x * m[1] + y * m[4] + z * m[7] + m[10],
            x * m[2] + y * m[5] + z * m[8] + m[11])


def col_scale(m):
    return max(math.sqrt(m[3 * c] ** 2 + m[3 * c + 1] ** 2 + m[3 * c + 2] ** 2) for c in range(3))


def world_sphere(me, it):
    return xform(it.m, *me.bounds[:3]), me.bounds[3] * col_scale(it.m)


def same_matrix(a, b, rtol=2e-3, ttol=0.05):
    return all(abs(a[i] - b[i]) <= rtol for i in range(9)) and all(abs(a[i] - b[i]) <= ttol for i in range(9, 12))


def remap_tris(tris, remap):
    b = bytearray(tris)
    for o in range(0, len(b), TRI_SIZE):
        tex = struct.unpack_from("<H", b, o)[0]
        if tex != SR_NO_TEXTURE:
            struct.pack_into("<H", b, o, remap[tex])
    return bytes(b)


def merge(out_path, paths):
    """PATH@meshes: take only that pack's new meshes (as moving objects), none of its scenery: a capture in another
    GEO window (the countdown's) would otherwise add its objects to the course."""
    texs, tex_index = [], {}
    meshes, key_index, statics, dynamic = [], {}, {}, {}
    for p in paths:
        meshes_only = p.endswith("@meshes")
        if meshes_only:
            p = p[:-len("@meshes")]
        ts, ms, its = load(p)
        tremap = []
        for t in ts:
            k = (t.w, t.h, t.flags, t.texels)
            if k not in tex_index:
                tex_index[k] = len(texs)
                texs.append(t)
            tremap.append(tex_index[k])
        remap = []
        for me in ms:
            me = Mesh(me.oba, me.obc, me.bounds, remap_tris(me.tris, tremap))
            # Same object = same oba/obc and geometry; texels and colours may differ between captures (the luma
            # baked into them is the game's per-draw, view-dependent polygon luminance): the first pack wins.
            key = (me.oba, me.obc, me.n_tris, tuple(round(b, 3) for b in me.bounds))
            if key not in key_index:
                key_index[key] = len(meshes)
                meshes.append(me)
            remap.append(key_index[key])
        for it in its:
            mi = remap[it.mesh]
            if meshes_only:
                dynamic.setdefault(mi, it.m)
                continue
            if it.flags & SR_INST_STATIC:
                lst = statics.setdefault(mi, [])
                if not any(same_matrix(it.m, o) for o in lst):
                    lst.append(it.m)
            else:
                dynamic.setdefault(mi, it.m)
    insts = []
    for mi in range(len(meshes)):
        for m in statics.get(mi, []):
            insts.append(Instance(mi, SR_INST_STATIC, m))
        if mi not in statics and mi in dynamic:
            insts.append(Instance(mi, 0, dynamic[mi]))
    used = sorted({it.mesh for it in insts})
    new_index = {old: new for new, old in enumerate(used)}
    meshes = [meshes[i] for i in used]
    for it in insts:
        it.mesh = new_index[it.mesh]
    # Keep only the textures the remaining meshes use, in first-use order.
    order, tnew = [], {}
    for me in meshes:
        for tex, _, _ in me.triangles():
            if tex != SR_NO_TEXTURE and tex not in tnew:
                tnew[tex] = len(order)
                order.append(tex)
    meshes = [Mesh(me.oba, me.obc, me.bounds, remap_tris(me.tris, tnew)) for me in meshes]
    texs = [texs[i] for i in order]
    save(out_path, texs, meshes, insts)
    print(f"merged {len(paths)} pack(s) -> {out_path}")
    return out_path


def index_plane_key(t):
    """Texel pattern with colours renamed by first appearance: textures that differ only in palette (the same patch
    under another colorbase or luma) share it. Returns (key, number of colours)."""
    names = {}
    plane = bytearray()
    for v in t.linear():
        if v not in names:
            names[v] = len(names)
        plane.append(min(names[v], 255))
    return hashlib.sha1(bytes(plane) + struct.pack("<HH", t.w, t.h)).digest(), len(names)


def report(path, texs, meshes, insts, list_meshes, list_textures, max_radius):
    n_static = sum(1 for it in insts if it.flags & SR_INST_STATIC)
    n_dyn = len(insts) - n_static
    mesh_tris = sum(me.n_tris for me in meshes)
    static_tris = sum(meshes[it.mesh].n_tris for it in insts if it.flags & SR_INST_STATIC)
    used = {it.mesh for it in insts}
    tex_bytes = sum(t.nbytes for t in texs)
    print(f"{path}")
    print(f"  textures          {len(texs)}, {tex_bytes} bytes ({tex_bytes / 1048576:.2f} MiB ARGB1555)")
    print(f"  meshes            {len(meshes)} ({mesh_tris} triangles; {len(meshes) - len(used)} without an instance)")
    print(f"  instances         {len(insts)}: {n_static} static, {n_dyn} dynamic")
    print(f"  static triangles  {static_tris} (instanced)")
    flat = cut = textured = 0
    layers = [0] * 16
    bad_tex = bad_vert = 0
    for me in meshes:
        for tex, fl, vs in me.triangles():
            if tex == SR_NO_TEXTURE:
                flat += 1
            elif tex >= len(texs):
                bad_tex += 1
            elif fl & SR_TRI_CUTOUT:
                cut += 1
            else:
                textured += 1
            layers[(fl >> 4) & 15] += 1
            for x, y, z, u, v, _ in vs:
                if not all(math.isfinite(c) for c in (x, y, z, u, v)):
                    bad_vert += 1
                elif math.dist((x, y, z), me.bounds[:3]) > me.bounds[3] * 1.0001 + 1e-3:
                    bad_vert += 1
    print(f"  triangles         textured {textured}, cut-out {cut}, flat {flat}")
    print("  decal layers      " + ", ".join(f"{i}: {n}" for i, n in enumerate(layers) if n))
    lo, hi, big = [math.inf] * 3, [-math.inf] * 3, 0
    for it in insts:
        if not it.flags & SR_INST_STATIC:
            continue
        c, r = world_sphere(meshes[it.mesh], it)
        if r > max_radius:
            big += 1
            continue
        for k in range(3):
            lo[k] = min(lo[k], c[k] - r)
            hi[k] = max(hi[k], c[k] + r)
    if n_static:
        print("  static bounds     x [%.1f, %.1f]  y [%.1f, %.1f]  z [%.1f, %.1f]" % (lo[0], hi[0], lo[1], hi[1], lo[2], hi[2]))
        print(f"                    ({big} instance(s) with radius > {max_radius:g} left out: ground/sky planes)")
    ok = bad_tex == 0 and bad_vert == 0
    print(f"  validation        {'ok' if ok else f'{bad_tex} bad texture indices, {bad_vert} bad vertices'}")
    # Palette estimate: distinct index planes and their colour counts.
    planes = {}
    for t in texs:
        k, ncol = index_plane_key(t)
        planes.setdefault(k, [t.w * t.h, ncol])
    pal4 = sum((a + 1) // 2 for a, n in planes.values() if n <= 16)
    pal8 = sum(a for a, n in planes.values() if n > 16)
    over = sum(1 for a, n in planes.values() if n > 256)
    print(f"  palette estimate  {len(planes)} distinct texel patterns (textures differing only in palette share one):"
          f" {pal4 + pal8} bytes as PAL4/PAL8 index planes + {len(texs)} palettes"
          f"{f'; {over} pattern(s) with > 256 colours' if over else ''}")
    if list_textures:
        for i in sorted(range(len(texs)), key=lambda i: -texs[i].nbytes):
            t = texs[i]
            fl = "".join(c for c, b in (("Ru", 1), ("Rv", 2), ("Mu", 4), ("Mv", 8)) if t.flags & b)
            print("  texture %4d %4dx%-4d %8d bytes flags %s" % (i, t.w, t.h, t.nbytes, fl))
    if list_meshes:
        counts = {}
        for it in insts:
            k = counts.setdefault(it.mesh, [0, 0])
            k[0 if it.flags & SR_INST_STATIC else 1] += 1
        for i, me in enumerate(meshes):
            s, d = counts.get(i, (0, 0))
            print("  mesh %4d oba=%08x obc=%6d tris=%5d r=%10.1f static=%4d dynamic=%d" % (i, me.oba, me.obc, me.n_tris, me.bounds[3], s, d))


def tri_colour(texs, tex, vs, cache):
    if tex == SR_NO_TEXTURE:
        a = vs[0][5]
        return ((a >> 16) & 255, (a >> 8) & 255, a & 255)
    if tex not in cache:
        cache[tex] = texs[tex].mean_rgb()
    return cache[tex]


def gather_tris(texs, meshes, insts, max_radius, skip=()):
    tris, cache = [], {}  # (mean y, [(x, z)*3], rgb, layer)
    for it in insts:
        if not it.flags & SR_INST_STATIC:
            continue
        me = meshes[it.mesh]
        if me.oba in skip or world_sphere(me, it)[1] > max_radius:
            continue
        for tex, fl, vs in me.triangles():
            pts = [xform(it.m, *v[:3]) for v in vs]
            tris.append((sum(p[1] for p in pts) / 3, [(p[0], p[2]) for p in pts], tri_colour(texs, tex, vs, cache),
                         (fl >> 4) & 15))
    # +y is up: lowest first, decals after their base at the same height.
    tris.sort(key=lambda t: (t[0] + 0.01 * t[3]))
    return tris


def image_or_svg(path):
    if path.lower().endswith(".svg"):
        return None
    try:
        from PIL import Image, ImageDraw
        return Image, ImageDraw
    except ImportError:
        return None


def draw(path, texs, meshes, insts, max_radius, size, skip=()):
    tris = gather_tris(texs, meshes, insts, max_radius, skip)
    if not tris:
        print("no static triangles to draw")
        return
    xs = [p[0] for _, pts, _, _ in tris for p in pts]
    zs = [p[1] for _, pts, _, _ in tris for p in pts]
    x0, x1, z0, z1 = min(xs), max(xs), min(zs), max(zs)
    span = max(x1 - x0, z1 - z0) or 1.0
    pad = 16
    scale = (size - 2 * pad) / span
    w = int((x1 - x0) * scale) + 2 * pad
    h = int((z1 - z0) * scale) + 2 * pad

    def to_px(p):
        return (pad + (p[0] - x0) * scale, h - pad - (p[1] - z0) * scale)

    pil = image_or_svg(path)
    if pil is None:
        if not path.lower().endswith(".svg"):
            path = path.rsplit(".", 1)[0] + ".svg"
        with open(path, "w") as f:
            f.write(f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}">\n')
            f.write(f'<rect width="{w}" height="{h}" fill="#202020"/>\n')
            for _, pts, c, _ in tris:
                q = " ".join("%.1f,%.1f" % to_px(p) for p in pts)
                f.write('<polygon points="%s" fill="#%02x%02x%02x"/>\n' % (q, *c))
            f.write("</svg>\n")
    else:
        Image, ImageDraw = pil
        img = Image.new("RGB", (w, h), (32, 32, 32))
        d = ImageDraw.Draw(img)
        for _, pts, c, _ in tris:
            d.polygon([to_px(p) for p in pts], fill=c)
        img.save(path)
    print(f"top-down view: {path} ({w}x{h}, {len(tris)} triangles, x [{x0:.0f}, {x1:.0f}] z [{z0:.0f}, {z1:.0f}])")


def contact_sheet(path, texs, count, cell=128):
    pil = image_or_svg(path)
    if pil is None:
        print("contact sheet needs PIL")
        return
    Image, ImageDraw = pil
    n = min(count, len(texs))
    cols = 8
    rows = (n + cols - 1) // cols
    img = Image.new("RGB", (cols * (cell + 4) + 4, rows * (cell + 18) + 4), (24, 24, 24))
    d = ImageDraw.Draw(img)
    for i in range(n):
        t = texs[i]
        lin = t.linear()
        im = Image.new("RGB", (t.w, t.h))
        px = []
        for y in range(t.h):
            for x in range(t.w):
                v = lin[y * t.w + x]
                if v & 0x8000:
                    px.append((((v >> 10) & 31) * 255 // 31, ((v >> 5) & 31) * 255 // 31, (v & 31) * 255 // 31))
                else:
                    px.append((200, 0, 200) if (x // 4 + y // 4) & 1 else (90, 0, 90))
        im.putdata(px)
        k = cell / max(t.w, t.h)
        im = im.resize((max(1, int(t.w * k)), max(1, int(t.h * k))), Image.NEAREST)
        cx, cy = 4 + (i % cols) * (cell + 4), 4 + (i // cols) * (cell + 18)
        img.paste(im, (cx, cy))
        d.text((cx, cy + cell + 2), f"{i} {t.w}x{t.h}", fill=(220, 220, 220))
    img.save(path)
    print(f"contact sheet: {path} ({n} textures; cut-out texels magenta checker)")


# ---------------------------------------------------------------- attachments
#
# OUT.attach: "SRATT01\0", uint32_t n, n x { uint32_t mesh (pack index), uint32_t pose_va, float local[12] }, Model 2
# column layout; world = Pose(pose_va) x local.
#
# Pose(va), fitted against the captured world matrices (residuals printed by --attach):
#   car pose block (va = 0x213b40 + slot * 0x58; slot 0 player, 1 the road-following car):
#       pos = f32 +0x00/+0x04/+0x08, pitch = +0x18, yaw = +0x1c, roll = +0x20 (radians)
#       read as it stands at the start of the tick (before that tick's pose_publish, 0x2BC08): the frame drawn in a
#       tick shows the previous tick's published pose
#   helicopter node (va = 0x500200):
#       pos = +0x18/+0x1c/+0x20, pitch = +0x24, yaw = +0x28, roll = +0x2c, read after the tick (as drawn)
#   Pose = T(pos) * Ry(-yaw) * Rx(-pitch) * Rz(-roll), column vectors, with
#       Rx(a) = [[1,0,0],[0,c,-s],[0,s,c]], Ry(a) = [[c,0,s],[0,1,0],[-s,0,c]], Rz(a) = [[c,-s,0],[s,c,0],[0,0,1]]
# Per-frame parts (their local here is the rest pose; the renderer composes the live part, all fitted exactly):
#   wheels w = 0 FL, 1 FR, 2 RL, 3 RR: local = T(x_w, pose[+0x30 + 4w] - 0.09, z_w) * Ry(-pose[+0x24]) (fronts) *
#       Rx(pose[+0x2c]) (spin); the record's T has y = -0.09 (pose value 0). Hi-LOD wheels 0x8154b9 show the spin
#       (steer was 0 while they were drawn), low-LOD 0x808596 the steer and no spin.
#   helicopter main rotor 0x82f4a3: local = T * Ry(-node[+0x60]); tail rotor 0x82f4c8: local = T * Rx(-node[+0x64])
#   the car shadow 0x815599 is projected onto the ground below the car: its record is the mean local (residual
#       ~0.09); the helicopter's shadows (0x82f4ed, the second 0x82f4a3 draw) are left out (terrain-following)

DYN_MAGIC = b"SRDYN01\0"
CAR_POSES = {"player": 0x213B40, "road car": 0x213B98}
HELI_NODE = 0x500200


def load_dyn(path):
    d = open(path, "rb").read()
    if d[:8] != DYN_MAGIC:
        raise SystemExit(f"{path}: not a capture dump")
    o, frames = 8, []
    while o < len(d):
        o += 1
        fr, rt = struct.unpack_from("<Ii", d, o)
        o += 8
        poses = [d[o + k * 0x58:o + (k + 1) * 0x58] for k in range(8)]
        o += 8 * 0x58
        nodes = [d[o + k * 0x100:o + (k + 1) * 0x100] for k in range(4)]
        o += 0x400
        n, = struct.unpack_from("<I", d, o)
        o += 4
        draws = []
        for _ in range(n):
            oba, obc, nt = struct.unpack_from("<III", d, o)
            draws.append((oba, obc, nt, struct.unpack_from("<12f", d, o + 12)))
            o += 60
        frames.append((fr, rt, poses, nodes, draws))
    return frames


def _f(b, off):
    return struct.unpack_from("<f", b, off)[0]


def _rx(a):
    c, s = math.cos(a), math.sin(a)
    return [[1, 0, 0], [0, c, -s], [0, s, c]]


def _ry(a):
    c, s = math.cos(a), math.sin(a)
    return [[c, 0, s], [0, 1, 0], [-s, 0, c]]


def _rz(a):
    c, s = math.cos(a), math.sin(a)
    return [[c, -s, 0], [s, c, 0], [0, 0, 1]]


def _mm(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(3)) for j in range(3)] for i in range(3)]


def _pose(block, pos_off, ang_off):
    R = _mm(_mm(_ry(-_f(block, ang_off + 4)), _rx(-_f(block, ang_off))), _rz(-_f(block, ang_off + 8)))
    return R, [_f(block, pos_off), _f(block, pos_off + 4), _f(block, pos_off + 8)]


def _local(R, T, w):
    """inverse(R, T) * world (column layout) -> (Rl rows, Tl)."""
    Rw = [[w[3 * c + r] for c in range(3)] for r in range(3)]
    Rt = [[R[j][i] for j in range(3)] for i in range(3)]
    Rl = _mm(Rt, Rw)
    d = [w[9] - T[0], w[10] - T[1], w[11] - T[2]]
    Tl = [sum(Rt[i][k] * d[k] for k in range(3)) for i in range(3)]
    return Rl, Tl


def _cols(Rl, Tl):
    return [Rl[0][0], Rl[1][0], Rl[2][0], Rl[0][1], Rl[1][1], Rl[2][1], Rl[0][2], Rl[1][2], Rl[2][2]] + list(Tl)


def _mean_local(samples):
    n = len(samples)
    m = [sum(s[k] for s in samples) / n for k in range(12)]
    res_r = max(abs(s[k] - m[k]) for s in samples for k in range(9))
    res_t = max(abs(s[k] - m[k]) for s in samples for k in range(9, 12))
    return m, res_r, res_t


def build_attach(pack_path, dyn_path, out_path):
    texs, meshes, insts = load(pack_path)
    index = {}
    for i, me in enumerate(meshes):
        index.setdefault(me.oba, i)
    frames = load_dyn(dyn_path)
    recs, report_lines = [], []

    def mesh_of(oba):
        if oba not in index:
            raise SystemExit(f"{pack_path}: no mesh with oba {oba:#x}")
        return index[oba]

    # --- cars: samples against the road car (slot 1), previous tick's pose block
    car = {}
    for i in range(1, len(frames)):
        blk = frames[i - 1][2][1]
        R, T = _pose(blk, 0x00, 0x18)
        draws = frames[i][4]
        wheel_lo = [w for o, _, _, w in draws if o == 0x808596]
        collapsed = len(wheel_lo) == 4 and max(math.dist(a[9:12], wheel_lo[0][9:12]) for a in wheel_lo) < 0.05
        for o, _, _, w in draws:
            if o not in (0x80E8DD, 0x815430, 0x815574, 0x814E11, 0x815599, 0x8154B9, 0x808596):
                continue
            if math.dist(w[9:12], T) > 5 or (o == 0x808596 and collapsed):
                continue
            Rl, Tl = _local(R, T, w)
            car.setdefault(o, []).append((blk, Rl, Tl))
    car_recs = []
    for oba, what in ((0x80E8DD, "body, near LOD (< ~25)"), (0x815430, "near-LOD body part"),
                      (0x815574, "near-LOD body part"), (0x814E11, "body, mid/far LOD (~25..400)"),
                      (0x815599, "shadow (ground-projected)")):
        sm = car.get(oba, [])
        if not sm:
            continue
        m, rr, rt = _mean_local([_cols(Rl, Tl) for _, Rl, Tl in sm])
        car_recs.append((oba, m, what, len(sm), rr, rt))
    for oba, what, steer, spin in ((0x8154B9, "wheel, near LOD", True, True), (0x808596, "wheel, mid LOD", True, False)):
        sm = car.get(oba, [])
        for wi, (sx, sz) in enumerate(((-1, 1), (1, 1), (-1, -1), (1, -1))):
            ws = [s for s in sm if (s[2][0] > 0) == (sx > 0) and (s[2][2] > 0) == (sz > 0)]
            if not ws:
                continue
            x = sum(s[2][0] for s in ws) / len(ws)
            z = sum(s[2][2] for s in ws) / len(ws)
            # residual of the per-frame model: T(x, susp - 0.09, z) * Ry(-steer) (front) * Rx(spin)
            rr = rt = 0.0
            for blk, Rl, Tl in ws:
                Rm = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
                if steer and sz > 0:
                    Rm = _mm(Rm, _ry(-_f(blk, 0x24)))
                if spin:
                    Rm = _mm(Rm, _rx(_f(blk, 0x2C)))
                pred_t = [x, _f(blk, 0x30 + 4 * wi) - 0.09, z]
                rr = max(rr, max(abs(Rm[a][b] - Rl[a][b]) for a in range(3) for b in range(3)))
                rt = max(rt, max(abs(pred_t[k] - Tl[k]) for k in range(3)))
            m = [1, 0, 0, 0, 1, 0, 0, 0, 1, x, -0.09, z]
            car_recs.append((oba, m, f"{what} {('FL', 'FR', 'RL', 'RR')[wi]}", len(ws), rr, rt))
    for who, va in CAR_POSES.items():
        for oba, m, what, n, rr, rt in car_recs:
            recs.append((mesh_of(oba), va, m))
            if who == "road car":
                report_lines.append("  %08x %-34s n %5d residual R %.4f T %.4f" % (oba, what, n, rr, rt))

    # --- helicopter: node 0x500200 after the tick
    heli = {}
    for F in frames:
        blk = F[3][2]
        R, T = _pose(blk, 0x18, 0x24)
        for o, _, _, w in F[4]:
            if o in (0x82F172, 0x82F56C, 0x82F4C8, 0x82F4A3, 0x82F4ED):
                Rl, Tl = _local(R, T, w)
                key = o if not (o == 0x82F4A3 and abs(Tl[1]) > 1) else "rotor shadow"
                heli.setdefault(key, []).append((blk, Rl, Tl))
    for key, what, spin in ((0x82F172, "helicopter body", None), (0x82F56C, "helicopter body part", None),
                            (0x82F4C8, "tail rotor", ("x", 0x64)), (0x82F4A3, "main rotor", ("y", 0x60))):
        # Left out: the helicopter's ground shadows (0x82f4ed, the second 0x82f4a3 draw) follow the terrain
        # height 9..13 below it (2.6 units of travel), the rotor one also spins: not a rigid attachment.
        sm = heli.get(key, [])
        if not sm:
            continue
        oba = 0x82F4A3 if key == "rotor shadow" else key
        if spin:
            t = [sum(s[2][k] for s in sm) / len(sm) for k in range(3)]
            rr = rt = 0.0
            for blk, Rl, Tl in sm:
                a = -_f(blk, spin[1])
                Rm = _rx(a) if spin[0] == "x" else _ry(a)
                rr = max(rr, max(abs(Rm[i][j] - Rl[i][j]) for i in range(3) for j in range(3)))
                rt = max(rt, max(abs(Tl[k] - t[k]) for k in range(3)))
            m = [1, 0, 0, 0, 1, 0, 0, 0, 1] + t
        else:
            m, rr, rt = _mean_local([_cols(Rl, Tl) for _, Rl, Tl in sm])
        recs.append((mesh_of(oba), HELI_NODE, m))
        report_lines.append("  %08x %-34s n %5d residual R %.4f T %.4f" % (oba, what, len(sm), rr, rt))
    with open(out_path, "wb") as f:
        f.write(b"SRATT01\0" + struct.pack("<I", len(recs)))
        for mi, va, m in recs:
            f.write(struct.pack("<II12f", mi, va, *m))
    print(f"attachments: {out_path} ({len(recs)} records; cars x {len(CAR_POSES)} poses + helicopter)")
    print("  per part (road car samples / helicopter), local fit residuals:")
    for line in report_lines:
        print(line)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("pack", nargs="*")
    ap.add_argument("--merge", metavar="OUT", help="merge the packs into OUT, then report on OUT")
    ap.add_argument("--meshes", action="store_true", help="list every mesh")
    ap.add_argument("--textures", action="store_true", help="list every texture, biggest first")
    ap.add_argument("--png", metavar="FILE", help="top-down image of the static instances (.png, or .svg)")
    ap.add_argument("--sheet", metavar="FILE", help="contact sheet of the textures (.png)")
    ap.add_argument("--sheet-count", type=int, default=64)
    ap.add_argument("--size", type=int, default=1600, help="image size in pixels (default 1600)")
    ap.add_argument("--skip", default="", metavar="OBA[,OBA...]", help="meshes (by oba) left out of the drawing")
    ap.add_argument("--attach", metavar="OUT", help="write the moving-object attachment table (needs --dyn)")
    ap.add_argument("--dyn", metavar="DUMP", help="capture dump (I960_HOST_EXPORT_DYNDUMP) for --attach")
    ap.add_argument("--max-radius", type=float, default=2000.0,
                    help="leave instances with a larger world bounding radius out of bounds and drawing")
    args = ap.parse_args()
    if not args.pack:
        ap.error("no pack given")
    path = merge(args.merge, args.pack) if args.merge else args.pack[0]
    texs, meshes, insts = load(path)
    report(path, texs, meshes, insts, args.meshes, args.textures, args.max_radius)
    if args.png:
        skip = {int(x, 0) for x in args.skip.split(",") if x.strip()}
        draw(args.png, texs, meshes, insts, args.max_radius, args.size, skip)
    if args.sheet:
        contact_sheet(args.sheet, texs, args.sheet_count)
    if args.attach:
        if not args.dyn:
            ap.error("--attach needs --dyn")
        build_attach(path, args.dyn, args.attach)


if __name__ == "__main__":
    sys.exit(main())
