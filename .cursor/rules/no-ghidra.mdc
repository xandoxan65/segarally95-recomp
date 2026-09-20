---
description: Forbid Ghidra tooling and Ghidra-derived output for RE / lift work
alwaysApply: true
---

# No Ghidra tooling or output

Do **not** use Ghidra (GUI, headless, scripts, exporters, or `tooling/ghidra_*`) for discovery, validation, lift, or docs in this project.

## Forbidden

- Running or invoking Ghidra / Ghidra scripts
- Reading or citing Ghidra project DBs, exported HTML/XML/JSON, or decompiler C as ground truth
- Wiring lift, xrefs, or “next steps” to Ghidra artifacts
- Suggesting “open in Ghidra” / “Ghidra decompile” as a resolution path

## Required instead

Derive behavior from:

1. **MAME i960 disassembly** — `python -m tools.disasm.mame_dasm`, `decomp/disasm/maincpu/`, `python -m tools.i960_decode`
2. **ROM / main_data** typed tables and xref scans (`tools/i960_scan.py`, `tools/i960_xrefs.py`, …)
3. **Uplifted C** corrected to match that disasm (see `decomp/Rules.txt`)

If a path under `tooling/ghidra*` exists in the tree, treat it as legacy — do not extend or depend on it.
