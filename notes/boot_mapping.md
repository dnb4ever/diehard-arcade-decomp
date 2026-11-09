# Die Hard Arcade (Sega ST-V) — boot & cart<->RAM mapping

Hitachi SH-2, big-endian. All "cart" offsets are into `cart.bin`.
Working rule used by the tooling: **SH2 addr = cart_offset + 0x02000000**
(so cart.bin offset 0 = SH2 0x02000000, the A-bus CS0 cartridge window).

cart.bin is 20 MiB (0x1400000) and is fully populated (no zero gaps).

## 1. IP / cartridge header (cart 0x200000 = SH2 0x02200000)

NOTE the program image is NOT at cart 0 — the "SEGA ST-V(TITAN)" header sits at
cart **0x200000**. (cart 0x000000..0x200000 is other A-bus data/bank.)

Parsed header fields:

| off   | field        | value                         |
|-------|--------------|-------------------------------|
| 0x000 | hw id        | `SEGA ST-V(TITAN)`            |
| 0x010 | maker        | `(C)SEGA`                    |
| 0x020 | product      | (spaces)                      |
| 0x02a | version      | `V1.000`                      |
| 0x030 | date         | `19960515`                    |
| 0x038 | device       | `CART-AM `                    |
| 0x040 | area         | `UET` (US/Europe/Taiwan…)     |
| 0x060 | title        | `DIE HARD ARCADE ...` (x3)     |
| 0x0e0 | IP size      | 0x1000                        |

The Saturn-CD-style fields at 0x0e8/0x0ec (master/slave SP) and 0x0f0/0x0f4
(1st-read addr/size) are **all zero** — ST-V boots differently from Saturn CD,
so these are not used. 0x200e00 is a region/name string table (NOT an entry),
as warned.

## 2. Master-SH2 entry / boot stub

First executed cartridge instruction is at **cart 0x200100 (SH2 0x02200100)**.
0x200100 starts with `mov.w;nop` then an embedded ASCII string
("PRODUCED UNDER LICENSE FROM SEGA ENTERPRISES,LTD") at 0x200108..0x20013b;
real boot code resumes at **0x20013c**.

The boot stub is a **position-independent** (PIC) Sega security/hardware-init
library: it computes all runtime addresses with `mova <next>,r0 ; add r0,Rn`
and calls via `bsrf`/pointer tables, so it runs correctly regardless of load
address. It:
  * zeroes/sets SMPC & system work via a pointer table read from SH2 0x03000000
    (`mov.l @0x03000000,r2 ; jsr @r2` — a BIOS/system dispatch table in cart),
  * initialises VDP2 / SCU / interrupt mask,
  * performs the ST-V security check; on **failure it hangs**: at 0x2005f6
    `ldc (I=15),sr ; bra 0x2005f6` (self-branch) near the "HANG SHANG" string.

The whole stub is **duplicated**: an identical copy exists at 0x2007a6+
(mirror of 0x20013c+). Separately, the entire program region
cart 0x200000..0x290000 is byte-identical to cart 0x300000..0x390000
(A-bus +0x100000 mirror).

## 3. Does code run from cart or from HWRAM 0x06000000? -> **RUNS FROM HWRAM**

Decisive evidence (the game program is **linked for HWRAM 0x06xxxxxx**, not for
the cart window 0x022xxxxx):

  * **Indirect call targets** (`mov.l @(disp,pc),Rn ; jsr/jmp @Rn`) over the
    program region 0x200400..0x290000 classify as:
      - HWRAM 0x06xxxxxx : **35**
      - BIOS / low RAM 0x0000xxxx..0x0002xxxx : 23  (Saturn/ST-V BIOS calls)
      - cart 0x022xxxxx  : **0**
    If code executed from the cart, absolute code pointers would be 0x022xxxxx.
    They are never cart addresses -> execution is from RAM.
  * **Literal-pool pointers** inside matched compilation units:
      HWRAM 0x06xxxxxx : 309   vs   cart 0x022/020-021 : ~20.
  * Example called code addresses: 0x06005020, 0x0604e2dc, 0x0608c19c…;
    data/BSS pointers run up to ~0x060f8000 (full 1 MiB HWRAM is used).

Cross-function calls inside the image use PC-relative `bsr`/`bsrf` (position
independent), which is why match.py can still byte-verify functions as stored
in the cart regardless of the final link address.

## 4. The cart-storage -> 0x06000000 copy mapping — **NOT a single linear base**

Attempts to pin one linear formula `RAM = 0x06000000 + (cart_off - BASE)` fail:
  * voting cart-function-starts / bsr-targets against pooled 0x06xxxxxx pointers
    gives **no sharp peak** (noise-floor ~11-16 hits);
  * spot-checks refute BASE=0x200000: e.g. code-pointer 0x06005020 would map to
    cart 0x205020 = data, 0x06000694 -> cart 0x200694 = mid-function.
  * The low HWRAM refs (0x0600000x..0x0600070x) are the Saturn BIOS-reserved
    work/vector area (0x06000000..~0x06001000), not game code.

No instruction in the matched program region ever loads the literal 0x06000000
/ 0x06002000 as a copy destination, so the bulk copy is **not** an explicit
`mov.l @Rm+ / mov.l Rn,@Rk+` loop in the matched code — it is performed by the
PIC boot library (via the SH2 0x03000000 BIOS dispatch table / SCU-DMA) and/or
is segmented/overlaid (jump-/pointer-tables dispatch into fixed HWRAM handler
blocks such as 0x0609868x and trampoline rows at 0x0608c1xx on 8-byte stride).

**Conclusion:** code executes from HWRAM 0x06xxxxxx; the exact
source/dest/length of the copy is NOT yet fully resolved and is almost
certainly not a single contiguous linear copy. This matches the task note that
the RAM mapping may be incomplete — the per-function byte verifier works
entirely in cart-offset space and does not depend on it.

### Open items for a follow-up
  * trace the SH2 0x03000000 dispatch table to find the BIOS copy/DMA call;
  * identify segment boundaries by clustering the 0x06xxxxxx code pointers;
  * once segment (src_cart, dst_ram, len) tuples are known, relocation-aware
    linking in project/build.py can be turned on.

## 5. Quick reference
  * program/code region: cart 0x200400..0x290000 (fn.py CODE_START/TEXT_END)
  * 1272 functions, ~0x8e67a (583 KiB) code, ~0xacf60 (690 KiB) code+pool
  * data region: SH2 0x03xxxxxx = cart 0x1000000+
  * hw regs seen: VDP2 0x25fe00xx, SMPC 0x20000000, SCU, HWRAM 0x06000000
