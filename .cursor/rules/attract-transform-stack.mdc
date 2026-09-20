---
description: Attract/3D transform stack — uplifted camera + MAME as reference, not gold standard
alwaysApply: true
---

# Attract / 3D transform stack

## Goal

Original-or-better fidelity for attract (and later race) 3D. **Not** “same as MAME including its limitations.” Decomp exists so lifted game semantics can exceed a broken or incomplete emulator.

## Sources of truth (priority)

1. **Uplifted i960** — camera composition (`comm_attract_geo_script_finish` modes, TGP markers, `copy_catalog` pose).
2. **MAME Model 2 sources as reference only** — operator contracts for GEO (`0x0B` matrix, `0x09` focal, `0x03` window/centers, project formula). Never acceptance criteria.
3. **Decoded point geometry** — catalog/PRG meshes and the Python track viewer for structural checks.

Do **not** use MAME runtime captures (see `no-mame-runtime-capture.mdc`).

## Closed transform stack

| Stage | Source |
|-------|--------|
| View | Uplifted attract/TGP composition → current matrix |
| Upload | TGP `0x05` → GEO `0x0B` + 12 floats (`copy_catalog` `+0x34`) |
| Object pose | `copy_catalog` push / translate / rotate / draw / pop |
| Project | GEO `0x09` + `0x03`; MAME `model2_3d_project` as formula reference |

Apply **only** transforms the lifted code and GEO commands issue. No invented FOV, look-at, or mesh auto-frame.

## Viewer role

OpenGL (or any host display) is a **transform debugger**, not a second renderer. Prefer:

- Modelview: identity when verts are already view-space (post-`0x0B` object draws)
- Projection: latched focal + window only

Defer a full rasterizer port until this stack is trusted.

## Current work order

1. Camera modes from disasm (`script_finish` jump table @ `0x11C20`), starting with modes 7 and 4
   — `script_finish` locals (`0x40(fp)`…`0x88(fp)`) must be a **private host frame**
     (stack buffer), not `fp = sp`. Attract `bind_fp` uses a shadow buffer while
     `script_frame_setup` was leaking `sp += 0x30`; tying locals to `sp` segfaulted.
     `ld (g0)` still reads the caller’s string_draw object; scene stays in the
     private frame.
   — `geo_attract_copro_vec_scale` is a real `call` (`0x123B0`→`0x11A80`): give it a
     **private host frame**. Unit temps at callee `0x40(fp)` must not share the
     caller's scene; `a2` (pre-call `lda 0x40(fp)`) keeps the scene pointer.
     Shared global `fp` was a lift bug (wrong eye.xy; eye.z overwritten by unit.z).
   — **Modes 4 never rotate R in script_finish.** Mode 7/8 `vec_scale` issues
     TGP `0x54` then `0x2f` then `0x27` with **no** following `0x25`. Race
     `geo_view_scene_frame` does `0x2f` then explicit `0x25` (normalize
     readback only). Host `0x2f` is **normalize-only**; look-along R comes from
     `0x54` (mode 7 keeps that R). Bare race `0x2f` (table_index_b, force_slot)
     must not orient — that invented side effect spun chase after road hits.
   — Mode 4: `0x54(scene)` then `T = −(obj−scene)`. `0x54` orients R along
     scene so cars at cam≈scene sit on +Z (close-up). Focus `@ 0x20220c` stores
     the delta (pen yaw), not a look target.
   — Mode 7: dolly `eye = scene − scale·normalize(obj−scene)` with
     `scale = radius − |delta|·falloff` from float pair at `lda 0x5acbe0[idx]`.
     `0x54(delta)` look-along, `0x2f` unit readback, then `0x27`.
   — Modes 2/8/9/11 read the **pair** object (`g1` / `pair_obj` from
     `0xa0(fp)`), not `g0`. Mode 7/1/4/5/10 use `g0`. Banner calls pass
     `g1=0` — those modes must not run (or no-op). Mode 7/8 share
     `vec_scale` @ `0x11A80` with private callee frame + restored `sp`.
   — Modes 5/6: `geo_attract_fp_series` @ `0x33C00` is cubic **B-spline**
     (N_i,3), not Bezier. At `t=0` cam ≠ table P0; for mode-6 table
     `@ 0x5ae400` smoke is `≈(-114, 38, -553)`. Load table via `i960_ld`
     (ROM mirror); `stl`/`ldl` at `0x50(fp)` are full g8:g9 pairs; private
     callee frame. Mode 6: `t=link/412`, endpoint immediates, `0x54(end−cam)`
     then `0x27(−cam)`. Mode 5: indexed base `0x5ae3d0+…`.
   — TGP `0x54`: store direction **and** set R to look along the unit (Y-up).
     `0x2f`: normalize readback only. Race `geo_view_scene_frame` clears with
     `0x25` after `0x2f` when it only wants the unit. Attract modes that
     translate after `0x54` without `0x25` keep that R — not an invented look-at.
   — Race `road_span_basis` @ `0x31320`: nested `0x25`/`0x27(bx,0,cx)` then
     TGP `0x51(bz,0,by)` orients R (fw `@ 0x48E`) before `0x2c(ox)` →
     `node+0x90/98` world XZ. Host must HLE `0x51` (look-along family as
     `0x54`); no-op left local snaps that pitch-wind chase. `0x60` stores
     matrix only (no R effect).
