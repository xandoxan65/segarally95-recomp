# TGP firmware command table (Sega Rally, MB86234)

Read from the firmware itself: the 0x681 words the i960 uploads from ROM
`0x5AE94` (count at `0x5C898`), disassembled with MAME's `mb86233d`. Rev B
(`srallycb`) and Rev C carry the same code: all 5,862 words under
`disasm/` and `asm/` (0x420–0x5DBE0) match Rev C byte for byte, so the Rev C
addresses in the Sega Ages notes below apply here unchanged.

Command word is `(op<<23) | (op<<8) | op` — the opcode is in it three times
(7 bits each), e.g. `0x2c005858` = op `0x58`. Dispatch: loop at `0x77`
(`0x70` for the first word), table at `0xAF + op`. "In"/"out" count FIFO
words. Handler PCs are given where they matter.

## Table ROMs

`copro_tgp_tables` (opr-14742a/14743a) holds **four** 16K-word tables, not
three. MAME's TGP ports:

| Port | Table | Base |
|------|-------|------|
| `$0x20` | sin/cos | `0x0000` |
| `$0x24` | **atan** | `0x4000` |
| `$0x28` | 1/x | `0x8000` |
| `$0x2a` | 1/sqrt | `0xC000` |

Float `sin`/`cos`/`tan` (ops 15–17, 29–2b) and `sqrt` (11, 58) do **not**
use the tables: they are computed in TGP code (Taylor polynomial, Newton).
Only the halfword-angle ops (32–36), atan2 (3e), 1/x (0c) and the normalise
paths (2e, 2f, 50, 55) read the table ROMs.

## Ops

VF2 column: the nearest function in the Sega Ages 2500 VF2 COP library
(`Fn_*` names). The two libraries share little beyond the arithmetic and the
matrix stack; the opcode numbers differ (VF2 has 112 ops, table at `0x3F`).

