#!/usr/bin/env python3
"""Decompile a ROM function with an EXPLICIT code-end (bounded extent).

fn.extent() is a heuristic walker. Around literal pools it sometimes OVER-RUNS a
function's real `rts`, swallowing the following (separate) translation units into
one giant body (e.g. 0x23ac9c runs its rts at 0x23b01a but extent reaches into
0x23b01e + 0x23b1c0). m2c then produces a merged, wrong function.

This wrapper lets you pin the real end. It monkeypatches fn.extent AT RUNTIME for
the one start address (it never edits the read-only fn.py / m2c_fn.py): the code
window is clamped to [start, end), branch targets past `end` are dropped, and the
literal pool that to_asm emits is naturally limited to what the kept instructions
reference. It then runs the normal braf_rewrite pipeline (or plain m2c_fn with
--no-braf) on the clamped function.

Find the real end with `fn.py 0xSTART` (the first true `rts`+delay-slot, i.e. the
address just after the delay slot = the next function's start).

usage:
  decomp_bounded.py 0x23ac9c name --end 0x23b01e            # rewrite + run m2c
  decomp_bounded.py 0x23ac9c name --end 0x23b01e --asm      # print asm only
  decomp_bounded.py 0xADDR    name --end 0xEND --no-braf    # skip braf rewrite
"""
import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import fn            # noqa: E402
import m2c_fn        # noqa: E402
import braf_rewrite  # noqa: E402

_BRA_OUT = re.compile(r"^\tbra\tfunc_([0-9a-f]+)\s*$")


def tailify_out_of_window_bra(asm: str) -> str:
    """Convert a direct `bra func_<addr>` (unconditional branch to an address
    OUTSIDE the clamped window, i.e. a tail-jump to a separate/shared function)
    into a tail call, mirroring braf_rewrite's braf handling: hoist the delay
    slot, then `bsr target; nop; rts; nop`. In-window branches use .L labels and
    are untouched; only to_asm's `func_<addr>` (out-of-window) targets match."""
    lines = asm.split("\n")
    out = []
    i = 0
    while i < len(lines):
        m = _BRA_OUT.match(lines[i])
        if not m:
            out.append(lines[i])
            i += 1
            continue
        slot = lines[i + 1] if i + 1 < len(lines) else "\tnop"
        out.append(slot)
        out.append(f"\tbsr\tfunc_{m.group(1)}\t! was bra (tail)")
        out.append("\tnop")
        out.append("\trts")
        out.append("\tnop")
        i += 2
    return "\n".join(out)


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("addr")
    ap.add_argument("name", nargs="?")
    ap.add_argument("--end", required=True, help="exclusive code end (cart hex)")
    ap.add_argument("--asm", action="store_true")
    ap.add_argument("--no-braf", action="store_true",
                    help="use plain m2c_fn (no braf/bsrf rewrite)")
    args, rest = ap.parse_known_args()
    start = int(args.addr, 16)
    end = int(args.end, 16)
    name = args.name or f"f_{start:x}"

    _orig = fn.extent

    def bounded(s, limit=0x2000):
        ce, ee, pool, tg = _orig(s, limit)
        if s == start:
            ce = min(ce, end)
            ee = min(ee, end)
            tg = {t for t in tg if start < t < end}
        return ce, ee, pool, tg

    fn.extent = bounded  # read-only source untouched; attribute patched at runtime
    try:
        if args.no_braf:
            asm = m2c_fn.to_asm(start, name)
        else:
            asm, _res, _resisted = braf_rewrite.rewrite(start, name)
        asm = tailify_out_of_window_bra(asm)
    finally:
        pass  # leave patch in place for the life of the process

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
