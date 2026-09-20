#!/usr/bin/env python3
"""Minimal ROM extractor for the game tree (no tools/ checkout required).

Writes the host load images used by model2_rom_load_default():
  out/i960/maincpu_deinterleaved.bin
  out/i960/main_data_deinterleaved.bin

Layout matches MAME ROM_LOAD32_WORD (same semantics as segamodel2-tools
tools.rom_io.load32_word_interleave).
"""

from __future__ import annotations

import argparse
import os
import sys
from pathlib import Path

# Socket label variants seen in dumps (PCB silkscreen vs MAME names).
ROM_ALIASES: dict[str, list[str]] = {
    "mpr-17754.28": ["mpr-17754.29"],
    "mpr-17755.29": ["mpr-17755.28"],
    "epr-17890a.30": ["epr-17890.30"],
}

MAINCPU_PAIR = ("epr-17888b.12", "epr-17889b.13")
MAIN_DATA_PAIRS = (
    ("mpr-17746.10", "mpr-17747.11"),
    ("mpr-17744.8", "mpr-17745.9"),
    ("mpr-17884.6", "mpr-17885.7"),
)

GAME_ROOT = Path(__file__).resolve().parents[1]


def resolve_rom_dir(explicit: Path | None) -> Path:
    if explicit is not None:
        return explicit.expanduser().resolve()
    env = os.environ.get("SEGAMOD2_ROM_DIR")
    if env:
        return Path(env).expanduser().resolve()
    for candidate in (
        GAME_ROOT / "ROMS" / "srallyc-b",
        GAME_ROOT / "ROMS" / "srallyc-c",
    ):
        if candidate.is_dir() and any(candidate.iterdir()):
            return candidate.resolve()
    raise FileNotFoundError(
        "ROM directory not found. Place dumps under ROMS/srallyc-b/, "
        "set SEGAMOD2_ROM_DIR, or pass --rom-dir."
    )


def find_rom(rom_dir: Path, name: str) -> Path:
    direct = rom_dir / name
    if direct.is_file():
        return direct
    for alt in ROM_ALIASES.get(name, []):
        p = rom_dir / alt
        if p.is_file():
            return p
    raise FileNotFoundError(f"ROM not found: {name} (in {rom_dir})")


def load32_word_interleave(rom_dir: Path, low_name: str, high_name: str) -> bytes:
    """MAME ROM_LOAD32_WORD: low 16 bits from low_name, high 16 from high_name."""
    low = find_rom(rom_dir, low_name).read_bytes()
    high = find_rom(rom_dir, high_name).read_bytes()
    if len(low) != len(high):
        raise ValueError(
            f"ROM size mismatch: {low_name} ({len(low)}) vs {high_name} ({len(high)})"
        )
    out = bytearray(len(low) * 2)
    for i in range(0, len(low), 2):
        word_index = i // 2
        out[word_index * 4 : word_index * 4 + 2] = low[i : i + 2]
        out[word_index * 4 + 2 : word_index * 4 + 4] = high[i : i + 2]
    return bytes(out)


def load32_word_region(rom_dir: Path, pairs: tuple[tuple[str, str], ...]) -> bytes:
    return b"".join(load32_word_interleave(rom_dir, low, high) for low, high in pairs)


def main() -> int:
    ap = argparse.ArgumentParser(
        description="Extract maincpu / main_data deinterleaved bins for the host lift"
    )
    ap.add_argument("--rom-dir", type=Path, default=None)
    ap.add_argument(
        "--out-dir",
        type=Path,
        default=GAME_ROOT / "out" / "i960",
        help="Output directory (default: <game>/out/i960)",
    )
    args = ap.parse_args()

    try:
        rom_dir = resolve_rom_dir(args.rom_dir)
    except FileNotFoundError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    out_dir = args.out_dir
    if not out_dir.is_absolute():
        out_dir = (Path.cwd() / out_dir).resolve()
    out_dir.mkdir(parents=True, exist_ok=True)

    try:
        maincpu = load32_word_interleave(rom_dir, *MAINCPU_PAIR)
        main_data = load32_word_region(rom_dir, MAIN_DATA_PAIRS)
    except (FileNotFoundError, ValueError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    maincpu_path = out_dir / "maincpu_deinterleaved.bin"
    main_data_path = out_dir / "main_data_deinterleaved.bin"
    maincpu_path.write_bytes(maincpu)
    main_data_path.write_bytes(main_data)
    print(f"Wrote {maincpu_path} ({len(maincpu)} bytes)")
    print(f"Wrote {main_data_path} ({len(main_data)} bytes)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
