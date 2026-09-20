# ROM dumps (not distributed)

Place a legally obtained **MAME-compatible** *Sega Rally Championship* Revision B ROM set here:

```
ROMS/srallyc-b/
```

The host load path (`make` / `make rom-blocks`) reads program EPROMs and `main_data` from this folder via `scripts/extract_rom_blocks.py`. See [manifest.yaml](manifest.yaml) for the full file list.

## Quick setup

**From a segamod2 monorepo checkout** (ROMs already under parent `ROMS/`):

```bash
ln -sf ../../ROMS/srallyc-b ROMS/srallyc-b
```

**Standalone clone:**

Copy or symlink your MAME `srallyc-b` folder into `ROMS/srallyc-b/`.

## Override path

```bash
export SEGAMOD2_ROM_DIR=/path/to/srallyc-b
# or
./build --rom-dir /path/to/srallyc-b all
```

Revision C program EPROMs (`ROMS/srallyc-c/`) share the same data ROMs; Rev B is the default target for this decomp corpus.
