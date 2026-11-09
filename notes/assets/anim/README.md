# Die Hard Arcade (Sega ST-V) — Region-C ANIMATION format

Decoded from `cart.bin` (BE). Supersedes the "MED/GUESS" note in
`../README.md` §4. Rule: SH-2 addr = cart_off + 0x02000000; stored pointers use
the uncached mirror 0x22xxxxxx (normalize `(v & 0x07FFFFFF) - 0x02000000`).

Confidence tags: **HIGH** = proven by code or exact structural math; **MED** =
strong structural evidence; **GUESS** = plausible, unverified.

---

## 1. What region C is — HIGH

Region C (`0xa00000..0xce83d8`, 3 MiB) is a library of **frame-sequential
animation clips**. It is played back by the engine routine **`animation_update`
@ SH2 `0x0222dc2c`** (confirmed by disassembly, see §4). There are **1236
clips**, described by a **clip directory** (§2); each clip is a run of
**`0x6c`-byte frame records** (§3), one record consumed per displayed frame.

**Decisive proof:** the sum of all 1236 clips' keyframe counts (directory field
`+0x14`) × `0x6c` = **3,041,496 bytes**, i.e. **99.8 %** of region-C's used span
(3,048,408 B). The directory's declared frame counts tile region C almost
exactly — so both the record stride and the per-clip counts are correct.

---

## 2. Clip directory — HIGH

A table of **1236 records, stride `0x1c` (28 B)**. The region-C data pointer
column sits at cart **`0x27913c`** (stride `0x1c`); the record base is
`ptrcol - 0x18`. (This table is part of the program/data image that boots into
HWRAM; the engine reads its HWRAM copy at `*0x0222de74` — `0x0607d124` — but the
ROM-resident source is the table dumped here. The cart↔HWRAM copy is non-linear,
see `../../boot_mapping.md` §4, so the HWRAM base is not a simple offset.)

Record layout (`0x1c` bytes), verified against `animation_update`'s field reads:
```
+0x00  u32  0            (reserved / always 0)
+0x04  u32  0            (reserved / always 0)
+0x08  u32  flag         (usually 0; small ints e.g. 3)
+0x0c  u32  event cue    (usually 0; packed 0x09xx00xx on some clips — sound/
                          event trigger?  GUESS)
+0x10  u16  type         (1 / 3 / 4 …)
+0x12  u16  sub          (0,1,2,3 …)
+0x14  u16  KEYFRAME COUNT        <- engine compares this to the frame counter
+0x16  u16  loop / flag  (0 or 1; 1 = looping clip)
+0x18  u32  ptr -> region-C frame data (0x22a.....; advanced +0x6c each frame)
```
Dumped in full to **`clips_index.json`** (id, cart offset of frame data, kf,
loop, type, sub, and non-zero flag/cue).

**How the engine uses it** (from `animation_update` @0x0222dc2c, HIGH):
- entity `+0x5a` (u16) = current clip id → directory index.
- `clip_rec = dir_base + clip_id * 0x1c`  (the `×0x1c` is in the code:
  `mov #28,r3 ; mul.l r3,r13 ; add <dir_base>,r13`).
- entity `+0x58` (u16) = frame counter, incremented each update; when it reaches
  `clip_rec[+0x14]` (keyframe count) the clip ends → loop (`+0x16`) or chain.
- the frame pointer is `clip_rec[+0x18]`, stepped **`+0x6c` per frame**
  (`mov.l @(24,r13),r2 ; add #108,r2`), and each `0x6c` record is copied into a
  **per-actor `0x6c`-byte state buffer** (indexed by actor slot × `0x6c`; the
  copy helpers are called with `r0 = 108`). `anim_set_state` @0x0220f816 also
  writes into `entity_data + 0x6c` (`mov #108,r1 ; add r6,r1`), confirming the
  actor carries one live `0x6c` animation record.

---

## 3. Frame record — the `0x6c` block = 54× s16 — MED

Each frame = **`0x6c` (108) bytes = 54 signed 16-bit BE words**. Consecutive
records within a clip change smoothly (small per-field deltas) — keyframed /
interpolated motion.

