---
description: No MAME runtime captures — validate from disasm, ROM, and static RE only
alwaysApply: true
---

# No MAME runtime capture

This project **cannot** use MAME (or any emulator) **runtime** memory dumps, scripted captures, or live hardware state as validation or discovery input. Do not propose, implement, or block on them.

## Forbidden

- Running or extending MAME Lua capture scripts to read workram, palram, colorxlat, lumaram, or compiled thunks at course load
- Treating MAME captures, “golden dumps”, or `out/palette/*.bin` from emulator sessions as ground truth
- Suggesting “capture `0x005FBF10` at desert load”, “workram snapshot”, or “MAME golden match” as a next step or acceptance criterion
- Adding CI, tests, or docs that **require** emulator execution to pass
- Using `decomp/notes/palette_test_vectors.json` `replay_slots_*` (Python replay snapshots) as independent hardware oracles — they are regression snapshots only

## Required instead

Derive behavior and validate from **static** sources:

1. **i960 maincpu disassembly** (`decomp/disasm/maincpu/`, `python -m tools.i960_decode`)
2. **ROM / main_data** blobs and typed tables (CGM blocks, DEF colorxlat @ `0x33454`, format jump table @ `0x05CFD0`)
3. **Disasm-backed simulators** that replay format semantics directly (`tools/model2_cgm_staging.py`, `tools/model2_cgm_1111.py`) — document RE gaps when thunks @ `0x005C9118` are not executed
4. **Tier-B test vectors** built from ROM: `(slot, bin_pos, raw_u16, disasm-proven mask) → color15` — not from emulator or from replay output alone
5. **Visual / structural checks** on exported assets (atlas verify, placement-stream colorbase demand, `palette_re` static replay) — wrong hues implicate decode logic, not missing MAME dumps

## When a value exists only in runtime workram

State the gap explicitly and proceed with disasm trace, format-table semantics, or ROM-side inference. Do **not** list emulator capture as the resolution path.

```python
# BAD — blocked on hardware
# "Next: capture workram at desert CGM load to read 0x005FBF10 row 10"

# GOOD — static RE continuation
# "Template @ 0x005FBF10 is zero in static ROM; infer D stub from 0x05D860 emit +
#  0x029C10 upload paths; compare decode modes via palette_d_path_compare"
```