2. Keep TGP HLE tied to uplifted call sites
3. Validate mesh against Python track viewer; if points match and camera differs, fix uplift/TGP/GEO — not the rasterizer
4. Attract inner loop: 0→9 lifted (`inner_8` @ `0xF7D0` resets `0x20209c`;
   `inner_9` @ `0x13810` → `comm_attract_board_tick` @ `0x16540`, flags/scene
   tick — not on cam path). After `vec_scale` private-frame fix: re-check mode
   7/8 framing; if still off, audit object pose / GEO project, not invented look-at.
   — `geo_draw_frame_entry` @ `0x4E90`: GEO `0x09` is `stl (450,450)` (both lanes);
     attract path uses `geo_fifo_bootstrap` focal **350** directly — correct for
     first shot. Feeder early path must `ldq`/`stq` catalog (word3 → `0x20b940`).
   — First desert shot is mode 7: `0x54` orients R along `normalize(obj−scene)`,
     `0x2f` returns the unit, then `0x27` dolly. Smoke: `obj_cam≈(0,0,35)`,
     `bearing_xz≈0`, `R=non-I`.
   — Closed dig revision (2026-07-18 / 2026-08-08): mode-7 i960 has no
     `0x29/0x2a/0x2b`; orientation is `0x54` (not bare `0x2f`). Race sites that
     only want normalize use bare `0x2f` (± later `0x25`). Scene/obj/scale
     inputs remain ROM-matched.
   — TGP `0x55` (`0x2a805555`, `copro_submit`): emits GEO `0x0B` from the
     10-float FIFO payload (diag scale + T from obj/eye words). eye.y is not in
     the FIFO; use workram `0x202210` (same `script_finish` eye triplet as
     FIFO `eye.xz`). Does **not** run on the first mode-7 shot (queue empty
     until later frames). Must restore the TGP working matrix after emit
     (course `0x05` must push the view, not the car). Do not invent extra
     view-R multiplies — TGP microcode for `0x55` is not in the i960 stream.
   — TGP `0x5d` (`0x2e805d5d`, course-obj `draw_4`): emit GEO `0x0B` with
     view-space T of obj.xyz, **keep current view R**, restore matrix. FIFO
     eye.xz is the side-bit packing, not a look-at. Invented look-along threw
     roadside catalogs off-screen.
   — TGP `0x2a` yaw HLE: sin signs are opposite textbook RH `Ry` so model +Z
     follows keyframe travel (desert yaw ≈ +125° ↔ heading ≈ −126°). `glyph_emit`
     feeds raw `obj+0x1c` (no `notbit`); flipping HLE — not object angles — is
     the disasm-consistent fix for “reversed car” faces.
   — TGP `0x29` pitch HLE: same opposite-textbook sin signs as yaw. Raw
     `obj+0x18` on object paths; textbook Rx pitched bodies into the road.
   — Disasm fixes (2026-07-18): `course_init` `lda`→load bugs (`*3` index,
     `0x215b5c` stack EA, printf format EAs); carousel negative path `span_b=-1`;
     inner_2/4/6 tex pick uses unsigned `subo 1` / `cmpobl 6`.
   — `copro_submit` @ `0x31FC0`: `lda 0x20d600[r7*4]` is the slot *address*;
     `ld (r6)` yields the object index. Lift had `ld` for the lda (then a second
     `ld` through the index as a pointer). Also keep r6/r8 dual-cursor compact.
   — `slot_call` @ `0x3B200`: centroid is ΣX(40/4c/58/64) and ΣZ(48/54/60/6c)
     /2. 0x2e burst 2 fifth word is **g7=v1.x `[4c]`** (still live from the
     pre-burst `ld 0x4c`), not `[5c]` — wrong lane selected wrong road span
     vs car poses. Bursts 3/4: `[58]`/`[48]` as last payload words.
   — `object_extra` @ `0x317B0`: each successful bit does `0x20` push then
     `0x21` pop @ `0x31F74` before the next bit; final `0x21` @ `0x31F9C`
     restores the camera view. Missing per-bit pop left car-local matrices
     on the TGP stack so course `0x05` emitted a non-view matrix (road holes
     under cars / show-through).
   — Wheel mesh draw is `glyph_emit` @ `0x23F18`: four iterations (`g13` 0..3,
     `cmpi`/`bg` vs 3). Lift had `iter < 3` and dropped RR (list[3]).
     Front yaw (`g13`≤1) is always `obj+0x24` from fixed `0xb0(fp)` — not the
     advancing scale cursor (`obj+0x28` was wrong for iter 1).
   — Body `0x20b940` bump after first catalog is `addo r7` between yaw and
     pitch of the second pose (`0x23D8C`), not immediately after the `stq`.
   — Wheel bit scale @ `0x31A5C`: even `bit_i` → negate half (`mulrl` +
     `notbit 31` on the double high word); odd → +half. Gate is original
     `ld 0x30(r7)` > 0. `r7` advances +4 per bit (`obj+0x30/34/38/3c`).
     Case `0x5D0B00` second `0x2c` Y is `frame+0x64 + r13*scale` (r13=2×depth),
     not `+ half_sc`. Table rows from ROM-mirrored `0x5d0660`/`0x5d0690`.
 — Attract car body pen: keyframe `obj+0x50` low nibble → `0x5c2780[pen]`.
 Pen 0 body ix 110/119 (Delta); pen 1 ix 96/106 (Celica). Inner_5 must
 not invent `0x22220000` pen rewrites (desert-only in inner_3).
 — Race integrate `@ 0x2D294` calls `0x30320` (not bare `0x2b0b0`):
 save `node+0x88`, `pack(pred, +0x88)`; if index≤0 leave fail; else
 **restore** prior index, `0x30380` XZ clamp, pack again; on success
 snap pred to `node+0x90/94/98` and restore index again. Keeping pack’s
 write was a lift bug (poisoned `table_index_a`).      Host force_slot keeps
 pose-burst `predicted[]` — do not re-feed `0x30380` scratch as corner 0
 (ROM aliases that scratch onto `fp+0x40`; host must not).
 — `--practice` START (until `0x214120`): recock analog 0x80 (`fov_scale`
 `@ 0x395D8` is `byte−0x80`). Do **not** restamp follow yaw from
 `pose_table` `@ 0x21CD8` (init-only from `cam_object_init`) or copy car
 heading onto slot 0. Desert `@ 0x1FA20` poses CPs through `*0x213980`
 euler (`0x2a`/`0x29`/`0x2b` then `0x27`); `scene_frame` publishes that
 via epilogue. Pre-GO, `214120==0` zeros follow vel X/Z only — yaw still
 `+=` rate. Spline t `@ 0x20ab58` stays live. Do not invent look-at.
 — `geo_view_copro_vec_push` @ `0x37200` is a real `call` from
 `scene_frame` (4×). Callee `0x40(fp)`/`0x70(fp)` are slot temps and
 `0x2f` staging; `g1`/`g5` keep the caller's AABB / force pointers.
 Shared global `fp` stomped those vectors (world-scale leftover into
 follow yaw → desert intro X/Y dolly). Private host frame, same as
 `vec_scale`.
