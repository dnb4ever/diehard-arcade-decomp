#!/usr/bin/env python3
"""Rewrite m2c_fn.py's emitted SH-2 asm so m2c can lower PIC computed branches.

Die Hard Arcade (ST-V) is compiled PIC: `braf Rn` (computed tail-jump) and
`bsrf Rn` (computed call) form their target as PC+4+Rn, where Rn is a signed
32-bit DELTA loaded from the literal pool. m2c cannot lower either form and
emits M2C_ERROR, which blocks the whole function.

This wrapper takes the GNU-as asm that m2c_fn.to_asm() emits, uses
braf_resolve.resolve() to compute each branch's fixed target, and substitutes
the computed branch with a direct `bsr func_<SH2>` to the resolved target.

The transform mirrors m2c_fn.py's PROVEN `jmp @Rn` tail-call handling: the
branch's delay-slot instruction is HOISTED to run before the call (it often sets
the call's argument, e.g. `mov r13,r4`), the computed branch becomes a direct
`bsr func_<SH2>` with a `nop` delay slot, and:
  * `bsrf Rn` (computed CALL, mid-body): emit `<slot>; bsr target; nop` and let
    control continue at the instruction after the slot -- exactly as bsrf did.
  * `braf Rn` (computed TAIL-JUMP): emit `<slot>; bsr target; nop; rts; nop`.
    The explicit `rts` is REQUIRED: a braf tail can sit inside a conditional
    block (not just at the function's physical end), so a bare `bsr` would fall
    through into whatever follows. The rts makes it a true tail call that returns
    (r0 survives rts, so value-returning tails are preserved). m2c renders it as
    `return target(...)`.
The dead pool DELTA load that used to feed Rn becomes an unused register write
and is removed by m2c's dead-code elimination (verified).

Table-driven dispatch (multi-target `dispatch_table_*`, i.e. the branch register
comes from a runtime `mov.b/mov.w @(r0,Rn)` table load) is reported as such and
left untouched -- never collapsed to a single fake target.

usage:
  braf_rewrite.py 0xADDR [name] --asm      # print the rewritten asm
  braf_rewrite.py 0xADDR [name]            # rewrite, then run m2c (sh2-gcc-c)
  braf_rewrite.py 0xADDR --report          # list resolved / table / unresolved
  (extra args after a recognised flag are forwarded to m2c)

Core tools (m2c_fn.py, fn.py, braf_resolve.py) are imported read-only; nothing
here modifies them.
"""
import argparse
import re
import subprocess
import sys
import tempfile

import braf_resolve
import fn  # noqa: F401  (ensures same ROM/paths as the other tools)
import m2c_fn

SH2_BASE = 0x02000000
_BR = re.compile(r"^\t(braf|bsrf)\t(r\d+)\s*$")


def rewrite(start: int, name: str):
    """Return (rewritten_asm, resolver_entries, resisted_list).

    resisted_list holds the resolver entries we could NOT turn into a direct
    call (table-driven / unresolved); the asm keeps the original braf/bsrf for
    those so m2c still flags them.
    """
    asm = m2c_fn.to_asm(start, name)
    res = braf_resolve.resolve(start)
    lines = asm.split("\n")
    out = []
    ri = 0
    resisted = []
    i = 0
    while i < len(lines):
        ln = lines[i]
        m = _BR.match(ln)
        if not m:
            out.append(ln)
            i += 1
            continue
        ent = res[ri] if ri < len(res) else None
        ri += 1
        if not (ent and ent.get("status") == "resolved"):
            resisted.append(ent or {"status": "unmatched", "op": m.group(1)})
            out.append(ln)  # leave computed branch -> m2c will M2C_ERROR it
            i += 1
            continue
        op = m.group(1)
        sh2 = (ent["target"] + SH2_BASE) & 0xFFFFFFFF
        slot = lines[i + 1] if i + 1 < len(lines) else "\tnop"
        # Hoist the delay-slot instruction, then the direct call (mirrors
        # m2c_fn's jmp handling). braf (tail jump) gets an explicit rts so it
        # cannot fall through into a following conditional block.
        out.append(slot)
        out.append(f"\tbsr\tfunc_{sh2:08x}\t! was {op} {m.group(2)}")
        out.append("\tnop")
        if op == "braf":
            out.append("\trts")
            out.append("\tnop")
        i += 2  # consumed the branch and its delay-slot line
    return "\n".join(out), res, resisted


def describe(res):
    lines = []
    for e in res:
        if e["status"] == "resolved":
            sh2 = (e["target"] + SH2_BASE) & 0xFFFFFFFF
            kind = "CALL (bsrf)" if e["op"] == "bsrf" else "TAIL (braf)"
            lines.append(f"  {e['at']:06x} {kind} -> func_{sh2:08x} "
                         f"(cart {e['target']:#08x}, delta {e['delta']:#x})")
        elif e["status"] == "table/computed":
            lines.append(f"  {e['at']:06x} {e['op']} -> TABLE/COMPUTED via "
                         f"`{e.get('via','?')}` (left as-is)")
        else:
            lines.append(f"  {e['at']:06x} {e['op']} -> UNRESOLVED (left as-is)")
    return "\n".join(lines) if lines else "  (no braf/bsrf)"


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("addr")
    ap.add_argument("name", nargs="?")
    ap.add_argument("--asm", action="store_true", help="print rewritten asm only")
    ap.add_argument("--report", action="store_true",
                    help="list resolved/table/unresolved branches only")
    args, rest = ap.parse_known_args()
    start = int(args.addr, 16)
    name = args.name or f"f_{start:x}"
    asm, res, resisted = rewrite(start, name)
    if args.report:
        print(f"--- {start:#x} ({len(res)} computed branch(es), "
              f"{len(resisted)} resisted)")
        print(describe(res))
        return
    if args.asm:
        sys.stdout.write(asm)
        return
    with tempfile.NamedTemporaryFile("w", suffix=".s") as f:
        f.write(asm)
        f.flush()
        r = subprocess.run([sys.executable, "-m", "m2c.main",
                            "-t", "sh2-gcc-c", *rest, f.name])
        sys.exit(r.returncode)


if __name__ == "__main__":
    main()
