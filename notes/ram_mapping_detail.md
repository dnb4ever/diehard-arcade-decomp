# Die Hard Arcade (ST-V) - cart -> HWRAM copy mapping (RESOLVED, main segment)

Hitachi SH-2, big-endian. "cart" = offset into `cart.bin` (SH2 = off + 0x02000000).
Companion code: `notes/rammap.py` (`ram_to_cart` / `cart_to_ram`, self-testing).

## TL;DR
The game program is copied to HWRAM as a **single linear block**:

    cart 0x200000 .. 0x28de20   ->   HWRAM 0x06004000 .. 0x06091e20
    RAM = cart_off + 0x05e04000            (cart_off = RAM - 0x05e04000)

i.e. **cart 0x200000 lands at RAM 0x06004000** (not 0x06000000 - the first
0x4000 of HWRAM is BIOS-reserved). This was PROVEN by an unambiguous anchor and
cross-checked against every in-range indirect-call target (0 failures).

Confidence: **HIGH** for the main segment. The low (BIOS) and high (runtime
data) regions are characterised but are not part of any static boot copy.

## 1. Entry / boot flow
* ST-V master-SH2 entry = **SH2 0x02200100** (cart 0x200100):
  `mov.w [0x200106],r0 ; braf r0 ; nop`. The literal is 0x0cfa so the braf
  computes 0x200e00 - but 0x200e00 is the **region-name string table**
  (AFRICA/TAIWAN/EUROPE/... each row is `bra +0x20; nop; "<name>"`). The real
  boot routine is the PIC stub beginning at **cart 0x20013c** (clean prologue
  `mov.l r14,@-r15 ; mov #0,r3 ; sts.l pr,@-r15`); 0x200100 reaches it after the
  embedded license string at 0x200108..0x20013b.
* The stub is a **position-independent** Sega security/HW-init library. Its idiom
  is `mov.l [pool],Rn ; mova <next>,r0 ; add r0,Rn` and `mov.l [pool],Rn ; bsrf Rn`
  where the pool word is a **relative delta** (absolute = runtime_pc + delta), so
  it runs correctly from any load address. Pool words that look like garbage
  pointers (0xe3d40600, 0x4d7c0000, 0x2cb00000 ...) are these PIC deltas, not
  addresses.
* It issues BIOS calls through the dispatch pointer at **SH2 0x03000000**
  (= cart 0x1000000): `mov.l [0x03000000],r2 ; jsr @r2 ; mov #0x40,r4`. The table
  at cart 0x1000000 holds small BIOS vector offsets (0x4000, 0x120, 0x500, 0x504,
  many 0x508, 0x572, 0x223a, 0x536) - it is the **BIOS call interface**, not a
  copy-descriptor table.
* **Security CRC** at cart 0x20074c: CRC-16-CCITT (seed 0xdebd..., poly 0x1021)
  run over cart banks (driver at cart 0x200700 loops 12 blocks from 0x22800000
  stride 0x400000, then cart 0x1000 len 0x7f000), results stored at 0x06080010+,
  ending in a self-branch hang at 0x200748 near the "HANG SHANG" strings.
* Hand-off: the stub ends with `mov.l [p],r3 ; mov.l @r3,r3 ; jmp @r3`
  (cart 0x200258, p -> 0x060c2fe6) i.e. it jumps through a HWRAM vector that is
  already populated -> **the program image is in HWRAM before the stub finishes**.

## 2. Where is the bulk copy done?
There is **no** bulk `mov.l` copy loop and **no** SCU-DMA / SH2-DMAC image copy in
the matched game code:
* SCU DMA regs (0x25fe0000-14) appear only in the game runtime helpers
  (cart 0x2147xx-0x2148xx = `scu_dma0_transfer`); SH2-DMAC refs are runtime too.
* cart-wide memcpy-loop scan finds only small fixed copies (matrix/fixmul).
* No instruction loads 0x06004000 / 0x06000000 as a copy destination.

Therefore the cart->HWRAM program copy is performed by the **ST-V BIOS boot
library** (reached via the 0x03000000 dispatch, before the cart stub regains
control), not by code present in cart.bin. The copy is nonetheless a plain
linear block move, which we recovered from the image itself (below).

