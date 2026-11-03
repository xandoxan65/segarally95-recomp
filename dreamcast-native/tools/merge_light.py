#!/usr/bin/env python3
"""Merge the course export's per-drive light files (src/host/lift_export_course.c write_light_file: the cars'
per-triangle lighting terms and brightness tables) into one, as course_pack_info.py --merge merges the packs: the
first drive's record of each object wins (the same drive order as the pack merge keeps its meshes), tables are
renumbered.

  merge_light.py OUT.light IN.light [IN.light ...]   (missing inputs are skipped)
"""
import os
import struct
import sys

MAGIC = b"SRLIT01\0"


def load(path):
    d = open(path, "rb").read()
    if d[:8] != MAGIC:
        raise SystemExit(f"{path}: not a light file")
    n_tab, = struct.unpack_from("<I", d, 8)
    o = 12
    tables = [struct.unpack_from("<17f", d, o + 68 * i) for i in range(n_tab)]
    o += 68 * n_tab
    n_mesh, = struct.unpack_from("<I", d, o)
    o += 4
    meshes = []
    for _ in range(n_mesh):
        oba, n = struct.unpack_from("<II", d, o)
        o += 8
        tris = [struct.unpack_from("<3f2BH", d, o + 16 * t) for t in range(n)]
        o += 16 * n
        meshes.append((oba, tris))
    return tables, meshes


def main():
    out, ins = sys.argv[1], [p for p in sys.argv[2:] if os.path.exists(p)]
    tables, index, meshes, seen = [], {}, [], set()
    for path in ins:
        tabs, ms = load(path)
        for oba, tris in ms:
            if oba in seen:
                continue
            seen.add(oba)
            new = []
            for amb, dif, ss, sc, mode, tab in tris:
                key = tabs[tab] if tab < len(tabs) else tuple([1.0] * 17)
                if key not in index:
                    index[key] = len(tables)
                    tables.append(key)
                new.append((amb, dif, ss, sc, mode, index[key]))
            meshes.append((oba, new))
    with open(out, "wb") as f:
        f.write(MAGIC + struct.pack("<I", len(tables)))
        for t in tables:
            f.write(struct.pack("<17f", *t))
        f.write(struct.pack("<I", len(meshes)))
        for oba, tris in meshes:
            f.write(struct.pack("<II", oba, len(tris)))
            for t in tris:
                f.write(struct.pack("<3f2BH", *t))
    print(f"merge_light: {out}: {len(meshes)} meshes, {len(tables)} tables", file=sys.stderr)


if __name__ == "__main__":
    main()
