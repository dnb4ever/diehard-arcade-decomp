#!/usr/bin/env python3
"""Resolve SH-2 computed tail/call branches (braf Rn / bsrf Rn) to fixed targets.

Die Hard Arcade (ST-V) is compiled PIC: a `braf Rn` / `bsrf Rn` forms its target
as `PC + 4 + Rn`, where PC is the address of the braf/bsrf instruction itself and
Rn usually holds a signed 32-bit DELTA loaded straight out of the literal pool
(`mov.l @(disp,pc),Rn ; ... ; braf Rn`). This makes the branch a statically
resolvable tail-jump (braf) or tail-call (bsrf) to one fixed function, which m2c
cannot lower on its own.

This tool, given a function's CART offset (SH2 addr - 0x02000000), finds every
braf/bsrf in the body, back-traces the branch register to its source, and - when
the source is a single pool constant - computes the fixed target (same cart-offset
space fn.py uses; add 0x02000000 for the SH2 address). When the register instead
comes from a runtime table load (mov.b/mov.w @(r0,Rn) - the 32-way dispatch_table_*
pattern) it is reported as table-driven, not a single target.

usage:  braf_resolve.py 0x21db6e [0x22077e ...]

Validated against known tails:
  isr_timer0 0x223dca -> 0x222f36 ; 0x22077e -> 0x2153f6 ; 0x21db6e -> 0x246282
(all three land on clean function prologues).
"""
import struct
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fn  # noqa: E402  (fn.py in same dir; provides ROM, MD, extent)

MASK = 0xFFFFFFFF


def pool_value(addr: int, size: int) -> int:
    if size == 4:
        return struct.unpack(">I", fn.ROM[addr:addr + 4])[0]
    v = struct.unpack(">H", fn.ROM[addr:addr + 2])[0]
    return v - 0x10000 if v & 0x8000 else v  # mov.w sign-extends


def dest_reg(ins) -> str:
    """Return the register this instruction writes, or '' if it writes memory/none.

    SH-2 two-operand form puts the destination last; a store's last operand is a
    memory reference (@rN / @(..) / @-rN), which we must NOT treat as a reg write.
    """
    ops = ins.op_str.replace(" ", "").split(",")
    if not ops:
        return ""
    last = ops[-1]
    if last.startswith("@") or "@" in last:
        return ""  # memory destination (store) -> no register written
    return last if last.startswith("r") else ""


def backtrace(ins_list, idx, reg, depth=0):
    """Resolve the value of `reg` as of instruction index `idx` (exclusive).

    Returns ('pool', value) | ('reg_from_other', instr) | ('unknown', None).
    Follows mov rA,rB copies; reads pool constants from mov.l/mov.w/mova @(..,pc).
    """
    if depth > 8:
        return ("unknown", None)
    for j in range(idx - 1, -1, -1):
        ins = ins_list[j]
        if dest_reg(ins) != reg:
            continue
        op = ins.op_str.replace(" ", "")
        src = op.split(",")[0]
        if ins.mnemonic in ("mov.l", "mov.w", "mova") and src.startswith("0x"):
            pa = int(src, 16)
            size = 4 if ins.mnemonic in ("mov.l", "mova") else 2
            return ("pool", pool_value(pa, size))
        if ins.mnemonic == "mov" and src.startswith("r"):
            return backtrace(ins_list, j, src, depth + 1)
        # Any other writer (add/table-load/alu): not a single pool constant.
        return ("via", ins)
    return ("unknown", None)


def resolve(start: int):
    code_end, _, _, _ = fn.extent(start)
    ins_list = list(fn.MD.disasm(fn.ROM[start:code_end], start))
    found = []
    for i, ins in enumerate(ins_list):
        if ins.mnemonic not in ("braf", "bsrf"):
            continue
        reg = ins.op_str.strip()
        kind, info = backtrace(ins_list, i, reg)
        entry = {"at": ins.address, "op": ins.mnemonic, "reg": reg}
        if kind == "pool":
            delta = info & MASK
            entry["target"] = (ins.address + 4 + delta) & MASK
            entry["delta"] = delta
            entry["status"] = "resolved"
        elif kind == "via":
            entry["status"] = "table/computed"
            entry["via"] = f"{info.mnemonic} {info.op_str}"
        else:
            entry["status"] = "unresolved"
        found.append(entry)
    return found


def main() -> None:
    if len(sys.argv) < 2:
        print(__doc__)
        return
    for a in sys.argv[1:]:
        start = int(a, 16)
        if not (fn.CODE_START <= start < len(fn.ROM)):
            print(f"--- {start:#x}: out of ROM range "
                  f"[{fn.CODE_START:#x},{len(fn.ROM):#x}); skipped")
            continue
        res = resolve(start)
        print(f"--- {start:#x} ({len(res)} computed branch(es))")
        if not res:
            print("    (no braf/bsrf)")
        for e in res:
            if e["status"] == "resolved":
                tail = "call" if e["op"] == "bsrf" else "jump"
                print(f"    {e['at']:06x} {e['op']} {e['reg']}  ->  "
                      f"{e['target']:#08x} (SH2 {e['target'] + 0x02000000:#x})  "
                      f"[{tail}; delta {e['delta']:#x}]")
            elif e["status"] == "table/computed":
                print(f"    {e['at']:06x} {e['op']} {e['reg']}  ->  "
                      f"TABLE/COMPUTED via `{e['via']}`")
            else:
                print(f"    {e['at']:06x} {e['op']} {e['reg']}  ->  UNRESOLVED")


if __name__ == "__main__":
    main()