**Fixed-point:** the large components saturate at exactly `±32768` (`0x8000`)
and `±16384` (`0x4000`) across all 28,226 frames → **Q1.15** (1.0 = 32768).
That is the same scale the engine's `sin_lookup`/`cos_lookup` (@0x02213190/a8,
result `<<1`) produce — i.e. these words are **orientation (sin/cos / rotation-
matrix) terms**, not raw coordinates. Smaller words (±hundreds…few-thousand) are
translation-like.

**Skeleton (global, 28,226 frames):** the record has a *fixed* active/zero
layout — it is structured, not opaque. Slots that are ~always zero:
`s13,s14,s22,s23,s27-29,s34,s43,s44`. Three slots are tiny integers (|v|≤14):
`s48,s50,s52` — small counters/flags, not geometry. The rest carry motion.

A repeating 6-word motif `[BIG, mid, mid, BIG, 0, 0]` recurs at `s9-14`,
`s18-23`, `s30-35` (the two BIGs are Q1.15 orientation terms, the mids are a
vector). Read together with the leading block `s0-8` and the `s36-47` / `s45-53`
tails, the record parses as **~5-6 sub-transform blocks** (orientation + a small
translation + a couple of flags) — i.e. a **multi-part / hierarchical transform
for one actor per frame**, plus a handful of control fields.

**Best interpretation (MED):** region-C frames drive **per-frame transforms
(orientation + translation) of a small set of parts/joints**, applied to the
actor's region-D mesh(es) by the engine's matrix×vertex-array routine
(`vec3_array_matmul` @0x02213ca4, a saturating 16.16 MAC transform over a vertex
array). They are **NOT** raw per-vertex keyframes (far too few words, and the
big words are rotational, not positional) and **NOT** a single rigid placement
(there are several independent transform blocks). The **exact per-field matrix
layout within the 54 words is not pinned** — the actual matrix build + vertex
transform happen in the HWRAM-resident glib (not in this cart image), so the
precise field→matrix mapping can't be traced from the ROM alone. GUESS on which
specific words are which matrix element.

---

## 4. Clip → model link — UNRESOLVED (GUESS)

The clip directory does **not** store a region-D model id. The association is
made by the **actor**: an entity holds its current clip id (`entity+0x5a`) and,
separately, its region-D mesh (from its actor type / spawn record). So
"which model does clip N animate" is a property of the *actor*, set by the
spawn/AI code (largely HWRAM-resident), and is **not recoverable from region C
alone**. Mapping clips to models would need the actor-type → (model, clip-set)
tables, an open item.

---

## 5. Sample animation — `sample_anim.json`

One clean looping clip exported in full for a viewer:
- `clip_id` 9, `cart_offset` `0xb04088`, 16 keyframes, `loop=1`, type 1.
- `frames`: 16 × 54 signed-int rows (the raw `0x6c` records).
- `model_id`: **null** — see §4; the clip's target mesh is an actor property, not
  in the data. A viewer can still animate: apply each frame's transform blocks
  to a chosen region-D mesh, Q1.15 for the orientation words.

Playback: advance one row per frame; at row 16 wrap to 0 (loop=1). Interpolate
between rows for smooth motion (the engine does — it keeps a frame counter and
copies/blends the `0x6c` record each tick).

---

## 6. Confidence summary
| Finding | Confidence |
|---|---|
| Region C = frame-sequential animation clips | HIGH |
| 1236-clip directory @ cart 0x27913c, stride 0x1c | HIGH |
| Record `+0x14`=keyframe count, `+0x18`=frame ptr (+0x6c/frame) | HIGH (code + 99.8% size tile) |
| Frame record = 54 s16, Q1.15 orientation + translation | MED |
| Record = multi-part (orientation+translation) transforms | MED |
| Exact 54-word → matrix field layout | GUESS |
| Clip → region-D model mapping | UNRESOLVED |

## Files (notes/assets/anim/)
- `make_anim.py` — regenerates the two JSONs from `cart.bin`.
- `clips_index.json` — all 1236 clips (id, cart offset, kf, loop, type, sub, cues).
- `sample_anim.json` — clip 9, 16 frames × 54 s16, for a viewer.
