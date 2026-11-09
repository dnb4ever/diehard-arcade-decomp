# Decomp tail pass (claude-opus-tail), 2026-10-30

Scope: fix 02247064, clear the 17-entry coverage_callgraph undone list and the three floors.
Nothing is committed. Every attempt has a line in work/attempts/<fn>.jsonl.

## Headline

| | before | after |
|---|---:|---:|
| defined (files) | 1015 | 1045 |
| distinct referenced (canonicalized) | 919 | 935 |
| universe | 1032 | 1047 |
| TRUE referenced-but-undefined | 17 | **2** (02204258, 022137a0; both newly surfaced, see below) |
| TRUE CALL-GRAPH COVERAGE | 98.4% | **99.8%** |
| non-canonical call-site refs | 5 | 0 |

The whole tree compiles: all 1045 files, per file, with `gcc -O1 -c -I project/src_functional`.
That is how the project builds, since there is no whole-tree link step.

**Read this before trusting the number.** The 17-item list understated the work. A number of
functions were invisible to coverage_callgraph.py:
- **Bodies inside multi-function files.** The coverage tool counts file stems. 02209188.c held
  9 functions; 0223e5e0.c, 0223fc06.c and 02246254.c held 2 each.
- **Short-form refs.** coverage_callgraph.py only matches 8-hex names. `func_<5-7 hex>` and
  `func_<9 hex>` were silently ignored. There were about 50 of these: about 30 pointed at
  existing files and were canonicalized, about 15 pointed at nothing, and 2 had 9 digits.
- **Unreferenced entries.** These are reached through task-handler slots or pointer tables,
  not through C calls (e.g. 0220134c and the six step-table entries before it, 02202d7e,
  02202dda, 02202e84).

## Per function

PASS = difftest PASS/PASS-void. MANUAL = difftest SKIP, then a line-by-line asm comparison
(logged "pass" and labelled as manual in the note). Most SKIPs come from difftest's
structural rules: indirect thunk calls, raw-pointer macros, and "callee must take exactly 4
register args". That last rule alone blocks every function whose callees take 0-3 args.

### Task 1
| fn | name | status | difftest | notes |
|---|---|---|---|---|
| 02247064 | leaf state handler | fixed | **PASS-void** (16 vec) | obj+0xE0 is two separate stores in ROM (AND-store, re-read, OR-store). The C had fused them. |

