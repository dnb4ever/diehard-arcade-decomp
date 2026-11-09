# Functional decomp - large-deferral finishing wave

Single worker, no forks. Targets: the two documented large deferrals (0x226eb8 and
0x206308 from notes/functional_singles.md) plus tractable siblings. Workflow:
find_entry first; m2c_fn.py --asm + a custom pool/table-skipping disassembler for the
computed-jump units; braf_resolve / ptr_resolve / pool_read for every computed call and
data pointer; each .c compiles at `gcc -c -fsyntax-only -Wall -Wextra` with ZERO
warnings and was hand-verified line-by-line against the assembly.

## Functions ADDED (3 files, all compile clean)

### 0226eb8.c - actor_clash_state (0x02226eb8)  [the big documented 11-way table - DONE]
The documented target whose jump table was decoded but handlers were "pending". Now
fully reconstructed. It is a per-frame task handler for a two-actor "clash"/tie-up
interaction (attacks or grabs colliding). task = arg0; the two combatants A = pair->16
(r14) and B = pair->20 (r13) are held as persistent callee-saved cursors across the
whole body, with r8 = animation_update (0x0222dc2c, via `mova 0x22705c; add #0x6bd0`),
r9 = 1, r12 = 0.

The 11-way computed `braf` at 0x227072 (base 0x227076) was decoded by reading the 16-bit
jump table at 0x227088 directly from ROM (m2c rendered the braf as linear fallthrough and
mis-disassembled the 22 table bytes):
  state 0->0x22709e, 1->0x2270c4, 2->0x2271d0, 3->0x2272f8, 4->0x22736a, 5->0x2273a4,
  6/7->0x227444, 8->0x22748e, 9->0x2274d2, 10->0x22759c. (state 0 FALLS THROUGH to 1.)
Preamble (~90 lines): bumps counter A->unk50; builds per-actor combo descriptor
desc = 0x06098734 + (s8)((unk34-1)*24) and a flag (unk34 < 3); on a priority gate
(global mode 0x06098670 == 1 and clash counter 0x06098830 >= 7) latches a hit record
(clamps 0x06098830 to 7, sets +13 of *0x0609882c, writes both unk34 and 0 into
*(*0x0609882c)->unk8 at +0xAE/0xB0/0xB2); and if A->unk48 != 0 && A->unk4A != B->unk34,
fires the PIC handler (table 0x060911BC indexed by 0x0609979A) and forces state 10.
Calls resolved via braf_resolve: func_02249b2 (0x2449b2, returns, ==1 tests),
func_0222ad7a (0x22ad7a), rng_next (0x210020), sibling bsr func_022275e4 (0x2275e4) and
func_0227602 (0x227602). The PIC dispatch appears 3x. fidelity HIGH (structure);
routine NAME is a GUESS. State 2 can set state 4 then fall through and have it overwritten
by state 3 or 5 - this is faithful to the asm (`bra` into the shared tail), noted inline.

### 02275e4.c - actor_combo_reset (0x022275e4)  [sibling helper]
Leaf. Clears the per-actor combo scratch (halfword slot unk34*2) across the four tables
0x06096E90 / 0x06096CA0 / 0x06096EA0 / 0x06096EB0 and resets actor->unk52. m2c-clean;
table bases from pool_read. HIGH.

### 0227602.c - actor_combo_advance (0x02227602)  [sibling helper]
Per-frame combo-input advance for actor_clash_state state 2. desc r4 (input bits at
desc->unk4), owner r5, partner r6. Builds a move-step pointer mv = *(0x060862F4 + sel*4)
+ step*8 (sel = 0x06096EA0[i], step = 0x06096CA0[i], i = owner->unk34*2); on a matching
input bit commits unk5C codes to both actors and latches owner->unk52; on owner-hit
advances/reseeds; bumps timer 0x06096EB0[i] clamped to 31. No computed branches; m2c
decoded cleanly, hand-verified vs --asm. HIGH; NAME is a GUESS.

## RESISTED / DEFERRED (with reasons)

