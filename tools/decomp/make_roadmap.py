#!/usr/bin/env python3
"""Rank compilation units into a grind roadmap from function_triage.tsv.

A unit is a good first target when it is: small, starts on a clean file
boundary, is made almost entirely of real C (leafC/callC), has no missegmented
members and few asm-origin deps, and (bonus) already has m2c drafts that parsed.
Writes notes/target_roadmap.md.  Run on p620 with the venv python.
"""
import os, re, sys, glob
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fn

BASE = 0x02000000
NOTES = os.path.expanduser("~/decomp/diehard-stv/notes")
DRAFTS = os.path.expanduser("~/decomp/diehard-stv/drafts")
TSV = f"{NOTES}/function_triage.tsv"

# ---- load triage rows -------------------------------------------------------
rows = []  # dict per function
with open(TSV) as fh:
    next(fh)
    for ln in fh:
        a, u, sz, sh, cc, cat, note = (ln.rstrip("\n").split("\t") + [""])[:7]
        rows.append(dict(addr=int(a, 16), unit=int(u, 16), size=int(sz, 16),
                         shape=sh, calls=int(cc), cat=cat, note=note))

# ---- m2c status + draft presence from INDEX.md / drafts/ --------------------
m2c = {}
idx = f"{DRAFTS}/INDEX.md"
if os.path.exists(idx):
    for ln in open(idx):
        m = re.match(r"\|\s*[0-9a-f]+\s*\|\s*([0-9a-f]+)\s*\|.*\|\s*(OK|M2C_ERROR[^|]*|CRASH[^|]*)\s*\|", ln)
        if m:
            m2c[int(m[1], 16)] = m[2].strip()
drafted = {int(os.path.basename(p)[:-2], 16) for p in glob.glob(f"{DRAFTS}/*.c")}

# ---- unit aggregation (use fn.units for authoritative boundaries) -----------
by_unit = {}
for r in rows:
    by_unit.setdefault(r["unit"], []).append(r)

units = []
for ustart, code_end, uend, members in fn.units():
    u = ustart + BASE
    fns = sorted(by_unit.get(u, []), key=lambda r: r["addr"])
    if not fns:
        continue
    cats = [f["cat"] for f in fns]
    n = len(fns)
    nmiss = cats.count("misseg")
    nasm = cats.count("asm")
    nleaf = cats.count("leafC")
    ncall = cats.count("callC")
    realC = nleaf + ncall
    clean_start = (members[0] + BASE) == u
    ok = sum(1 for f in fns if m2c.get(f["addr"], "") == "OK")
    err = sum(1 for f in fns if m2c.get(f["addr"], "").startswith("M2C_ERROR"))
    hasdraft = sum(1 for f in fns if f["addr"] in drafted)
    tlen = uend - ustart
    units.append(dict(u=u, end=uend + BASE, tlen=tlen, n=n, nmiss=nmiss,
                      nasm=nasm, nleaf=nleaf, ncall=ncall, realC=realC,
                      clean=clean_start, ok=ok, err=err, draft=hasdraft, fns=fns))

# ---- scoring ----------------------------------------------------------------
# Grind-first = pure real C, clean boundary, small, drafts that parsed.
def score(u):
    if u["nmiss"] or u["n"] == 0:
        return -1  # never grind a unit with a missegmented member first
    s = 0.0
    s += 100 * (u["realC"] / u["n"])          # fraction real C
    s += 40 if u["clean"] else 0              # starts at a file boundary
    s -= 12 * u["nasm"]                        # asm deps are a drag
    s += 30 * (u["ok"] / u["n"])              # m2c already parses members
    s += min(u["draft"], u["n"]) * 4          # drafts exist to concatenate
    s -= u["tlen"] / 120.0                     # smaller is easier
    s += 6 if 2 <= u["n"] <= 5 else 0          # digestible unit size
    return s

for u in units:
    u["score"] = score(u)

ranked = sorted([u for u in units if u["score"] >= 0],
                key=lambda u: u["score"], reverse=True)

# ---- category totals --------------------------------------------------------
from collections import Counter
tot = Counter(r["cat"] for r in rows)

