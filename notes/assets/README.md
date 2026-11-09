# Die Hard Arcade (Sega ST-V) — ASSET DATA map

Reverse-engineered from `cart.bin` (20 MiB, big-endian).
Rule: **SH-2 addr = cart_offset + 0x02000000** (A-bus CS0 window). Many stored
pointers use the uncached mirror `0x22xxxxxx`; normalize with `v & 0x07FFFFFF`
then subtract `0x02000000` to get the cart offset.

Scope of this doc: the **asset ROM** at cart `0x400000..0x1400000` (16 MiB).
Code lives earlier (`0x200000..0x290000`, mirrored at `0x300000`). This work is
byte-analysis + capstone only (no Ghidra).

Confidence tags: **HIGH** = proven by code or exact structural math; **MED** =
strong structural evidence; **GUESS** = plausible, unverified.

---

## 0. Top-level layout — HIGH

The 16 MiB is 5 regions of fixed size. Starts are on clean `0x300000` strides
(except sound, 4 MiB). Each region's base appears as a literal constant in the
code pools (cached `0x02xxxxxx` and uncached `0x22xxxxxx`, in both the main code
and its `0x300000` mirror), so the engine indexes each region by `base + offset`.

| Region | cart range            | used end   | size alloc | base const in code      | content (confidence) |
|--------|-----------------------|------------|------------|-------------------------|----------------------|
| **A**  | `0x400000..0x700000`  | `0x6fd584` | 3 MiB      | `0x22400000` (4 hits)   | 4bpp packed graphics / 2D sprites+tiles (MED) |
| **B**  | `0x700000..0xa00000`  | `0x9f5778` | 3 MiB      | `0x22700000` (4 hits)   | sprite/texture table + pixel blobs (MED) |
| **C**  | `0xa00000..0xd00000`  | `0xce83d8` | 3 MiB      | `0x02/22a00000` (3 hits)| frame-sequential transform/animation records (MED/GUESS) |
| **D**  | `0xd00000..0x1000000` | `0xf27b24` | 3 MiB      | `0x02/22d00000` (12 hits)| **3D model (mesh) library** (HIGH) |
| **SND**| `0x1000000..0x1400000`| per-bank   | 4 MiB      | `0x03/23000000` (confirmed in `sound_init`) | 8 SCSP sound-RAM images (HIGH) |

Regions are `0xff`-padded up to their boundary. Gaps found by the padding
scanner (`seg.py`): A ends +`0x2a7c` pad, B +`0xa888`, C +`0x17c28`, D +`0xd84dc`.

---

## 1. Region D — 3D MODEL LIBRARY @ `0xd00000` — HIGH

The reported "offset/pointer table" family. A directory of **1560 models**.

**Directory** starts `0xd00014` (record 0 at `0xd00000` is a null/zero entry),
stride **0x14 bytes**, runs to ~`0xd005a0`... (full 1560 recs span to `0xd1a7xx`).

Record layout (0x14 bytes), verified:
```
+0x00  u8   vertex_count
+0x01  u8   face_count
+0x02  u16  0
+0x04  u32  ptr -> vertex array   (0x22d0xxxx)
+0x08  u32  ptr -> face   array   (0x22d0xxxx)
+0x0c  u32  0
+0x10  u32  type: 0x02000000 (1476 recs) | 0x04000000 (83 recs)
```

**Vertex array** = `vertex_count` × 16 bytes = `(X, Y, Z, W)` as **4× s32 BE**,
in **16.16 fixed-point** (divide by `65536` for floats) — **HIGH / proven**.
Simple models give exact round values: the 8-vtx box (model 4) has coords
`±32768 / 0 / 65536` = `±0.5 / 0.0 / 1.0`; the quad (model 3) `±32768, 65536`
= `±0.5, 1.0`; model 2 `±8192` = `±0.125`. So authored unit space: a typical
model stands ~1.0 unit tall and ~±0.5 wide (origin at the feet / base, `Y` up).
Magnitudes: median vertex |coord| ≈ 0.44, per-model max ≈ 0.5 (p50) rising to
~3.0 (p90) and ~10.6 (absolute max, raw 697237). W (4th word) is 0 for all but
1477/27320 verts, where it is exactly `1` (a flag, not geometry) — safe to drop.

