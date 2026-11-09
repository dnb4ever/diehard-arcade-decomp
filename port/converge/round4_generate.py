#!/usr/bin/env python3
"""Lower frozen readable guest C through difftest v1.09 with explicit ABI adapters.

All generated-code changes happen in an output copy. The register residue is
deliberately retained: this pilot replaces C-visible effects, not every machine
instruction. There is no call to the original selected port function.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import tempfile

HERE = Path(__file__).resolve().parent


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_oracle(snapshot):
    spec = importlib.util.spec_from_file_location('converge_difftest', snapshot/'tools/decomp/difftest.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def functions(folder):
    result = {}
    for p in sorted(folder.glob('p_dh_[0-9]*.cpp')):
        for m in re.finditer(r'void f_([0-9A-F]{8})\(SH2Context& c\) \{\n(.*?)\n\}',p.read_text(),re.S):
            result[int(m[1],16)] = (p,m[0],m[2])
    return result


def residue(body, internal_switches=False):
    if 'SH2_POLL' in body:
        raise ValueError('scheduler poll requires a resumable C continuation')
    if re.search(r'sh2_(?:mac|sleep|trapa|set_sr|hook)',body):
        raise ValueError('nontransactional helper')
    # braf and direct bra tail targets. Preserve all code before the dispatch,
    # including the delay-slot load/pop, but let readable C choose/forward it.
    if internal_switches:
        # Preserve intra-body switch labels; only an external default is a tail.
        body=body.replace('default: sh2_call(c, x); return;',
                          'default: throw converge::Tail{x};')
    else:
        body = re.sub(r'switch \(x\) \{ case .*? default: sh2_call\(c, x\); return; \}',
                      'throw converge::Tail{x};', body)
    body = re.sub(r'(?:f_([0-9A-F]{8})\(c\)|sh2_call\(c, (0x[0-9A-F]+u)\)); return;',
                  lambda m: 'throw converge::Tail{' + (m[2] or '0x'+m[1]+'u') + '};', body)
    if re.search(r'\bf_[0-9A-F]+\(|\bsh2_call\(',body):
        raise ValueError('call is not a supported terminal tail branch')
    body = re.sub(r'\bld(8|16|32)\(([^;]*?)\)',lambda m: 'tx.read('+m[2]+','+str(int(m[1])//8)+')',body)
    # Stores have simple comma-free operands in the emitted instruction subset.
    body = re.sub(r'\bst(8|16|32)\(([^,;]+), ([^;]+)\);',
                  lambda m:'tx.write('+m[2]+','+str(int(m[1])//8)+','+m[3]+');',body)
    if re.search(r'\b(?:ld|st)(?:8|16|32)\(',body):
        raise ValueError('unparsed memory access')
    body = re.sub(r'    ran\[\d+\] = 1;\n','',body)
    return body


def _generate(snapshot, output, overrides, mod='off', round2=False):
    snapshot,output=Path(snapshot).resolve(),Path(output).resolve()
    if output==snapshot or snapshot in output.parents:
        raise ValueError('output must be separate from the frozen snapshot')
    source=snapshot/'port/run/P41/recomp'
    if output.exists():
        shutil.rmtree(output)
    shutil.copytree(source,output)
    dt=load_oracle(snapshot)
    cache=snapshot.parent/'symbols.json'
    if cache.exists():
        symbols=json.loads(cache.read_text())
    else:
        symbols=dt.load_symbols()
        cache.write_text(json.dumps(symbols,sort_keys=True)+'\n')
    entries=functions(output)
    addresses=[]
    for line in Path(overrides).read_text().splitlines():
        token=line.split('#',1)[0].strip()
        if token:
            addresses.append(int(token,16))
    if len(addresses)!=len(set(addresses)) or len(addresses)>256:
        raise ValueError('duplicate or oversized override list')
    declarations=['#pragma once\n#include "saturn/sh2.h"\nnamespace converge {\n']
    changes={}
    manifest=[]
    recipes=json.loads((HERE/('round4_recipes.json' if round2 else 'abi_recipes.json')).read_text())
    certificate=json.loads((snapshot.parent/'qualification.json' if round2 else HERE/'qualification.json').read_text())
    exclusions=json.loads((HERE/'round4_exclusions.json').read_text()) if round2 else {}
    if certificate['harness_sha256']!=digest(snapshot/'tools/decomp/difftest.py'):
        raise ValueError('difftest changed; rerun qualify.py in the frozen snapshot')
    if certificate['cart_sha256']!=digest(snapshot/'cart.bin'):
        raise ValueError('cartridge changed; rerun qualify.py in the frozen snapshot')
    for index,address in enumerate(addresses):
        if f'{address:08x}' in exclusions:
            raise ValueError(f'{address:08x}: excluded boundary requires review: '+exclusions[f'{address:08x}']['reason'])
        pc=dt.source_entry(dt.canonical(address))
        p,original,body=entries[pc]
        abi=residue(body,internal_switches=round2)
        recipe=recipes.get(f'{address:08x}')
        if recipe and recipe['body_sha256']!=hashlib.sha256(body.encode()).hexdigest():
            raise ValueError('ABI recipe machine body changed; review it before reuse')
        src=snapshot/'project/src_functional'/f'{address:08x}.c'
        qualified=certificate['functions'].get(f'{address:08x}',{})
        if qualified.get('status') not in (('PASS','PASS-void') if round2 else ('PASS',)) or qualified.get('source_sha256')!=digest(src):
            raise ValueError(f'{address:08x}: source has no matching PASS certificate; rerun qualify.py')
        text=src.read_text()
        modified=(mod=='sin-half' and address==0x02213190) or (round2 and mod=='playfield-shift' and address==0x02215000)
        if modified:
            text=(HERE/'mods'/('playfield_shift.c' if mod=='playfield-shift' else 'sin_half.c')).read_text()
        metadata={}
        c,pointers=dt.prepare(text,symbols,pc,metadata)
        if (not metadata['r0_checked'] and not round2) or metadata['stack_locals']:
            raise ValueError(f'{address:08x}: requires PASS word ABI without fixed scratch frame')
        if metadata['entry_arg_count']>16:raise ValueError('too many arguments')
        name=f'{pc:08X}'
        # Keep a directly reviewable copy of exactly the source being compiled.
        (output/f'source_{address:08x}.c.txt').write_text(text)
        fn=dt.select_function(dt.parse(text),symbols,pc).decl.name
        cfile=output/f'converge_{name}.c'
        runtime=dt.HARNESS[dt.HARNESS.index('static u32 dt_rt_divzero'):]
        cfile.write_text(dt.PRELUDE + '\n#include <stdint.h>\n' +
            'extern u32 dt_read(u32,int);\nextern u32 dt_write(u32,int,u32);\n' +
            'extern u32 dt_callv(u32,unsigned,const u32*);\n' +runtime+
            f'\n#define {fn} dhc_{name}\n#define dt_entry converge_c_{name}\n'+c+'\n')
        cppfile=output/f'converge_{name}.cpp'
        invocation=f'    invoke(c,residue,converge_c_{name},{metadata["entry_arg_count"]},{index},{str(modified).lower()});\n'
        if round2:
            invocation=f'    invoke(c,residue,converge_c_{name},{metadata["entry_arg_count"]},{index},{str(modified).lower()},{str(metadata["r0_checked"]).lower()});\n'
        spans=json.loads((HERE/'round4_contracts.json').read_text())['stack_outputs'].get(f'{address:08x}',[])
        if spans:
            invocation=invocation.replace(');\n',', {'+','.join('{c.r[%d],%d}'%(reg,size) for reg,size in spans)+'});\n')
        recipe_code=''
        if recipe:
            recipe_code='static void recipe(SH2Context& c,const uint32_t* args,uint32_t result) {\n'+recipe['code']+'\n}\n'
            invocation=f'    invoke_direct(c,residue,converge_c_{name},{metadata["entry_arg_count"]},{index},recipe,{str(metadata["r0_checked"]).lower()});\n'
        descriptor=json.loads((HERE/'round4_contracts.json').read_text()).get('register_abis',{}).get(f'{address:08x}')
        if descriptor:
            if not recipe:raise ValueError('custom ABI requires a reviewed direct recipe')
            regs=descriptor['args'][:]
            for arg,reg in descriptor.get('out',[]):
                if regs[arg] is not None:raise ValueError('register output overlaps input descriptor')
                regs[arg]=-reg
            if any(r is None or abs(r)>15 for r in regs):raise ValueError('invalid register descriptor')
            mask=sum(1<<reg for _,reg in descriptor.get('out',[]))
            invocation=invocation.replace(');\n',', {'+','.join(map(str,regs))+'},'+str(mask)+','+str(descriptor.get('return') or 0)+');\n')
        cppfile.write_text('#include "bridge.h"\n'+
            f'extern "C" uint32_t converge_c_{name}(uint32_t*);\n'+
            'namespace {\nstatic void residue(SH2Context& c, converge::Transaction& tx) {\n'+abi+'\n}\n'+recipe_code+'}\n'+
            f'void converge::f_{name}(SH2Context& c) {{\n'+
            f'    p_dh::ran[{re.search(r"ran\[(\d+)\]",body)[1]}]=1;\n'+
            invocation+'}\n')
        # ran is declared in the existing generated header.
        cppfile.write_text('#include "p_dh_funcs.h"\n#include "converge_declarations.h"\n'+cppfile.read_text())
        declarations.append(f'void f_{name}(SH2Context&);\n')
        replacement=f'void f_{name}(SH2Context& c) {{ converge::f_{name}(c); }}'
        changes.setdefault(p,[]).append((original,replacement))
        manifest.append(dict(index=index,address=f'{address:08x}',pc=name,name=fn,
            source_sha256=digest(src),generated_c_sha256=digest(cfile),
            residue_sha256=hashlib.sha256(abi.encode()).hexdigest(),mod=modified,
            bridge='direct' if recipe else 'transactional-residue',
            source_lines=len(text.splitlines()),c_lines=len(c.splitlines()),
            residue_lines=len(abi.splitlines()),metadata=metadata))
    declarations.append('}\n')
    (output/'converge_declarations.h').write_text(''.join(declarations))
    for p,replacements in changes.items():
        text=p.read_text()
        for a,b in replacements:
            assert text.count(a)==1
            text=text.replace(a,b)
        p.write_text('#include "converge_declarations.h"\n'+text)
    for name in ('bridge.cpp','bridge.h','abi_probe.cpp','small_vector.h'):
        shutil.copyfile(HERE/('round4_'+name if round2 and (name.startswith('bridge.') or name=='small_vector.h') else name),output/('round4_small_vector.h' if name=='small_vector.h' else name))
    cmake=output/'CMakeLists.txt'
    cmake.write_text(cmake.read_text()+'\n# Generated decomp convergence switch\n'+
        'target_sources(recomp PRIVATE bridge.cpp\n'+
        ''.join(f'    converge_{m["pc"]}.cpp converge_{m["pc"]}.c\n' for m in manifest)+')\n'+
        'target_compile_options(recomp PRIVATE $<$<COMPILE_LANGUAGE:C>:-fwrapv;-fno-strict-aliasing>)\n'+
        'target_link_options(saturn_stv PRIVATE -Wl,--wrap=_Exit)\n'+
        ('add_executable(converge_abi_probe abi_probe.cpp)\n'+
         'target_link_libraries(converge_abi_probe saturn_core saturn_stub recomp Threads::Threads)\n'+
         'target_link_options(converge_abi_probe PRIVATE -Wl,--wrap=_Exit)\n'
         if all(int(a,16) in addresses for a in recipes) else '')+
        # The inherited state fingerprint covers p_dh_*.cpp, but it cannot know
        # about these extra C bodies. Distinguish normal/mod quick states too.
        'file(GLOB _converge_identity_sources CONFIGURE_DEPENDS\n'+
        '    "${CMAKE_CURRENT_SOURCE_DIR}/converge_*.c" "${CMAKE_CURRENT_SOURCE_DIR}/converge_*.cpp"\n'+
        '    "${CMAKE_CURRENT_SOURCE_DIR}/bridge.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/bridge.h" "${CMAKE_CURRENT_SOURCE_DIR}/round4_small_vector.h"\n'+
        '    "${CMAKE_CURRENT_SOURCE_DIR}/continuations.*" "${CMAKE_CURRENT_SOURCE_DIR}/round3_pulse.*" "${CMAKE_CURRENT_SOURCE_DIR}/round4_compute.*")\n'+
        'set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${_converge_identity_sources})\n'+
        'set(_converge_hashes "${_state_identity}")\n'+
        'foreach(_converge_source IN LISTS _converge_identity_sources)\n'+
        '    file(SHA256 "${_converge_source}" _converge_hash)\n'+
        '    string(APPEND _converge_hashes "${_converge_hash}")\n'+
        'endforeach()\n'+
        'string(SHA256 _converge_identity "${_converge_hashes}")\n'+
        'string(SUBSTRING "${_converge_identity}" 0 16 _converge_short)\n'+
        'file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/p410_identity.h" "#define STV_STATE_BUILD 0x${_converge_short}ull\\n")\n')
    (output/'overrides.json').write_text(json.dumps(manifest,indent=2)+'\n')
    return manifest


def generate(snapshot, output, overrides, mod='off', round2=False):
    snapshot=Path(snapshot).resolve()
    output=Path(output).resolve()
    if (snapshot.parent not in output.parents or output==snapshot or snapshot in output.parents):
        raise ValueError('generated output must be in the private work tree, outside its snapshot')
    output.parent.mkdir(parents=True,exist_ok=True)
    # Generate first, then synchronize changed files. Failed generation leaves
    # the previous usable build intact, and no-op builds retain object mtimes.
    with tempfile.TemporaryDirectory(prefix='glue-',dir=output.parent) as temporary:
        staging=Path(temporary)/'generated'
        result=_generate(snapshot,staging,overrides,mod,round2)
        output.mkdir(exist_ok=True)
        wanted={p.relative_to(staging) for p in staging.rglob('*') if p.is_file()}
        for relative in wanted:
            source,dest=staging/relative,output/relative
            if not dest.exists() or source.read_bytes()!=dest.read_bytes():
                dest.parent.mkdir(parents=True,exist_ok=True)
                shutil.copy2(source,dest)
        for path in output.rglob('*'):
            if path.is_file() and path.relative_to(output) not in wanted:path.unlink()
    return result


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--snapshot',type=Path,default=Path('work/converge/snapshot'))
    p.add_argument('--output',type=Path,required=True)
    p.add_argument('--overrides',type=Path,required=True)
    p.add_argument('--mod',choices=['off','sin-half'],default='off')
    p.add_argument('--round2',action='store_true')
    a=p.parse_args()
    rows=generate(a.snapshot,a.output,a.overrides,a.mod,a.round2)
    print(f'Generated {len(rows)} overrides ({sum(r["bridge"]=="direct" for r in rows)} direct) in {a.output}')
