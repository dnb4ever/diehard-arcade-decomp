#!/usr/bin/env python3
"""Die Hard Arcade (Sega ST-V, SH-2 BE) - cart.bin <-> HWRAM (0x06xxxxxx) mapping.

Derived by tracing the boot/copy (notes/ram_mapping_detail.md) and cross-checked:
all 22 HWRAM code-call targets that fall in the main segment map back onto exact
SH-2 instruction boundaries in the cart (0 mid-instruction, 0 failures).

Memory model
------------
cart.bin offset 0            == SH-2 A-bus addr 0x02000000   (cached CS0 window)
                                also mirrored at 0x22000000  (cache-through view)
cart 0x200000..0x290000      mirrored at cart 0x300000..0x390000 (A-bus +0x100000)

HWRAM segments at runtime (0x06000000..0x060FFFFF, 1 MiB, mirrored to 0x07FFFFFF):

  LOW   0x06000000..0x06004000  BIOS/system reserved (Saturn/ST-V IPL work +
                                 the BIOS function-pointer table @0x06000300).
                                 NOT copied from cart.  -> no cart source.
  MAIN  0x06004000..0x06091e20  the game program image (code + rodata),
                                 a single LINEAR copy of cart 0x200000..0x28de20.
                                 RAM = cart_off + 0x05e04000      (PROVEN)
  HIGH  0x06091e20..0x060f8000  runtime data: BSS, stacks, the asset/decompress
                                 work buffers (~0x06098000, 0x060e0000 sound).
                                 Populated at run time, NOT a static boot copy.
"""

DELTA      = 0x05e04000          # RAM = cart_offset + DELTA  (main segment)
CART_LO    = 0x00200000          # first copied cart offset
CART_HI    = 0x0028de20          # one past last populated cart byte (then 0xff fill)
RAM_LO     = CART_LO + DELTA      # 0x06004000
RAM_HI     = CART_HI + DELTA      # 0x06091e20

CART_BASE_SH2 = 0x02000000       # cart offset 0 in the cached A-bus window
MIRROR_LO, MIRROR_HI = 0x00300000, 0x00390000   # cart +0x100000 A-bus mirror


def _norm_cart(addr):
    """Accept a raw cart.bin offset OR an SH-2 A-bus address (cached 0x02xxxxxx /
    uncached 0x22xxxxxx) and return a plain cart.bin offset. Collapses the
    cart 0x300000-0x390000 image mirror back onto 0x200000-0x290000."""
    a = addr & 0xffffffff
    if 0x02000000 <= a < 0x02400000:        # cached A-bus view
        a -= 0x02000000
    elif 0x22000000 <= a < 0x22400000:      # cache-through A-bus view
        a -= 0x22000000
    if MIRROR_LO <= a < MIRROR_HI:          # A-bus program mirror
        a -= 0x00100000
    return a


def cart_to_ram(addr):
    """cart.bin offset (or SH-2 cart address) -> HWRAM address, or None.

    Returns None for cart bytes that are not part of the main boot copy
    (header region below 0x200000, the 0xff-fill tail, assets, etc.)."""
    off = _norm_cart(addr)
    if CART_LO <= off < CART_HI:
        return off + DELTA
    return None


def ram_to_cart(addr):
    """HWRAM address (0x06xxxxxx, also its 0x07xxxxxx mirror) -> cart.bin offset,
    or None if the address is not in the linearly-copied main segment.

    None means: BIOS/system area (<0x06004000), or the HIGH runtime-data region
    (>=0x06091e20) which has no static cart source."""
    a = addr & 0xffffffff
    if 0x07000000 <= a < 0x08000000:        # HWRAM high mirror -> fold to 0x060xxxxx
        a = 0x06000000 | (a & 0x000fffff)
    if RAM_LO <= a < RAM_HI:
        return a - DELTA
    return None


def classify_ram(addr):
    """Human-readable region tag for a HWRAM address."""
    a = addr & 0xffffffff
    if 0x07000000 <= a < 0x08000000:
        a = 0x06000000 | (a & 0x000fffff)
    if a < 0x06000000 or a >= 0x06100000:
        return "not-HWRAM"
    if a < RAM_LO:
        return "LOW: BIOS/system reserved (no cart source)"
    if a < RAM_HI:
        return "MAIN: cart 0x%06x (code/rodata)" % (a - DELTA)
    return "HIGH: runtime data/BSS/asset buffer (no static cart source)"


if __name__ == "__main__":
    import sys
    if len(sys.argv) > 1:
        for s in sys.argv[1:]:
            v = int(s, 16)
            if 0x06000000 <= v < 0x08000000:
                print("RAM  %08x -> cart %s   [%s]" %
                      (v, ("%06x" % ram_to_cart(v)) if ram_to_cart(v) is not None else "None",
                       classify_ram(v)))
            else:
                r = cart_to_ram(v)
                print("cart %08x -> RAM  %s" % (v, ("%08x" % r) if r is not None else "None"))
    else:
        # self-test against the proven anchor and the cross-check targets
        assert cart_to_ram(0x201020) == 0x06005020
        assert ram_to_cart(0x06005020) == 0x201020
        assert cart_to_ram(0x02201020) == 0x06005020      # SH-2 cached view
        assert cart_to_ram(0x301020) == 0x06005020        # A-bus mirror
        assert ram_to_cart(0x0604e2dc) == 0x24a2dc
        assert ram_to_cart(0x0608c19c) == 0x28819c
        assert ram_to_cart(0x06000694) is None            # BIOS low
        assert ram_to_cart(0x06099578) is None            # HIGH runtime
        print("rammap self-test OK:  cart 0x200000 <-> RAM 0x06004000, delta 0x05e04000")
