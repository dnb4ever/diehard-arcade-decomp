#!/usr/bin/env python3
"""Per-function decomp-permuter driver for Die Hard Arcade (Sega ST-V, Hitachi SH-2, big-endian).

usage:
    permute.py <addr_hex> <base.c> [--name FUNC] [--flags "..."]
               [--out DIR] [--time SECONDS] [--threads N] [--no-run]

Given a ROM function address (a cart.bin offset; SH2 addr = offset + 0x02000000) and a
candidate C file, this:
  1. reads the function's extent bytes (code + literal pool) from cart.bin via fn.extent,
  2. wraps those raw SH-2 bytes in a big-endian SH-2 ELF object  ->  target.o,
  3. writes compile.sh (the real Hitachi SHC v5.0r32 + rof2elf recipe), settings.toml, base.c,
  4. runs decomp-permuter on the scratch dir and reports the best score.

The scratch dir defaults to <this_dir>/perm/scratch_<addr>/.
A score of 0 means the candidate's asm matches the ROM function exactly.
"""
import argparse
import os
import re
import signal
import subprocess
import sys
import time

HOME = os.path.expanduser("~")
BASE = f"{HOME}/decomp/diehard-stv"
DECOMP = f"{BASE}/tools/decomp"
COMP = f"{BASE}/tools/compilers"
CROSS = f"{BASE}/tools/cross/bin"
PERMUTER = f"{BASE}/tools/decomp-permuter"
PY = f"{DECOMP}/venv/bin/python"
PERMROOT = f"{DECOMP}/perm"
OBJDUMP = f"{CROSS}/sh-elf-objdump"
AS = f"{CROSS}/sh-elf-as"
DEFAULT_FLAGS = "-comment=nonest -cpu=sh2 -endian=big -sjis -string=const -optimize=1"

sys.path.insert(0, DECOMP)
import fn  # read-only helper: fn.ROM (cart.bin), fn.extent(addr)


def extent_bytes(addr):
    """(raw_bytes, code_len, extent_len) for the function at `addr` (cart.bin offset)."""
    code_end, end, _pool, _targets = fn.extent(addr)
    return fn.ROM[addr:end], code_end - addr, end - addr


def detect_func_name(src):
    """Best-effort: name of the last top-level function *definition* in C source."""
    s = re.sub(r"/\*.*?\*/", " ", src, flags=re.S)
    s = re.sub(r"//.*", " ", s)
    names = re.findall(r"\b([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{", s)
    return names[-1] if names else None


def compile_sh(flags):
    return f"""#!/usr/bin/env bash
# Compile one C file to an SH-2 ELF .o using the real Hitachi SHC v5.0r32 + rof2elf.
# Invoked by decomp-permuter as:  ./compile.sh <input.c> -o <output.o>
set -euo pipefail
INPUT="$(realpath "$1")"
OUTPUT="$(realpath "$3")"
COMP="{COMP}"
SHCBIN="$COMP/shc-v5.0r32/bin"
PY="{PY}"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
# symlink the SHC toolchain into a private writable work dir (cheap; no 5MB copy per run)
for f in "$SHCBIN"/*; do ln -s "$f" "$work/"; done
# SHC wants DOS line endings and SJIS; normalize then add CR
tr -d '\\r' < "$INPUT" | sed 's/$/\\r/' > "$work/src.c"
cd "$work"
SHC_LIB=. SHC_TMP=. "$COMP/wibo" ./shc.exe src.c {flags} -object=src.obj >/dev/null 2>&1
"$PY" "$COMP/rof2elf.py" src.obj src.elf --isa=sh2 >/dev/null 2>&1
cp src.elf "$OUTPUT"
"""


def settings_toml(func_name):
    return (
        f'func_name = "{func_name}"\n'
        'compiler_type = "base"\n'
        f'objdump_command = "{OBJDUMP} -drz"\n'
    )


def write_target_o(raw, func_name, out_dir):
    """Wrap raw SH-2 bytes as a big-endian SH-2 ELF object with a symbol for the function."""
    s_path = os.path.join(out_dir, "target.s")
    o_path = os.path.join(out_dir, "target.o")
    body = ", ".join(f"0x{b:02x}" for b in raw)
    with open(s_path, "w") as f:
        f.write("\t.section .text\n")
        f.write(f"\t.globl {func_name}\n")
        f.write(f"\t.type {func_name}, @function\n")
        f.write(f"{func_name}:\n")
        f.write(f"\t.byte {body}\n")
    subprocess.run([AS, "--isa=sh2", "--big", s_path, "-o", o_path], check=True)
    return o_path