## 3. Deriving the main-segment delta (the proof)
Simple linear voting (function-starts vs pooled 0x06xxxxxx pointers) sits at the
noise floor (~11/35) - as the previous notes found. The decisive anchor is a
self-contained jump stub at **cart 0x201000**:

    20100e  mov.l [0x201018],r1   ; r1 = 0x060987cc   (mode var)
    201010  mov.l r0,@r1
    201012  mov.l [0x20101c],r0   ; r0 = 0x06005020
    201014  jmp  @r0              ; -> RAM 0x06005020
    ...
    201020  mov.l r14,@-r15       ; <-- the function body jumped to, right here

The stub (running in-segment) jumps to RAM **0x06005020**, whose body is the very
next thing in cart at **0x201020**. Hence cart 0x201020 == RAM 0x06005020:

    delta = 0x06005020 - 0x201020 = 0x05e04000
    => cart 0x200000 == RAM 0x06004000

Independent agreement: the pointer literal to 0x06005020 is stored at cart
0x20101c (4 bytes before its own target 0x201020); the pointer to 0x0604e2dc is
stored at cart 0x24a290 and its target maps to cart 0x24a2dc - every pooled
code pointer sits a short, sane distance from the body it names.

## 4. Cross-check (the task acceptance test)
With cart = RAM - 0x05e04000, every indirect-call (`jsr/jmp @Rn`) target that
lands in the main RAM range maps to an **exact SH-2 instruction boundary**:

    MAIN-segment code targets: 22/22 on instruction boundary, 0 mid-instruction.
      06005020->201020  0604e2dc->24a2dc  0604e38a->24a38a  0604e49e->24a49e
      0604eb2c->24ab2c  0604eb4a->24ab4a  0608c00c->28800c  0608c19c->28819c
      0608c1a4->2881a4  ... 0608c220->288220 (8-byte-stride jump table)
      0609029c->28c29c  060902a0->28c2a0  060902f0->28c2f0  0609053c->28c53c

Pooled HWRAM pointers in 0x200400..0x290000 (n=4637 incl. dups): 1403 land in the
main segment; of the 1276 main-range *data* pointers, 1264 are even (12 odd =
byte-granular data), all consistent with pointing at copied code/rodata.

## 5. Full HWRAM address map

| HWRAM range             | source                                | notes |
|-------------------------|---------------------------------------|-------|
| 0x06000000 - 0x06004000 | BIOS / IPL (NOT from cart)            | work area + BIOS fn-ptr table @0x06000300; game fills vectors 0x06000628/640/694 at runtime |
| 0x06004000 - 0x06091e20 | cart 0x200000 - 0x28de20 (LINEAR)     | the program: code + rodata. delta 0x05e04000. HIGH confidence |
| 0x06091e20 - 0x060f8000 | runtime (BSS / stacks / buffers)      | asset+decompress work area ~0x06098000 (2033 refs), sound work ~0x060e0000 (339 refs); loaded/zeroed at run time, no static cart source |

Notes on the non-main code-ptr "targets" (14 of the 36): 0x06000640/694 are BIOS
low vectors; 0x06098674/5, 0x06098688/9, 0x0609868c, 0x060986ae, 0x060987d0/f4,
0x06099578, 0x060997ac, 0x060c2fe6, 0x060e0ddc are entries inside mixed
pointer/struct tables in the HIGH data region (some odd-addressed) - they are
vtable/vector/data slots, not statically-copied functions, so they correctly do
NOT resolve to cart code.

## 6. Caveats / open items
* Main-segment extent end (cart 0x28de20) is the last non-0xff byte; the linked
  image may pad a little further in RAM (BSS follows). Treated as code/rodata end.
* The exact BIOS copy call (function # passed to the 0x03000000 dispatch, r4=0x40
  /0x41 seen) cannot be stepped without the ST-V BIOS ROM; the linear result is
  recovered from the image and cross-checked instead.
* HIGH region: individual asset blocks (resource table at cart 0x2028f0:
  {ram_dst 0x06098xxx, sizes ~0xd000, ...}) are loaded on demand by the engine;
  if a static per-asset cart->RAM table is ever needed, start from that table.
* The cart program image is duplicated on the A-bus: cart 0x300000-0x390000 is
  byte-identical to 0x200000-0x290000 (+0x100000). rammap.py folds that mirror.
