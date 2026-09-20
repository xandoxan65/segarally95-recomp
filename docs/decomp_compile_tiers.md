# Decomp compile tiers

## Lift (primary)

Disassembly → i960-ML IR → semantic C:

```bash
./build.sh lift --pilot
./build.sh lift --function libc_printf
```

| Output | Path |
|--------|------|
| Semantic C | `src/libc/<name>.c` |
| IR JSON | `out/lift/<name>.ir.json` |
| Pseudocode | `out/lift/<name>.lifted` |

Edit lifted C by hand. All scratch registers are `u32`; `#include "../i960_lift.h"`.

Lifted C should call lifted functions **directly** (`#include "lift_syms.h"`) where the disasm has a static `call`/`bal` to a known symbol. Use `i960_call_indirect` for function-pointer tables in workram (`symbols/staging_tables.yaml`) and `i960_call_rom` only for callees not yet ported. Do not shorten countdowns or skip original main-loop bodies on the host.

## Analyze / disasm

```bash
./build.sh analyze
./build.sh disasm
```

## ROM stitch

`make reference && make` — reference ROM fill via `maincpu.image.yaml`. Lifted semantic C is not ROM-compiled yet.

## Removed

Ghidra headless decompile, i960-elf-gcc tier-3, and the Debian container toolchain were removed. Use the lift pipeline instead.
