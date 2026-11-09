"""Generate probes from the frozen machine bodies, never from live port files."""
from pathlib import Path
import round3_generate as generate

def install(snapshot,out):
 entries=generate.functions(snapshot/'port/run/P41/recomp')
 refs=['#include "p_dh_funcs.h"\nnamespace reference { using namespace p_dh;\n']
 for pc in (0x06027c74,0x06018194,0x06018284,0x0601800c,0x060454ee):
  refs.append(entries[pc][1].replace(f'void f_{pc:08X}',f'void ref_{pc:08X}'))
 refs.append('}\n')
 (out/'round3_references.h').write_text('\n'.join(refs))
 for name in ('round3_pulse_probe.cpp','round3_contract_probe.cpp'):
  (out/name).write_text(Path(__file__).with_name(name).read_text())
 with (out/'CMakeLists.txt').open('a') as f:
  f.write('''
foreach(_probe round3_pulse_probe round3_contract_probe)
add_executable(${_probe} ${_probe}.cpp)
target_link_libraries(${_probe} saturn_core saturn_stub recomp Threads::Threads)
target_link_options(${_probe} PRIVATE -Wl,--wrap=_Exit)
endforeach()
target_link_options(round3_pulse_probe PRIVATE -Wl,--wrap=_Z8sh2_pollR10SH2Context)
''')
