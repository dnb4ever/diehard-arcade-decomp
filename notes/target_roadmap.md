# Die Hard Arcade (ST-V/SH-2) — Unit Grind Roadmap

Auto-generated from `notes/function_triage.tsv` + `fn.units()` + `drafts/INDEX.md`.
Pivot: match whole COMPILATION UNITS (fn.units() pool groups) in file order, because
SHC/GCC codegen is stateful per source file. Grind the cleanest units first.

## Category totals (all 1272 functions, 258 units)

- **asm-origin** (keep as asm, do NOT grind): 9 — tiny call-free math/runtime-lib
  (64-bit mul / MAC / div primitives with non-standard r0/r1 return, e.g. fixmul_16_16,
  vec3_dot_fixed). `mac.l`/`div1` seen INSIDE larger C functions are inlined, not asm — those
  stay `callC` with the opcode noted.
- **leafC** (leaf C target, no calls, real logic): 295
- **callC** (call-bearing C target): 547
- **misseg** (fn.py boundary error — data decoded as code, NOT a real function): 421

### Where misseg clusters (avoid)
- **Low zone `0x2200400–0x2201020`**: the warned region. Interleaved data tables and real
  functions; drafts here (f_20042e, f_20062a, f_200f44 …) are M2C_ERROR garbage.
- **High tail `~0x2250000–0x228ffff`**: `fn.units()` runs past the end of real code into
  data tables near `TEXT_END` (0x290000). Most 'functions' there are data.
- The clean, grindable C is the **middle band (~0x2201000–0x224f000)**.

## Top grind targets (ranked)

Score favors: 100%-real-C members, clean file-boundary start, no misseg members,
few asm deps, members whose m2c already parsed (draft exists to concatenate), small size.

| unit | len | #fn | leafC | callC | asm | m2c(ok/err) | flags |
|------|-----|-----|-------|-------|-----|-------------|-------|
| `2214918` | 0x88 | 4 | 4 | 0 | 0 | 4ok/0err | clean-start |
| `2215fac` | 0xf4 | 4 | 0 | 4 | 0 | 4ok/0err | clean-start |
| `222f3ae` | 0x13a | 6 | 6 | 0 | 0 | 5ok/1err | clean-start |
| `222a872` | 0xfe | 3 | 2 | 1 | 0 | 3ok/0err | clean-start |
| `2244874` | 0x114 | 3 | 3 | 0 | 0 | 3ok/0err | clean-start |
| `220ad58` | 0x340 | 4 | 0 | 4 | 0 | 4ok/0err | clean-start |
| `2214328` | 0x292 | 5 | 4 | 1 | 0 | 4ok/1err | clean-start |
| `2216714` | 0x23a | 3 | 3 | 0 | 0 | 3ok/0err | clean-start |
| `22148ac` | 0x6c | 2 | 2 | 0 | 0 | 2ok/0err | clean-start |
| `2214818` | 0x94 | 2 | 2 | 0 | 0 | 2ok/0err | clean-start |
| `22160a0` | 0x9e | 2 | 0 | 2 | 0 | 2ok/0err | clean-start |
| `2245eb8` | 0x10c | 2 | 2 | 0 | 0 | 2ok/0err | clean-start |
| `220abfe` | 0x15a | 4 | 1 | 3 | 0 | 3ok/1err | clean-start |
| `2214194` | 0x194 | 2 | 2 | 0 | 0 | 2ok/0err | clean-start |
| `221694e` | 0x1c6 | 2 | 2 | 0 | 0 | 2ok/0err | clean-start |
| `22145ba` | 0x25e | 4 | 2 | 2 | 0 | 3ok/1err | clean-start |
| `2217c10` | 0x3ca | 4 | 3 | 1 | 0 | 3ok/1err | clean-start |
| `2245db2` | 0x106 | 3 | 2 | 1 | 0 | 2ok/1err | clean-start |
| `222acf8` | 0x124 | 3 | 1 | 2 | 0 | 2ok/1err | clean-start |
| `220ffd2` | 0x126 | 3 | 3 | 0 | 0 | 2ok/1err | clean-start |
| `2244988` | 0x440 | 4 | 2 | 2 | 0 | 3ok/1err | clean-start |
| `22416ac` | 0x1f4 | 6 | 4 | 2 | 0 | 3ok/3err | clean-start |
| `2249ea0` | 0x160 | 4 | 2 | 2 | 0 | 2ok/2err | clean-start |
| `2214b70` | 0x16c | 4 | 2 | 2 | 0 | 2ok/2err | clean-start |
| `2215da8` | 0x204 | 5 | 2 | 3 | 0 | 2ok/3err | clean-start |
| `2223c56` | 0x21a | 5 | 4 | 1 | 0 | 2ok/3err | clean-start |
| `220a7dc` | 0x27e | 4 | 0 | 4 | 0 | 2ok/2err | clean-start |
| `223c638` | 0x324 | 5 | 1 | 4 | 0 | 2ok/3err | clean-start |
| `223e104` | 0x41c | 5 | 0 | 5 | 0 | 2ok/3err | clean-start |
| `223c95c` | 0x43c | 3 | 2 | 1 | 0 | 2ok/1err | clean-start |

## How to use with unit_grind.py

```
# grind the top unit (concatenate its member .c from drafts/, compile as one TU):
python unit_grind.py 0x<unit_addr>
# swap compiler recipe when the SHC-vs-GCC investigation lands:
python unit_grind.py 0x<unit_addr> --recipe gcc
```

Start at the top of the table and walk down. A unit at 100% real C with `clean-start`
and existing drafts is the ideal first match: the whole-TU context gives SHC the
file-order state that isolated-function matching was missing.

## Pure-leaf-C quick wins (single clean leaf units, for warm-up / oracle calibration)

| unit | len | m2c |
|------|-----|-----|
| `223e5bc` | 0x10 | - |
| `223e5cc` | 0x14 | - |
| `22131a8` | 0x20 | - |
| `2213434` | 0x40 | - |
| `2221800` | 0x42 | - |
| `224a830` | 0x54 | - |
| `2213fb4` | 0x58 | - |
| `22134d8` | 0x60 | - |
| `2213474` | 0x64 | - |
| `2223e70` | 0x70 | - |
| `224a7a8` | 0x88 | - |
| `223e520` | 0x9c | - |
