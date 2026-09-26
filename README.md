# Sega Rally Championship (1995) — recomp

Host-compiled uplift of Sega Rally Championship (Model 2A / i960). This repo is the **game** tree: lifted C under `src/`, portable Model 2 runtime under `lib/model2/`.

## Build

Prerequisites: CMake 3.16+ and a C compiler. Viewer extras: **SDL2**, **libpng**, and OpenGL. Without SDL2/libpng the tree still links; viewer / PNG / GL features are compiled out.

```bash
cmake -B build
cmake --build build
```

That produces `build/segamod2` (`build/segamod2.exe` on Windows). On macOS/Linux, `make lift` still works: it configures CMake if needed, then builds the `lift` target.

```bash
cmake --build build --target lift          # compile + link
cmake --build build --target lift-check    # same (compile only)
cmake --build build --target lift-boot-viewer     # cold boot + SDL
cmake --build build --target lift-boot-practice   # skip to desert practice START
```

Running the binary with no arguments starts the SDL cold-boot viewer. Pass `--harness` for the short dispatch trace.

On first run (or if `out/i960/*.bin` are missing / wrong size), the host auto-extracts `maincpu_deinterleaved.bin` and `main_data_deinterleaved.bin` from board dumps under `ROMS/srallyc-b/` (or `SEGAMOD2_ROM_DIR`). No separate extract step is required. `scripts/extract_rom_blocks.py` / `cmake --build build --target rom-blocks` remain available if you want the bins without launching the game.

### macOS

```bash
brew install cmake libpng sdl2
cmake -B build && cmake --build build
```

### Windows

Use **Visual Studio 2022** (MSVC) with CMake. For the live viewer, install SDL2 and libpng via **vcpkg** (recommended):

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg install sdl2:x64-windows libpng:x64-windows
```

Configure and build with the vcpkg toolchain:

```powershell
cd path\to\segarally95-recomp
cmake -B build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=C:\vcpkg\scripts\buildsystems\vcpkg.cmake
cmake --build build --config RelWithDebInfo --target lift-check
```

Configure should report `SDL2=ON PNG=TRUE GL=ON`. Re-run `cmake -B build …` after installing packages if an earlier configure already ran without them.

**MSYS2 MinGW** (alternative toolchain — use a MinGW64 shell, not MSVC):

```bash
pacman -S mingw-w64-x86_64-cmake mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-SDL2 mingw-w64-x86_64-libpng
cmake -B build && cmake --build build
```

**Manual SDL2 SDK:** download the Visual C++ development libraries from [libsdl.org](https://github.com/libsdl-org/SDL/releases), then pass `-DSDL2_DIR=C:\path\to\SDL2\cmake`. You still need libpng separately for PNG dumps; OpenGL comes from Windows (`opengl32`).

### Optional tools

Coverage, `gen_lift_main`, stitch, and other RE helpers live in a separate tools repo. Clone into `./tools` only when you need those:

```bash
git clone git@github.com-xandoxan65:xandoxan65/segamodel2-tools.git tools
```

Then:

- `make lift-main` — regenerate `lift_main` / host invoke glue
- `make compare` / `make coverage` — ROM pipeline beyond extract
- `make sync-runtime` — pull canonical `tools/runtime` into `lib/model2`

### ROMs

Place board dumps under `ROMS/srallyc-b/` (see `ROMS/README.md` and `ROMS/manifest.yaml`). ROM binaries are not committed.

## Layout

| Path | Role |
|------|------|
| `src/game`, `src/boot`, `src/libc`, `src/irq` | Uplifted game / libc |
| `src/host` | Game-side host bridges (viewer, CGM, NVRAM, …) |
| `src/lift_main.c`, `i960_host_*.c` | Committed entry + generated glue |
| `lib/model2/{geo,hw,tgp,snd,host}` | Portable Model 2 runtime |
| `lib/model2/include` | Shared host headers |
| `disasm/`, `symbols/`, `asm/` | Static RE artifacts |
| `scripts/extract_rom_blocks.py` | Minimal host ROM extract (no tools/) |
| `lib/host_compat/` | Windows pthread/time/GL loader (no-op on POSIX) |
| `tools/` | Optional clone of segamodel2-tools (gitignored) |

## License / ROMs

You must supply your own legally obtained ROM set. This repository does not distribute game binaries.
