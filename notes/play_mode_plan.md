# Play-mode issues — work plan

Uplifted i960 must stay **semantically equivalent to the disasm**. No host
workarounds in lifted game/TGP/GEO leaves: no invented clamps, sticky planes,
mass/impulse extras, look-at, or “recovery” springs. If host C cannot share
i960 fp aliasing, model that alias (uncleared callee frame / leftover out
words) — do not invent a parallel cache. Private host frames are ABI only
(`call` allocates a frame). i960 invalid-op (leave dest) is hardware, not a
workaround.

Sources: `decomp/disasm/maincpu/`, `python -m tools.i960_decode`. No Ghidra,
no MAME runtime capture.

---

## Order

**1. Road exit / chase sky-bounce (now)**  
Car and chase share the course table. Chase is a second body
(`scene_prep` @ `0x34380` → `scene_frame` @ `0x34F40`). Attract camera is a
different stack — do not apply mode-7 notes here.

Split one scrape in logs: car `node+0x14/18/1c`, `+0xa8`, force_slot flag/height;
chase follow XYZ / `0x2138d0` flags. Then match disasm, do not add clamps.

Disasm leaves:

| Site | ROM | Host invention to remove |
|------|-----|--------------------------|
| `force_slot` @ `0x30140` | `lda 0x40(fp)` → `table_index_b`; miss stores only `out+0x1c=15`; leftover plane/height; `chkbit 7` or height`<0` → zero springs | Per-slot sticky + `h'=h−N·ΔP`; treat flag 15 as apply=0 |
| `index_refresh` @ `0x30320` | pack; `cmpible 0,g4` keep if `packed>=0` (cell 0 is a hit); miss → restore index, `xz_clamp` @ `0x30380` (callee fp scratch), pack again; still miss → snap `+0x90/94/98` | `packed<=0` miss gate; alias clamp onto `predicted[0]` |
| `contact_solve` @ `0x2F8A0` | `scale=(0.5*mass*iy)/denom`; pending `+= F/mass`; no same-frame hard-sep | (mass already restored; do not re-add vel-kill) |
| Drive `a8` @ `0x30D44` | lerp only | `motion_clamp_ox` / `lim=4` fallback |
| Cold tail @ `0x30D78` | `cmpr \|x\|` vs `*0x20cae0` | poison cap, substitute 4 |
| `scene_prep` @ `0x346FC` | miss leaves `0x2138d0+0x0c..0x18` | cam sticky rewrite |

TGP: `0x51` orients span basis; bare `0x2f` is normalize-only.

**2. Spectators / roadside**  
Course-list kinds 4/21 (`draw_4` @ `0x2B420`, TGP `0x5d`). Kind 21 skips when
`0x202230!=0` (`--practice`) — ROM. If kind 4 missing: list seed vs `0x5d`
(view-space T, keep view R, restore matrix). Do not disable the practice gate.

**3. Steering feel**  
Host I/O only first (poll before `game_io_poll`; stop slamming `0x20`/`0xe0`).
No invented deadzone (`0x80` rest). Then re-check vs ROM `pitch_smooth` / `a8`.

**4. Auto gear**  
No lifted AT writer. Pin `214354` (AT vs MT) and host shifter vs `0x202044`
(also camera view). Unbind Q/E from view for AT playtests.

**5. Celica roof yellow / flash**  
Solid colorbase / luma / fillmap — last; does not block 1–4.

Do not “fix” spectators by dropping the practice gate, or steering with a
deadzone, or sky-bounce with look-at.