**Face array** = `face_count` × 16 bytes:
```
+0x00  u8[4]  four vertex indices (quad; degenerate quad = triangle)
+0x04  u16    mode/flag word — {0,1,2}                         (MED)
+0x06  u16    flag word — almost always 0x2000 or 0x3000 (±low bit); a 1-bit
              per-face flag (facing / colour-calc?), NOT the colour            (MED)
+0x08  u16    0 (always)
+0x0a  u16    small {0,1,6,7,9…} — rarely set                  (GUESS)
+0x0c  u16    0 (almost always)
+0x0e  u16    **per-face colour / CLUT index** — see §6         (HIGH that this
              is the colour selector; the palette it indexes is §6-open)
```
Quads match the Saturn **VDP1 distorted-sprite / polygon** primitive and the
engine's `vec3_array_matmul` (transforms a vertex array) + `vdp1_add_sprite_cmd`.

**Proof:** for 909 / 1559 adjacent records, `next.ptr1 - ptr1 == vcount*16`
**and** `next.ptr2 - ptr2 == fcount*16` exactly (the rest differ only because
the vertex and face blobs interleave, so neighbours in index order aren't always
adjacent in memory). Every sampled face's 4 indices are `< vertex_count` (valid
topology). Model 0 = 92 verts / 100 faces.

Run `parse_assets.py --models` to list; `extract_model.py N out.obj` to dump a
model to Wavefront OBJ (quads) for inspection in Blender etc.

---

## 2. Region SND — 8 SCSP sound-RAM images @ `0x1000000` — HIGH

Proven by decompiling `sound_init` @ SH2 `0x02215da8`:
1. write `0x300` → SCSP ctrl `0x25b00400`
2. clear `0x25a70000` (len `0x1000`) and `0x25a00000` (len `0x400`) via HWRAM libs
3. **copy `0x23000000` → `0x25a00000`, `0x4000` bytes** (68000 sound driver)
4. **copy `0x23004000` → `0x25a04000`, `0x6c000` bytes** (samples + sequences)
5. kick driver (HWRAM call `0xffffeb80`), delay, optional `sound_queue_push`.

`0x23000000` normalizes to cart `0x1000000`. So each 68k sound-RAM image =
`0x4000` driver + `0x6c000` data = `0x70000` used, laid out one per **0x80000**
bank, `0xff`-padded. **8 banks** at `0x1000000, 0x1080000, … 0x1380000`
(likely one sound set per stage/scene; the whole SCSP RAM is swapped wholesale).

Every bank header is identical: `00004000 00000120` (banks 0/1 use `0x6f988`,
banks 2–7 use `0x6fc2c`). SCSP RAM is 512 KiB = `0x80000` — matches the stride.

The **"increasing-offset table"** at `0x1000000` (values `0x120, 0x500, 0x508…`
with `0x508` repeated as a default/null) is the 68k driver's internal
sequence/sample pointer table — offsets *within* sound RAM, part of the driver
image. Sub-format (sequence vs PCM boundaries, ADPCM vs 8/16-bit PCM) not yet
decoded; needs the 68k driver disassembly. Driver = M68000 (ST-V sound CPU).

`sound_queue_push` request word example from `sound_init`: `0xa00200ff`.

---

## 3. Region B — sprite / texture table @ `0x700000` — MED

Same **0x14-byte** record format family as region D, **137 records**
(`0x700000..0x700ab4`), then the data blobs:
```
+0x00 u32 ptrA -> descriptor array (region B, ~0x732xxx; 16-bit params)
+0x04 u32 ptrB -> pixel/data blob  (region B, ~0x713xxx), sized
+0x08 u32 running index (0,1,2,3,9,0xb,…)  — cumulative sub-item count
+0x0c u32 count          — sub-items in this entry
+0x10 u32 offset         — cumulative byte offset into the blob stream
```
`ptrB` deltas track the `offset` field (variable-size chunks). `ptrA` targets
(`0x732e08`: `0000 0410 0020 0010 04a0 …`) look like per-sprite descriptors
(sizes / coords / VRAM or CLUT addresses). Best read: a **sprite/texture atlas**
— `count` sub-sprites per entry, descriptors in ptrA, bitmaps in ptrB. Exact
descriptor fields unconfirmed.

