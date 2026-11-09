#!/usr/bin/env python3
"""v1.01: bounded, reproducible SH-2 / host-C differential tests.

PASS means all requested vectors completed in the supported integer ABI/model,
not a proof. Unsupported source, undefined r0, unmodelled hardware, or any
faulting vector is SKIP. See notes/difftest_report.md for model limitations.
Requires tools/decomp/venv (pycparser), gcc, and the local saturn-recomp tree.
"""
from __future__ import annotations
import argparse
import collections
import concurrent.futures
import datetime
import hashlib
import json
import pathlib
import random
import re
import subprocess
import sys
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/saturn-recomp/src'))
from saturnrecomp import sh2emu
from pycparser import c_ast as A, c_parser, c_generator

VERSION = '1.01'
M32 = 0xffffffff
RAM = 0x06000000
STACK = 0x060ff000
SCRATCH = 0x00280000
PRELUDE = '''typedef signed char s8; typedef unsigned char u8;
typedef short s16; typedef unsigned short u16; typedef int s32;
typedef unsigned int u32; typedef long long s64; typedef unsigned long long u64;
typedef float f32; typedef double f64; typedef unsigned long uintptr_t;
typedef unsigned int uint32_t; typedef int int32_t;
typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned char uint8_t; typedef signed char int8_t;
'''
MACROS = {}
for prefix, sign in [('M', True), ('MU', False), ('FS', True), ('FU', False)]:
    for bits in (8, 16, 32):
        MACROS[prefix + str(bits)] = (bits // 8, sign, True)
for prefix, sign in [('MEM', False), ('MEMS', True)]:
    for bits in (8, 16, 32):
        MACROS[prefix + str(bits)] = (bits // 8, sign, False)
TYPES = {'s8':(1,True), 'u8':(1,False), 's16':(2,True), 'u16':(2,False),
         's32':(4,True), 'u32':(4,False), 'int':(4,True),
         'unsigned int':(4,False), 'short':(2,True), 'unsigned short':(2,False),
         'signed char':(1,True), 'unsigned char':(1,False)}

class Unsupported(Exception):
    pass


def canonical(a):
    a &= M32
    if a >> 28 == 2:
        a &= 0x1fffffff
    if 0x02200000 <= a < 0x0228de20:
        return a - 0x02200000 + 0x06004000
    return a


def preprocess(source):
    """Expand directives with GCC, retaining typed guest-memory marker calls.

    The first pass resolves includes/conditionals but leaves macro definitions;
    the second expands aliases and expressions after replacing accessors. A tiny
    stdint header supplies only the integer typedefs used by this source subset.
    No host system declarations are silently discarded.
    """
    with tempfile.TemporaryDirectory(dir=ROOT / 'work', prefix='difftest-cpp-') as tmp:
        tmp = pathlib.Path(tmp)
        (tmp / 'stdint.h').write_text(PRELUDE)
        command = ['gcc', '-E', '-P', '-undef', '-nostdinc', '-I', str(tmp),
                   '-I', str(ROOT / 'project/src_functional'), '-x', 'c', '-']
        first = subprocess.run(command[:2] + ['-fdirectives-only', '-dD'] + command[2:],
                               input=source, text=True, capture_output=True, timeout=20)
        if first.returncode:
            raise Unsupported('preprocessor: ' + first.stderr.splitlines()[0])
        def marker(match):
            name, params, body = match.groups()
            if name not in MACROS:
                return match[0]
            size, signed, offset = MACROS[name]
            # The project headers sometimes give MEM16/MEM32 signed semantics.
            cast = re.search(r'\(\s*(?:volatile\s+)?([su](?:8|16|32))\s*\*', body)
            if not cast:
                raise Unsupported('unrecognized memory macro definition: ' + name)
            size, signed = int(cast[1][1:]) // 8, cast[1][0] == 's'
            tag = 'dt_mem_%s%d_%d' % ('s' if signed else 'u', size * 8, offset)
            return '#define %s(%s) %s(%s)' % (name, params, tag, params)
        text = re.sub(r'^#define\s+(\w+)\(([^)]+)\)\s+([^\n]*)', marker, first.stdout, flags=re.M)
        second = subprocess.run(command, input=text, text=True, capture_output=True, timeout=20)
        if second.returncode:
            raise Unsupported('preprocessor: ' + second.stderr.splitlines()[0])
        return second.stdout


for _size in (1, 2, 4):
    for _signed in (False, True):
        for _offset in (False, True):
            MACROS['dt_mem_%s%d_%d' % ('s' if _signed else 'u', _size*8, _offset)] = (_size, _signed, _offset)


def parse(source):
    try:
        return c_parser.CParser().parse(PRELUDE + preprocess(source))
    except Unsupported:
        raise
    except Exception as e:
        raise Unsupported('C parser: ' + str(e).splitlines()[0]) from e


def select_function(tree, symbols, entry=None):
    funcs = [n for n in tree.ext if isinstance(n, A.FuncDef)]
    if len(funcs) == 1:
        return funcs[0]
    matches = [n for n in funcs if entry is not None and
               function_address(n.decl.name, symbols) == canonical(entry)]
    if len(matches) != 1:
        raise Unsupported('translation unit has no unique address-matching function')
    return matches[0]


def function_address(name, symbols):
    match = re.fullmatch(r'(?:func_|sub_)([0-9a-fA-F]{8})', name)
    return canonical(int(match[1], 16)) if match else symbols.get(name)


def typename(t):
    while isinstance(t, (A.TypeDecl, A.Typename)):
        t = t.type
    if isinstance(t, A.IdentifierType):
        return ' '.join(t.names)
    raise Unsupported('non-integer or aggregate type')


class Generator(c_generator.CGenerator):
    """Instrument only explicit memory macros and address-named scalar globals.

    Raw pointer dereferences/arrays/structs are rejected, never executed natively.
    This keeps the instrumented source's loads/stores observable and big endian.
    """
    def __init__(self, fn, globals_, symbols, signatures):
        super().__init__()
        self.globals = globals_
        self.symbols = symbols
        self.signatures = signatures
        self.locals = set()
        self.fn = fn
        self.calls = set()
        class Locals(A.NodeVisitor):
            def visit_Decl(inner, n):
                self.locals.add(n.name)
                inner.generic_visit(n)
        Locals().visit(fn)

    def memory(self, n):
        if isinstance(n, A.FuncCall) and isinstance(n.name, A.ID) and n.name.name in MACROS:
            size, signed, offset = MACROS[n.name.name]
            args = n.args.exprs if n.args else []
            if len(args) != (2 if offset else 1):
                raise Unsupported('memory macro arity')
            addr = '(uintptr_t)(%s)' % self.visit(args[0])
            if offset:
                addr += '+(%s)' % self.visit(args[1])
            return addr, size, signed
        if isinstance(n, A.ID) and n.name in self.globals and n.name not in self.locals:
            return self.globals[n.name]
        return None

    def load(self, mem):
        addr, size, signed = mem
        cast = ('s' if signed else 'u') + str(size * 8)
        return '((%s)dt_read((u32)(%s),%d))' % (cast, addr, size)

    def visit_ID(self, n):
        mem = self.memory(n)
        if mem:
            return self.load(mem)
        if n.name in self.locals:
            return n.name
        if n.name == 'NULL':
            return '0'
        raise Unsupported('unresolved identifier: ' + n.name)

    def visit_FuncCall(self, n):
        mem = self.memory(n)
        if mem:
            return self.load(mem)
        if not isinstance(n.name, A.ID):
            raise Unsupported('indirect C call')
        name = n.name.name
        args = n.args.exprs if n.args else []
        if name not in self.symbols:
            raise Unsupported('unresolved callee: ' + name)
        if len(args) != 4:
            raise Unsupported('callee has fewer/more than four explicit register arguments')
        signature = self.signatures.get(name)
        if signature is None or signature.args is None or len(signature.args.params) != 4:
            raise Unsupported('callee prototype does not describe four word arguments')
        for param in signature.args.params:
            if not isinstance(param, A.Decl) or isinstance(param.type, A.PtrDecl) or TYPES.get(typename(param.type), (0,False))[0] != 4:
                raise Unsupported('callee prototype has unsupported parameter type')
        ret = typename(signature.type)
        if ret != 'void' and TYPES.get(ret, (0,False))[0] != 4:
            raise Unsupported('callee prototype has unsupported return type')
        self.calls.add(name)
        call = 'dt_call(0x%x,%s)' % (self.symbols[name], ','.join('(u32)(uintptr_t)(%s)' % self.visit(a) for a in args))
        return '((%s)%s)' % (ret,call)

    def visit_Assignment(self, n):
        mem = self.memory(n.lvalue)
        if not mem:
            return super().visit_Assignment(n)
        addr, size, signed = mem
        # Avoid evaluating side effects twice in a compound memory assignment.
        if n.op != '=':
            raise Unsupported('compound memory assignment')
        return '((%s)dt_write((u32)(%s),%d,(u32)(%s)))' % (('s' if signed else 'u') + str(size*8), addr, size, self.visit(n.rvalue))

    def visit_UnaryOp(self, n):
        if n.op in ('*', '&', 'sizeof'):
            raise Unsupported('raw pointer/address/sizeof operation')
        if self.memory(n.expr):
            raise Unsupported('memory increment or unary operation')
        return super().visit_UnaryOp(n)

    def visit_Constant(self, n):
        if n.type in ('float', 'double', 'string') or re.search(r'[lL]', n.value):
            raise Unsupported('floating/string/wide constant')
        return super().visit_Constant(n)

    def visit_ArrayRef(self, n):
        raise Unsupported('raw array access')

    def visit_StructRef(self, n):
        raise Unsupported('struct access')

    def visit_Decl(self, n, no_type=False):
        if isinstance(n.type, A.ArrayDecl) or (isinstance(n.type, A.FuncDecl) and n is not self.fn.decl):
            raise Unsupported('local array/function declaration')
        if isinstance(n.type, A.TypeDecl) and isinstance(n.type.type, A.IdentifierType) and n.type.type.names == ['uintptr_t']:
            raise Unsupported('host-sized uintptr_t local')
        if 'static' in n.storage or n.bitsize:
            raise Unsupported('static local or bitfield')
        return super().visit_Decl(n, no_type)

    def visit_IdentifierType(self, n):
        name = ' '.join(n.names)
        if name not in TYPES and name not in ('void', 'uintptr_t'):
            raise Unsupported('unsupported host type: ' + name)
        return super().visit_IdentifierType(n)


def prepare(source, symbols, entry=None, metadata=None):
    tree = parse(source)
    fn = select_function(tree, symbols, entry)
    is_void = typename(fn.decl.type.type) == 'void'
    if not is_void and typename(fn.decl.type.type) not in TYPES:
        raise Unsupported('void/non-integer return: r0 has no faithful C representation')
    if metadata is not None:
        metadata['r0_checked'] = not is_void
    params = fn.decl.type.args.params if fn.decl.type.args else []
    if len(params) == 1 and isinstance(params[0], A.Typename) and typename(params[0]) == 'void':
        params = []
    if len(params) > 4 or any(not isinstance(p, A.Decl) for p in params):
        raise Unsupported('nonstandard/stack parameter ABI')
    globals_ = {}
    for n in tree.ext:
        if not isinstance(n, A.Decl) or isinstance(n.type, A.FuncDecl):
            continue
        match = re.fullmatch(r'g_([0-9a-fA-F]{8})', n.name or '')
        if not match or 'extern' not in n.storage:
            continue
        try:
            size, signed = TYPES[typename(n.type)]
            globals_[n.name] = ('0x' + match[1], size, signed)
        except (Unsupported, KeyError):
            pass
    signatures = {n.name:n.type for n in tree.ext if isinstance(n,A.Decl) and isinstance(n.type,A.FuncDecl)}
    symbols = dict(symbols)
    for other in tree.ext:
        if isinstance(other, A.FuncDef):
            signatures[other.decl.name] = other.decl.type
            address = function_address(other.decl.name, symbols)
            if address is not None:
                symbols[other.decl.name] = address
    gen = Generator(fn, globals_, symbols, signatures)
    generated = gen.visit(fn)
    casts = []
    pointers = []
    for p in params:
        pointer = isinstance(p.type, A.PtrDecl)
        pointers.append(pointer)
        if pointer:
            # Only byte/void pointers: host pointer arithmetic must match SH-2.
            if typename(p.type.type) not in ('void', 's8', 'u8', 'signed char', 'unsigned char'):
                raise Unsupported('non-byte pointer parameter')
            cast = gen.visit(p.type)
        else:
            cast = typename(p.type)
            if cast not in TYPES or TYPES[cast][0] != 4:
                raise Unsupported('non-word parameter ABI')
        casts.append('(%s)(uintptr_t)args[%d]' % (cast, len(casts)))
    wrapper = '\nu32 dt_entry(u32 *args) { return (u32)%s(%s); }\n' % (fn.decl.name, ','.join(casts))
    if is_void:
        wrapper = '\nu32 dt_entry(u32 *args) { %s(%s); return 0; }\n' % (fn.decl.name, ','.join(casts))
    return generated + wrapper, pointers


HARNESS = r'''
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
static unsigned char high[0x100000], low[0x100000];
static unsigned events;
static void dt_event(void) { if(++events>100000) exit(76); }
static unsigned char *dt_ptr(u32 a, int n) {
 if((a>>28)==2) a &= 0x1fffffff;
 if(a>=0x06000000 && (uint64_t)a+n<=0x06100000) return high+a-0x06000000;
 if(a>=0x00200000 && (uint64_t)a+n<=0x00300000) return low+a-0x00200000;
 fprintf(stderr,"unmapped C memory %08x/%d\n",a,n); exit(73);
}
static u32 dt_read(u32 a,int n) {
 unsigned char *p=dt_ptr(a,n); u32 v=0;
 for(int i=0;i<n;i++) v=(v<<8)|p[i]; return v;
}
static u32 dt_write(u32 a,int n,u32 v) {
 dt_event(); unsigned char *p=dt_ptr(a,n); u32 result=v;
 printf("W %08x %d %08x\n",a,n,v);
 for(int i=n-1;i>=0;i--){p[i]=v&255;v>>=8;} return result;
}
static u32 dt_call(u32 t,u32 a,u32 b,u32 c,u32 d) {
 dt_event(); printf("C %08x %08x %08x %08x %08x\n",t,a,b,c,d);
 return t ^ a ^ ((b<<7)|(b>>25)) ^ ((c<<13)|(c>>19)) ^ d ^ 0x9e3779b9u;
}
'''
MAIN = r'''
int main(int argc,char **argv) {
 u32 args[4];
 FILE *f=fopen(argv[1],"rb"); if(!f) return 74;
 if(fread(args,4,4,f)!=4 || fread(high,1,sizeof high,f)!=sizeof high ||
    fread(low,1,sizeof low,f)!=sizeof low) return 75;
 fclose(f); u32 out=dt_entry(args); printf("R %08x\n",out); return 0;
}
'''


def stub(t, args):
    a, b, c, d = args
    return (t ^ a ^ ((b << 7) | (b >> 25)) ^ ((c << 13) | (c >> 19)) ^ d ^ 0x9e3779b9) & M32


class Oracle(sh2emu.CPU):
    def __init__(self, mem, entries=(), entry=None):
        super().__init__(mem)
        self.calls = []
        self.entries = set(entries)
        self.entry = entry

    def step(self):
        ins = self.fetch(self.pc)
        if ins.op in ('rte', 'trapa', 'sleep'):
            raise Unsupported('system control flow')
        call = ins.op in ('bsr', 'jsr', 'bsrf')
        computed = ins.op in ('jmp', 'braf')
        super().step()  # includes the delay slot, before observing arguments
        tail = ins.op in ('bra', 'jmp', 'braf') and self.pc in self.entries and self.pc != self.entry
        if computed and not tail:
            raise Unsupported('computed tail/switch target not a known callee')
        if call or tail:
            target = canonical(self.pc)
            args = self.r[4:8]
            self.calls.append([target] + args)
            self.r[0] = stub(target, args)
            self.pc = self.pr


def writes(log):
    """Final value of EVERY externally visible written byte, including no-ops.

    Stack spills are ABI-private, so excluded. Store order/width need not match
    optimized C, but the byte footprint (including writes of unchanged data) must.
    """
    out = {}
    for kind, addr, size, value in log:
        if kind != 'w':
            continue
        addr = addr & 0x1fffffff if addr >> 28 == 2 else addr
        if STACK - 0x8000 <= addr < STACK + 0x100:
            continue
        for i, b in enumerate((value & ((1 << (size * 8)) - 1)).to_bytes(size, 'big')):
            out[addr + i] = b
    return out


def write_trace(log):
    """Byte-normalized chronological stores, retaining intermediate values."""
    out = []
    for event in log:
        out.extend(writes([event]).items())
    return out


def reference(entry, args, high, low, noise, entries=()):
    mem = sh2emu.Memory()
    mem.add(RAM, len(high), high)
    mem.add(0x00200000, len(low), low)
    mem.log = []
    cpu = Oracle(mem, entries, entry)
    cpu.r = [noise] * 16
    cpu.r[4:8] = args
    cpu.r[15] = STACK
    cpu.sr = noise & 0x3f3
    cpu.gbr = cpu.vbr = cpu.mach = cpu.macl = noise
    cpu.call(entry, max_steps=20000)
    if any(k == 'w' and 0x06004000 <= (a & 0x1fffffff) < 0x06091e20 for k,a,_,_ in mem.log):
        raise Unsupported('writes to original code/rodata image')
    return cpu.r[0], writes(mem.log), cpu.calls, write_trace(mem.log)


def difference(ref, got, check_r0=True):
    if check_r0 and ref[0] != got[0]:
        return {'kind':'r0', 'reference':f'{ref[0]:08x}', 'candidate':f'{got[0]:08x}'}
    if ref[2] != got[2]:
        i = next((i for i,(a,b) in enumerate(zip(ref[2],got[2])) if a != b), min(len(ref[2]),len(got[2])))
        return {'kind':'calls', 'index':i, 'reference':ref[2][i:i+1], 'candidate':got[2][i:i+1]}
    if ref[1] != got[1]:
        for a in sorted(ref[1].keys() | got[1].keys()):
            if ref[1].get(a) != got[1].get(a):
                return {'kind':'memory write', 'address':f'{a:08x}', 'reference':ref[1].get(a), 'candidate':got[1].get(a)}
    if len(ref) > 3 and ref[3] != got[3]:
        i = next((i for i,(a,b) in enumerate(zip(ref[3],got[3])) if a != b), min(len(ref[3]),len(got[3])))
        return {'kind':'memory write trace', 'index':i, 'reference':ref[3][i:i+1], 'candidate':got[3][i:i+1]}
    return None


def load_symbols():
    out, ambiguous = {}, set()
    for p in sorted((ROOT / 'project/src_functional').glob('*.c')):
        try:
            addr = canonical(int(p.stem, 16))
            out['func_' + p.stem] = addr
            tree = parse(p.read_text())
            funcs = [n for n in tree.ext if isinstance(n, A.FuncDef)]
            for fn in funcs:
                mapped = function_address(fn.decl.name, {})
                if len(funcs) != 1 and mapped is None:
                    continue
                name = fn.decl.name
                addr = mapped if mapped is not None else addr
                if name in out and out[name] != addr:
                    ambiguous.add(name)
                out.setdefault(name, addr)
        except (ValueError, Unsupported):
            pass
    return {name:addr for name,addr in out.items() if name not in ambiguous}


def test_one(task):
    address, vectors, symbols, override = task
    result = {'address':f'{address:08x}', 'status':'SKIP', 'vectors':0}
    entry = canonical(address)
    if not 0x06004000 <= entry < 0x06091e20:
        result['reason'] = 'no original bytes in main program image'
        return result
    source_path = ROOT / 'project/src_functional' / f'{address:08x}.c'
    if not source_path.exists() and 0x06004000 <= address < 0x06091e20:
        source_path = ROOT / 'project/src_functional' / f'{address - 0x06004000 + 0x02200000:08x}.c'
    try:
        source = pathlib.Path(override).read_text() if override else source_path.read_text()
        # Known compiler helpers use r0/r1, never infer their ABI from C names.
        if 0x0604e000 <= entry < 0x0604f000:
            raise Unsupported('compiler runtime/nonstandard register ABI')
        generated, pointers = prepare(source, symbols, entry, result)
        with (ROOT / 'cart.bin').open('rb') as image:
            image.seek(0x200000)
            cart = image.read(0x8de20)
        if len(cart) != 0x8de20:
            raise Unsupported('truncated cart program image')
        result['source_sha256'] = hashlib.sha256(source.encode()).hexdigest()
        with tempfile.TemporaryDirectory(prefix='difftest-', dir=ROOT / 'work') as tmp:
            tmp = pathlib.Path(tmp)
            cfile, exe, state = tmp/'candidate.c', tmp/'candidate', tmp/'state.bin'
            cfile.write_text(PRELUDE + HARNESS + generated + MAIN)
            proc = subprocess.run(['gcc','-std=c11','-O1','-Wall','-Werror=uninitialized','-Werror=maybe-uninitialized','-fwrapv','-fno-strict-aliasing',
                                   '-Werror=implicit-function-declaration','-fsanitize=shift-exponent,integer-divide-by-zero,bounds','-fno-sanitize-recover=all',str(cfile),'-o',str(exe)],
                                  capture_output=True, text=True, timeout=20)
            if proc.returncode:
                raise Unsupported('host compile: ' + proc.stderr.splitlines()[0])
            rng = random.Random(0xd1ff0000 ^ entry)
            for index in range(vectors):
                high = bytearray(rng.randbytes(0x100000))
                low = rng.randbytes(0x100000)
                high[0x4000:0x91e20] = cart
                # Mix edge values, bounded counts and full words reproducibly.
                edge = [0,1,2,7,15,31,32,0x7fffffff,0x80000000,0xffffffff]
                args = [rng.choice(edge) if index < 10 else rng.getrandbits(32) for _ in range(4)]
                for i, pointer in enumerate(pointers):
                    if pointer:
                        args[i] = SCRATCH + 0x1000 * i + rng.randrange(0,256,4)
                result['vector'] = index
                result['args'] = [f'{a:08x}' for a in args]
                try:
                    ref = reference(entry, args, high, low, 0x13579bdf, symbols.values())
                    alternate = reference(entry, args, high, low, 0x2468ace0, symbols.values())
                except (sh2emu.MemoryError_, RuntimeError, ValueError, IndexError) as e:
                    raise Unsupported('reference cannot run: ' + str(e)) from e
                if difference(ref, alternate, result['r0_checked']):
                    raise Unsupported('depends on registers outside r4-r7 / undefined r0')
                # C only models addressable RAM, never host pointers or hardware.
                import struct
                state.write_bytes(struct.pack('=4I', *args) + high + low)
                try:
                    proc = subprocess.run([str(exe),str(state)], capture_output=True, text=True, timeout=2)
                except subprocess.TimeoutExpired as e:
                    raise Unsupported('candidate timeout') from e
                if proc.returncode:
                    raise Unsupported('candidate cannot run: ' + (proc.stderr.strip() or f'exit {proc.returncode}'))
                log, calls, ret = [], [], None
                for line in proc.stdout.splitlines():
                    fields = line.split()
                    if fields[0] == 'R':
                        ret = int(fields[1],16)
                    elif fields[0] == 'C':
                        calls.append([int(x,16) for x in fields[1:]])
                    elif fields[0] == 'W':
                        log.append(('w', int(fields[1],16), int(fields[2]), int(fields[3],16)))
                diff = difference(ref, (ret, writes(log), calls, write_trace(log)), result['r0_checked'])
                result['vectors'] += 1
                if diff:
                    result.update(status='FAIL', diff=diff)
                    return result
            result.update(status='PASS' if result['r0_checked'] else 'PASS-void')
            result.pop('args', None)
            result.pop('vector', None)
    except (Unsupported, OSError, subprocess.TimeoutExpired) as e:
        result['reason'] = str(e)
    return result


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('addresses', nargs='*', help='hex SH-2 addresses (022xxxxx or 060xxxxx)')
    ap.add_argument('--all', action='store_true')
    ap.add_argument('--vectors', type=int, default=16)
    ap.add_argument('--jobs', type=int, default=1)
    ap.add_argument('--json', type=pathlib.Path)
    ap.add_argument('--candidate', type=pathlib.Path, help='alternate C file for a single address (mutation testing)')
    ap.add_argument('--version', action='version', version=VERSION)
    args = ap.parse_args(argv)
    if args.vectors < 1 or args.jobs < 1:
        ap.error('--vectors and --jobs must be positive')
    if args.all == bool(args.addresses):
        ap.error('choose addresses or --all')
    if args.candidate and (args.all or len(args.addresses) != 1):
        ap.error('--candidate requires exactly one address')
    try:
        addresses = sorted(int(p.stem,16) for p in (ROOT/'project/src_functional').glob('*.c')) if args.all else [int(s,16) for s in args.addresses]
    except ValueError:
        ap.error('addresses must be hex')
    (ROOT/'work').mkdir(exist_ok=True)
    symbols = load_symbols()
    tasks = [(a,args.vectors,symbols,str(args.candidate) if args.candidate else None) for a in addresses]
    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for result in pool.map(test_one,tasks):
            results.append(result)
            attempt_dir = ROOT/'work/attempts'
            attempt_dir.mkdir(exist_ok=True)
            attempt = {'ts':datetime.datetime.now(datetime.timezone.utc).isoformat(), 'agent':'codex',
                       'result':'difftest ' + result['status'],
                       'note':f"v{VERSION}; {result['vectors']}/{args.vectors} vectors; " + result.get('reason', str(result.get('diff', 'equivalent'))) + ('; mutated candidate' if args.candidate else '')}
            with (attempt_dir/(result['address']+'.jsonl')).open('a') as stream:
                stream.write(json.dumps(attempt)+'\n')
            detail = result.get('reason') or (json.dumps(result['diff']) if 'diff' in result else f"{result['vectors']} vectors")
            print(f"{result['address']} {result['status']} {detail}", flush=True)
    tally = collections.Counter(r['status'] for r in results)
    counts = {status:tally[status] for status in ('PASS','PASS-void','FAIL','SKIP')}
    report = {'version':VERSION, 'cart_sha256':hashlib.sha256((ROOT/'cart.bin').read_bytes()).hexdigest(), 'vectors':args.vectors, 'seed':'0xd1ff0000 XOR canonical entry',
              'model':'integer r4-r7 ABI; big-endian RAM; all byte-normalized stores and final footprint; private stack excluded',
              'counts':counts, 'results':results}
    destinations = set()
    if args.all:
        destinations.add(ROOT/'work/difftest_all.json')
    if args.json:
        destinations.add(args.json)
    for path in destinations:
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(counts,sort_keys=True))
    return 1 if counts.get('FAIL') else 0

if __name__ == '__main__':
    raise SystemExit(main())
