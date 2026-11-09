#!/usr/bin/env python3
"""llm_gen.py - local-model bulk decompiler for Die Hard Arcade (Sega ST-V, SH-2).

Selects easy undone leaf functions from census, builds context from the SH-2
toolchain (m2c_fn/braf_resolve/ptr_resolve), prompts the shared Ollama 122B,
verifies with gcc -fsyntax-only, and STAGES passing C for Mac-Claude review.
Never touches git or project/src_functional/ (writes only to tools/decomp/llm_stage/).

Run on p620:  tools/decomp/venv/bin/python3 tools/decomp/llm_gen.py --easy --limit 10
"""
import argparse, csv, json, re, subprocess, sys, time, urllib.request, pathlib

TOOLS = pathlib.Path(__file__).resolve().parent          # ~/decomp/diehard-stv/tools/decomp
ROOT  = TOOLS.parent.parent                               # ~/decomp/diehard-stv
PY    = str(TOOLS / "venv" / "bin" / "python3")
SRC   = ROOT / "project" / "src_functional"
STAGE = TOOLS / "llm_stage"
OLLAMA = "http://127.0.0.1:11434/api/chat"   # native endpoint: supports think:false
MODEL  = "qwen3.5:122b-a10b-q4_K_M"

def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True)

def tool_out(script, *args):
    return run([PY, str(TOOLS / script), *args]).stdout

def census_rows():
    out = tool_out("census.py", "--tsv")
    return list(csv.DictReader(out.splitlines(), delimiter="\t"))

def done_set():
    s = set()
    for p in SRC.glob("*.c"):
        try: s.add(int(p.stem, 16))
        except ValueError: pass
    return s

def select(limit, max_insn, addrs=None):
    rows_all = census_rows()
    if addrs:
        want = {int(a, 16) for a in addrs}
        return [r for r in rows_all if int(r["sh2_addr"], 16) in want]
    done = done_set()
    skip = set()
    sf = TOOLS / "llm_skip.txt"
    if sf.exists():
        for line in sf.read_text().split():
            try: skip.add(int(line, 16))
            except ValueError: pass
    cands = []
    for r in rows_all:
        if r["triage_cat"].strip() != "leafC": continue
        if r["bytefrac_class"].strip() != "code": continue
        sh2 = int(r["sh2_addr"], 16); cart = int(r["cart_addr"], 16)
        if sh2 in done or cart in done: continue
        if sh2 in skip or cart in skip: continue
        try: n = int(r["ninsn"])
        except ValueError: continue
        if n > max_insn: continue
        cands.append((n, r))
    cands.sort(key=lambda x: x[0])
    # return a POOL (loop skips misseg candidates, stops after `limit` real generations)
    return [r for _, r in cands[:max(limit * 4, 40)]]

def header_for(sh2):
    return "dh_band21.h" if 0x02210000 <= sh2 < 0x02220000 else "dh.h"

SYS_PROMPT = """You are decompiling Sega ST-V (Hitachi SH-2, big-endian) machine code into
equivalence-faithful C for the Die Hard Arcade functional decomp (NOT byte-exact). /no_think

Rules you MUST follow:
- Output ONLY the C source for one file. No markdown fences, no prose outside the code.
- First line exactly: #include "{HDR}"
- Function name: func_<8hexSH2> (e.g. func_022005fa). Keep the signature if the m2c draft shows one.
- Types: s8/u8/s16/u16/s32/u32. Memory access via the header's macros:
    dh.h        -> M8/MU8/M16/MU16/M32/MU32(ptr, byteoffset); MP(ptr, off) for a pointer slot.
    dh_band21.h -> MEM8/MEM16/MEMU16/MEM32/MEMU32(addr); PTR(type, addr).
- HWRAM globals are g_<8hexSH2>; declare the ones you use as `extern` at the top.
- Called functions are func_<8hexSH2>; declare them `extern`. TRUST the resolved branch TARGETs
  given below, never the label text in the asm.
- Variable (register-count) shifts are helper calls: func_0224a360(value, count) = shift-left,
  func_0224a420(value, count) = arithmetic shift-right. m2c OFTEN DROPS these. If the asm shifts by
  a register (shld/shad or a shift in a loop), you MUST call the helper. Fixed-count shifts use << >>.
- NONSTANDARD r0/r1 ABI: some tiny helper functions (notably the 0x0224a3xx rotate/shift pocket)
  take their VALUE in r0 (and a count in r1), not r4/r5, and return in r0. If the asm reads r0 (or
  r1) at entry WITHOUT first assigning it, that register is an INPUT PARAMETER - model it as the
  function's argument (e.g. `u32 func_xxxxxxxx(u32 r0)`). NEVER initialize it to 0 and NEVER leave it
  as an uninitialized local. m2c flags these as read-before-write errors; that is the signal, not a bug.
- m2c's draft is only a hint; it is often wrong about dropped variable shifts, dropped stack
  arguments, and phantom branches from literal pools. The ASM is ground truth.
- Do NOT write your reasoning, analysis, or instruction-by-instruction tracing anywhere in the
  file. The ONLY comment permitted is a SINGLE one-line /* fidelity: <confidence + caveat> */ at the
  very end. A multi-line analysis/thinking comment is a failure - keep it out of the file entirely.
"""

USER_TMPL = """Function at SH-2 0x{sh2:08x} (cart 0x{cart:x}). Include header: "{hdr}".

Ground-truth SH-2 assembly:
```
{asm}
```

m2c draft C (hint only, may be wrong):
```
{c}
```

Resolved computed branches / data pointers (trust these TARGET addresses):
```
{res}
```
Write the single C file now (start with the #include line)."""