def setup_scratch(out_dir, raw, base_c_src, func_name, flags):
    os.makedirs(out_dir, exist_ok=True)
    cs = os.path.join(out_dir, "compile.sh")
    with open(cs, "w") as f:
        f.write(compile_sh(flags))
    os.chmod(cs, 0o755)
    with open(os.path.join(out_dir, "settings.toml"), "w") as f:
        f.write(settings_toml(func_name))
    with open(os.path.join(out_dir, "base.c"), "w") as f:
        f.write(base_c_src)
    write_target_o(raw, func_name, out_dir)
    return out_dir


def sanity_compile(out_dir):
    """Compile base.c via compile.sh to prove the recipe works; returns (ok, msg)."""
    base_o = os.path.join(out_dir, "base.o")
    r = subprocess.run(
        ["bash", os.path.join(out_dir, "compile.sh"),
         os.path.join(out_dir, "base.c"), "-o", base_o],
        capture_output=True, text=True,
    )
    if r.returncode != 0 or not os.path.exists(base_o):
        return False, (r.stderr or r.stdout or "").strip()[-800:]
    return True, base_o


def run_permuter(out_dir, seconds, threads):
    """Run decomp-permuter with a wall-clock budget; return (best_score, base_score, lines)."""
    cmd = [PY, os.path.join(PERMUTER, "permuter.py"), out_dir,
           "--stop-on-zero", "--quiet", "-j", str(threads)]
    env = dict(os.environ, PYTHONUNBUFFERED="1")
    p = subprocess.Popen(cmd, cwd=PERMUTER, stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT, text=True, bufsize=1,
                         start_new_session=True, env=env)
    base_score = best_score = None
    lines = []
    deadline = time.time() + seconds
    try:
        while True:
            if p.poll() is not None:
                lines.extend(x.rstrip("\n") for x in p.stdout.readlines())
                break
            if time.time() > deadline:
                break
            line = p.stdout.readline()
            if not line:
                if p.poll() is not None:
                    break
                continue
            line = line.rstrip("\n")
            lines.append(line)
            print(line, flush=True)
            m = re.search(r"base score = (\d+)", line)
            if m:
                base_score = int(m.group(1))
                best_score = base_score if best_score is None else min(best_score, base_score)
            m = re.search(r"(?:found new best score!|found a better score!|tied best score!)"
                          r"\s*\((\d+)", line)
            if m:
                v = int(m.group(1))
                best_score = v if best_score is None else min(best_score, v)
            if "Found zero score!" in line:
                best_score = 0
                break
    finally:
        if p.poll() is None:
            try:
                os.killpg(os.getpgid(p.pid), signal.SIGTERM)
                time.sleep(1)
                os.killpg(os.getpgid(p.pid), signal.SIGKILL)
            except (ProcessLookupError, PermissionError):
                pass
    return best_score, base_score, lines


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("addr", help="function address as a cart.bin offset, e.g. 0x214a06")
    ap.add_argument("base_c", help="candidate C file defining the function")
    ap.add_argument("--name", help="function name in base.c (default: auto-detect)")
    ap.add_argument("--flags", default=DEFAULT_FLAGS, help="SHC compile flags")
    ap.add_argument("--out", help="scratch dir (default: perm/scratch_<addr>)")
    ap.add_argument("--time", type=int, default=120, help="wall-clock seconds (default 120)")
    ap.add_argument("--threads", type=int, default=4, help="permuter worker threads")
    ap.add_argument("--no-run", action="store_true", help="set up the dir but don't run")
    a = ap.parse_args()

    addr = int(a.addr, 16)
    base_c_src = open(a.base_c).read()
    func_name = a.name or detect_func_name(base_c_src)
    if not func_name:
        sys.exit("Could not detect a function name in base.c; pass --name.")

    raw, code_len, ext_len = extent_bytes(addr)
    out_dir = a.out or os.path.join(PERMROOT, f"scratch_{addr:x}")
    setup_scratch(out_dir, raw, base_c_src, func_name, a.flags)

    print(f"addr 0x{addr:x}  func '{func_name}'  code {code_len:#x}  extent {ext_len:#x} "
          f"({ext_len} bytes)")
    print(f"scratch: {out_dir}")

    ok, msg = sanity_compile(out_dir)
    if not ok:
        print("WARNING: base.c failed to compile via compile.sh:")
        print(msg)
    else:
        print("base.c compiles via SHC recipe OK.")

    if a.no_run:
        print("--no-run: setup complete.")
        return

    best, base, _ = run_permuter(out_dir, a.time, a.threads)
    print("\n=== RESULT ===")
    print(f"base score : {base}")
    print(f"best score : {best}" + ("   (EXACT MATCH)" if best == 0 else ""))
    print(f"improved candidates saved in: {out_dir}")


if __name__ == "__main__":
    main()
