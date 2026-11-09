#!/usr/bin/env python3
"""Frozen round-4 build; defaults never touch the normal port build."""
import argparse,json,hashlib,subprocess,time,shutil
from pathlib import Path
import round4_generate as generate
HERE=Path(__file__).resolve().parent
WORK=Path('work/converge/round4')

def make(source,build,targets=['saturn_stv']):
 snap=(WORK/'snapshot').resolve()
 subprocess.run(['cmake','-S',str(source),'-B',str(build),f'-DSATURN_RUNTIME={(WORK/"capture-runtime").resolve()}',f'-DSATURN_EXTRA={snap}/port/stv.cmake','-DSATURN_NO_SDL=ON'],check=True)
 subprocess.run(['cmake','--build',str(build),'--target',*targets,'-j4'],check=True)

def main():
 p=argparse.ArgumentParser(description=__doc__);variants=p.add_mutually_exclusive_group();variants.add_argument('--mod',action='store_true');variants.add_argument('--stock',action='store_true');p.add_argument('--rebase',action='store_true');p.add_argument('--generate-only',action='store_true');a=p.parse_args()
 if a.mod:p.error('Round 4 is qualified with mods off; the separate round2_build.py --mod retains the older mod demonstration')
 start=time.monotonic();s=WORK/'snapshot';out=WORK/('generated-rebase' if a.rebase else 'generated')
 if a.stock:
  if not a.generate_only:make(s/'port/run/P41/recomp',WORK/'build-stock')
  return
 rows=generate.generate(s,out,HERE/'round4_overrides.txt',mod='playfield-shift' if a.mod else 'off',round2=True)
 if a.mod:
  bridge=out/'bridge.cpp'
  text=bridge.read_text();old='b[i].address>=0x06099788u && b[i].address<0x0609978cu'
  assert old in text
  # This opt-in executable authorizes two bytes in the third VDP1 header.
  # The live pointer has not been committed: it is still the invocation base.
  bridge.write_text(text.replace(old,'uint32_t(b[i].address-physical(ld32(0x060cf0c4u)+0x54u))<2u'))
 entries=generate.functions(out)
 pollcert=json.loads((WORK/'qualification-polls/report.json').read_text())
 pins=json.loads((HERE/'round4_poll_pins.json').read_text())
 for addr,kind in [('02213164','root'),('02228106','slot')]:
  pc=int(addr,16)+0x3e04000;pfile,original,body=entries[pc]
  q=next(r for r in pollcert['results'] if r['address']==addr)
  digest=lambda b:hashlib.sha256(b).hexdigest()
  if q['status']!='PASS' or q['source_sha256']!=digest((s/'project/src_functional'/f'{addr}.c').read_bytes()) or pins[addr]['machine']!=digest(body.encode()) or pins[addr]['source']!=q['source_sha256']:raise ValueError('poll source/ABI not qualified')
  index=len(rows);pfile.write_text('#include "bridge.h"\n'+pfile.read_text().replace(original,f'void f_{pc:08X}(SH2Context& c) {{ p_dh::ran[{__import__("re").search(r"ran\[(\d+)\]",body)[1]}]=1; converge::poll_{kind}(c,{index}); }}'))
  rows.append(dict(index=index,address=addr,pc=f'{pc:08X}',bridge='resumable-C',source_sha256=q['source_sha256'],residue_lines=0))
 for name in ('continuations.h','continuations.c','continuations.cpp'):shutil.copyfile(HERE/'polls'/name,out/name)
 for name in ('round2_abi_probe.cpp','round2_poll_probe.cpp','round2_mod_probe.cpp'):shutil.copyfile(HERE/name,out/name)
 cmake=out/'CMakeLists.txt'
 cmake.write_text(cmake.read_text()+'''\ntarget_sources(recomp PRIVATE continuations.c continuations.cpp)
foreach(_probe round2_abi_probe round2_poll_probe round2_mod_probe)
add_executable(${_probe} ${_probe}.cpp)
target_link_libraries(${_probe} saturn_core saturn_stub recomp Threads::Threads)
target_link_options(${_probe} PRIVATE -Wl,--wrap=_Exit)
endforeach()
target_link_options(round2_poll_probe PRIVATE -Wl,--wrap=_Z8sh2_pollR10SH2Context)
''')
 if a.mod:
  with cmake.open('a') as f:f.write('\ntarget_compile_definitions(round2_mod_probe PRIVATE CONVERGE_PLAYFIELD_MOD=1)\n')
 # The pulse loop is an explicitly mapped resumable readable-C body.
 pc=0x06027c74;pfile,original,body=entries[pc]
 q=json.loads((WORK/'qualification.json').read_text())['functions']['02223c74']
 pins=json.loads((HERE/'round4_pulse_pins.json').read_text())
 digest=lambda b:hashlib.sha256(b).hexdigest()
 assert q['status']=='PASS-void' and pins['machine']==digest(body.encode()) and pins['source']==digest((s/'project/src_functional/02223c74.c').read_bytes())==q['source_sha256']
 pfile.write_text('#include "bridge.h"\n'+pfile.read_text().replace(original,'void f_06027C74(SH2Context& c) { p_dh::ran[551]=1; converge::poll_pulse(c,'+str(len(rows))+'); }'))
 rows.append(dict(index=len(rows),address='02223c74',pc='06027C74',bridge='resumable-C',source_sha256=q['source_sha256'],residue_lines=0))
 for name in ('round3_pulse.h','round3_pulse.c','round3_pulse.cpp'):shutil.copyfile(HERE/name,out/name)
 with cmake.open('a') as f:f.write('\ntarget_sources(recomp PRIVATE round3_pulse.c round3_pulse.cpp)\n')
 import round4_make_probes
 round4_make_probes.install(s,out)
 if not a.rebase:
  import round4_compute_install
  round4_compute_install.install(s,out,rows)
 (out/'overrides.json').write_text(json.dumps(rows,indent=2)+'\n')
 if a.generate_only:return
 build=WORK/('build-rebase' if a.rebase else 'build-round4');make(out,build,([] if a.rebase else ['round4_compute_probe'])+['saturn_stv','round2_abi_probe','round2_poll_probe','round2_mod_probe','converge_abi_probe','round3_pulse_probe','round4_contract_probe'])
 binary=build/'saturn_stv';(WORK/('mod-build-metrics.json' if a.mod else 'build-metrics.json')).write_text(json.dumps(dict(seconds=time.monotonic()-start,bytes=binary.stat().st_size,binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest()),indent=2)+'\n')
if __name__=='__main__':main()
