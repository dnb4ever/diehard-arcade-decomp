#!/usr/bin/env python3
"""Link an independent probe against the already built, unchanged application objects."""
import shlex,subprocess,os
from pathlib import Path
w=next(p for p in Path(__file__).resolve().parents if (p/'STATE.md').is_file())/os.environ.get('P421_WORK','work/p421');build=w/'build'
obj=(w/'abi-probe.o').resolve();out=(w/'abi-probe').resolve()
subprocess.run(['c++','-std=c++20','-O2','-I'+str(w/'generated'),'-I'+str(w/'runtime/include'),'-I'+str(w/'runtime/src/machine'),'-I'+str(build),'-c',str(w/'port/p421_abi_probe.cpp'),'-o',str(obj)],check=True)
cmd=shlex.split((build/'CMakeFiles/saturn_stv.dir/link.txt').read_text())
cmd=[str(obj) if x.endswith('main_stv.cpp.o') else x for x in cmd];cmd[cmd.index('-o')+1]=str(out)
subprocess.run(cmd,cwd=build,check=True)
with (w/'abi-probe.log').open('w') as f:subprocess.run([str(out),str((w/'data/mods').resolve())],env=dict(os.environ,CONVERGE_AUDIT='1'),stdout=f,stderr=f,check=True)
print((w/'abi-probe.log').read_text())