### The original 17 undone + floors
| fn | name | status | difftest | notes |
|---|---|---|---|---|
| 020160a0 | (bad ref) | ref fixed | n/a | The 0222b7e8 tail is braf 0x22bae4+0xFFFEA5BC = 0x022160a0 (already defined). Fixed the caller and symbols.txt. |
| 022093f8 | results HUD header | split to own file | MANUAL | Was in 02209188.c |
| 022094c4 | results state 1 | split | MANUAL | |
| 022095cc | results state 2 | split + **fixed** | MANUAL | Flag words were snapshotted at entry; the ROM re-reads them before each test. |
| 02209814 | results state 3 (continue countdown) | split | MANUAL | |
| 02209a8c | results state 4 (config editor) | split + **fixed** | MANUAL | g_06098808 / config bytes are re-read per store and glyph (cached in the old C). |
| 02209d70 | results state 5 (stats readout) | split + **fixed** | MANUAL | g_06098804 re-read per call; field fetch now comes before the cursor stores (order was wrong). |
| 0220a0b0 | results state 8 | split + **fixed** | MANUAL | Fetch/print interleave was batched; base re-read. |
| 0220a21c | results state 9 | split + **fixed** | MANUAL | Interleave; group count re-read per iteration and compared unsigned (was cached signed). |
| 0220e55c | (bad ref) | ref fixed | n/a | Mid-function. The 0220ebde jsr is mova 0x20ec60+0x18FC = 0x0221055c lookat_angles_xz. |
| 0220f45c | (bad ref) | ref fixed | n/a | Inside func_0220f454. The 0220ef02 jsr is 0x20ef88+0x15D4 = 0x0221055c. |
| 0222059a | (phantom) | removed | n/a | Its only ref was **02207342.c, which is data decoded as code** (0x207340 = .long 0x06050e48) and has no callers. **Deleted 02207342.c.** 0x22059a is mid-function. |
| 0223e632 | actor post/render dispatcher | split from 0223e5e0.c | MANUAL | Also canonicalized func_022ca98 -> func_0222ca98 and fixed a comment typo (0x0223dc2c -> 0x0222dc2c). |
| 0223fc58 | twin dispatcher | split from 0223fc06.c | MANUAL | asm is identical to 0223e632 apart from scheduling and addresses. |
| 0224627e | voice-object task step | split from 02246254.c | MANUAL | Rewritten as a tail call to the existing real entry func_02246282(task->8) instead of duplicating it. 02246254 now tail-calls 02246282 too. |
| 0224a280 | SHC struct/block copy | new | MANUAL + emulator harness | Real ABI: r0=count, r1=dst, r2=src. Short counts (<=64) copy through a descending unrolled ladder; longer counts copy ascending. 125 harness cases matched exactly. |
| 0224a518 | soft-float **double subtract** (not __divdf3) | new | MANUAL + emulator harness | Stack ABI: [&res, a.hi, a.lo, b.hi, b.lo]; the callee pops 20 bytes. Bit-level C. 6008 vectors bit-exact vs ROM. Two more entries (0x24a530 b-a, 0x24a5e0 a+b) have no callers. |
| 0222d450 (floor) | skeletal-mesh draw | compound assigns rewritten | MANUAL | **Not a jump table.** The 43 "branches" are bsrf calls. Call/store order for both cull paths was checked by script against the ROM. difftest now SKIPs only on the callee-arity rule. |
| 02225ee0 (floor) | (not a function) | closed | n/a | This is the tail of func_02223ee0, using its frame and epilogue. 02223ee0.c already has C for it, verified by hand. Logged "skip". |
| 02227ca8 (floor) | slot distance tables | new | MANUAL | A normal void(void). The nonstandard ABI belongs to its *callee* 0224a280, which passes two structs by value to cache-purge with length 0. Documented in the header. The only caller is 0220134c. |

### Surfaced by this pass (hidden real functions or bad refs)
| fn | name | status | difftest | notes |
|---|---|---|---|---|
| 0220134c | frame-pipeline step 7 | new | MANUAL | Step table at cart 0x28c278. Calls displaylist sort, 02227ca8, and vdp1 cmdlist. |
| 0220121e 0220123c 0220125a 02201278 022012e0 02201320 | frame-pipeline steps 1-6 | new | MANUAL | Nothing in the C tree reads the step table yet. |
| 02215bf4 | per-frame quad drawlist (twin of 02215cb4) | new | MANUAL | Called by step 6. |
| 02202d7e, 02202dda | 02202d14-family scene bring-up / trampoline | new | MANUAL | Task-handler entries. |
| 02202e84 | scene-run FSM (4 states) | new | MANUAL | Pointer in the scene descriptor table (cart 0x270154/0x270294). Contains 5 embedded pools. |
| 0220fb98 | double-tap direction detector | new | **PASS-void** (also checked by hand) | Was the hidden short ref func_20fb98. |
| 0221654a, 02216582 | record copy + DMA tail | new | MANUAL | Short refs in 022165c6. |
| 02222f16 | text row writer | new | MANUAL | Short ref in 02223142. |
| 02238d5e | falling-trail task | new | MANUAL | Spawn ctor from 0222f4e8/0222f6a8. |
| 02235dc0 | mesh-part emitter | new | MANUAL | 3 callers. |
| 02245bcc | facing test | **renamed from 02245bbe.c** | MANUAL | 0x245bbe is pool data. |
| 022449b2, 02228906, 022016a0, 02201a98, 0220394a, 02208b0e, 02209656, 02225c2c | (bad refs) | refs fixed | n/a | Mis-shortened names, wrong mova sums, or comment-only phantoms. |
| 0223d768 (caller) | | fixed | MANUAL | The soft-float call now models unk70 = dtoi(itod(unk70) - 1296.0) with the real 0224a518 ABI. |
| **02204258** | 3-state FSM handler | **NOT ATTEMPTED** | | Newly referenced by 02202e84. Next in the queue. |
| **022137a0** | mac.l matrix x vec | **NOT ATTEMPTED** | | Newly referenced by 02215bf4. bt/s branches *back* to 0x21377c, so it probably shares a body with code before it. Needs a boundary pass first. |