---

## 4. Region C — animation / transform records @ `0xa00000` — MED/GUESS

**UPDATE — decoded, see `anim/README.md`.** Region C = 1236 frame-
sequential animation clips, played by `animation_update` @0x0222dc2c. A clip
directory (cart `0x27913c`, stride `0x1c`, ptr+0x18 -> frame data, +0x14 =
keyframe count) indexes runs of `0x6c`-byte frame records (54 s16, Q1.15
orientation + translation). Proof: Σ(keyframe counts)×0x6c = 99.8% of region-C
size. Full format, confidence, and a sample clip in `anim/`.

Fixed **0x6c-byte (108) records**, each 54× s16 BE. Consecutive records are
near-identical with small per-field deltas → **frame-sequential** data
(keyframed motion / vertex or bone animation for the region-D models). Blocks of
~10 contiguous records then a jump (a sig scan of the first marker found runs of
0x6c stride broken at `0x6390`, `0xf51c`), i.e. grouped into clips. 54 shorts ≈
9×6 (pos+rot per bone) or a 3×4 matrix + extras — not pinned down. No in-region
directory found; probably indexed from code/HWRAM. **GUESS** on exact meaning;
HIGH that it is 0x6c-stride frame records.

---

## 5. Region A — 4bpp packed graphics @ `0x400000` — MED

Dense **4bpp paletted pixel data** (packed nibbles; flat-fill runs like
`0x33333333`, `0x22cccccc` and ASCII-looking nibble strings `"DDDA"`,`"EQgqDQfq"`
are the giveaway — low palette indices 0–0xF). Starts with a 16-entry all-white
block (`0xffff` + 15× `0x7fff`) at `0x400000`, then zero pad, then pixels from
`~0x400220`. Likely 2D sprites / VDP2 tiles / HUD / text / title art. No 0x14
directory in-region; indexing is from code or an as-yet-unlocated table.

---

## 6. Model colour — FLAT-SHADED via CLUT (not textured, not direct-RGB) — MED/HIGH

**Conclusion:** region-D models are **flat-shaded polygons coloured through a
colour-RAM (CRAM) CLUT**, one palette colour per face. They are **not
texture-mapped** and the face records **do not** store direct RGB555 values.

