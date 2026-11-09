# Functional decomp batch report (Opus)

Scope: functional (equivalence-oriented) C for un-done leafC / small callC functions.
All new TUs use the shared `dh.h` accessor style and compile clean on host gcc
(`gcc -c -fsyntax-only -Wall -Wextra`, no warnings).

## Functions completed this batch (14)

| cart off | SH2 addr  | file                | name / behavior |
|----------|-----------|---------------------|-----------------|
| 0x201b8e | 0x2201b8e | 02201b8e.c | func_02201b8e - empty stub (rts;nop) |
| 0x20074c | 0x220074c | 0220074c.c | **crc16_buffer** - CRC-16 (seed 0xDEBDEB00, poly 0x1021 hi-half) over a buffer |
| 0x210858 | 0x2210858 | 02210858.c | func_02210858 - register val into g_060E1218[field34], clear g_060E1298[field34] |
| 0x2418ce | 0x22418ce | 022418ce.c | func_022418ce - table dispatch via g_0608DA88[g_0608DA94[fieldBA&0x3F]] |
| 0x237e6c | 0x2237e6c | 02237e6c.c | func_02237e6c - draw-param setup + fn-ptr dispatch (sibling of func_02237e0e) |
| 0x24158c | 0x224158c | 0224158c.c | func_0224158c - retrigger/cooldown timer helper (g_060987BC window) |
| 0x22f84e | 0x222f84e | 0222f84e.c | func_0222f84e - actor pose init: copy vector record from g_06097D58 table |
| 0x219ba4 | 0x2219ba4 | 02219ba4.c | func_02219ba4 - 2-phase actor swing action (ramp a target angle) |
| 0x219ad2 | 0x2219ad2 | 02219ad2.c | func_02219ad2 - 3-phase actor swing/track action |
| 0x22378c | 0x222378c | 0222378c.c | **fmt_uint_base2** - right-justified binary number formatter |
| 0x223968 | 0x2223968 | 02223968.c | **fmt_uint_base16** - right-justified hex number formatter |
| 0x22c220 | 0x222c220 | 0222c220.c | func_0222c220 - gate+arm a special move; returns -1 when armed |
| 0x22a708 | 0x222a708 | 0222a708.c | func_0222a708 - per-frame flag refresh over two slot arrays |
| 0x21af30 | 0x221af30 | 0221af30.c | func_0221af30 - crosshair/aim turn adjust from input bits |

Named (added to notes/symbols.txt, HIGH confidence): crc16_buffer, fmt_uint_base2,
fmt_uint_base16. The rest kept generic `func_0<addr>` (behavior clear, role name not
confident enough to assert).

All 14 were produced from m2c drafts cross-checked against the raw SH-2 asm
(m2c_fn.py --asm) so field widths (M8/M16/M32) and signedness match the loads/stores
exactly, not just the m2c guess.

## Compile status
- 14 / 14 new TUs compile clean on host gcc (syntax + -Wall -Wextra, no warnings),
  matching the established src_functional convention.

## Functional count
- src_functional function files: 186 -> **200** (+14).
- Functional coverage: 14.6% -> **15.72%** (200 / 1272 splitter functions).
- Overall C-written (functional + nonmatching + byte-exact): **261 / 1272 = 20.52%**.
- Note: project/progress.py tracks only the *byte-exact* oracle (build_results.json,
  3 funcs); it does not count functional TUs. The numbers above are the functional
  denominator (fn.units = 1272) computed directly.

## Resisted / deferred (honest list)
Screened the remaining clean leafC pool; these did NOT yield safe functional C this batch:
- **0x21f952, 0x22ea0e** - m2c "missing delay slot" on a tail `bra`/`rts`; tail-branch
  to another unit, needs manual tail-call modeling.
- **0x238f46, 0x238fdc** - m2c "cannot find branch target func_2386fc" (branch leaves the
  segmented extent) -> misseg-suspect.
- **0x21db6e, 0x223dca (isr_timer0), 0x22077e, 0x20e970** - clean bodies but end in a
  `braf` computed dispatch / jump-table tail that m2c can't lower. The bodies are readable;
  worth a follow-up that models the braf target explicitly.
- **0x22f628** - `div1` single-step divide (sibling of 0x22f84e with a leading division);
  needs the full divide-step sequence reconstructed.
- **0x244e50, 0x23cfdc, 0x24537a, 0x24a510, 0x213d90, and the 0x24a39c-0x24a516 cluster** -
  "stubs" that are actually bare epilogue fragments (`add #N,r15; lds.l @r15+,pr; rts`) or
  asm dispatch thunks (braf/jsr through a table with uncaptured register args). These look
  like mis-segmented tails, not real standalone leaves - left for the asm band, not faked.
- **0x23c95c, 0x214e2e, 0x2227c4** - clean m2c but large and pointer-arithmetic-heavy
  (bucket/radix bin builds, 60-80 temporaries). Deferred to avoid hand-transcription bugs;
  these are mechanical and a good next target with more care.

## Recommendation for next band
1. The remaining **clean leafC** (several still in 0x2208xx / 0x224xxx) - fastest wins,
   same method (m2c draft + asm width check).
2. The **braf-tail cluster** (0x21db6e, 0x22077e, 0x223dca, 0x20e970): the engine's actor
   state handlers. Build a tiny helper to resolve `braf r3` (pool delta + PC) to a fixed
   target so m2c's bodies can be adopted - unlocks a whole family of FSM handlers.
3. The large-but-clean copies (0x23c95c, 0x214e2e, 0x2227c4) once (2) tooling exists.
Avoid the misseg band and the 0x24a3xx-0x24a5xx jump-table fragment cluster until the
segmentation there is re-derived.