### 0x206308 -> real function 0x0220600e (scene/attract director)  DEFERRED - NOT SHIPPED
KEY FINDING: the requested target 0x02206308 is NOT a standalone function. find_entry
reports it "already real" only in the sense that it is a clean instruction boundary; it is
the MIDDLE of phase 2 of a larger routine. The extent tool, started at 0x206308, stops at
the first shared epilogue it reaches and so reported a ~0x560-byte "function" - that is a
tooling artifact. The actual function:

  * TRUE ENTRY 0x0220600e (`mov #16,r0` feeding the dispatch, then the prologue pushes
    r8-r14 at 0x206010). SINGLE epilogue at 0x02206856 (add #12,r15; lds.l pr; pop
    r8-r14; rts). So 0x20600e .. 0x20686a (~0x85c bytes) is ONE function.
  * It is a 4-phase scene director dispatching on MU8(arg0,16):
      state 0 -> 0x206064 : heavy VDP/sprite init via *0x06098F08 (read-modify-write of
                 attribute fields +0/+0xE0/+0xF0..+0xFC), ~15 subroutine calls, zeroes
                 game-state globals, sets mode globals (0x06098670/0x060987D0/0x0609861C/
                 0x06099578)=1, spawns the scene object (func_0220fece, PIC 0x021000f8),
                 sets 0x06098580 |= 0x20000, advances state to 1.
      state 1 -> 0x206242 : if (*(r12+60) & 2) call *0x06000680 (1) and set state 2.
      state 2 -> 0x20629e : the text-layout block (CONTAINS the requested 0x206308). Sets
                 draw param globals 0x060997A4 / 0x060997A8 then issues many conditional
                 string draws through *0x060902A0 and *0x0609029C; flag-gated spawns via
                 func_0220fece with PIC 0x021001ce.
      state 3 -> 0x20625a : conditionally falls into state-2 code, else a small block.
      default -> 0x20684a : common tail - call *0x06090298; set 0x0609869C = 1.
  * r8-r14 are reused as region-local cursors with DIFFERENT values per phase (r8 = 2 in
    phases 1/3 as a bitmask but = 0x060987D8 object in phase 2; r9 = 0x06099578 in phase 0,
    = 0x0609029C draw-fn-ptr in phase 2, = -20 in the spawn section; r10/r13 likewise), so
    r14 = 1 constant and r10 = 0 constant. m2c cannot track any of this.

WHY DEFERRED (not merely hard): beyond the r8-r14 aliasing, the text phases make ~15
conditional string draws through *0x060902A0 whose calling convention passes arguments on
the stack (push longs via mov.l @-r15; `add #8,r15` cleanup) rather than in r4/r5 - i.e.
the draw-function ABI is ambiguous for a functional model, and the phase-2/tail branch mesh
(flag checks on 0x060987DB/0x060987DF/0x06098674/0x06098675/0x06098810/0x06098580 and 3
separately-gated spawns) is a ~430-instruction graph. Shipping this at the project's
"zero-warning, hand-verified, no stubs" bar requires faithfully resolving all of it;
delivering it unverified would violate "don't ship unverified". FULLY RESOLVED for the
next pass so it can be completed without re-deriving:
  Computed calls (braf_resolve on 0x20600e):
    0x206068->0x216154(r4=-1)  0x2060a8->0x222e44(r4=7)  0x2060b8->0x214d26
    0x206132->0x2150d4  0x20613a->0x215228  0x206142->0x223ac0  0x20616c->0x227a84
    0x206172->0x23c95c (= hud_round_data_init, already decomped)  0x20617a->0x207224
    0x206182->0x207772  0x20618a->0x2082be  0x2061ac->0x20fece (spawn_obj)
    0x206294->0x20fdc8  0x2066a0/0x20675a/0x20682c -> 0x20fece (the 3 documented spawns)
    (the braf_resolve "0x20620c via stc sr" hit is spurious - it is literal-pool data.)
  PIC data pointers (ptr_resolve on 0x20600e):
    0x2061aa->0x2100f8 (phase-0 spawn handler)   0x20614e->0x20686a (in-ROM data table)
    0x206272->0x204868   0x20669e/0x206756/0x20682a -> 0x2101ce (the 3 documented spawns)
  Indirect RAM vectors called: *0x06000660, *0x06000664, *0x06000680, *0x06090294,
    *0x06090298, *0x0609029C, *0x060902A0 (draw); status halfword *0x06000654.
