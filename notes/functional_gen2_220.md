# Functional decomp - Gen-2, band 0x220000-0x228000 (cart) = runtime 0x02220000-0x02227fff

Single worker, no forks. Equivalence-oriented (not byte-matching). Every file below compiles with
`gcc -c -fsyntax-only -Wall -Wextra` producing ZERO warnings, and each was hand-verified against
`m2c_fn.py --asm` (not trusting m2c's C where it emitted M2C_ERROR / spurious pool arithmetic).
PIC branches resolved with braf_resolve / ptr_resolve (trusting resolved target addresses).
find_entry.py was run first on every candidate to discard pool-contaminated phantoms.

## Functions ADDED this wave (9, all HIGH fidelity)

Task state machines (engine frame executive):
- `022201d8` func_022201d8 - scanline/strip VDP1 upload state machine. dest pointer computed as
  0x25F00000 + (rec.off>>1)*4; cell assembled hi/lo per a flag; uploaded via bsrf 0x0221632e; stops
  at a 0xFFFF terminator. Faithful note: work->unk64 (loaded each call) is the record ptr in state 0
  and the VRAM ptr afterwards - the aliasing is reproduced exactly as the ROM does it.
- `022202ca` func_022202ca - preset-driven launch/animation task. Snapshots g_060755A4 (s16[4]) and
  g_060755AC (4 x 8-byte records) onto the stack, indexes by preset; 4-state countdown; tail 0x022154fc.
- `022203ce` func_022203ce - scripted camera + SFX-cue timeline task. state1 fires paired sound cues at
  frames 62/122/182/360/422/425/430 through func_0215f6c (all gated by SFX-enable g_060E0DDC); when the
  camera-ready bits are set it installs continuation handler func_0222078c via func_0220ff54; per-frame
  it steps the angle, runs func_0221080a, adds g_06098B1C[0..2] camera deltas, tail-calls func_0221b330.
  jsr @r12 and the handler pointer confirmed with ptr_resolve (0x0215f6c, 0x0222078c).
- `022205fc` func_022205fc - projectile/animated-object physics task; seeds work on state 0, each frame
  calls func_0221400c(unk7C, unk60, &unk8, &unk24), integrates phase/position, tail-calls func_0221b330.
- `022206ae` func_022206ae - phased swing/rotate animation task. **This function is NOT a separate triage
  entry** - it was hidden inside the 0x022205fc compilation unit (triage/fn.extent merged the two; the
  unit has two distinct prologues). Preset from g_060755CC[*g_06098668 - 76]; phase1 waits 30 frames,
  phase2 adjusts unk26/unk14 over 16 frames, then steady; tail-calls func_022154fc.

Text / formatting subsystem:
- `02223142` / `02223460` func_02223142 / func_02223460 - printf engines for normal / large font. They
  format into a local line buffer and render via text_write_tilemap (0x02222fe0) / text_write_big
  (0x02223080); newline flushes the line (func_02222f16 / func_02222f34). Specifiers %%, %d, %u, %b,
  %x, %X, %s dispatch to the shared formatters 0x02223862/0x0222378c/0x02223968/0x02223a52. The ABI is
  unusual: the first four register args are unused and the format string is the first *stack* argument
  (sp+0x140, after 8 saved regs + 288-byte frame), followed by 4-byte varargs - modelled with 4 unused
  params + `const char *fmt, ...`. Field-width ('0' flag + up to two digits) is parsed and consumed but
  deliberately NOT applied (the ROM's width stores are dead); 3+ width digits or an unknown specifier
  abort with return 0. The two are byte-twins differing only in the flush/render callees.
- `02223862` func_02223862 - signed/unsigned decimal formatter (%d / %u backend). Digits built backward
  with the unsigned runtime div/mod helpers 0x0224a120 (x/10) and 0x0224a1c8 (x%10) (both verified to be
  div0u/div1 unsigned routines, args in r1/r0); emits optional '-', left-pads to the emit-context field
  width (pad char '0' or ' '), then the digits. Zero is a single '0'.

Hit / interaction subsystem:
- `02225fac` func_02225fac - append up to two per-slot interaction/shot records into the ring region
  g_06096CB0 (0x3C bytes/slot, 6-byte entries), tracking count/limit in g_06096E90/EB0 and attributes in
  g_06096C74/EA0; sets an overflow flag (arg0+0x52) when the slot is full. Leaf (no PIC). Same array
  family as wave-1's func_02227602. The ROM's (s8)-truncated entry-offset arithmetic is kept verbatim.

## Resisted / deferred (with reasons)

- `02221bd4`, `0222156c`, `0222184c` - family of VDP gun/light color-register builders writing packed
  6-bit fields of *g_06098F08. Each is dominated by many calls (33 / 17 / 33) to the variable
  arithmetic-shift helper 0x0224a420 with **per-site, sign-dependent shift amounts** plus a 0xDA200000
  fixed-point bias added to each component before masking. m2c corrupts every call site
  (spurious "+0xDA200000", M2C_ERROR bsrf). Faithful reconstruction means hand-tracing all 83 shift
  sites and is a focused effort best done as its own pass on the three together (they share the helper
  and field-packing idiom). Deferred to keep fidelity high.
- `02226eb8` - ~1836-byte (0x72c) per-frame processor with an embedded computed jump table
  (braf r0 via mov.w @(r0,r1)) and calls to 0x2449b2 / 0x22ad7a / 0x210020. Large and jump-table-driven;
  deferred rather than risk an unverified reconstruction.
- `02225ee0` - has **no prologue of its own** (first op reads r0; body reads deep stack slots @(50/52,r15)
  set up by a caller it shares a frame with, and tail-branches to func_225c8c). It is a shared-frame
  continuation/branch target, not a standalone ABI function - not safe to emit as a C function. Deferred.
- `02221354` / `02221384` - explicitly out of scope (non-standard ABI; not touched).
- Pool-contaminated phantoms rejected by find_entry (NOT functions): 0x2223cce (-> done 0x223cf0),
  0x222c06 (-> 0x221bd4), 0x222dec (-> done 0x222e44), 0x222f5c (-> done 0x222fe0), 0x223abe (-> done
  0x223c20), 0x223c56 (-> done 0x223c74), 0x227a72 (-> done 0x227a84), 0x22154e (-> 0x22156c),
  0x227c86 (-> done 0x228106, out of band).

## Remaining genuine undone in band after this wave
- 0x02221bd4, 0x0222156c, 0x0222184c (color-register builder family - see above)
- 0x02226eb8 (large jump-table processor)
- 0x02225ee0 (shared-frame continuation - likely should be folded into its owning function)
- 0x02221354, 0x02221384 (reserved, non-standard ABI)
- misseg/phantom data blobs in range (0x2220828, 0x2220ade, 0x2220e26, 0x222109e, 0x2223ee0,
  0x2226148) were not pursued (triage marks them data-as-code; most are tables).
