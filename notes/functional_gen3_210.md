# Gen-3 functional decomp — band 0x210000–0x218000 (runtime 0x02210000–0x02217fff)

Single worker, no forks. All new TUs in `project/src_functional/`, MEM-macro style,
include `dh_band21.h`, compile clean with `gcc -c -fsyntax-only -Wall -Wextra`
(zero warnings), hand-verified against `m2c_fn.py --asm`. Call targets resolved with
`braf_resolve.py`; entries confirmed with `find_entry.py` (pool contamination +
merged extents are rife in this band — see "notes on the band" below).

## Added (10)

| file | name | fidelity | summary |
|------|------|----------|---------|
| 022100f8.c | recoil_axis_ramp | HIGH | 3-phase recoil/shake channel on ctrl->unk8 (s16 triple 0x34/36/38, clamp ±0xFF), tail-call 0x022386fc. Models own body only (extent merges siblings 0x2101ce/0x210298). |
| 0221039a.c | obj_intensity_ramp | HIGH | fade/intensity 3-phase ramp; 16.16 fields 0x40/0x44/0x50 via fxmul 0x022131c8 + 0x02213234; writes 4-bit nibble into *0x06098f08 unk22. |
| 0221047c.c | vec2_magnitude | HIGH | sqrt((|a|>>8)^2+(|b|>>8)^2) via isqrt tail-call 0x02213164. First of 3 merged siblings in the extent (0x2104b4 3D form, 0x2104f6 larger — left as siblings). |
| 02210876.c | anim_channel_interp | HIGH | keyframe sampler (list 0x060e1218, countdown 0x060e1298, durations 0x0604ebe4, 0x18-byte frames); 32-bit outputs use cur-prev delta. Helper 0x0024a420 modelled as 2-arg call. |
| 022109d4.c | anim_channel_interp_abs | HIGH | sibling of 0x02210876; 32-bit outputs use the absolute keyframe value instead of the delta. |
| 022119a4.c | actor_play_anim_seq | MED | actor anim-script player (seq table 0x06053344): init length via 0x00228106, per-frame sample 0x022109d4, build transform, cull-submit model via 0x02215a2c, deactivate on end/timeout. |
| 02214e2c.c | displaylist_radix_sort | HIGH | 3-pass LSD radix sort of the deferred display records by the 24-bit key rec[0x20]; bucket tables 0x060bacc0/0x060bf4c0, 12-byte link nodes, two source lists. No external calls. |
| 022151d8.c | clear_sprite_tables | HIGH | zero-fill 0x060cf0cc(0x3800)+0x060d28cc(0x3400) via 0x022148ac. Stub; extent (0x1ae) over-runs a hidden texture-upload sibling. |
| 02215386.c | clear_sprite_scratch | HIGH | zero-fill 0x060d5ccc(0x200) via 0x022148ac. Stub; extent over-runs into already-done 0x2153f6. |
| 02215a2c.c | submit_model_culled | HIGH | model id -> clip table 0x0609ebd0, screen-bounds + near-plane (x>=0x1999) cull, then transform-ring stage (depth/4<3) or immediate draw 0x02215cb4. |

## Notes on the band (why the census over-counts)

- The reported band census (~233 rows) is heavily inflated. After `find_entry` correction
  the authoritative set of genuine, *not-yet-done* real entries in [0x210000,0x218000) is
  **25** (plus 0x2214e2e, which is the mid-prologue second push of 0x02214e2c — a phantom,
  not a function).
- Several "functions" are tiny **tail-call stubs** whose fn.extent over-runs a large hidden
  sibling or the next (already-done) function: 0x215386, 0x2151d8 here; 0x2100f8 / 0x21047c
  merge 3 real functions each. I modelled the genuine triage entry per the existing
  precedent in 0x02214cdc.c (model own body, document the braf tail-branch, note the over-run).

## Resisted / deferred (with reasons)

Entity-behaviour state machines ending in `braf -> 0x22eee4` (shared tail dispatcher) with
many helper calls, several out of band and of unknown arity/semantics. Decodable but not
verifiable to the zero-warning, hand-checked, faithful bar this session; deferred honestly
rather than shipped half-right:

- 0x2211108 (0x3a8, sym hint actor_update_thrown), 0x22114b0 (0x3ac), 0x221185c (0x148),
  0x2211b04 (0x646), 0x221214a (0x206), 0x2212350 (0x41e), 0x2212b20 (0x160),
  0x2212c80 (0x180), 0x2212e00 (0x27c) — large actor updaters, 10–40+ resolved calls each,
  many to 0x228106/0x23e4a0/0x23cbb0/0x23a924/0x22eee4 (out of band).
- 0x210690 (0xc8) — vector/matrix transform with an indirect `jsr` (fn ptr = 0x02213538)
  plus a hidden sibling helper 0x210622; mixed in-band/out-band calls.
- 0x22154fc (0x134) — draw dispatcher with an indirect call through a per-node callback
  (init = 0x02213434, reloaded from node->unk2C) + 4-way switch; needs the callback ABI pinned.
- 0x2215964 (0x9e) — twin of submit_model_culled but gated by out-of-band 0x0024a06c with an
  ambiguous r1/r4 convention; its draw path is 0x02215cb4 (below).
- 0x02215cb4 (0xac) — model draw: walks sub-objects and ends in an **indirect jump-table**
  tail-call (table 0x06090794) with leftover-register args; table + arg ABI unresolved.
- 0x216b14 (0x112), 0x21632e (0x18c) — command-list interpreters driven by a `braf` **jump
  table** (`mov.w @(r0,r1),r0`); require full switch-table reconstruction via ptr_resolve
  (18+ cases, interleaved with the table data) to decode without M2C_ERROR stubs.
- 0x2216c38 (0x41c) — fix() reaches it only by scanning past a pool-contaminated unit head
  (0x216c36 first word 0xffff); genuineness is plausible (4 bsrf resolve to real fns) but the
  entry is unconfirmed; deferred pending a cleaner boundary.
- Hidden siblings inside merged extents (0x2101ce, 0x210298, 0x2104b4, 0x2104f6, the
  0x2151d8 texture routine) are genuine but not distinct triage/unit members; left for a
  dedicated pass.

## Remaining in band after this pass

25 genuine undone -> 10 added -> **15 remaining** (all listed under Resisted above, plus the
hidden siblings). The remaining work is dominated by the out-of-band-helper entity updaters
and the two jump-table interpreters.
