---
description: Require i960 static RE for geometry; forbid ROM signature/span heuristics
alwaysApply: true
---

# Geometry discovery: i960 RE, not ROM signatures

## Required approach

Derive **catalog indices**, **draw order**, **transforms**, and **data addresses** from:

1. **i960 maincpu disassembly** (`tools/disasm/`, `decomp/disasm/`, `python -m tools.i960_decode`)
2. **ROM xref / immediate scans** tied to known symbols (`tools/i960_scan.py`, `tools/i960_geo_feed.py`, `tools/i960_xrefs.py`)
3. **Typed ROM tables** (e.g. vehicle descriptors `0x43830`, draw lists `0x34E40`/`0x34E88`)
4. **main_data catalog** (`0x2864B40`) and **confirmed streams** (placement `@ 0x02867C20`)
5. **Descriptor / part-record blobs** in main_data (`tools/model2_vehicle_transforms.py`)

Do **not** use MAME or emulator runtime captures — see rule `no-mame-runtime-capture.mdc`.

## Forbidden for discovery or export logic

Do **not** add or extend logic that infers geometry by:

- Scanning polygon ROM for “likely meshes” (vertex count, bounding span, peak coordinate)
- Fingerprinting raw ROM byte patterns to guess catalog indices or part roles
- Walking `walk_polygon_rom` / open-ended ROM sweeps to **find** vehicles, tracks, or assemblies
- `discover_placements` / placement-group heuristics as a **primary** source (legacy only)
- Span thresholds alone to classify or select body shells, wheels, or trim for export

```python
# BAD — discovery from mesh shape
for index in range(782):
    if verts >= 1000 and span < 10:
        body_indices.append(index)

# GOOD — indices from RE-backed tables, mesh stats only for reporting
indices = parse_vehicle_catalog_tables(rom_dir)["race_draw_list"]
for index in indices:
    report.append({**catalog_mesh_report_row(index), "source": "re_catalog_index"})
```

## Allowed uses of mesh stats

- **Validation / reporting** on indices already linked by RE (`tools/i960_vehicles.catalog_mesh_report`)
- **Viewer metadata** (span, vertex count) on exported OBJs
- **Sanity checks** after parse — not to choose what to export

## Canonical pipelines (extend these, don’t bypass)

| Asset | Source of truth |
|-------|-----------------|
| Track placements | `parse_placement_stream` @ `0x02867C20`, geo feeder `0x023CC8` |
| Vehicles | ROM descriptors + `copy_catalog` / `draw_catalog` / `draw_car_primary` chain |
| Transforms | Descriptor 20-word records + workram `0x5E3E00` (not raw 12-float at word 0 alone) |

## Legacy code — do not extend

- `tools/model2_placements.discover_placements` — scenes legacy scan only
- `tools/extract/scenes.py` “Heuristic placement scan” block
- `tools/model2_geo.walk_polygon_rom` — debug / bulk mesh export, not vehicle/track RE

When removing ambiguity, prefer disassembling the calling function and wiring a new xref over adding another ROM heuristic.
