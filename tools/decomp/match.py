#!/usr/bin/env python3
"""Per-function match oracle for the Die Hard Arcade (ST-V) decomp grind.

usage: match.py <addr_hex> <candidate.c> [--flags "-optimize=1"]

Compiles candidate.c with the real Hitachi SHC v5.0r32 (under wibo), converts
to ELF, extracts .text, and compares to the ROM function's extent (code+pool)
at <addr> (a cart.bin offset; SH2 addr = offset + 0x02000000).

Prints: EXACT? , byte score, instruction-level score, and an instruction diff.
The candidate must define exactly one function (any name).
"""
import sys, os, subprocess, tempfile, argparse
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fn
import capstone as cs

HOME = os.path.expanduser("~")
COMP = f"{HOME}/decomp/diehard-stv/tools/compilers"
SHCDIR = f"{COMP}/shc-v5.0r32"
CROSS = f"{HOME}/decomp/diehard-stv/tools/cross/bin"
MD = cs.Cs(cs.CS_ARCH_SH, cs.CS_MODE_SH2 | cs.CS_MODE_BIG_ENDIAN); MD.skipdata = True


def dis(b):
    return [f"{i.mnemonic} {i.op_str}" for i in MD.disasm(b, 0)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("addr"); ap.add_argument("cand")
    ap.add_argument("--flags", default="-optimize=1")
    a = ap.parse_args()
    addr = int(a.addr, 16)
    code_end, end, pool, tg = fn.extent(addr)
    target = fn.ROM[addr:end]
    tcode = fn.ROM[addr:code_end]

    work = tempfile.mkdtemp(prefix="match_")
    subprocess.run(["bash", "-c", f"cp -r {SHCDIR}/bin/. {work}/"])
    with open(f"{work}/src.c", "w", newline="\r\n") as f:
        f.write(open(a.cand).read())
    r = subprocess.run(["bash", "-c",
        f"cd {work} && SHC_LIB=. SHC_TMP=. {COMP}/wibo ./shc.exe src.c "
        f"-comment=nonest -cpu=sh2 -endian=big -sjis -string=const {a.flags} -object=src.obj"],
        capture_output=True, text=True)
    if not os.path.exists(f"{work}/src.obj"):
        print("COMPILE_FAIL"); print((r.stdout or "")[-800:]); sys.exit(2)
    subprocess.run(["bash", "-c", f"python3 {COMP}/rof2elf.py {work}/src.obj {work}/src.elf --isa=sh2"], capture_output=True)
    subprocess.run(["bash", "-c", f"{CROSS}/sh-elf-objcopy -O binary -j .text {work}/src.elf {work}/t.bin"], capture_output=True)
    got = open(f"{work}/t.bin", "rb").read()

    exact = got == target
    n = min(len(got), len(target)); same = sum(got[i] == target[i] for i in range(n))
    denom = max(len(got), len(target), 1)
    gi = dis(got[:max(code_end - addr, 0)] if len(got) >= code_end - addr else got)
    ti = dis(tcode)
    m = min(len(gi), len(ti)); imatch = sum(gi[i] == ti[i] for i in range(m))
    idenom = max(len(gi), len(ti), 1)
    print(f"ADDR 0x{addr:x}  flags[{a.flags}]")
    print(f"{'*** EXACT 100% ***' if exact else 'not exact'} | bytes {same}/{denom} ({100*same/denom:.0f}%) "
          f"| insns {imatch}/{idenom} ({100*imatch/idenom:.0f}%)")
    print("target:", target.hex())
    print("got:   ", got.hex())
    if not exact:
        print("--- insn diff (target | got) ---")
        for i in range(max(len(ti), len(gi))):
            t = ti[i] if i < len(ti) else ""
            g = gi[i] if i < len(gi) else ""
            print(f"  {t:30s} | {g}{'' if t == g else '   <-'}")


if __name__ == "__main__":
    main()
