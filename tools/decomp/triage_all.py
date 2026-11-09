#!/usr/bin/env python3
"""Whole-codebase function triage for the Die Hard Arcade (ST-V/SH-2) decomp.

Enumerates every function in every fn.units() compilation unit, disassembles its
code extent, and classifies it:

  asm      asm-origin (math/runtime-lib): hand-written math (mac.l/dmuls.l/div1/
           clrmac + non-standard r0/r1 return). Keep as asm, do not grind.
  leafC    leaf C target: no calls, real logic, decodes cleanly.
  callC    call-bearing C target: has jsr/bsr, decodes cleanly.
  misseg   data-missegmented: undecodable bytes (.byte) inside the code region
           => fn.py unit/extent boundary error; not a real function.

Writes notes/function_triage.tsv.  Run on p620 with the venv python.
"""
import os, sys, struct
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fn

BASE = 0x02000000  # sh2 addr = cart offset + BASE  (matches drafts/symbols.txt)

# Strong asm-origin opcode markers: 64-bit mul / MAC / hardware+soft divide.
ASM_OPS = {"clrmac", "mac.l", "mac.w", "dmuls.l", "dmulu.l",
           "div1", "div0s", "div0u"}

AP = fn.all_pool()  # every PC-relative literal-pool address in the image

# ---- load symbol names (offset -> name) -------------------------------------
SYMS = {}
sp = os.path.expanduser("~/decomp/diehard-stv/notes/symbols.txt")
if os.path.exists(sp):
    for ln in open(sp):
        ln = ln.strip()
        if ln.startswith("0x"):
            parts = ln.split()
            try:
                a = int(parts[0], 16) - BASE
                SYMS[a] = parts[1]
            except (ValueError, IndexError):
                pass


def classify(start, next_start):
    """Return dict of facts for function at cart offset `start`."""
    code_end, end, pool, targets = fn.extent(start)
    # clamp code to this function (never bleed into the next member)
    clamp = code_end
    if next_start and next_start < clamp:
        clamp = next_start
    code = fn.ROM[start:clamp]
    ins = list(fn.MD.disasm(code, start))
    mns = [i.mnemonic for i in ins]

    # Undecodable bytes (capstone id==0) that are NOT a known literal pool are
    # real garbage in the code stream => fn.py put data where code should be.
    # Inline pools (after bra/rts) are legitimately in all_pool and excluded.
    bad = [i.address for i in ins if i.id == 0 and i.address not in AP]
    ninsn = max(len(ins), 1)
    ratio = len(bad) / ninsn

    calls = sum(1 for m in mns if m in ("jsr", "bsr", "bsrf"))
    has_ret = any(m in ("rts", "jmp", "braf", "rte") for m in mns)
    asm_hit = sorted({m for m in mns if m in ASM_OPS})

    # non-standard return: result shuffled in the rts delay slot (xtrct/swap/...)
    nonstd = ""
    for k, m in enumerate(mns):
        if m == "rts" and k + 1 < len(ins):
            d = ins[k + 1].mnemonic
            if d in ("xtrct", "swap.w", "swap.b"):
                nonstd = d
            break

    # --- category decision (priority: misseg > asm > leaf/call) ---
    size = (next_start - start) if next_start else (end - start)
    note = []
    # misseg: mostly data, or a non-trivial garbage fraction, or empty/no-return
    is_misseg = (ratio >= 0.10 or len(bad) >= 8
                 or (len(bad) >= 2 and ratio >= 0.05)
                 or (len(ins) < 2) or (not has_ret and len(ins) < 3))
    if bad:
        note.append(f"garbage={len(bad)}/{ninsn}")
    # asm-origin = tiny, call-free runtime-lib math with the 64-bit-mul/MAC/div
    # primitives OR a non-standard r0/r1 return (result shuffled in rts delay
    # slot). Larger call-bearing functions use mac.l/div1 INLINE from plain C,
    # so the marker alone is not enough -- it is only noted for them.
    is_asm = calls == 0 and size <= 0x60 and (asm_hit or nonstd)
    if asm_hit:
        note.append("ops:" + ",".join(asm_hit))
    if nonstd:
        note.append("ret:" + nonstd)

    if is_misseg:
        cat = "misseg"
        if not has_ret:
            note.append("no-ret")
    elif is_asm:
        cat = "asm"
    elif calls == 0:
        cat = "leafC"
    else:
        cat = "callC"

    if start in SYMS:
        note.append("sym:" + SYMS[start])
    return dict(code_end=code_end, size=size, ninsn=len(ins), calls=calls,
                shape=fn.shape(start, code_end) if clamp > start else "?",
                cat=cat, note=" ".join(note))


def main():
    out = os.path.expanduser("~/decomp/diehard-stv/notes/function_triage.tsv")
    groups = fn.units()
    rows = []
    for ustart, code_end, uend, members in groups:
        ms = sorted(members)
        for idx, m in enumerate(ms):
            nxt = ms[idx + 1] if idx + 1 < len(ms) else uend
            f = classify(m, nxt)
            sh = "0" if f["shape"] == "O0" else "o"
            rows.append((m, ustart, f["size"], sh, f["calls"], f["cat"], f["note"]))

    with open(out, "w") as fh:
        fh.write("sh2_addr\tunit_start\tsize\tshape\tcall_count\tcategory\tnote\n")
        for m, u, sz, sh, c, cat, note in rows:
            fh.write(f"{m+BASE:07x}\t{u+BASE:07x}\t0x{sz:x}\t{sh}\t{c}\t{cat}\t{note}\n")

    # ---- summary to stdout ----
    from collections import Counter
    cc = Counter(r[5] for r in rows)
    print(f"units={len(groups)} functions={len(rows)}")
    for k in ("asm", "leafC", "callC", "misseg"):
        print(f"  {k:7s} {cc.get(k,0)}")
    print("wrote", out)


if __name__ == "__main__":
    main()