| Op | Handler | In → Out | Does | VF2 |
|----|---------|----------|------|-----|
| 00 | | 0 → 0 | nothing | |
| 01 | | 1 → 0 | `$7c = in` (`$200`) | |
| 02 | | 0 → 1 | return `$7c` | |
| 03 | | 1 → 0 | `$65 = in`; list cursor `$7d` = bufferram `0x6000` (i960 `0x918000`); clear flag bit 2 | |
| 04 | | 0 → 1 | return `$65` | |
| 05 | `0x14E` | 0 → 0 | pop a destination from the bufferram list, write `0xB` + current 12-float matrix there (GEO 0x0B) | |
| 06 | | 1 → 1 | read TGP RAM | |
| 07 | `0x16D` | 0 → 1 | pop a word from the bufferram list, return `copro_data[word]` (no FIFO args) | |
| 08 | | 2 → 1 | add | `Fn_add` (13) |
| 09 | | 2 → 1 | sub | 14 |
| 0a | | 2 → 1 | mul | 15 |
| 0b | | 2 → 1 | div (`fdvd`) | `Fn_div` (16) |
| 0c | | 2 → 1 | fast div: 1/x table + refinement | |
| 0d | | 1 → 1 | fabs | |
| 0e | | 1 → 1 | negate | |
| 0f, 10, 12 | | | no-op | |
| **11** | `0x1A7` | 1 → 1 | **sqrt(\|x\|)**, Newton (helper `0x1AB`: seed 0.41731 + 0.59017·m from `$23a/$23b`, halve exponent, ×1/√2 from `$23c`) | `Fn_sqr` (1a) |
| 13 | | 1 → 1 | int → float (`cxfd`) | `Fn_cvtws` (17) |
| 14 | | 1 → 1 | float → int (`cfxd`) | `Fn_cvtsw` (18) |
| 15 / 16 / 17 | | 1 → 1 | sin / cos / tan of radians, Taylor (coefs `$220–$225`, π/2 at `$3b`) | sin 21, cos 22 (VF2 uses the table) |
| 20 | | 0 → 0 | push matrix (depth `$9`, limit `$a`) | push (01) |
| 21 | | 0 → 0 | pop | pop (02) |
| 22 | | 12 → 0 | load matrix | load (04) |
| 23 | | 0 → 0 | reset the stack | |
| **24** | `0x286` | 12 → 0 | **current = current × in** (multiply, not load) | |
| 25 | | 0 → 0 | identity | base |
| 26 | | 0 → 12 | get matrix | get matrix (05) |
| 27 | | 3 → 0 | translate | translate |
| 28 | | 3 → 0 | scale | scale |
| 29 / 2a / 2b | | 1 → 0 | rotate X / Y / Z, radians (polynomial sin/cos) | x/y/z rot (08–0a) |
| 2c | | 3 → 3 | transform point | |
| 2d | | 0 → 1 | stack depth | |
| 2e | | 9 → 3 | triangle normal, normalised (1/sqrt table) | |
| 2f | | 3 → 3 | normalise (1/sqrt table) | |
| 32 / 33 / 34 | | 1 → 0 | rotate X / Y / Z by halfword angle (`0x8000` = π), sin/cos table | |
| 35 / 36 | | 1 → 1 | table sin / cos of an integer angle | |
| 3e | | 2 → 1 | `atan2(in1, in0)` via the atan table, sign-extended 16-bit angle | |
| 40 / 41 | | 1 → 0 | vector slot = 0 / 1.0 | |
| 42 / 43 | | | slot store / read | |
| 44–47 | | | slot add / sub / mul / div (slot ÷ in) | |
| 48 | | | transform slot in place | |
| 50 | | 6 → 1 | \|B − A\|, inputs A.xyz then B.xyz (1/sqrt table) | |
| 51 | `0x48E` | 3 → 0 | look along a vector (yaw then pitch; helpers `0x4E3` Y, `0x4F8` X) | |
| 52 | `0x58A` | 9 → 0 | fill a slot from `copro_data` (see firmware README) | |
| 53 | `0x63C` | 1 → 17 | read the slot back | |
| 54 | | 3 → 0 | look-along variant; stores no direction | |
| **55** | `0x66E` | 10 → 0 | **push; translate(in0–2); face the eye about Y (in3–6); scale(in7–9); emit GEO 0x0B; pop** | |
| 56 / 57 | | 2 → 1 | sin(a)·b / cos(a)·b | |
| **58** | `0x51C` | 6 → 1 | **sqrt Σ(b−a)², inputs paired (a0,b0,a1,b1,a2,b2)**, Newton sqrt | |
| 59 | | 6 → 1 | dot product of three pairs | |
| 5a | | 6 → 1 | cosine of the angle between two vectors | |
| 5b | | 6 → 3 | cross product (odd input order) | |
| 5c | | | 2D side test | |
| **5d** | | 7 → 0 | **as 55 without the scale** | |
| 5e | | | translate + face the eye only | |
| 5f | | 5 → 0 | push; translate; halfword Z rot; halfword X rot; emit; pop | |
| 60 / 61 | `0x3DB` / `0x3E9` | 0 → 0 | store / load the matrix at `0x7f00 + selector` in bufferram (selector popped from the list) | |

Face-the-eye (helper `0x652`, used by 55/5d/5e): `dx = in3 − in4`,
`dz = in5 − in6`, normalise through the 1/sqrt table, rotate about Y.
With the i960 payload `ox, oy, oz, eye.x, ox, eye.z, oz, …` that is the
eye-to-object direction on XZ: a Y billboard.

## Where `model2_tgp.c` differs from the firmware

From the disassembly only; none of these has been checked against a trace
yet, and a call site may already compensate.

| Op | `model2_tgp.c` | Firmware |
|----|----------------|----------|
| 11 | returns `1/x` | `sqrt(\|x\|)`. Callers multiply the result (`scene_frame` 0x366xx, `hud_speed` 0x1DFC0), so the value shown should change |
| 24 | treated as a load ("alternate marker") | multiplies the current matrix by the 12 inputs |
| 55 | view-space translate + `diag(scale)`, no rotation | translate, Y billboard toward the eye, scale |
| 5d | | same face-the-eye rotation, no scale |
| 54 | stores a direction (`g_copro_vec`) | stores none |
| 58 | reads A.xyz, B.xyz like 0x50 | handler pairs the inputs (a0,b0,…). **Open:** the `geo_view_scene_frame` sites send `0,0,0,x,y,z`, which under the pairing gives `sqrt(x² + (z−y)²)`, not \|(x,y,z)\|. Worth a trace before changing anything |
| 60/61 | TGP RAM `$7 + a` | bufferram `0x7f00 + selector` (needs a second look) |

Ops 3e (atan2 argument order), 52, 53 and 59 match.

## Firmware README

- "validates the duplicated opcode byte": the byte is there three times.
- Related ROMs: `copro_tgp_tables` is sin-cos / **atan** / 1/x / 1/sqrt.
