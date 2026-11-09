# Naming-normalization consolidation - Die Hard Arcade (ST-V, SH-2) decomp

Applied 2026-10-21. Target: `project/src_functional/*.c`. Spec: `notes/consolidation_audit.md`.
Method: data-driven - the on-disk tree + `nm` (defined `T` / undefined `U` symbols from a real
`-O1` compile) are ground truth; the audit is the spec/cross-check. Canonical name of a ROM entry =
its filename as a full 8-hex SH-2 address (`func_<8hexSH2>`), unless the surviving file uses a
semantic name, which is kept. Address resolution: `v = int(hex); addr = v if v >= 0x02000000 else
v + 0x02000000` (normalises census low-24 and missing-leading-zero spellings).

The live tree had drifted from the audit snapshot (945 files vs the audit's 934; some census files
were already renamed, e.g. `020d58c.c` was already `0220d58c.c`), so every operation was made
**conditional on the actual filesystem** rather than blindly applied from the audit lists.

## Result summary

| Action | Count |
|---|---|
| Hard duplicate-symbol collisions resolved (d.1) | 3 |
| Redundant duplicate-entry files deleted (d.2) | 39 |
| Non-canonical filenames renamed to `<8hexSH2>.c` | 88 |
| Internal entry-symbol renames to `func_<8hexSH2>` | 66 |
| Call-site / extern reference token replacements | 2357 (across 333 files) |
| - of which: semantic-name redirects (orphaned by d.2 deletes) | 9 names |
| Divergent duplicate pairs left untouched (NOT in audit) | 3 (flagged below) |
| **Final compile (`gcc -O1 -c -o /dev/null -Wall -Wextra`)** | **906 files, 0 errors** |
| Residual warnings | 4 files / 7 lines, all pre-existing, unrelated to naming |
| Internal dangling refs (ref to a func defined in-tree under another name) | **0** |

Before this pass there were **211** such internal dangling references (the audit's category-c
link defects); after, **0**. The 3 hard-collision symbols (`func_02229640`, `func_02233250`,
`func_022332d4`) are now each defined exactly once, and a full-tree scan finds **zero duplicate
public symbols** - the tree would link as one.

## 1. Hard duplicate-symbol collisions (d.1) - resolved

All three were caused by the census file `0229640.c` / `233250.c` redundantly re-defining symbols
that also exist in the canonical 8-hex files. Bodies were compared per-function before deletion.

| symbol | kept (canonical) | deleted (redundant) | equivalence |
|---|---|---|---|
| `func_02229640` | `02229640.c` | `0229640.c` | identical bodies (verified) |
| `func_02233250` | `02233250.c` | `233250.c` | equivalent logic, HIGH fidelity both; kept file uses clean `func_` naming |
| `func_022332d4` | `022332d4.c` | `233250.c` | equivalent logic, HIGH fidelity both |

`233250.c` defined BOTH `func_02233250` and `func_022332d4` (a two-function census TU); deleting it
resolved the second and third collisions at once. The kept files are equivalent re-decompilations
of the same ROM routines (scene-setup + particle-list emitter), confirmed by reading both.

## 2. Duplicate entry-address pairs (d.2) - 39 deleted

For each, the census/unpadded sibling was deleted and the canonical 8-hex file kept. 9 of the
deleted siblings used a **semantic** name that was still referenced by other files; those callers
were redirected to the survivor's `func_<8hex>` name (see section 4). The audit's assertion that
both TUs compile the same ROM routine was spot-verified on representative pairs (`0x02213290`,
`0x02227602`, `0x0223c95c`, plus the two hard-collision pairs) - equivalent, differing only in
naming/offset convention.

Deleted (39): `2205812.c 220581a.c 220d98e.c 220e000.c 220e0aa.c 220f15e.c 220f19e.c 220f202.c
220f26a.c 220fb58.c 220fc3c.c 220fc82.c 220fcc4.c 220fd52.c 220fd90.c 220fdc8.c 220fe48.c
220fe98.c 220fece.c 220feec.c 220ff0a.c 220ff28.c 220ff54.c 220ff72.c 220ff90.c 220ffae.c
2213290.c 02275e4.c 0227602.c 0229640.c 022ae40.c 232610.c 233250.c 0239fc6.c 023c95c.c
023df30.c 0242272.c 02425d0.c 0243066.c`

(`0229640.c` and `233250.c` are the d.1 hard-collision files; the other 37 are name-distinct
duplicate entries.)

## 3. Non-canonical filenames - 88 renamed

Each census / missing-leading-zero filename was renamed to the full `<8hexSH2>.c` form (e.g.
`232610.c -> 02232610.c`, `0216c38.c -> 02216c38.c`, `2213190.c -> 02213190.c`). The audit listed
134 such files; the 88 applied + 39 deleted-as-duplicate + 3 blacklisted (section 5) + 4 already in
canonical form on disk (prior partial pass) = 134, fully reconciled.

