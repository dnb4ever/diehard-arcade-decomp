# Functional decomp batch 5 (Opus) — PIC data-pointer resolver + generator/boot/VDP2 unlocks

Direct follow-on to batch 4. Task-1 (the key unlock) was the **mova/add DATA-pointer resolver**
batch 4 asked for; task 2 was the generator/spawn-template family; task 3 the port-relevant
boot/VDP2 init cluster. All new TUs use the `dh.h` accessor style, were cross-checked
field-by-field against the raw SH-2 asm (`m2c_fn.py --asm`), and compile clean on host gcc
(`gcc -c -fsyntax-only -Wall -Wextra`, **zero warnings**). Core read-only tools (m2c_fn.py /
fn.py / match.py) untouched.

## 1. NEW TOOL — tools/decomp/ptr_resolve.py (task 1, the key unlock)

The SHC PIC idiom for forming an absolute DATA/FUNCTION pointer is:

```
    mova  .Lanchor, r0        ; r0 = ADDRESS of a pool slot (cart offset A) — value AT it is junk
    mov.l .Ldelta,  rX        ; rX = signed 32-bit DELTA from the literal pool
    add   r0, rX              ; rX = A + DELTA  (the absolute target)
```

`ptr_resolve.py` disassembles a function, finds every `add rA,rB`, back-traces both operands
(following `mov` copies), and when one traces to a `mova` anchor and the other to a pool/immediate
delta, reports `target = anchor_addr + delta` (SH2 = `+0x02000000`). A `mova ... ; add #imm,r0`
variant is handled too. It is the same math batch 4 did by hand for next-state pointers and spawn
templates — now automatic.

**Usage:** `ptr_resolve.py 0xADDR [...]` (cart offsets); `--all 0xADDR` also lists the
braf/bsrf branches (via braf_resolve). It classifies each target as in-ROM code/table, HWRAM
global, or HW register.

**Validation (reproduces batch-4 hand resolutions EXACTLY):**
```
0x22a970 : add@22a9ec  anchor 0x22a9ec + 0x12       -> 0x0222a9fe   (next-state ptr)
0x22a9fe : add@22aa8e  anchor 0x22aa90 - 0x4c6      -> 0x0222a5ca   (next-state ptr)
0x202c5c : add@202c9a  anchor 0x202c9c + 0x770      -> 0x0220340C   (spawn template)
           add@202caa  anchor 0x202cac + 0x14e      -> 0x02202dfa   (next-state ptr)
```
All four match batch 4's hand notes to the hex. It then recovered, with no hand work, the spawn
templates and fn-ptr-table bases that batch 4 had deferred (see §4 for the spawn-list payoff).

## 2. Functions added this batch (7) — all compile clean, all new addresses (+7 real)

### Generator / spawn-template family (task 2)
| cart off | file | behaviour |
|----------|------|-----------|
| 0x239fc6 | 0239fc6.c | **actor_state_gen_main** — the main combat-state generator. `cmd = g_060E0F0C[unk94]`; clears the queue mark `g_060E0F2C[unk94]` (the inverse of actor_queue_mark 0x23a394). Switch on unk88 (0x4F-0x53): each case swaps the saved anim words between the live actor record `g_06096248[unk34]` (unk8/unkE) and the save array `g_060E11E8[unk34]`, installs the new action into the grab slots unk6C/6D/6E/6F (left vs right arm), ORs the type mask `g_0604EBE4[cmd]` into unk8C, loads the next anim word from `g_060E1148`/`g_060E10A8` (cmd*8 + unk68*2), and cases 0x52/0x53 fetch the frame count from the move-script pointer table `g_06091678`. Shared tail force-releases a stuck grab (unk6D==7 && unk70) back to action 0xC, calls func_0223b254, and for player slots (unk34<3) restores the hit-anim `g_06096C74[unk34] = g_0608CE74[unk6D]`. Returns a small int → caller does `state = ret + 0x21`. |
| 0x23b01e | 0223b01e.c | **actor_state_gen_throw** — throw/projectile spawn generator; returns 6 (caller → state 0x27). Spawns via `spawn_obj_a(g_06099580, template@0x0223D1B6)` [ptr_resolve], seeds the projectile's world pos from the owner's slot record `g_06097D88[unk34]`, facing-based velocity unk14, then validates the action via func_0223cbb0 (bails if NULL) and latches the throw into the actor + anim slots. |
| 0x229488 | 0229488.c | **actor_hit_state_step** — hit/grab state handler (entity=r5, ctx=r4); switch on unk88 (0x45-0x49). Case 0x47 **spawns** a partner/effect actor via `func_0220ff72(g_06099580, template@0x02226EB8)` [ptr_resolve] and links `spawned->unk8->unk10 = entity`, `->unk14 = g_060995B0[unk4B]->unk8`. Other cases restore anim / reset to idle / arm timers; tails into func_02216110 / animation_update / actor_pose_dispatch. |

