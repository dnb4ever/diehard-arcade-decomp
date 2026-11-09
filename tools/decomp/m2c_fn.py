#!/usr/bin/env python3
"""Emit a ROM function as GNU-as SH-2 assembly and optionally run m2c on it.

Branch targets and literal-pool entries become local labels, so m2c sees
`mov.l Lpool_18948,r6` / `.long 0x0000dfff` the way it does for compiler
output, and calls through pool addresses show up as named functions.

usage: m2c_fn.py 0x188ac [name] [--asm] [--context include/x.h] [m2c args...]
"""
import argparse
import re
import struct
import subprocess
import sys
import tempfile

import fn

LABEL = ".L{:x}"


def to_asm(start: int, name: str) -> str:
    code_end, end, pool, targets = fn.extent(start)
    ins = list(fn.MD.disasm(fn.ROM[start:code_end], start))
    longs = {}  # pool address -> size
    for i in ins:
        if i.mnemonic in ("mov.l", "mova") and i.op_str.startswith("0x"):
            longs[int(i.op_str.split(",")[0], 16)] = 4
        elif i.mnemonic == "mov.w" and i.op_str.startswith("0x"):
            longs.setdefault(int(i.op_str.split(",")[0], 16), 2)
    out = [".text", ".align 2", f".global _{name}", f"_{name}:"]
    # Drop a pool that sits mid-function after a jump: the words some load
    # reads (possibly one in an earlier function) and the padding before them.
    skip, after_jump = set(), False
    for k, i in enumerate(ins):
        a = i.address
        if after_jump and a not in targets and (a in fn.all_pool() or (
                fn.hw(a) in (0x0009, 0xFFFF, 0x0000) and a + 2 in fn.all_pool())):
            skip.add(a)
            continue
        after_jump = k > 0 and ins[k - 1].mnemonic in ("bra", "rts", "jmp")
    tail = set()  # delay slots of tail calls, emitted before the call
    for k, i in enumerate(ins):
        if i.mnemonic == "jmp" and k + 1 < len(ins):
            tail.add(ins[k + 1].address)
    ins_by_addr = {i.address: i for i in ins}
    for i in ins:
        if i.address in skip or i.address in tail:
            continue
        if i.mnemonic == "jmp":
            # A tail call: run its delay slot, call, and return. m2c would
            # otherwise look for a jump table.
            slot = ins_by_addr.get(i.address + 2)
            if slot is not None:
                out.append(f"\t{slot.mnemonic.replace('/s', '.s')}\t{slot.op_str.replace(' ', '')}")
            out += [f"\tjsr\t{i.op_str.replace(' ', '')}", "\tnop", "\trts", "\tnop"]
            continue
        if i.address in targets and i.address != start:
            out.append(LABEL.format(i.address) + ":")
        op = i.op_str.replace(" ", "")
        m = re.match(r"0x([0-9a-f]+)(.*)", op)
        if m:
            a = int(m[1], 16)
            if i.mnemonic in ("mov.l", "mov.w", "mova"):
                op = ".Lpool_{:x}{}".format(a, m[2])
            elif a == start:
                op = f"_{name}{m[2]}"
            elif start <= a < code_end:
                op = LABEL.format(a) + m[2]
            else:
                op = "func_{:x}{}".format(a, m[2])
        if (i.mnemonic, op) in (("sts.l", "macl,@-r15"), ("lds.l", "@r15+,macl")):
            # m2c only models pr save/restore; MACL is saved as callee-saved
            # under SHC's -macsave=1 and carries no meaning for the C.
            out.append(f"\t! {i.mnemonic}\t{op}")
            continue
        mn = i.mnemonic.replace("/s", ".s")  # capstone bt/s, m2c bt.s
        out.append(f"\t{mn}\t{op}")
    out.append("\t.align 2")
    for a in sorted(longs):
        out.append(".Lpool_{:x}:".format(a))
        if longs[a] == 4:
            v = struct.unpack(">I", fn.ROM[a:a + 4])[0]
            out.append(f"\t.long\t0x{v:08x}")
        else:
            v = struct.unpack(">H", fn.ROM[a:a + 2])[0]
            out.append(f"\t.short\t0x{v:04x}")
    return "\n".join(out) + "\n"


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("addr")
    ap.add_argument("name", nargs="?")
    ap.add_argument("--asm", action="store_true", help="print the assembly only")
    args, rest = ap.parse_known_args()
    start = int(args.addr, 16)
    name = args.name or f"f_{start:x}"
    asm = to_asm(start, name)
    if args.asm:
        sys.stdout.write(asm)
        return
    with tempfile.NamedTemporaryFile("w", suffix=".s") as f:
        f.write(asm)
        f.flush()
        r = subprocess.run([sys.executable, "-m", "m2c.main", "-t", "sh2-gcc-c", *rest, f.name])
        sys.exit(r.returncode)


if __name__ == "__main__":
    main()
