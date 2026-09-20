# Sega Rally Championship (1995) — recomp

Host-compiled uplift of Sega Rally Championship (Model 2A / i960). This repo is the **game** tree: lifted C under `src/`, portable Model 2 runtime under `lib/model2/`.

## Build

```bash
git clone <this-repo> segarally95-recomp
cd segarally95-recomp
make lift
```

Produces `build/segamod2`. **No Python is required** for the default `make lift` — it compiles the committed `src/lift_main.c` and friends with gcc/`cc` only.

### Optional tools

Python helpers (ROM extract, coverage, `gen_lift_main`, stitch) live in a separate tools repo. Clone into `./tools` if you need them:

```bash
git clone git@github.com-xandoxan65:xandoxan65/segamodel2-tools.git tools
```

Then:

- `make lift-main` — regenerate `lift_main` / host invoke glue
- `make rom-blocks` / `make compare` / `make coverage` — ROM pipeline
- `make sync-runtime` — pull canonical `tools/runtime` into `lib/model2`

### ROMs

Place board dumps under `ROMS/srallyc-b/` (see `ROMS/README.md` and `ROMS/manifest.yaml`). ROM binaries are not committed.

### Viewer deps

Track / boot viewers need **libpng** and **SDL2** (and OpenGL on the host). Without them, `make lift` still links; viewer features are compiled out or limited.

```bash
# macOS (Homebrew)
brew install libpng sdl2

make lift-viewer          # desert track
make lift-boot-viewer     # cold boot + SDL
make lift-boot-practice   # skip to desert practice START
```

## Layout

| Path | Role |
|------|------|
| `src/game`, `src/boot`, `src/libc`, `src/irq` | Uplifted game / libc |
| `src/host` | Game-side host bridges (viewer, CGM, NVRAM, …) |
| `src/lift_main.c`, `i960_host_*.c` | Committed entry + generated glue |
| `lib/model2/{geo,hw,tgp,snd,host}` | Portable Model 2 runtime |
| `lib/model2/include` | Shared host headers |
| `disasm/`, `symbols/`, `asm/` | Static RE artifacts |
| `tools/` | Optional clone of segamodel2-tools (gitignored) |

## License / ROMs

You must supply your own legally obtained ROM set. This repository does not distribute game binaries.