def strip_c(txt):
    m = re.search(r"```(?:c)?\s*\n(.*?)```", txt, re.S)
    if m: txt = m.group(1)
    txt = re.sub(r"<think>.*?</think>", "", txt, flags=re.S)
    return txt.strip() + "\n"

def chat(messages):
    body = {"model": MODEL, "stream": False, "think": False, "messages": messages,
            "options": {"temperature": 0, "num_ctx": 16384, "num_predict": 6000}}
    req = urllib.request.Request(OLLAMA, data=json.dumps(body).encode(),
                                 headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=900) as resp:
        return json.load(resp)["message"]["content"]

def valid_c(c, name):
    """Reject empty / function-missing output (an empty .c passes gcc -fsyntax-only)."""
    return (f"func_{name}" in c) and ("{" in c) and len(c.strip()) > 20

def looks_misseg(asm):
    """Detect m2c asm that is really mis-disassembled DATA, before wasting a slot on it.
    Neither census nor find_entry catches these; asm shape does."""
    import collections
    lines = [l.strip() for l in asm.splitlines()
             if l.strip() and not l.strip().startswith(".") and not l.strip().endswith(":")]
    for l in lines:
        # a real function never loads the stack pointer (r15) from a literal pool or register
        if l.replace("\t", " ").replace(",r15", ", r15").endswith(", r15") and l.split()[0] in ("mov.l", "mov", "mov.w"):
            return "SP (r15) loaded from pool/reg"
    if lines:
        top, n = collections.Counter(lines).most_common(1)[0]
        if n >= 4 and ("@(" in top or top.startswith("mov.b")):   # same odd store repeated = data
            return f"instruction x{n}: {top}"
    return None

def build_prompt(row):
    sh2 = int(row["sh2_addr"], 16); cart = int(row["cart_addr"], 16)
    hdr = header_for(sh2)
    asm = tool_out("m2c_fn.py", hex(cart), "--asm")[:12000]
    cdr = tool_out("m2c_fn.py", hex(cart))[:4000]
    res = (tool_out("braf_resolve.py", hex(cart))[:3000] +
           tool_out("ptr_resolve.py", hex(cart))[:2000]) or "(none)"
    sysp = SYS_PROMPT.replace("{HDR}", hdr)
    user = USER_TMPL.format(sh2=sh2, cart=cart, hdr=hdr, asm=asm, c=cdr, res=res)
    return sysp, user, asm

def gcc_check(path):
    # Real compile at -O1 so dataflow warnings (uninitialized use) fire; treat ANY warning as fail.
    r = run(["gcc", "-O1", "-c", "-o", "/dev/null", "-Wall", "-Wextra", "-I", str(SRC), str(path)])
    ok = r.returncode == 0 and "warning:" not in r.stderr
    return ok, r.stderr

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--easy", action="store_true", help="(flag kept for clarity; selection is always leaf+code)")
    ap.add_argument("--limit", type=int, default=10)
    ap.add_argument("--max-insn", type=int, default=20)
    ap.add_argument("--addrs", help="comma-separated 8-hex SH2 addrs to target specifically")
    a = ap.parse_args()
    STAGE.mkdir(exist_ok=True)
    addrs = [x.strip() for x in a.addrs.split(",")] if a.addrs else None
    rows = select(a.limit, a.max_insn, addrs)
    print(f"candidate pool: {len(rows)} (target {a.limit} real generations)", flush=True)
    manifest = []; produced = 0
    for row in rows:
        if not addrs and produced >= a.limit: break
        sh2 = int(row["sh2_addr"], 16); name = f"{sh2:08x}"; ninsn = row["ninsn"]
        sysp, user, asm = build_prompt(row)
        mis = looks_misseg(asm)
        if mis and not addrs:                         # skip mis-disassembled data before spending a slot
            print(f"SKIP {name}: misseg ({mis})", flush=True)
            manifest.append({"addr": name, "status": "misseg_skip", "why": mis}); continue
        produced += 1
        msgs = [{"role": "system", "content": sysp}, {"role": "user", "content": user}]
        t0 = time.time()
        try:
            out = chat(msgs)
        except Exception as e:
            print(f"ERR  {name}: {e}", flush=True)
            manifest.append({"addr": name, "status": "gen_error", "err": str(e)}); continue
        c = strip_c(out)
        p = STAGE / f"{name}.c"; p.write_text(c)
        gok, err = gcc_check(p)
        ok = gok and valid_c(c, name)
        if not ok:                                    # one retry (gcc error OR empty/invalid)
            reason = err[:1200] if not gok else "Your reply was empty or missing the function definition."
            msgs += [{"role": "assistant", "content": c},
                     {"role": "user", "content": f"Problem:\n{reason}\nOutput ONLY the complete corrected C file for func_{name} - start with the #include line, no reasoning, no markdown fences."}]
            try:
                c = strip_c(chat(msgs)); p.write_text(c)
                gok, err = gcc_check(p); ok = gok and valid_c(c, name)
            except Exception as e:
                err = f"{err}\n[retry failed: {e}]"
        dt = time.time() - t0
        status = "ok" if ok else "gcc_fail"
        rec = {"addr": name, "status": status, "ninsn": ninsn, "sec": round(dt, 1)}
        if not ok: rec["err"] = err[:400]
        manifest.append(rec)
        print(f"{'OK ' if ok else 'BAD'} {name} ninsn={ninsn} {dt:.0f}s", flush=True)
    (STAGE / "manifest.json").write_text(json.dumps(manifest, indent=1))
    nok = sum(1 for m in manifest if m["status"] == "ok")
    secs = [m["sec"] for m in manifest if "sec" in m]
    avg = round(sum(secs) / len(secs), 1) if secs else 0
    print(f"\n{nok}/{len(manifest)} compiled clean. avg {avg}s/fn. staged in {STAGE}", flush=True)

if __name__ == "__main__":
    main()