## 4. Internal entry-symbol renames (66) + call-site fixes (2357 tokens)

- **66 internal def renames**: each surviving file's public entry symbol was set to
  `func_<8hexSH2>` when it was a truncated/short `func_`/`f_` placeholder (covers the audit's b.1
  31 "canonical filename, truncated internal" cases plus the renamed b.2 files). Semantic entry
  names (e.g. `sin_lookup`, `mtx_load_abc`, `actor_apply_damage`) were preserved per the audit's
  note that semantic names are acceptable when callers agree.
- **2357 reference token replacements across 333 files**: every `func_<addr>` / `f_<addr>` call
  site and in-file `extern` declaration whose spelling did not match its definition was rewritten
  to the definition's exact name - numerically by resolved address (covers audit c.1 `func_`/`f_`
  rows and all of c.2), and for 9 semantic names orphaned by the d.2 deletions, by explicit
  redirect: `clear_vec4_array->func_02213290`, `actor_combo_reset->func_022275e4`,
  `actor_combo_advance->func_02227602`, `actor_pose_resolve->func_0222ae40`,
  `hud_round_data_init->func_0223c95c`, `hud_draw_player_status->func_0223df30`,
  `actor_pose_hold_state->func_02242272`, `actor_throw_sequence->func_022425d0`,
  `actor_special_move_state->func_02243066`.
  (The 2357 token count >> the audit's 210 unique-symbol rows because each unresolved symbol has
  many call sites plus a per-file extern declaration.) Multi-function TUs
  (`0223e5e0.c`, `0223fc06.c`, `02246254.c`, `02247508.c`) were handled so their secondary entry
  symbols were never clobbered.

## 5. NOT applied - 3 divergent duplicate pairs (flag for human review)

These were discovered on disk but are **not in the audit's dedup list** - the audit only listed
them as simple b.2 renames, not knowing both files existed:

| ROM entry | 8-hex file | census sibling | problem |
|---|---|---|---|
| `0x022473ee` | `022473ee.c` (`func_022473ee`) | `02473ee.c` (`actor_move_init`) | **divergent bodies** |
| `0x022475a8` | `022475a8.c` (`func_022475a8`) | `02475a8.c` (`actor_move_step`) | **divergent bodies** |
| `0x02247924` | `02247924.c` (`func_02247924`) | `0247924.c` (`actor_face_resolve`) | **divergent bodies** |

Unlike the 39 d.2 pairs (same routine, cosmetic naming differences), these are genuinely
**different decompilations** of the same ROM address - e.g. `func_022473ee` case 2/3 tail-call
`func_02247a16`/`func_02247a3e`, while `actor_move_init` phase 2/3 call
`func_0223afc4`/`func_0223ac50` and write a different set of state fields. Choosing which is
higher-fidelity requires reading the ROM, which is beyond a mechanical naming pass. Crucially they
**do not break the link** (distinct symbol names), and each name is referenced by live callers
(`actor_move_init` <- `0247508.c`; `func_022473ee` <- `02247a16.c`, `02247a3e.c`), so deleting
either would orphan callers. **Action taken: left all 6 files untouched** (no delete, no rename,
no def change), but their *outgoing* call-site spellings WERE normalized so the tree still links.
The 3 remaining non-canonical filenames in the tree (`02473ee.c`, `02475a8.c`, `0247924.c`) are
exactly these siblings. **Recommend a human pick the authoritative decomp for each and delete the
other.**

## 6. Residual compiler warnings (pre-existing, out of scope)

Final full-directory compile: **906 files, 0 errors**, 4 files with warnings (7 lines), all
`-Wint-to-pointer-cast`: `02203de8.c`, `02203e0a.c`, `02203e5e.c`, `0220cab0.c`. These are a
strict subset of the 6 pre-existing warning files in the baseline (the other 2 vanished because
their files were deleted as d.2 redundant). They arise from compiling 32-bit SH-2 target code
(`(void*)(0x060xxxxx + ...)`) on a 64-bit host and are unrelated to naming; fixing them would mean
editing decomp logic or the `FPTR` macro in `decomp.h` (out of scope / forbidden). **This pass
introduced zero new warnings.**

## 7. Residual dangling references (pre-existing, external)

118 `func_`/`f_` references still resolve to no in-tree definition - but **0** of them point to an
address that has a file in `src_functional` (confirmed by cross-checking every dangling symbol's
resolved address against the file set). They are calls into functions not yet decompiled into this
directory (or in other ROM regions); they would resolve against other objects at final link. This
is pre-existing (the original tree had 125 such external refs) and not a regression.