# ---- write markdown ---------------------------------------------------------
def frow(u):
    flags = []
    if u["clean"]:
        flags.append("clean-start")
    if u["nasm"]:
        flags.append(f"{u['nasm']}asm")
    draft = f"{u['ok']}ok/{u['err']}err" if u["draft"] else "-"
    return (f"| `{u['u']:07x}` | 0x{u['tlen']:x} | {u['n']} | {u['nleaf']} | "
            f"{u['ncall']} | {u['nasm']} | {draft} | {', '.join(flags) or '-'} |")

out = [
"# Die Hard Arcade (ST-V/SH-2) — Unit Grind Roadmap",
"",
"Auto-generated from `notes/function_triage.tsv` + `fn.units()` + `drafts/INDEX.md`.",
"Pivot: match whole COMPILATION UNITS (fn.units() pool groups) in file order, because",
"SHC/GCC codegen is stateful per source file. Grind the cleanest units first.",
"",
"## Category totals (all 1272 functions, 258 units)",
"",
f"- **asm-origin** (keep as asm, do NOT grind): {tot['asm']} — tiny call-free math/runtime-lib",
f"  (64-bit mul / MAC / div primitives with non-standard r0/r1 return, e.g. fixmul_16_16,",
f"  vec3_dot_fixed). `mac.l`/`div1` seen INSIDE larger C functions are inlined, not asm — those",
f"  stay `callC` with the opcode noted.",
f"- **leafC** (leaf C target, no calls, real logic): {tot['leafC']}",
f"- **callC** (call-bearing C target): {tot['callC']}",
f"- **misseg** (fn.py boundary error — data decoded as code, NOT a real function): {tot['misseg']}",
"",
"### Where misseg clusters (avoid)",
"- **Low zone `0x2200400–0x2201020`**: the warned region. Interleaved data tables and real",
"  functions; drafts here (f_20042e, f_20062a, f_200f44 …) are M2C_ERROR garbage.",
"- **High tail `~0x2250000–0x228ffff`**: `fn.units()` runs past the end of real code into",
"  data tables near `TEXT_END` (0x290000). Most 'functions' there are data.",
"- The clean, grindable C is the **middle band (~0x2201000–0x224f000)**.",
"",
"## Top grind targets (ranked)",
"",
"Score favors: 100%-real-C members, clean file-boundary start, no misseg members,",
"few asm deps, members whose m2c already parsed (draft exists to concatenate), small size.",
"",
"| unit | len | #fn | leafC | callC | asm | m2c(ok/err) | flags |",
"|------|-----|-----|-------|-------|-----|-------------|-------|",
]
out += [frow(u) for u in ranked[:30]]
out += [
"",
"## How to use with unit_grind.py",
"",
"```",
"# grind the top unit (concatenate its member .c from drafts/, compile as one TU):",
"python unit_grind.py 0x<unit_addr>",
"# swap compiler recipe when the SHC-vs-GCC investigation lands:",
"python unit_grind.py 0x<unit_addr> --recipe gcc",
"```",
"",
"Start at the top of the table and walk down. A unit at 100% real C with `clean-start`",
"and existing drafts is the ideal first match: the whole-TU context gives SHC the",
"file-order state that isolated-function matching was missing.",
"",
"## Pure-leaf-C quick wins (single clean leaf units, for warm-up / oracle calibration)",
"",
]
leafunits = [u for u in ranked if u["n"] == 1 and u["nleaf"] == 1][:12]
out.append("| unit | len | m2c |")
out.append("|------|-----|-----|")
for u in leafunits:
    st = "OK" if u["ok"] else ("err" if u["err"] else "-")
    out.append(f"| `{u['u']:07x}` | 0x{u['tlen']:x} | {st} |")

with open(f"{NOTES}/target_roadmap.md", "w") as fh:
    fh.write("\n".join(out) + "\n")

print(f"ranked {len(ranked)} grindable units; wrote {NOTES}/target_roadmap.md")
print("top 10:")
for u in ranked[:10]:
    print(f"  {u['u']:07x} len 0x{u['tlen']:<5x} n={u['n']} realC={u['realC']} "
          f"asm={u['nasm']} miss={u['nmiss']} ok={u['ok']} score={u['score']:.0f}")
