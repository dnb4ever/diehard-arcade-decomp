# Functional decomp report - band 0x20fxxx (SH2 0x0220f000-0x02210000)

Worker: band20f. All files written to `project/src_functional/`, header `dh.h`, every file
compiles with `gcc -c -fsyntax-only -Wall -Wextra` emitting ZERO warnings.

## Summary
31 new functions decompiled. Together with the 3 pre-existing files (0220fd06 list_node_alloc,
0220fde4, 0220feb0) this completes **every genuine (non-phantom, non-data) function** in the
0x20fxxx cluster. The only genuine census member left undone is 0x20ffd2, which is a
pool/data-contaminated unit header whose real functions start at 0x210020 (next band, not ours).

## What's in this band
The band is dominated by three subsystems:

1. **Object-node pool/allocator family** (fc18 init; fc3c/fc82/fcc4 allocators; the fd52/fd90/
   fe48 batch helpers; fdc8 prune) plus a set of thin constructors (fece/feec/ff0a/ff28/ff54/
   ff72/ff90/ffae) that each allocate a node and call `func_02214bb8(node,1,SIZE)`. The pool
   control block is at g_06092238 (free head), g_0609223c / g_06092240 (two active-list
   anchors), g_06092244 (sentinel). Nodes are 0x20 bytes; fc18 builds 511 of them.

2. **Object world-transform updaters** (f0ec/f15e/f19e/f202/f26a sprite-attr init family;
   f28e rotated transform; f454 non-rotated sibling; f5b8 debug free-fly camera). These read the
   global stage/source record g_06099584 with a shared selector `g->unk10 + g->unk14`.

3. **Scene-object animation steppers** (f6f6/f880 + shared helper f816) and the **controller
   input** code (f9d4 per-frame state machine, f990 reset, fb58 settle).

## Key findings / corrections
- **The allocator clear-loop runs 8 iterations, not 7.** m2c's structural rewrite of the SH-2
  `tst/bf.s/add #-1` delay-slot loop shows 7 stores; the hardware does 8 (the prior worker's
  list_node_alloc already used 8). fc3c/fc82/fcc4/fe48 all follow the 8-count form.
- **m2c dropped arithmetic in func_0220f28e.** The three "unk14 = arg2->unk0" lines are really
  `obj->unk = func_0224a420(arg2->unk, *g_06096c90)` (ashr_var, arithmetic shift-right), and the
  "unk90 += ..." lines are `+= sdiv32(delta, 4)`. Both resolved from the callees' pinned ABIs
  (func_0224a420: r0=value, r1=count; sdiv32: r1/r0). f454 is the sibling with divisor 8 and no
  shift/matrix.
- **m2c aborted on f454/f6f6/f880** (shared pool-adjacent labels / embedded braf). All three were
  hand-transcribed from the fn.py disassembly and verified against braf_resolve.
- **Phantom / pool-contaminated members skipped** (verified via find_entry + fn.py raw disasm):
  0x20f0ee (2 bytes into f0ec), 0x20f426/f442/f446/f44a (real entry f454), 0x20f59e/f5aa/f5ae/f5b2
  (real entry f5b8), 0x20fb6a (pure pool), 0x20fc16 (real entry fc18), 0x20fe9e (real entry fece),
  0x20ffd2 (data; real functions at 0x210020+, out of band).
- **Two hidden (uncatalogued) functions recovered** - not fn.units members because the preceding
  function's braf/pool swallowed them into the merged extent:
  - **0x20f816** `func_0220f816` - the anim-state helper shared by f6f6 and f880.
  - **0x20f990** `func_0220f990` - the controller button-record reset routine.
- **PIC branch targets all verified with braf_resolve** and cross-checked against existing
  src_functional files for the callees (func_02214bb8, ashr_var/0224a420, sdiv32/0224a06c,
  mtx_push_rotate_apply/0221080a, func_0221055c, mtx_build_fwd_inv/02214a7a, func_0220ad58).
- **Shared data pointer 0x022199da** - the sprite-init family (f19e/f202/f26a) all form the same
  PIC-relative pointer (mova base + const) to ROM record 0x022199da, passed to func_0220ff72.

## Deferred
Nothing deferred. Full genuine coverage of the band achieved.

## Files added (31)
0220f0ec f15e f19e f202 f26a f28e f454 f5b8 f6f6 f816 f880 f990 f9d4 fb58 fc18 fc3c fc82 fcc4
fd52 fd90 fdc8 fe48 fe98 fece feec ff0a ff28 ff54 ff72 ff90 ffae
(all prefixed 0220, .c; see notes/symbols_band20f.txt for names/summaries/globals)
