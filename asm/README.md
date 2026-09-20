# Approved maincpu assembly slices

Tier-1 verified MAME disasm slices used by `make` / `maincpu.image.yaml`. Each file
must pass `./build reasm --slice asm/...`.

| Path | ROM range | Notes |
|------|-----------|-------|
| `libc/maincpu_05cdc8_03c.asm` | `0x05cdc8` + `0x3c` | `libc_strcpy` (trimmed oracle) |
| `libc/maincpu_05daa0_140.asm` | `0x05daa0` + `0x140` | `libc_memcpy` |
| `boot/maincpu_000420_150.asm` | `0x000420` + `0x150` | `maincpu_reset_entry` (IP @ ROM `0x0C`) |

MAME listings for RE live under `disasm/maincpu/`; copy slices here once tier-1 reasm passes.
