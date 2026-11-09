#!/usr/bin/env python3
"""Whole-COMPILATION-UNIT match oracle for the Die Hard Arcade (ST-V/SH-2) decomp.

Isolated-function matching stalls because SHC/GCC codegen is stateful per source
file (literal-pool layout, string pooling, register/alloc order carry across the
functions in one translation unit). This grinds a whole fn.units() pool-group as
ONE .c, giving the compiler the file context it needs, and scores it against the
ROM both per-function and for the whole unit.

What it does:
  1. Resolve the unit at <addr> via fn.units()  (addr = sh2 0x22xxxxx or raw cart
     offset; must be a unit START, else the containing unit is used with a warning).
  2. Concatenate each member function's C, in ascending-address (file) order, from
     a source dir (default: drafts/<sh2>.c) into one src.c, optionally prefixed by
     a --context header.
  3. Compile with a PLUGGABLE recipe (default `shc`, copied verbatim from match.py;
     `gcc` drives the Cygnus GCC 2.7 cc1 for the SHC-vs-GCC investigation).
  4. objcopy the .text and byte-compare against the ROM unit extent [ustart,uend).
  5. Report per-function and whole-unit byte match % (and whole-unit insn %).

Compiler is swappable so the compiler-identity investigation can A/B without
touching match.py:
    --recipe shc|gcc           which toolchain          (default shc)
    --shc-version shc-v5.0r32   SHC build to use         (r10/r26/r28/r31/r32)
    --flags "-optimize=1"       override compiler flags
    --src DIR                   candidate .c dir         (default drafts/)
    --context HDR.h             header prepended to the TU
    --keep                      keep the temp workdir and print its path

usage:
    python unit_grind.py 0x2214918
    python unit_grind.py 0x2214918 --src ~/decomp/diehard-stv/src --context inc/types.h
    python unit_grind.py 0x2214918 --recipe gcc --flags "-O1 -m2"
    python unit_grind.py 0x2214918 --shc-version shc-v5.0r26

NOTE: copies match.py's compile logic on purpose -- do NOT import match.py here,
and do NOT modify match.py/fn.py/m2c_fn.py (a compiler agent is using them).
"""
import argparse
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fn
import capstone as cs

BASE = 0x02000000  # sh2 addr = cart offset + BASE  (matches drafts/symbols.txt)
HOME = os.path.expanduser("~")
COMP = f"{HOME}/decomp/diehard-stv/tools/compilers"
CROSS = f"{HOME}/decomp/diehard-stv/tools/cross/bin"
DRAFTS = f"{HOME}/decomp/diehard-stv/drafts"
CYGNUS = f"{COMP}/cygnus-2.7-96Q3"

MD = cs.Cs(cs.CS_ARCH_SH, cs.CS_MODE_SH2 | cs.CS_MODE_BIG_ENDIAN)
MD.skipdata = True


def dis(b):
    return [f"{i.mnemonic} {i.op_str}" for i in MD.disasm(b, 0)]


def sh(cmd):
    return subprocess.run(["bash", "-c", cmd], capture_output=True, text=True)


# --------------------------------------------------------------------------- #
# Compiler recipes. Each takes (src_c_path, workdir, args) and returns
# (text_bytes | None, log). Adding a recipe = adding a function to RECIPES.
# --------------------------------------------------------------------------- #
def recipe_shc(src_c, work, args):
    """COPIED from match.py: real Hitachi SHC under wibo -> rof2elf -> objcopy."""
    shcdir = f"{COMP}/{args.shc_version}"
    flags = args.flags if args.flags is not None else "-optimize=1"
    sh(f"cp -r {shcdir}/bin/. {work}/")
    # SHC wants CRLF source (match.py writes newline="\r\n"); rewrite in place.
    data = open(src_c, "rb").read().replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
    open(f"{work}/src.c", "wb").write(data)
    r = sh(f"cd {work} && SHC_LIB=. SHC_TMP=. {COMP}/wibo ./shc.exe src.c "
           f"-comment=nonest -cpu=sh2 -endian=big -sjis -string=const {flags} "
           f"-object=src.obj")
    log = (r.stdout or "") + (r.stderr or "")
    if not os.path.exists(f"{work}/src.obj"):
        return None, "COMPILE_FAIL\n" + log[-1500:]
    sh(f"python3 {COMP}/rof2elf.py {work}/src.obj {work}/src.elf --isa=sh2")
    o = sh(f"{CROSS}/sh-elf-objcopy -O binary -j .text {work}/src.elf {work}/t.bin")
    if not os.path.exists(f"{work}/t.bin"):
        return None, "OBJCOPY_FAIL\n" + (o.stderr or "")
    text = open(f"{work}/t.bin", "rb").read()
    if not text:
        return None, "EMPTY_TEXT (no code emitted)\n" + log[-1500:]
    return text, log


