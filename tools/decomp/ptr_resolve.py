#!/usr/bin/env python3
"""Resolve SH-2 PIC DATA-pointer computations (mova/add) to fixed addresses.

Companion to braf_resolve.py (which handles computed BRANCHES: braf/bsrf Rn).
Die Hard Arcade (ST-V) is compiled position-independent. Besides computed
branches, the compiler forms absolute DATA / FUNCTION pointers with the idiom:

    mova  .Lanchor, r0        ; r0 = ADDRESS of a pool slot (cart offset A)
    mov.l .Ldelta, rX         ; rX = signed 32-bit DELTA from the literal pool
    add   r0, rX              ; rX = A + DELTA  (the absolute target)

The value stored AT .Lanchor is irrelevant (it is only a position anchor); what
matters is the slot's ADDRESS. Capstone resolves `mova`/`mov.l @(disp,pc)`
operands to their absolute pool address, so the target is simply

    target = anchor_addr + delta           (mod 2^32)
    SH2 address = target + 0x02000000

This is the exact math batch-4 did by hand for task-form next-state pointers
(0x22a970 -> 0x22a9fe, 0x22a9fe -> 0x22a5ca) and spawn templates
(0x202c5c -> 0x0220340C). This tool finds every such mova/add and reports the
resolved pointer, so the decompiled C can name the g_<addr> global / table it
references instead of an opaque computed pointer.

A variant `mova .Lanchor,r0 ; add #imm,r0` (small immediate delta) is also
handled. Register copies (mov rA,rB) are followed. When one add operand is a
`mova` anchor and the other a pool/immediate delta, the add is a resolvable
data pointer; otherwise it is an ordinary arithmetic add and is ignored.

usage:  ptr_resolve.py 0x239fc6 [0x23a3a2 ...]
        ptr_resolve.py --all 0x239fc6        # also list braf/bsrf (via braf_resolve)

Validated (reproduces batch-4 hand resolutions exactly):
  0x22a970 : add@22a9e8 -> 0x0222a9fe   (anchor 22a9ec + 0x12)
  0x22a9fe : add@22aa8c -> 0x0222a5ca   (anchor 22aa90 - 0x4c6)

Boot-cluster halfword-swap (batch 11) -- 0x200xxx mova+add deltas (start in
BOOT_LO..BOOT_HI only; everything else is byte-for-byte unchanged):
  0x200668 : add@200680 raw->0xe7b40680 (invalid) / swap->0x0220ec14 (ROM)
             => use 0x0220ec14  -- confirms the bsrf sibling at 200684 -> 0x20ec1c
                (the computed fn-ptr is the alt-entry 0x20ec14 of that same callee).
  0x200668 : add@200694 raw->0x02505bd6 (ROM, valid) / swap->0x576206c4 (invalid)
             => use 0x02505bd6  -- RAW is correct here; the pick is per-constant,
                driven by which half-order lands in a valid window, not a blanket swap.
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fn  # noqa: E402
import pool_read  # noqa: E402  (reuse WINDOWS classify + swap16 for the boot-block swap)

MASK = 0xFFFFFFFF
SH2_BASE = 0x02000000

# The 0x200xxx boot/init cluster stores its literal pools at 2-aligned offsets, so a
# mov.l @(disp,pc) (which forces (pc&~3) alignment) reads a longword straddling two
# constants -- the running value is the HALFWORD-SWAP of the 4-aligned read (see
# pool_read.py). This applies to mova+add DATA/FUNCTION-pointer deltas here too. We
# only augment resolutions inside this cluster so every non-boot case (0x22xxxx ...)
# is byte-for-byte unchanged, and inside it we pick the half-order whose target lands
# in a valid address window (both-or-neither -> keep raw + flag for review).
BOOT_LO = 0x200400
BOOT_HI = 0x201020


def pool_value(addr: int, size: int) -> int:
    if size == 4:
        return struct.unpack(">I", fn.ROM[addr:addr + 4])[0]
    v = struct.unpack(">H", fn.ROM[addr:addr + 2])[0]
    return v - 0x10000 if v & 0x8000 else v  # mov.w sign-extends


def _imm(tok: str) -> int:
    return int(tok[1:], 0)  # '#38' / '#0x12' / '#-1'


def dest_reg(ins) -> str:
    """Register this instruction writes, or '' for a store / non-reg dest."""
    ops = ins.op_str.replace(" ", "").split(",")
    if not ops:
        return ""
    last = ops[-1]
    if "@" in last:
        return ""  # memory destination (store)
    return last if last.startswith("r") else ""


def trace(ins_list, idx, reg, depth=0):
    """Value of `reg` as of index `idx` (exclusive), scanning backwards.

    Returns ('mova', addr) | ('pool', value) | ('imm', value)
          | ('via', instr)  | ('unknown', None).
    """
    if depth > 8:
        return ("unknown", None)
    for j in range(idx - 1, -1, -1):
        ins = ins_list[j]
        if dest_reg(ins) != reg:
            continue
        mn = ins.mnemonic
        op = ins.op_str.replace(" ", "")
        src = op.split(",")[0]
        if mn == "mova" and src.startswith("0x"):
            return ("mova", int(src, 16))
        if mn in ("mov.l", "mov.w") and src.startswith("0x"):
            sz = 4 if mn == "mov.l" else 2
            return ("pool", pool_value(int(src, 16), sz), sz)
        if mn == "mov" and src.startswith("r"):
            return trace(ins_list, j, src, depth + 1)
        if mn == "mov" and src.startswith("#"):
            return ("imm", _imm(src))
        return ("via", ins)  # add/alu/table-load: not a single pool/anchor
    return ("unknown", None)


def _classify(target: int) -> str:
    """Rough bucket for the resolved cart target (for the human report)."""
    sh2 = (target + SH2_BASE) & MASK
    if fn.CODE_START <= target < fn.TEXT_END:
        return f"in-ROM (SH2 {sh2:#010x}) code/table"
    if 0x06000000 <= sh2 < 0x06100000:
        return f"HWRAM global g_{sh2 & 0xFFFFFFFF:08X}"
    if 0x25000000 <= sh2 < 0x26000000:
        return f"VDP/SCU HW reg {sh2:#010x}"
    return f"SH2 {sh2:#010x}"


def _augment_swap(e: dict) -> None:
    """For a boot-cluster mova+add whose delta came from a 4-byte pool, add the
    halfword-swapped alternative and recommend the half-order that lands in a valid
    address window (mirrors pool_read.recommend). Leaves `target` untouched; adds
    `target_swap`, `win_raw`, `win_swap`, `pick`, `pick_tag`."""
    sw = pool_read.swap16(e["delta"])
    e["delta_swap"] = sw
    e["target_swap"] = (e["anchor"] + sw) & MASK
    rok = pool_read.classify((e["target"] + SH2_BASE) & MASK)
    sok = pool_read.classify((e["target_swap"] + SH2_BASE) & MASK)
    e["win_raw"], e["win_swap"] = rok, sok
    if sok and not rok:
        e["pick"], e["pick_tag"] = e["target_swap"], f"SWAPPED (raw win invalid; -> {sok})"
    elif rok and not sok:
        e["pick"], e["pick_tag"] = e["target"], f"raw ok ({rok})"
    elif rok and sok:
        e["pick"], e["pick_tag"] = e["target"], f"AMBIGUOUS (raw={rok}, swap={sok}) - verify"
    else:
        e["pick"], e["pick_tag"] = e["target"], "NEITHER valid - likely a bitmask/immediate"


def resolve(start: int):
    code_end, _, _, _ = fn.extent(start)
    ins_list = list(fn.MD.disasm(fn.ROM[start:code_end], start))
    do_swap = BOOT_LO <= start < BOOT_HI
    found = []
    for i, ins in enumerate(ins_list):
        if ins.mnemonic != "add":
            continue
        ops = ins.op_str.replace(" ", "").split(",")
        if len(ops) != 2 or not ops[1].startswith("r"):
            continue
        dst, src = ops[1], ops[0]
        kd = trace(ins_list, i, dst)
        if src.startswith("#"):
            if kd[0] == "mova":
                tgt = (kd[1] + _imm(src)) & MASK
                found.append({"at": ins.address, "anchor": kd[1],
                              "delta": _imm(src) & MASK, "target": tgt,
                              "form": "mova+#imm"})
            continue
        if not src.startswith("r"):
            continue
        ks = trace(ins_list, i, src)
        kinds = (kd[0], ks[0])
        if "mova" not in kinds:
            continue
        anchor = kd[1] if kd[0] == "mova" else ks[1]
        other = ks if kd[0] == "mova" else kd
        if other[0] not in ("pool", "imm"):
            continue
        delta = other[1] & MASK
        e = {"at": ins.address, "anchor": anchor, "delta": delta,
             "target": (anchor + delta) & MASK, "form": "mova+add"}
        if do_swap and other[0] == "pool" and len(other) > 2 and other[2] == 4:
            _augment_swap(e)
        found.append(e)
    return found


def main() -> None:
    args = sys.argv[1:]
    show_br = False
    if args and args[0] == "--all":
        show_br = True
        args = args[1:]
    if not args:
        print(__doc__)
        return
    for a in args:
        start = int(a, 16)
        if not (fn.CODE_START <= start < len(fn.ROM)):
            print(f"--- {start:#x}: out of ROM range; skipped")
            continue
        res = resolve(start)
        print(f"--- {start:#x} ({len(res)} data-pointer computation(s))")
        for e in res:
            sh2 = (e["target"] + SH2_BASE) & MASK
            print(f"    add@{e['at']:06x}  anchor {e['anchor']:#08x} + "
                  f"delta {e['delta']:#x}  ->  {e['target']:#08x} "
                  f"(SH2 {sh2:#010x})  [{e['form']}; {_classify(e['target'])}]")
            if "pick" in e:
                psh2 = (e["pick"] + SH2_BASE) & MASK
                print(f"        swap-delta {e['delta_swap']:#x} -> {e['target_swap']:#08x} "
                      f"(SH2 {(e['target_swap'] + SH2_BASE) & MASK:#010x})  "
                      f"=> use {e['pick']:#08x} (SH2 {psh2:#010x})  {e['pick_tag']}")
        if not res:
            print("    (no mova/add data pointers)")
        if show_br:
            import braf_resolve
            br = braf_resolve.resolve(start)
            print(f"    --- {len(br)} computed branch(es) ---")
            for e in br:
                if e["status"] == "resolved":
                    sh2 = (e["target"] + SH2_BASE) & MASK
                    print(f"    {e['at']:06x} {e['op']} -> {e['target']:#08x} "
                          f"(SH2 {sh2:#010x})")
                else:
                    print(f"    {e['at']:06x} {e['op']} -> {e['status']}")


if __name__ == "__main__":
    main()
