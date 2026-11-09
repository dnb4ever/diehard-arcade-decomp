#!/usr/bin/env python3
"""Count dynamically executed emitted guest instructions and true body entries.

Profiles the stock copy, attributing baseline instruction work to installed C
functions. Delay slots count separately. Counters do not alter guest timing.
"""
import argparse,json,re,shutil
from pathlib import Path
import round4_generate as generate

def instrument(snapshot, output):
 source=snapshot/'port/run/P41/recomp'
 if output.exists():shutil.rmtree(output)
 shutil.copytree(source,output)
 entries=generate.functions(output); rows=[]
 for index,(pc,(p,original,body)) in enumerate(sorted(entries.items())):
  body=re.sub(r'(    ran\[\d+\] = 1;)',rf'    converge_profile_entry({index},c.cpu);\n\1',body)
  lines=[]
  for line in body.splitlines():
   m=re.search(r'// ([0-9A-F]{8}) (.*)',line)
   if m:
    # Emitter emits one instruction, or a control edge plus its delay slot.
    n=2 if ' ; ' in m[2] else 1
    lines.append(f'    converge_profile_insn({index},c.cpu,{n});')
   if 'sh2_stv_idle_batch(c);' in line:
    line=line.replace('sh2_stv_idle_batch(c);',f'{{ int before=c.budget; sh2_stv_idle_batch(c); converge_profile_batch({index},c.cpu,uint64_t(before-c.budget)/5*31); }}')
   lines.append(line)
  text=p.read_text();text=text.replace(original,original.replace(original.split('\n',1)[1].rsplit('\n}',1)[0],'\n'.join(lines)))
  p.write_text(text)
  rows.append(dict(index=index,pc=f'{pc:08X}',address=f'{pc-0x3e04000:08x}',body_sha256=__import__('hashlib').sha256(original.encode()).hexdigest()))
 for p in output.glob('p_dh_[0-9]*.cpp'):p.write_text('#include "converge_profile.h"\n'+p.read_text())
 (output/'converge_profile.h').write_text('#pragma once\n#include <cstdint>\nvoid converge_profile_batch(unsigned,int,uint64_t);\nvoid converge_profile_entry(unsigned,int);\nvoid converge_profile_insn(unsigned,int,unsigned);\n')
 (output/'converge_profile.cpp').write_text('''#include <cstdio>
#include <cstdlib>
#include <cstdint>
static uint64_t entries[4096][2]{},instructions[4096][2]{},batched[4096][2]{};
void converge_profile_batch(unsigned i,int cpu,uint64_t n){batched[i][cpu]+=n;}
void converge_profile_entry(unsigned i,int cpu){++entries[i][cpu];}
void converge_profile_insn(unsigned i,int cpu,unsigned n){instructions[i][cpu]+=n;}
extern "C" [[noreturn]] void __real__Exit(int);
extern "C" [[noreturn]] void __wrap__Exit(int code){
 for(unsigned i=0;i<4096;++i)if(entries[i][0]||entries[i][1]||instructions[i][0]||instructions[i][1])
 std::fprintf(stderr,"GUEST_PROFILE %u %llu %llu %llu %llu\\n",i,(unsigned long long)entries[i][0],(unsigned long long)entries[i][1],(unsigned long long)instructions[i][0],(unsigned long long)instructions[i][1]);
 for(unsigned i=0;i<4096;++i)if(batched[i][0]||batched[i][1])
 std::fprintf(stderr,"GUEST_BATCH_PROFILE %u %llu %llu\\n",i,(unsigned long long)batched[i][0],(unsigned long long)batched[i][1]);
 __real__Exit(code);
}
''')
 cmake=output/'CMakeLists.txt';cmake.write_text(cmake.read_text()+'\ntarget_sources(recomp PRIVATE converge_profile.cpp)\ntarget_link_options(saturn_stv PRIVATE -Wl,--wrap=_Exit)\n')
 (output/'profile-functions.json').write_text(json.dumps(rows,indent=2)+'\n')
 return rows
if __name__=='__main__':
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--snapshot',type=Path,default=Path('work/converge/round4/snapshot'));p.add_argument('--output',type=Path,default=Path('work/converge/round4/profile'))
 a=p.parse_args();print('Instrumented',len(instrument(a.snapshot,a.output)),'stock functions')