def recipe_gcc(src_c, work, args):
    """Cygnus GCC 2.7 (cc1) -> sh-elf-as -> objcopy. For the SHC-vs-GCC probe.

    cc1 is the bare compiler (no cpp): feed it already-simple C or a --context
    header with the needed typedefs. Flags/endianness may need tuning per the
    investigation; stderr is surfaced on failure so they can be dialed in.
    """
    cc1 = f"{CYGNUS}/native64/cc1-native64"
    if not os.path.exists(cc1):
        cc1 = f"{CYGNUS}/native/cc1-native"
    flags = args.flags if args.flags is not None else "-O1"
    r = sh(f"{cc1} {src_c} {flags} -o {work}/src.s 2>&1")
    log = r.stdout or ""
    if not os.path.exists(f"{work}/src.s"):
        return None, "CC1_FAIL\n" + log[-1500:]
    a = sh(f"{CROSS}/sh-elf-as -big -isa=sh2 {work}/src.s -o {work}/src.o")
    if not os.path.exists(f"{work}/src.o"):
        return None, "AS_FAIL\n" + (a.stderr or "")
    sh(f"{CROSS}/sh-elf-objcopy -O binary -j .text {work}/src.o {work}/t.bin")
    if not os.path.exists(f"{work}/t.bin"):
        return None, "OBJCOPY_FAIL"
    text = open(f"{work}/t.bin", "rb").read()
    if not text:
        return None, "EMPTY_TEXT (cc1 emitted no code -- check flags/typedefs)\n" + log[-1500:]
    return text, log


RECIPES = {"shc": recipe_shc, "gcc": recipe_gcc}


# --------------------------------------------------------------------------- #
def find_unit(off):
    """Return (ustart, code_end, uend, members) for the unit at/containing off."""
    for ustart, code_end, uend, members in fn.units():
        if ustart == off:
            return ustart, code_end, uend, members, True
    for ustart, code_end, uend, members in fn.units():
        if ustart <= off < uend:
            return ustart, code_end, uend, members, False
    return None


def pct(a, b):
    return 100.0 * a / b if b else 0.0


def main():
    ap = argparse.ArgumentParser(description="whole-unit match oracle")
    ap.add_argument("addr", help="unit start, sh2 (0x22xxxxx) or cart offset")
    ap.add_argument("--recipe", choices=sorted(RECIPES), default="shc")
    ap.add_argument("--shc-version", default="shc-v5.0r32")
    ap.add_argument("--flags", default=None, help="override compiler flags")
    ap.add_argument("--src", default=DRAFTS, help="dir of <sh2>.c candidates")
    ap.add_argument("--context", default=None, help="header prepended to the TU")
    ap.add_argument("--keep", action="store_true", help="keep + print the workdir")
    a = ap.parse_args()

    addr = int(a.addr, 16)
    off = addr - BASE if addr >= BASE else addr
    u = find_unit(off)
    if not u:
        print(f"no unit at {off+BASE:#x}"); sys.exit(2)
    ustart, code_end, uend, members, exact_start = u
    members = sorted(members)
    if not exact_start:
        print(f"WARNING: {off+BASE:#x} is inside unit {ustart+BASE:#x}, not its "
              f"start; grinding the whole unit.")

    # --- assemble the translation unit in file order ---
    parts, missing = [], []
    if a.context:
        parts.append(open(a.context).read())
    for m in members:
        p = f"{a.src}/{m+BASE:x}.c"
        if os.path.exists(p):
            parts.append(f"/* ---- f_{m+BASE:x} ---- */\n" + open(p).read())
        else:
            missing.append(m)
    source = "\n\n".join(parts) + "\n"

    work = tempfile.mkdtemp(prefix=f"unitgrind_{off+BASE:x}_")
    src_c = f"{work}/unit.c"
    open(src_c, "w").write(source)

    print(f"UNIT {ustart+BASE:07x}-{uend+BASE:07x}  len 0x{uend-ustart:x}  "
          f"funcs {len(members)}  recipe={a.recipe}"
          f"{'/'+a.shc_version if a.recipe=='shc' else ''}")
    if missing:
        print("  missing src for: " + " ".join(f"{m+BASE:x}" for m in missing)
              + "  (those regions will not match)")

    got, log = RECIPES[a.recipe](src_c, work, a)
    if got is None:
        print(log)
        if a.keep:
            print("workdir:", work)
        sys.exit(2)

    target = fn.ROM[ustart:uend]

    # --- whole-unit score ---
    n = min(len(got), len(target))
    same = sum(got[i] == target[i] for i in range(n))
    denom = max(len(got), len(target), 1)
    exact = got == target
    gi, ti = dis(got), dis(target)
    im = min(len(gi), len(ti)); isame = sum(gi[i] == ti[i] for i in range(im))
    idenom = max(len(gi), len(ti), 1)
    print(f"{'*** UNIT EXACT 100% ***' if exact else 'unit not exact'} | "
          f"bytes {same}/{denom} ({pct(same,denom):.1f}%) | "
          f"insns {isame}/{idenom} ({pct(isame,idenom):.1f}%) | "
          f"got {len(got)}B target {len(target)}B")

    # --- per-function score (slice by ROM member boundaries) ---
    print("  per-function:")
    for i, m in enumerate(members):
        nxt = members[i + 1] if i + 1 < len(members) else uend
        o, ln = m - ustart, nxt - m
        t = target[o:o + ln]
        g = got[o:o + ln] if len(got) >= o else b""
        k = min(len(g), len(t))
        s = sum(g[j] == t[j] for j in range(k))
        d = max(len(g), len(t), 1)
        tag = "EXACT" if g == t and t else f"{pct(s,d):5.1f}%"
        mark = " (no src)" if m in missing else ""
        last = " +pool" if i + 1 == len(members) and uend > code_end else ""
        print(f"    f_{m+BASE:07x}  0x{ln:<4x}  {tag}{last}{mark}")

    if a.keep:
        print("workdir:", work, "\nsource:", src_c)
    else:
        sh(f"rm -rf {work}")


if __name__ == "__main__":
    main()