## Mechanical cleanup
- Canonicalized every short-form `func_<5-7hex>` ref whose canonical file exists: about 30
  names across 43 files. Each one was confirmed against the caller's ROM extent with
  work/opus_tail/reftargets.py, which resolves bsr/bsrf/braf/mova+pool targets.
- Fixed the 9-digit names func_0222220be / func_0222223c8 -> func_022220be / func_022223c8 in 022348cc.c.
- Fixed the 3 non-canonical refs in 02226148.c.
- The 02209188 state-0 handler literal 0x0020B098 -> 0x0220B098 (mova 0x209244+0x1E54),
  to follow the SH2-form convention.

## Known issues left (not fixed here)
1. **Hidden-entry backlog.** work/opus_tail/hidden_scan.py finds 50 targets that some ROM
   word points at (absolute 0x06xxxxxx), that look like an entry point, and that have no file.
   About half show a clean preceding rts/bra boundary. Not all are real; 02202e84 had no clean
   boundary and was real. This is the honest next queue and a good fit for GLM:
   02201020 0220c3e0 0220c858 0220cdf6 0220d424 0220e5bc 02210b58 02218eac 0221a9e0 0221ab1c
   0221b5a0 0221b730 0221b8e4 0221bb40 0221bd08 0221bdce 0221cad4 0221cd58 0221cfa4 0221d59c
   0221d910 0221da60 0221debc 0221df80 0221ec08 0221edec 0221f9c4 02222efc 02228874 02228aa6
   02228ca4 02229054 022294b4 022383f8 022384e4 0223875c 022387e0 022387fc 022388b8 02238960
   02238a9c 0223f95c 02241b9c 02241bee 02241c40 02241d18 02247530 02247580 02248318 02248f86.
   mova+delta handler installs are NOT covered by this scan. 022041d4.c also names a sibling
   at 0x204308.
2. **coverage_callgraph.py** should also flag `func_` tokens that are not 8 hex digits, and
   files that define more than one func_. Otherwise this blind spot comes back.
3. **The tree would not link as a whole.** Many files define descriptive names (mtx_load_abc,
   sound_queue_push, ...) while callers use func_<8hex>, and some globals are declared with
   different types in different files. This predates this pass.
4. **Duplicates and strays.** 0239ce8.c duplicates 02239ce8.c with different content.
   02001b8e.c duplicates 02201b8e.c. project/src_functional/e is an empty tracked file.
   Left for the PM.
5. **difftest coverage (for the difftest owner).** The "callee must have exactly 4 register
   args" rule and the indirect-call rule cause most SKIPs, including nearly everything here.
   F3 also noted that the MACROS table lacks MEMU32 and treats MEM32 as unsigned, while
   dh_band21.h defines it as signed.
6. floors.tsv still lists 0222d450 / 02225ee0 / 02227ca8 as lever-pending, and 0224a518 as
   __divdf3. Suggested change: 0222d450 and 02227ca8 -> done(manual); 02225ee0 -> closed (tail
   of 02223ee0); 0224a518 -> softfloat dsub.

## Helper scripts (work/opus_tail/, uncommitted)
logatt.py (attempt logger), shdis.py (capstone listing + bsrf/braf pool resolution),
reftargets.py (does caller X's ROM extent reference target Y), hidden_scan.py,
f2_verify.py / f2_stats.py (soft-float/blockcopy ROM-vs-C emulator harness),
02227ca8_test.c (difftest-only ABI variant).