### Boot / VDP2 init cluster (task 3 — port-relevant)
| cart off | file | behaviour |
|----------|------|-----------|
| 0x201b8e | 0201b8e.c | **noop_handler** — empty `rts;nop`. A shared default/no-op callback: func_02016a0 installs its address as a default fn-ptr slot three times, func_0201a98 once (explains those PIC mova+add slots). |
| 0x201b92 | 0201b92.c | **boot_input_region_init** — sibling of boot_globals_init. Inits sound (func_02215f24/0220f990); region gate via `g_060511DC[(g_06000248>>12)&0xF]`; reads the peripheral buffer through the BIOS callback `*0x06000660` and sets controller-type flag g_06098668 (status byte unk10 + ids unk3/unk7); language `g_06099794 = g_060511D2`; config byte RMWs `g_060987EC[g_06098808]`; sound-enable `g_060E0DDC = (g_06000798 & 0x40) ? 1 : 0`. Called by the boot state machine func_02016a0. |
| 0x201c9c | 0201c9c.c | **vdp2_update_a = screen_fade_sequencer** — CRAM/palette fade. Ramps fade level `g_06098668` 2→10, computes palette base `g_06074000 + level*0x50`, applies it each frame via func_0220fece (or hands it to the BIOS fade `*0x0600060C` when the fast-path config bit is clear); on completion enables the VDP2 display (TVMD reg 0x25F80000 = shadow `g_06098F08->unk0 & 0x7FFF`) and plays a sound. func_0220fdc8 target recovered by ptr_resolve (mova 0x201cb4 + 0xE114). |
| 0x201f20 | 0201f20.c | **vdp2_update_b = scene_display_setup_seq** — 3-state (task->unk10) VDP2 setup. State 0 does the full VDP2 register-shadow (`*g_06098F08`) program (unk0/unkE/unk10..unk1C/unkE0/unkF0..unkFC) + subsystem kicks (func_02222d3c, HWRAM hook *g_06090294, vdp1_draw_kick, func_02227a84/0223c95c/02216c38/02207604); state 1 counts down then turns the display on (TVMD); state 2 tails func_0220fdc8. |

## 3. Resolver validation evidence
See §1 — `ptr_resolve.py 0x22a970 0x22a9fe 0x202c5c` reproduces all three batch-4 hand cases to
the hex. Additional in-batch confirmations where the recovered pointer was cross-checked against
the asm and the result is a plausible template/function: 0x23b01e→0x0223D1B6, 0x229488→0x02226EB8,
0x23a3a2→0x02227906, 0x23ac9c→{0x0223d2a0,0x0223d3d0,0x0223d558,0x0223d1b6, spawn_obj_0x7c 0x20ff28},
0x201c9c→0x0220fdc8.

## 4. Spawn system insight (for the port's empty-spawn-list probe)
The generator/handler spawns all go through the same shape:
`spawn = spawn_obj_*( g_06099580 , <ROM template ptr> )` then the spawned task's **entity**
(`spawn->unk8`) is seeded. Observed call sites: actor_state_gen_throw (0x23b01e, spawn_obj_a +
template 0x0223D1B6), actor_hit_state_step (0x229488, func_0220ff72 + template 0x02226EB8),
actor_state_release_gen (0x23a3a2, spawn_obj_a + template 0x02227906), actor_state_gen_special
(0x23ac9c, spawn_obj_0x7c + several templates).

- **`g_06099580` is the single spawn pool/list context passed to every spawn.** The value is read
  (`mov.l @0x06099580`) and handed to the allocator as its first arg. If that allocator returns an
  empty/NULL object (pool exhausted or `g_06099580` not yet initialised), the whole spawn chain
  stalls — the seeded fields are written through a bad pointer or skipped.
- `g_06099580` is NOT written by any TU in this batch (0x201b92 clears the unrelated g_06098580).
  It must be set up elsewhere before process_spawn_list (0x0222A5D0) runs. The port side should
  check **who initialises g_06099580 (and the freelist it points at) and whether the boot-bypass
  reaches that init**. If it doesn't, every actor spawn draws from an empty/NULL pool → the empty
  spawn list the parallel investigation is chasing. The generators here only *consume* g_06099580.
