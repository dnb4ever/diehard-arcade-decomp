# Functional decomp - Wave 1, band 0x220000-0x228000 (cart) = runtime 0x02220000-0x02227fff

Dual SH-2 (big-endian), equivalence-oriented (not byte-matching). Every file below compiles with
`gcc -c -fsyntax-only -Wall -Wextra -I.` producing ZERO warnings, and each was hand-verified against
`m2c_fn.py --asm`. PIC branches resolved with braf_rewrite/ptr_resolve; globals/regs declared per-TU.

## Functions ADDED this wave (20, all HIGH fidelity)

Engine / display-object transform:
- `0222078c` func_0222078c - ballistic sub-object task update (seed-on-first-frame, gravity `+0x18 -= 0x100`,
  spin, 60-frame life / Y<0 expiry), tail-calls renderer 0x022153f6.
- `02221460` / `022214d2` func_02221460 / func_022214d2 - configure interpolated transform slots 0/1 on
  *0x06098F08 (weights via lerp helper 0x02213234). Note: the ROM saves MACL, so m2c's "arg5/arg6" are
  really the 5th/6th stack args - 6 args, all used.
- `02221f68` / `02222052` func_02221f68 / func_02222052 - mode 4/5 and 6/7 setup (reset transform / clear
  buffer, then builder 0x0221636c). **Both pass a 5th stack argument to the builder that m2c dropped** -
  recovered from asm (element size 0x1C for 4/5; arg2 for 6/7).
- `022220b0` func_022220b0 - thin tail-call wrapper to func_02213290(handle, count>>2).
- `02222494` func_02222494 - command-list interpreter splicing {cmd,val} pairs into the packed 16-bit
  bitfields 0x3A/0xB2/0xB4 of the display object (all 9 mask/shift cases verified).
- `022225f6` func_022225f6 - view-transform builder: sin/cos (0x02213190/0x022131a8) rotation matrix via
  sum/difference identities + camera-relative (0x060755D4) position, into 0x060B8B90/94; tail func_02222c08.

VDP2 text subsystem:
- `02222d3c` / `02222e44` func_02222d3c / func_02222e44 - per-mode VDP2 setup (DMA palette/gradient to CRAM,
  upload tiles via 0x022145d0, install 4 draw callbacks, flag ready). Callback slots hold absolute code
  pointers (two land mid-thunk-block at 0x02222e..).
- `02222ecc` func_02222ecc - draw-callback thunk (clear 0x400-word buffer, tail-call 0x02213290).
- `02222fe0` func_02222fe0 (text_write_tilemap) and `02223080` func_02223080 (big-font sibling) - emit a
  string into the VDP2 nametable with a persistent wrapping cursor; return glyph count. Byte-level pointer
  arithmetic and the col-64/128, block-wrap logic preserved.

Entity system (root 0x06099584 / context 0x06099580):
- `02227790` func_02227790 - constructor (alloc owner, publish context, install handler 0x02227830,
  clear slots, reset, tail func_0223c95c). The bytes past the braf tail are the 0x227830 handler, not this fn.
- `02227a84` func_02227a84 - full reset (8 slots, drop owned pointers, clear 8x8 relation matrix to -1, aux
  words, 24-byte block at 0x060996D4). Matrix cleared with clean row/col loops; final state matches ROM.
- `02227b60` / `02227bc4` func_02227b60 / func_02227bc4 - register spawned object (dynamic free slot / fixed
  id arg0+1) into the table; stamp slot id into obj's sub-struct (*(obj+8)+0x34).
- `02227602` func_02227602 - per-entity animation/interaction step (frame record from 0x060862F4[], input-bit
  latch, dwell counter clamp 0x1F).
- `02227906` func_02227906 - "handoff/throw" body: 4-phase state machine (borrow transform, drift, quarter-
  step ease toward 0x06087ECC target, expire); every frame tail-calls func_022154fc.

Timing:
- `02223c74` func_02223c74 - pulse-width timing calibration over polled byte 0x060986AF (modelled volatile).

## RESISTED (deferred, with reason) - still in band, NOT shipped

- `0222109e`, `02220828`, `02220ade`, `02220e2c` - command-list interpreters driven by a **0x33-entry PIC
  `braf` jump table** (register loaded from a runtime table, not a single pool constant). braf_resolve reports
  these as table-driven; m2c emits garbage after the braf. Prologue (set *0x06098F08 +0x20 bit, loop {cmd,val})
  is clear, but the 0x33 per-command VDP handlers cannot be verified without reconstructing the table. Needs a
  dedicated jump-table pass.
- `0222156c`, `0222184c`, `02221bd4` - heavy users of the bit-reader func_0224a420 (near the 0x24a360 band).
  m2c loses whole registers ("Read from unset register $r4/$r5/$r7") around the nested dispatch, so the field
  writes are unverifiable as emitted. Need manual register tracking.
- `02223142`, `02223460` - printf-style format engines over a large on-stack context struct that m2c cannot
  allocate (renders as `sp->unk140` etc.). `02223862` is their integer-formatter helper (m2c lost the r6/r7
  context/buffer pointers). The whole printf family wants one focused pass together.
- `02225ee0` - m2c unset-register ($r0) plus an unmodeled stack parameter.
- `02225fac` - entity-trail recorder with mis-typed 6-byte-stride index casts; recoverable but needs a careful
  asm pass to pin the record layout.
- `02223ee0` - exactly 0x2000 bytes; a single very large function (real prologue, not misseg). Almost certainly
  contains table dispatch; dedicated pass.
- `02226148` (0xd70) and `02226eb8` (0x72c) - large entity interaction-logic functions. Coherent (no
  M2C_ERROR) but too long to hand-verify reliably in this wave; dedicated multi-pass.
- 0x022220b0's extent also contains an **unreferenced** command-list interpreter sharing its literal pool
  (no recognised entry); left as band remaining.
- `02222ee2`, `02222ed6`, `02222efc` - mid-block draw-callback thunk entries (installed by d3c/e44); real
  entry points but not segmented by find_entry; left for completeness later.

## EXCLUDED per instructions
- `02221354`, `02221384` - non-standard r0/r1/r2 ABI calling the banned 0x24a360 dispatch band. Not touched.

## Band remaining (approx)
~14 functions deferred above + the large 223ee0/226148/226eb8 + the hidden 2220b0 interpreter. The clean,
single-target-resolvable members of the band are now done; what remains is dominated by (a) the 0x33-entry
braf jump-table family, (b) the func_0224a420 nested-dispatch family, and (c) the printf family - each best
handled as a dedicated focused pass rather than piecemeal.