Evidence (byte-analysis over all 16451 faces):
- **Colour selector = the last u16 of each 16-byte face record (offset +0x0e).**
  It varies per face *within* a model (1277/1560 models carry ≥2 distinct
  values; e.g. box model 64's six faces = `0x32,0x33,0x34,0x35,0x36`), i.e. a
  real per-face material index, not a global constant.
- **Not direct RGB555:** bit15 is **0** for every one of the 16451 faces. On
  VDP1, a direct RGB colour in `CMDCOLR` *requires* bit15=1; bit15=0 means the
  field is a **CRAM address / colour-bank reference**. So the value is a palette
  index, decoded through CRAM, not an inline colour.
- **Not textured:** the face record has **no per-vertex UVs and no texture
  source address** — a textured VDP1 sprite needs `CMDSRCA`+`CMDSIZE` per
  primitive, which there is nowhere to store here. The selector also ranges to
  `0x1fff` (8191), far beyond region-B's 137 texture entries, so it is not a
  texture index either.
- Values cluster in two banks (top nibble 0 = `0x000–0xfff`, nibble 1 =
  `0x1000–0x1fff`), which lines up with the +0x06 flag's two states
  (`0x2000`/`0x3000`) — plausibly a 2-bank CLUT select + 12-bit colour address.

**RESOLVED — the actual RGB (see §10).** `cidx` is a **direct global index**
into one contiguous **RGB555 master palette** at cart `0x9bb438`
(SH2 `0x229bb438`, region B). Proven empirically: every model's `cidx` block
decodes to coherent per-material shading ramps, and the whole model index range
`0..0x18ab` lands on valid palette entries (bit15=0, or `0xffff` white sentinels).
See `palette.json` for the decoded table.

## 7. How the engine indexes assets (summary)
- Region bases are literal pool constants (`0x224/227/22a/22d00000`, `0x23000000`).
- Region D & B: `base`-relative `0x14` directories → (ptr, ptr, counts) → blobs.
- Sound: `sound_init` bulk-copies a whole `0x80000` bank into SCSP RAM; the 68k
  driver's own in-RAM tables then index sequences/samples.
- Regions A & C have no in-region directory located — indexed from the HWRAM
  code copy (consistent with `boot_mapping.md`: hot code runs from `0x06000000`).

## 8. Open items
1. Decode the M68000 sound driver (`0x1000000..0x1004000`) to split sequence vs
   PCM and determine sample format.
2. ~~Locate the RGB555 CRAM CLUT that region-D faces index~~ — DONE (§10):
   master palette at cart `0x9bb438`, `cidx` = direct global index. `palette.json`.
3. ~~Pin region-C record semantics~~ — DONE (`anim/README.md`): clip directory
   @`0x27913c`, `0x6c` frame records decoded. Clip->model link still open (it is
   an actor property, not stored in region C).
4. ~~Find region C index table~~ — DONE: clip directory @cart `0x27913c`
   (1236 × `0x1c`). Region A index table still unlocated.
5. ~~Decode region-D face attribute bytes~~ — DONE (§6): colour index is the
   face record's last u16 (+0x0e); other attr words mapped.

## Files (notes/assets/)
- `README.md` — this map.
- `parse_assets.py` — dumps the top-level map, region-D model table, region-B
  table, sound banks. `--models`, `--sound`, `--segb`, `--map`.
- `extract_model.py` — export a region-D model (by index) to Wavefront `.obj`.
- `models.json` — 54 curated models for the browser viewer (see §9).
- `models_all.json` — ALL 1560 models for the full browser library (§11).
- `make_models_all.py` — regenerates `models_all.json`.
- `anim/` — region-C animation: `README.md` (format), `clips_index.json` (1236
  clips), `sample_anim.json` (one clip), `make_anim.py`.
- `seg.py` — ff-padding segment-boundary scanner (how the map was derived).
- `palette.json` — decoded RGB555 model master palette (cidx → [r,g,b]); see §10.
- `make_palette.py` — regenerates `palette.json` from `cart.bin`.

---

## 9. Browser-viewer export — `models.json` — HIGH

54 curated models for the web 3D viewer, chosen as an even spread across
face-count (1–119 faces, 4–132 verts) plus the clean geometric primitives
(models 2,3,4,6,30,37,64). Format:
```json
[{ "id": N,
   "verts": [[x,y,z], ...],          // floats, 16.16 raw / 65536, 4-dp
   "faces": [[i,j,k,l], ...],        // 4 vertex indices per quad (degenerate
                                     //   quad = repeated index = triangle)
   "cidx":  [c, ...] }, ...]         // per-face raw colour index (face +0x0e);
                                     //   see §6 — not yet true RGB
```
- **Coord scale / normalization:** raw s32 **÷ 65536** (the proven 16.16 scale)
  and nothing else. That already lands a typical model at ~`-0.5..1.0` per axis
  (origin at the base, `Y` up); outliers reach ±2–3. Models are **not**
  per-model re-scaled, so relative proportions are preserved — the viewer should
  auto-frame/centre. `W` and the face attribute bytes other than `cidx` are
  dropped. Precision truncated to 4 decimals.
- **Size:** ~30 KB (well under the 2 MB budget).
- **Colours:** `cidx` only (raw palette index); true RGB pending §6.
- Regenerate with `make_models_json.py`.

---

## 10. Model colour palette — RGB555 master CLUT @ `0x9bb438` — HIGH

The region-D flat-shaded models colour each face through a **single contiguous
RGB555 master palette** in region B. The per-face `cidx` (face record `+0x0e`,
bit15=0, §6) is a **direct global colour index** into it.

**Location (HIGH):** cart `0x9bb438` — SH2 / A-bus addr `0x229bb438`.
~`0x1900` (6400) entries; models use `cidx` `0x0000..0x18ab` (6316), which maps
**contiguously** with **zero** invalid entries (every non-`0xffff` value has
bit15=0). `palette.json` dumps `0..0x18ab`.

**Format (HIGH):** Saturn **BGR555**, 16-bit big-endian, one colour per u16:
```
bit15 = 0 (unused / msb)
bits 14..10 = Blue   (5-bit)
bits  9.. 5 = Green  (5-bit)
bits  4.. 0 = Red    (5-bit)     ->  0b_bbbbb_ggggg_rrrrr
```
5->8 bit: `(c<<3)|(c>>2)`. `0xffff` entries are white sentinels (mask `&0x7fff`).

**Bit-order proof (HIGH):** model 805 face block `0xff0..0xff4` =
`0008 0072 01be 239f 43ff`. Decoded **BGR** that is a textbook fire ramp
dark-red->red->orange->yellow->pale-yellow
`[66,0,0] [148,24,0] [247,107,0] [255,231,66] [255,255,132]`. The RGB reading
would be a cold blue->cyan ramp for the same object — BGR is correct (and is
the documented Saturn hardware order). More ramps: model 8 `0x4e..0x54` is a
smooth blue->white shade ramp; model 37 `0xe3..0xe8` a green foliage ramp.

**How it reaches CRAM (MED):** a CLUT-pointer table at cart `0x700aa0`
(`{ptr, 0x00200000}` pairs; each `ptr` = a 256-colour sub-CLUT in region B,
spaced `0x200` bytes) drives per-scene loads. The stage-setup state machine
(SH2 `0x0221b8e4`) picks a sub-CLUT pointer (e.g. `@(0x22700b70)`), and the
transfer enqueue routine (SH2 `0x0021632e`, transfer-type `14`) queues a
`0x200`-byte (256-colour) DMA to **VDP2 CRAM `0x25f00000`**; the queue is
flushed to hardware per frame. Saturn CRAM holds ≤2048 colours, so only the
current scene's sub-CLUTs are resident at once — the full 6316-entry master
palette lives in ROM and `cidx` indexes that master set. The leading CLUT
(shared grey+orange shade ramps, `cidx 0..0xff`) is duplicated at several region-B
group bases (`0x702xxx`, `0x90exxx`, `0x90f178`, `0x93cxxx`, ...); `0x9bb438` is the
one group that is contiguous across the **entire** model index range, so it is
the master used for `palette.json`.

**Caveats:** (1) The exact runtime `cidx`->CRAM-slot addressing (whether the
renderer reads CRAM, or converts master-palette `cidx` to a VDP1 direct colour)
was not traced through the HWRAM glib draw primitives — but the RGB values are
identical either way, and are validated by the material ramps above.
(2) `cidx 0 = 0x20a0` (dark navy) and `cidx 1 = 0x0000` (black); some blocks
contain `0xffff` whites. (3) If a per-stage palette ever diverges from the
`0x9bb438` master beyond the shared leading CLUT, a stage-specific model could
differ — unobserved in the 54 curated models, all of which decode coherently.

---

## 11. Full model library — `models_all.json` — HIGH

All **1560** region-D models exported for an in-browser model browser (the §9
`models.json` is a 54-model curated subset). Zero malformed records (every
record has valid pointers, in-range face indices, non-zero counts).

Format:
```json
{ "scale": 65536, "count": 1560, "wflag": 1477, "skipped": [],
  "models": [ { "id": N,
                "v": [[x,y,z], ...],      // s32 16.16 / 65536, 3 dp (W dropped)
                "f": [[i,j,k,l], ...],    // quad vertex indices (dup = triangle)
                "c": [cidx, ...] } ... ]} // per-face colour index -> palette.json
```
- **Size:** ~0.80 MB (27,320 verts, 16,451 faces total) — well under budget; a
  browser loads it once and pages by `id`.
- Coords raw ÷65536 (proven 16.16), no per-model rescale (proportions preserved;
  viewer should auto-frame). `wflag` = count of verts whose dropped 4th word
  (W) was non-zero (a flag, not geometry — see §1).
- `c` indexes `palette.json` (§10) for RGB.
- Regenerate with `make_models_all.py`.
