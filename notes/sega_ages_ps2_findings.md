# Sega Ages 2500 Vol. 25 (PS2) findings

*Sega Rally 2006* (SLPM-62703) carries a C port of the arcade game. These
notes come from reading that port alongside the arcade code. Its addresses
are Rev C ROM addresses; Rev B code is identical (see
`tgp_firmware_commands.md`), so they apply to `srallycb` as they are.

## Controls

| Work RAM | Input | Written from |
|----------|-------|--------------|
| `0x202050` | HANDLE (wheel) | `0x01C0001E` read at ROM `0x26C0` |
| `0x202051` | ACCEL | same |
| `0x202052` | BRAKE | same |

- **`game_start_mode_select_frame` (0x13CE0)** reads `0x202051` and calls
  it `handle`; it is the **accelerator**, so the `0xAF/0xB0` thresholds are
  throttle presses, not steering.
- **`scene_lookup_fn` (0x14788)** is the menu stepper (the port has it at
  `0x14780`): it reads `0x202050` against the range tables at
  `0x5B375C` (ROM `0x146E0–0x14770`). Wheel `> 0x9F` steps right,
  `< 0x61` steps left.
- Race steering, ROM `0x28FB8`: `x = (h − 0x80) / 128`, dead zone 0.1, then
  × 0.1. Not lifted under that address yet.
- Gear scale at ROM `0x198C0` (not lifted yet).
- Drive board test: writes 7 and expects 7 back.

## Course data and TGP

- `cpdata.bin` in the port is the TGP data ROM (`copro_data`), checked with
  a CRC-16.
- The ground query (TGP 0x52/0x53) is a point-in-polygon walk: try the
  current segment, then the next, then the previous. Polygons are 16 words;
  corners at words 0/2, 3/5, 6/8, 9/11; attr bit 2 means a quad. This
  agrees with the firmware README's `0x52` section.
- The GEO list ends with `0x07800000`; the buffer is 128 KB.
