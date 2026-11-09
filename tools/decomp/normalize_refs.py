#!/usr/bin/env python3
"""Normalize non-canonical func_ call-site references to canonical sh2 form.
cart-offset (00xxxxxx)+0x02000000; runtime/HWRAM (06xxxxxx)-0x03E04000.
Only rewrites references whose form differs from canonical; leaves canonical ones."""
import pathlib, re
SRC = pathlib.Path("project/src_functional")
pat = re.compile(r"func_([0-9a-fA-F]{8})\b")

def canon(a):
    if 0x00200000 <= a <= 0x0029ffff:
        return a + 0x02000000
    if 0x06000000 <= a <= 0x06100000:
        return a - 0x03E04000
    return a

changed_files = 0
total_subs = 0
for p in sorted(SRC.glob("*.c")):
    txt = p.read_text(errors="replace")
    subs = {}
    for m in set(pat.findall(txt)):
        a = int(m, 16)
        c = canon(a)
        if c != a:
            subs[m] = "%08x" % c
    if not subs:
        continue
    new = txt
    n = 0
    for raw, cc in subs.items():
        new, k = re.subn(r"func_" + raw + r"\b", "func_" + cc, new)
        n += k
    if new != txt:
        p.write_text(new)
        changed_files += 1
        total_subs += n
        print("fixed %-14s: %s" % (p.name, ", ".join("func_%s->func_%s" % (r, c) for r, c in subs.items())))
print("\nchanged %d files, %d substitutions" % (changed_files, total_subs))
