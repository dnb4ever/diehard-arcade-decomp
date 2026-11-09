"""Install only reviewed, hash-pinned resumable compute bodies in a private copy."""
from pathlib import Path
import hashlib,json,re,shutil
import round4_generate as generate
HERE=Path(__file__).resolve().parent
TARGETS=[('02214e2c','poll_sort(c,{index},false)'),('02214e2e','poll_sort(c,{index},true)'),('02213ca4','poll_translate(c,{index})'),('02213b5c','poll_transform(c,{index})'),('0221326c','poll_copy(c,{index})')]
def install(snapshot,out,rows):
    entries=generate.functions(snapshot/'port/run/P41/recomp')
    pins=json.loads((HERE/'round4_compute_pins.json').read_text())
    cert=json.loads((snapshot.parent/'qualification-compute/report.json').read_text())
    assert cert['harness_sha256']==generate.digest(snapshot/'tools/decomp/difftest.py')
    assert cert['cart_sha256']==generate.digest(snapshot/'cart.bin')
    refs=['#include "p_dh_funcs.h"\n#include "bridge.h"\n#include "round4_compute.h"\nnamespace compute_reference {using namespace p_dh;']
    for a,call in TARGETS:
        pc=int(a,16)+0x3e04000;p,original,body=entries[pc]
        q=next(r for r in cert['results'] if r['address']==a)
        digest=generate.digest(snapshot/f'project/src_functional/{a}.c')
        assert q['status']=='PASS-void' and q['source_sha256']==digest==pins[a]['source']
        assert pins[a]['machine']==hashlib.sha256(body.encode()).hexdigest()
        p=out/p.name;s=p.read_text();assert original in s
        index=len(rows);ran=re.search(r'ran\[(\d+)\]',body)[1]
        p.write_text('#include "bridge.h"\n'+s.replace(original,f'void f_{pc:08X}(SH2Context& c) {{ p_dh::ran[{ran}]=1; converge::'+call.format(index=index)+'; }'))
        rows.append(dict(index=index,address=a,pc=f'{pc:08X}',bridge='resumable-C',source_sha256=digest,residue_lines=0))
        refs.append(original.replace(f'void f_{pc:08X}',f'void ref_{pc:08X}'))
    refs.append('}');(out/'round4_compute_references.h').write_text('\n'.join(refs)+'\n')
    for name in ('round4_compute.h','round4_compute.c','round4_compute.cpp','round4_compute_probe.cpp'):
        shutil.copyfile(HERE/name,out/name)
    with (out/'CMakeLists.txt').open('a') as f:f.write('''
target_sources(recomp PRIVATE round4_compute.c round4_compute.cpp)
add_executable(round4_compute_probe round4_compute_probe.cpp)
target_link_libraries(round4_compute_probe saturn_core saturn_stub recomp Threads::Threads)
target_link_options(round4_compute_probe PRIVATE -Wl,--wrap=_Exit -Wl,--wrap=_Z8sh2_pollR10SH2Context -Wl,--wrap=_Z11sh2_io_readji)
''')
    return rows
