#!/usr/bin/env python3
"""Add portable continuation labels to ST-V generated C++; no guest timing changes.

Normal calls/branches stay intact. State loading discards C++ stacks and resumes
at a saved post-poll label, routing later returns by the guest PC. The locals
live across a poll (indirect target and delayed condition) are saved explicitly.
Run after generation; idempotent for an already instrumented tree.
"""
from pathlib import Path
import re, sys

def instrument(folder):
 folder=Path(folder)
 files=sorted(folder.glob('p_dh_[0-9]*.cpp'))
 if not files:raise SystemExit('No generated ST-V functions')
 marked=[p.read_text().startswith('// P410_CONTINUATIONS_V1') for p in files]
 if all(marked) and (folder/'p410_continuations.cpp').exists():return
 if any(marked):raise SystemExit('Partial continuation tree: regenerate with port/p41_build.py --generate-only')
 # Returns can land only after a generated call or at a function entry.
 # Polls get separate labels below. Keeping other instructions unlabelled
 # lets the normal-path compiler retain its original straight-line optimizations.
 resumable_pcs=set()
 for path in files:
  text=path.read_text()
  resumable_pcs.update(int(m,16) for m in re.findall(r'void f_([0-9A-F]+)',text))
  resumable_pcs.update(int(m,16)+4 for m in re.findall(r'// ([0-9A-F]{8}) (?:bsr|bsrf|jsr)\b',text))
 tag=1; table=[]; pcmap={}
 for path in files:
  src=path.read_text();out=['// P410_CONTINUATIONS_V1\n'];lines=src.splitlines(True);i=0
  while i<len(lines):
   line=lines[i]
   m=re.match(r'void (f_[0-9A-F]+)\(SH2Context& c\) {',line)
   if not m:out.append(line);i+=1;continue
   fn=m[1];i+=1;body=[]
   while lines[i]!='}\n':body.append(lines[i]);i+=1
   i+=1;new=[];labels=[]
   for b in body:
    # Instruction labels also cover every return PC. Existing branch labels remain.
    a=re.search(r'// ([0-9A-F]{8}) ',b)
    if a and int(a[1],16) in resumable_pcs:
     t=tag;tag+=1;labels.append(t);table.append((t,fn));pcmap.setdefault(int(a[1],16),t)
     new.append(f'R_{t}:;\n')
    if 'SH2_POLL(c);' in b:
     # Declare cross-poll locals at function scope, so entry gotos are legal.
     b=b.replace('const uint32_t x =','x =').replace('const bool k =','k =').replace('uint32_t v =','v =')
     def poll(_):
      nonlocal tag
      t=tag;tag+=1;labels.append(t);table.append((t,fn))
      return f'c.cont = {t}; c.cont_x = x; c.cont_k = k; SH2_POLL(c); R_{t}:;'
     b=re.sub(r'SH2_POLL\(c\);',poll,b)
    new.append(b)
   out.append(line)
   out.append('    uint32_t x = c.cont_x, v = 0; bool k = c.cont_k;\n')
   out.append('    uint32_t entry = c.cont_entry; c.cont_entry = 0;\n    switch (entry) {\n')
   out.extend(f'    case {t}: goto R_{t};\n' for t in labels)
   out.append('    default: break;\n    }\n')
   out.extend(new);out.append('}\n')
  path.write_text(''.join(out))
 code=['#include "p_dh_funcs.h"\n#include <algorithm>\n#include <iterator>\n',
       'struct Entry { uint32_t tag; SH2Func fn; };\nstatic const Entry entries[] = {\n']
 code.extend(f'{{{t}, p_dh::{fn}}},\n' for t,fn in table)
 code+=['};\nstruct PC { uint32_t pc, tag; };\nstatic const PC pcs[] = {\n']
 code.extend(f'{{0x{pc:08X}u, {t}}},\n' for pc,t in sorted(pcmap.items()))
 code+=['};\n', '''bool stv_continue(SH2Context& c, uint32_t tag) {
    if (!tag || tag > sizeof(entries)/sizeof(entries[0])) return false;
    for (;;) {
        c.cont_entry = tag;
        entries[tag-1].fn(c);
        auto it = std::lower_bound(std::begin(pcs), std::end(pcs), c.pc,
                                  [](const PC& a, uint32_t pc) { return a.pc < pc; });
        if (it == std::end(pcs) || it->pc != c.pc) return false;
        tag = it->tag;
    }
}
''']
 (folder/'p410_continuations.cpp').write_text(''.join(code))
 p=folder/'CMakeLists.txt';p.write_text(p.read_text().replace('    modules.cpp)', '    p410_continuations.cpp\n    modules.cpp)'))
 print(f'P4.10: {tag-1} continuation labels, {len(pcmap)} guest return addresses')
if __name__=='__main__':instrument(sys.argv[1] if len(sys.argv)>1 else 'port/run/P41/recomp')