- The per-slot command plumbing is also now visible: `g_060E0F0C[slot]` holds the pending command
  and `g_060E0F2C[slot]` the mark; actor_queue_mark (0x23a394) sets the mark, actor_state_gen_main
  clears it. An empty command queue (`g_060E0F0C` all zero) would make gen_main fall to its default
  and spawn nothing.

## 5. Resisted / not-taken (honest)
- **0x23a3a2 actor_state_release_gen** (task 2): switch on unk88 (0x52/0x53) sets `var_r13` from
  unk6C/unk6D, but the **default path (unk88 ∉ {0x52,0x53}) falls into the shared tail which reads
  `var_r13` uninitialised** (m2c emits `saved_reg_r13`). Correct C needs the caller's guarantee that
  unk88 is always 0x52/0x53; can't verify this sitting. Deferred (same class as batch-4's 0x229988
  register-provenance defer). Its spawn template (0x02227906) is already resolved for next time.
- **0x23ac9c actor_state_gen_special** (task 2): `fn.extent` **over-runs its rts** (at 0x23b01a) and
  swallows the following functions 0x23b01e + 0x23b1c0 (its reported extent 0x5ee reaches 0x23b28a;
  real code ends at 0x23b01e). Decompiling it cleanly needs manual asm truncation at the rts, which
  I couldn't verify safely without tool changes. Its 6 data pointers ARE resolved (see §3) so a
  follow-up with a bounded extent is cheap.
- **0x2016a0 boot state machine** (task 3): the core is a **10-way `braf` jump-table** dispatch on
  task->unk10 (m2c: `M2C_ERROR unknown instruction: braf $r0`), plus unset-register reads in the
  BIOS `*0x0600060C` call. Per the table-dispatch constraint this would be a switch/stub, not a
  faithful body; deferred. (It does confirm the boot order: unk10 states install func_0201b8e as a
  default handler, run boot_input_region_init 0x201b92, then vdp2_update_a 0x201c9c, then
  func_022058a8 — useful for the port's boot-bypass question even without the full body.)
- **Small actor state-step runway** (0x2296da, 0x228d76, from scan_family): m2c loses the entity
  base (shows bogus `0x4F22` / `-0x2D3C` constants + `saved_reg`/`M2C_LOAD_SR`), i.e. the entity is
  reached via a GBR-relative or pool-global base m2c doesn't model. Deferred until that base pattern
  is handled.

## 6. Real counts (tools/decomp/venv/bin/python project/progress_functional.py)
```
functions total (fn.units) : 1272
  functional  (src_functional):  244    19.18%   (was 237 -> +7, all new addresses)
  nonmatching (src_nonmatching):   73     5.74%
  byte-exact  (src files)      :    5     0.39%
  byte-exact  (oracle EXACT)   :    3     0.24%
  ANY C written (deduped)      :  322    25.31%   (was 315 -> +7)
```
7 new verified .c files, net +7 (no supersedes this batch). Lower than the 20-30 target by count,
but each generator/VDP2 handler is large and every file is fully asm-verified; two generators and
two boot state machines were deferred rather than shipped unverifiable (§5).

## 7. Next-band recommendation
1. **Finish the generator family** now that ptr_resolve has decoded their pointers:
   - 0x23ac9c: add an explicit code-end override (e.g. a `--end` arg to m2c_fn/braf_rewrite, or a
     one-off bounded-extent helper) so its rts at 0x23b01a isn't over-run; then transcribe.
   - 0x23a3a2: resolve the default-path `var_r13` by reading the single caller (0x22979c) to confirm
     unk88 is always 0x52/0x53 (then the default is dead code and it's writable).
2. **The GBR-relative entity-base pattern** blocks a whole band of small actor state-step handlers
   (0x2296da, 0x228d76, and likely many 0x228xxx/0x229xxx). Worth a small m2c-input rewrite (map the
   GBR base to a known global) or a note so these can be hand-transcribed with the right base. This
   would unlock the largest remaining runway cheaply.
3. **Port side: trace the writer of `g_06099580`** (the spawn pool/list head) and whether the
   boot-bypass reaches it before process_spawn_list — this batch shows every spawn consumes it, so
   it's the prime suspect for the empty spawn list (§4).
Still avoid the misseg band and the 0x24a3xx-0x24a5xx jump-table fragment cluster.
