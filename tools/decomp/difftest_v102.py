#!/usr/bin/env python3
"""v1.02: bounded, reproducible SH-2 / host-C differential tests.

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

VERSION = '1.02'
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
for prefix, sign in [('MEM', True), ('MEMS', True), ('MEMU', False)]:
    for bits in (8, 16, 32):
        MACROS[prefix + str(bits)] = (bits // 8, sign, False)
MACROS['MEM8'] = (1, False, False)
TYPES = {'s8':(1,True), 'u8':(1,False), 's16':(2,True), 'u16':(2,False),
         's32':(4,True), 'u32':(4,False), 'int':(4,True),
         'unsigned int':(4,False), 'short':(2,True), 'unsigned short':(2,False),
         'signed char':(1,True), 'unsigned char':(1,False),
         'signed int':(4,True), 'signed short':(2,True),
         'short int':(2,True), 'unsigned short int':(2,False), 'signed short int':(2,True)}

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
    with tempfile.TemporaryDirectory(dir=None, prefix='difftest-cpp-') as tmp:
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


def resolve_type(t, aliases):
    seen = set()
    while isinstance(t, (A.TypeDecl, A.Typename, A.IdentifierType)):
        if isinstance(t, A.IdentifierType):
            name = ' '.join(t.names)
            if name == 'uintptr_t':
                return A.IdentifierType(['unsigned', 'int'])
            if name not in aliases:
                return t
            if name in seen:
                raise Unsupported('cyclic typedef')
            seen.add(name)
            t = aliases[name]
        else:
            t = t.type
    return t


def type_size(t, aliases):
    t = resolve_type(t, aliases)
    if isinstance(t, A.PtrDecl):
        return 4, False
    name = typename(t)
    if name not in TYPES:
        raise Unsupported('unsupported guest type: ' + name)
    return TYPES[name]


class Generator(c_generator.CGenerator):
    """Instrument typed guest loads/stores and calls using the pycparser AST.

    Guest pointers stay 32-bit words. No guest dereference or function pointer
    reaches native execution; unsupported types and ambiguous ABIs remain SKIP.
    """
    def __init__(self, fn, globals_, symbols, signatures, metadata, aliases, vartypes):
        super().__init__()
        self.aliases = aliases
        self.vartypes = dict(vartypes)
        self.call_specs = {}
        self.inferred = {}
        self.indirect_counts = set()
        self.globals = globals_
        self.symbols = symbols
        self.signatures = signatures
        self.locals = set()
        self.fn = fn
        self.calls = set()
        class Locals(A.NodeVisitor):
            def visit_Decl(inner, n):
                if n is fn.decl:
                    for param in fn.decl.type.args.params if fn.decl.type.args else []:
                        if isinstance(param, A.Decl):
                            inner.visit(param)
                    return
                original = c_generator.CGenerator()
                if n.name in self.locals and original._generate_type(self.vartypes[n.name], emit_declname=False) != original._generate_type(n.type, emit_declname=False):
                    raise Unsupported('shadowed local has conflicting types')
                self.locals.add(n.name)
                self.vartypes[n.name] = n.type
                # Callback prototype parameter names have no local binding.
                if n.init is not None:
                    inner.visit(n.init)

            def visit_Typedef(inner, n):
                raise Unsupported('local typedef declaration')
        Locals().visit(fn)

    def resolved(self, t):
        return resolve_type(t, self.aliases) if t is not None else None

    def expr_type(self, n):
        if isinstance(n, A.ID):
            return self.vartypes.get(n.name) or self.signatures.get(n.name)
        if isinstance(n, A.Cast):
            return n.to_type
        if isinstance(n, A.UnaryOp):
            t = self.resolved(self.expr_type(n.expr))
            if n.op == '*' and isinstance(t, A.PtrDecl):
                return t.type
            if n.op == '&' and t is not None:
                return A.PtrDecl([], t)
            return t if n.op in ('p++','p--','++','--') else None
        if isinstance(n, A.ArrayRef):
            t = self.resolved(self.expr_type(n.name))
            return t.type if isinstance(t, (A.PtrDecl, A.ArrayDecl)) else None
        if isinstance(n, A.BinaryOp) and n.op in ('+', '-'):
            left, right = self.resolved(self.expr_type(n.left)), self.resolved(self.expr_type(n.right))
            pointers = (A.PtrDecl, A.ArrayDecl)
            if isinstance(left, pointers) and not isinstance(right, pointers):
                return A.PtrDecl([],left.type)
            if n.op == '+' and isinstance(right, pointers) and not isinstance(left, pointers):
                return A.PtrDecl([],right.type)
        if isinstance(n, A.Assignment):
            return self.expr_type(n.lvalue)
        if isinstance(n, A.TernaryOp):
            a, b = self.resolved(self.expr_type(n.iftrue)), self.resolved(self.expr_type(n.iffalse))
            pointers = (A.PtrDecl, A.ArrayDecl)
            if isinstance(a, pointers) or isinstance(b, pointers):
                def null(expr):
                    return (isinstance(expr,A.ID) and expr.name == 'NULL') or (isinstance(expr,A.Constant) and expr.value == '0')
                if a is None and null(n.iftrue):
                    return b
                if b is None and null(n.iffalse):
                    return a
                if a is None or b is None or c_generator.CGenerator().visit(a) != c_generator.CGenerator().visit(b):
                    raise Unsupported('ambiguous pointer conditional type')
                return a
        if isinstance(n, A.FuncCall):
            t = self.resolved(self.expr_type(n.name))
            if isinstance(t, A.PtrDecl):
                t = self.resolved(t.type)
            if isinstance(t, A.FuncDecl):
                return t.type
        return None

    def _generate_type(self, n, modifiers=None, emit_declname=True, emit_base_type=True):
        # Guest pointers are words, never native pointers. Keep the original AST
        # types separately for scaled arithmetic, loads and callback signatures.
        t = self.resolved(n)
        if isinstance(t, A.PtrDecl):
            inner = n
            while hasattr(inner, 'type') and not isinstance(inner, A.TypeDecl):
                inner = inner.type
            name = getattr(inner, 'declname', '') if emit_declname else ''
            return ('u32 ' if emit_base_type else '') + (name or '')
        return super()._generate_type(n, modifiers, emit_declname, emit_base_type)

    def pointer_stride(self, t):
        t = self.resolved(t)
        if not isinstance(t, (A.PtrDecl, A.ArrayDecl)):
            raise Unsupported('untyped guest pointer')
        return type_size(t.type, self.aliases)[0]

    def array_address(self, n):
        stride = self.pointer_stride(self.expr_type(n.name))
        return '((u32)(%s)+%d*(u32)(%s))' % (self.visit(n.name), stride, self.visit(n.subscript))

    def visit_BinaryOp(self, n):
        left, right = self.resolved(self.expr_type(n.left)), self.resolved(self.expr_type(n.right))
        lp, rp = isinstance(left, (A.PtrDecl, A.ArrayDecl)), isinstance(right, (A.PtrDecl, A.ArrayDecl))
        if n.op in ('+', '-') and (lp or rp):
            a, b = self.visit(n.left), self.visit(n.right)
            if lp and rp:
                if n.op != '-' or self.pointer_stride(left) != self.pointer_stride(right):
                    raise Unsupported('invalid/ambiguous pointer arithmetic')
                return '((s32)((u32)(%s)-(u32)(%s))/%d)' % (a,b,self.pointer_stride(left))
            if lp:
                return '((u32)((%s)%s%d*(u32)(%s)))' % (a,n.op,self.pointer_stride(left),b)
            if n.op == '+':
                return '((u32)((%s)+%d*(u32)(%s)))' % (b,self.pointer_stride(right),a)
            raise Unsupported('integer minus pointer')
        return super().visit_BinaryOp(n)

    def memory(self, n):
        if isinstance(n, A.UnaryOp) and n.op == '*':
            t = self.resolved(self.expr_type(n.expr))
            if not isinstance(t, A.PtrDecl):
                raise Unsupported('untyped raw pointer dereference')
            size, signed = type_size(t.type, self.aliases)
            return self.visit(n.expr), size, signed
        if isinstance(n, A.ArrayRef):
            size, signed = type_size(self.expr_type(n), self.aliases)
            return self.array_address(n), size, signed
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
        if n.name in self.vartypes and isinstance(self.resolved(self.vartypes[n.name]), A.ArrayDecl):
            match = re.fullmatch(r'g_([0-9a-fA-F]{8})', n.name)
            if match:
                return '0x' + match[1]
        address = function_address(n.name, self.symbols)
        if address is not None and n.name in self.signatures:
            return '0x%x' % address
        if n.name == 'NULL':
            return '0'
        raise Unsupported('unresolved identifier: ' + n.name)

    def visit_FuncCall(self, n):
        mem = self.memory(n)
        if mem:
            return self.load(mem)
        args = n.args.exprs if n.args else []
        name = n.name.name if isinstance(n.name, A.ID) else None
        callee_type = self.resolved(self.expr_type(n.name))
        indirect = isinstance(callee_type, A.PtrDecl) or (isinstance(callee_type, A.FuncDecl) and (name is None or name in self.locals))
        if indirect:
            signature = self.resolved(callee_type.type) if isinstance(callee_type, A.PtrDecl) else callee_type
            if not isinstance(signature, A.FuncDecl):
                raise Unsupported('indirect call lacks function-pointer type')
            target_expr = self.visit(n.name.expr) if isinstance(n.name, A.UnaryOp) and n.name.op == '*' else self.visit(n.name)
            self.indirect_counts.add(len(args))
            label = 'indirect@' + str(n.coord)
        else:
            if name is None:
                raise Unsupported('indirect call lacks function-pointer type')
            target = function_address(name, self.symbols)
            if target is None:
                raise Unsupported('unresolved callee: ' + name)
            if 0x0604e000 <= canonical(target) < 0x0604f000:
                raise Unsupported('compiler runtime/nonstandard callee ABI')
            signature = self.signatures.get(name)
            target_expr = '0x%x' % target
            if target in self.call_specs and self.call_specs[target] != len(args):
                raise Unsupported('conflicting callee argument counts for target')
            self.call_specs[target] = len(args)
            self.calls.add(name)
            label = name
        params = word_params(signature, self.aliases) if signature is not None else None
        if params is None:
            count = len(args)
            if label in self.inferred and self.inferred[label]['args'] != count:
                raise Unsupported('inconsistent inferred callee argument count: ' + label)
            self.inferred[label] = {'args':count, 'return':'int' if signature is None else 'declared'}
        elif len(args) != len(params):
            raise Unsupported('call argument count differs from prototype: ' + str(name))
        ret_type = self.resolved(signature.type) if signature is not None else A.IdentifierType(['int'])
        if isinstance(ret_type, A.PtrDecl):
            ret = 'u32'
        else:
            ret = typename(ret_type)
            if ret != 'void' and TYPES.get(ret, (0,False))[0] != 4:
                raise Unsupported('callee prototype has unsupported return type')
        call = 'dt_callv((u32)(%s),%d,(u32[]){%s})' % (target_expr, len(args),
            ','.join('(u32)(%s)' % self.visit(a) for a in args) or '0')
        return '((%s)%s)' % (ret,call)

    def visit_Assignment(self, n):
        mem = self.memory(n.lvalue)
        if not mem:
            t = self.resolved(self.expr_type(n.lvalue))
            if isinstance(t, A.PtrDecl) and n.op in ('+=', '-='):
                return '(%s %s %d*(u32)(%s))' % (self.visit(n.lvalue),n.op,self.pointer_stride(t),self.visit(n.rvalue))
            return super().visit_Assignment(n)
        addr, size, signed = mem
        # Avoid evaluating side effects twice in a compound memory assignment.
        if n.op != '=':
            raise Unsupported('compound memory assignment')
        return '((%s)dt_write((u32)(%s),%d,(u32)(%s)))' % (('s' if signed else 'u') + str(size*8), addr, size, self.visit(n.rvalue))

    def visit_UnaryOp(self, n):
        if n.op == '*':
            return self.load(self.memory(n))
        if n.op == '&':
            if isinstance(n.expr, A.UnaryOp) and n.expr.op == '*':
                return self.visit(n.expr.expr)
            if isinstance(n.expr, A.ArrayRef):
                return self.array_address(n.expr)
            if isinstance(n.expr, A.ID) and n.expr.name not in self.locals:
                mem = self.memory(n.expr)
                if mem:
                    return mem[0]
                if n.expr.name in self.signatures:
                    return self.visit(n.expr)
            raise Unsupported('local/unsupported address taking')
        if n.op == 'sizeof':
            raise Unsupported('sizeof operation')
        if n.op in ('p++','p--','++','--'):
            if self.memory(n.expr):
                raise Unsupported('memory increment or unary operation')
            t = self.resolved(self.expr_type(n.expr))
            if isinstance(t, A.PtrDecl):
                if not isinstance(n.expr, A.ID):
                    raise Unsupported('complex pointer increment')
                stride = self.pointer_stride(t)
                op = '+' if '+' in n.op else '-'
                value = '(%s %s= %d)' % (self.visit(n.expr),op,stride)
                if n.op.startswith('p'):
                    value = '((u32)(%s %s %d))' % (value,'-' if op == '+' else '+',stride)
                return value
        return super().visit_UnaryOp(n)

    def visit_Constant(self, n):
        if n.type in ('float', 'double', 'string') or re.search(r'[lL]', n.value):
            raise Unsupported('floating/string/wide constant')
        return super().visit_Constant(n)

    def visit_ArrayRef(self, n):
        return self.load(self.memory(n))

    def visit_CompoundLiteral(self, n):
        raise Unsupported('local aggregate literal')

    def visit_StructRef(self, n):
        raise Unsupported('struct access')

    def visit_Decl(self, n, no_type=False):
        if isinstance(n.type, A.ArrayDecl) or (isinstance(n.type, A.FuncDecl) and n is not self.fn.decl):
            raise Unsupported('local array/function declaration')
        if 'static' in n.storage or n.bitsize:
            raise Unsupported('static local or bitfield')
        return super().visit_Decl(n, no_type)

    def visit_IdentifierType(self, n):
        resolved = self.resolved(n)
        name = typename(resolved)
        if name not in TYPES and name != 'void':
            raise Unsupported('unsupported host type: ' + name)
        return name


def word_params(signature, aliases):
    """None means a missing/old-style prototype, [] means explicit void."""
    if signature.args is None:
        return None
    params = signature.args.params
    if len(params) == 1 and isinstance(params[0], A.Typename) and typename(params[0]) == 'void':
        return []
    for p in params:
        if not isinstance(p, (A.Decl, A.Typename)):
            raise Unsupported('variadic/non-word callee ABI')
        if type_size(p.type, aliases)[0] != 4:
            raise Unsupported('callee prototype has unsupported parameter type')
    return params


def prepare(source, symbols, entry=None, metadata=None):
    tree = parse(source)
    aliases = {n.name:n.type for n in tree.ext if isinstance(n, A.Typedef)}
    vartypes = {n.name:n.type for n in tree.ext if isinstance(n, A.Decl)}
    fn = select_function(tree, symbols, entry)
    is_void = typename(fn.decl.type.type) == 'void'
    if not is_void and typename(fn.decl.type.type) not in TYPES:
        raise Unsupported('void/non-integer return: r0 has no faithful C representation')
    if metadata is not None:
        metadata['r0_checked'] = not is_void
    params = fn.decl.type.args.params if fn.decl.type.args else []
    if len(params) == 1 and isinstance(params[0], A.Typename) and typename(params[0]) == 'void':
        params = []
    if any(not isinstance(p, A.Decl) for p in params):
        raise Unsupported('nonstandard/stack parameter ABI')
    globals_ = {}
    for n in tree.ext:
        if not isinstance(n, A.Decl) or isinstance(n.type, A.FuncDecl):
            continue
        match = re.fullmatch(r'g_([0-9a-fA-F]{8})', n.name or '')
        if not match or 'extern' not in n.storage:
            continue
        try:
            size, signed = type_size(n.type, aliases)
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
    gen = Generator(fn, globals_, symbols, signatures, metadata, aliases, vartypes)
    generated = gen.visit(fn)
    if metadata is not None:
        metadata['call_specs'] = gen.call_specs
        metadata['indirect_counts'] = sorted(gen.indirect_counts)
        metadata['inferred_prototypes'] = dict(sorted(gen.inferred.items()))
    casts = []
    pointers = []
    for p in params:
        pointer = isinstance(resolve_type(p.type, aliases), A.PtrDecl)
        pointers.append(pointer)
        if pointer:
            cast = 'u32'
        else:
            cast = typename(resolve_type(p.type, aliases))
            if cast not in TYPES or TYPES[cast][0] != 4:
                raise Unsupported('non-word parameter ABI')
        casts.append('(%s)(uintptr_t)args[%d]' % (cast, len(casts)))
    wrapper = '\nu32 dt_entry(u32 *args) { return (u32)%s(%s); }\n' % (fn.decl.name, ','.join(casts))
    if is_void:
        wrapper = '\nu32 dt_entry(u32 *args) { %s(%s); return 0; }\n' % (fn.decl.name, ','.join(casts))
    if metadata is not None:
        metadata['entry_arg_count'] = len(params)
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
static u32 dt_callv(u32 t,unsigned n,const u32 *args) {
 dt_event(); printf("C %08x",t);
 u32 a[4]={0,0,0,0};
 for(unsigned i=0;i<n;i++) { printf(" %08x",args[i]); if(i<4) a[i]=args[i]; }
 printf("\n");
 u32 h=t ^ a[0] ^ ((a[1]<<7)|(a[1]>>25)) ^ ((a[2]<<13)|(a[2]>>19)) ^ a[3] ^ 0x9e3779b9u;
 for(unsigned i=4;i<n;i++) h=((h<<5)|(h>>27)) ^ args[i] ^ (i*0x9e3779b9u);
 return h;
}
'''
MAIN = r'''
#ifndef DT_ARG_WORDS
#define DT_ARG_WORDS 4
#endif
int main(int argc,char **argv) {
 u32 args[DT_ARG_WORDS];
 FILE *f=fopen(argv[1],"rb"); if(!f) return 74;
 if(fread(args,4,DT_ARG_WORDS,f)!=DT_ARG_WORDS || fread(high,1,sizeof high,f)!=sizeof high ||
    fread(low,1,sizeof low,f)!=sizeof low) return 75;
 fclose(f); u32 out=dt_entry(args); printf("R %08x\n",out); return 0;
}
'''


def stub(t, args):
    a, b, c, d = (list(args) + [0]*4)[:4]
    h = (t ^ a ^ ((b << 7) | (b >> 25)) ^ ((c << 13) | (c >> 19)) ^ d ^ 0x9e3779b9) & M32
    for i, value in enumerate(args[4:], 4):
        h = (((h << 5) | (h >> 27)) ^ value ^ (i * 0x9e3779b9)) & M32
    return h


class Oracle(sh2emu.CPU):
    def __init__(self, mem, entries=(), entry=None, call_specs=None, indirect_counts=()):
        super().__init__(mem)
        self.calls = []
        self.call_specs = call_specs
        self.indirect_counts = set(indirect_counts)
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
            target = self.pc & M32
            if 0x0604e000 <= canonical(target) < 0x0604f000:
                raise Unsupported('compiler runtime/nonstandard callee ABI')
            if self.call_specs is None:  # low-level harness tests use the legacy four-word default
                count = 4
            else:
                counts = set(self.indirect_counts)
                if canonical(target) in self.call_specs:
                    counts.add(self.call_specs[canonical(target)])
                if len(counts) != 1:
                    raise Unsupported('unknown/ambiguous callee argument count')
                count = counts.pop()
            args = self.r[4:4+min(count,4)]
            # SHC word ABI: fifth and later arguments start at the call-time SP.
            # Observe after the delay slot, before any callee prologue/spills.
            args += [self.mem.read((self.r[15]+4*i)&M32,4) for i in range(max(0,count-4))]
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


def reference(entry, args, high, low, noise, entries=(), call_specs=None, indirect_counts=()):
    mem = sh2emu.Memory()
    mem.add(RAM, len(high), high)
    mem.add(0x00200000, len(low), low)
    mem.log = []
    cpu = Oracle(mem, entries, entry, call_specs, indirect_counts)
    cpu.r = [noise] * 16
    cpu.r[4:8] = args[:4]
    for i,value in enumerate(args[4:]):
        mem.write(STACK+4*i,4,value)
    mem.log = []
    cpu.r[15] = STACK
    cpu.sr = noise & 0x3f3
    cpu.gbr = cpu.vbr = cpu.mach = cpu.macl = noise
    cpu.call(entry, max_steps=20000)
    if any(k == 'w' and 0x06004000 <= (a & 0x1fffffff) < 0x06091e20 for k,a,_,_ in mem.log):
        raise Unsupported('writes to original code/rodata image')
    return cpu.r[0], writes(mem.log), cpu.calls, write_trace(mem.log)


def difference(ref, got, check_r0=True):
    if check_r0 and (ref[0] & M32) != (got[0] & M32):
        return {'kind':'r0', 'reference':f'{ref[0] & M32:08x}', 'candidate':f'{got[0] & M32:08x}'}
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
        result['source_sha256'] = hashlib.sha256(source.encode()).hexdigest()
        # Known compiler helpers use r0/r1, never infer their ABI from C names.
        if 0x0604e000 <= entry < 0x0604f000:
            raise Unsupported('compiler runtime/nonstandard register ABI')
        generated, pointers = prepare(source, symbols, entry, result)
        with (ROOT / 'cart.bin').open('rb') as image:
            image.seek(0x200000)
            cart = image.read(0x8de20)
        if len(cart) != 0x8de20:
            raise Unsupported('truncated cart program image')
        with tempfile.TemporaryDirectory(prefix='difftest-', dir=None) as tmp:
            tmp = pathlib.Path(tmp)
            cfile, exe, state = tmp/'candidate.c', tmp/'candidate', tmp/'state.bin'
            cfile.write_text('#define DT_ARG_WORDS %d\n' % max(4,len(pointers)) + PRELUDE + HARNESS + generated + MAIN)
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
                args = [rng.choice(edge) if index < 10 else rng.getrandbits(32) for _ in range(max(4,len(pointers)))]
                for i, pointer in enumerate(pointers):
                    if pointer:
                        args[i] = SCRATCH + 0x1000 * i + rng.randrange(0,256,4)
                for i,value in enumerate(args[4:]):
                    offset = STACK-RAM+4*i
                    high[offset:offset+4] = value.to_bytes(4, 'big')
                result['vector'] = index
                result['args'] = [f'{a:08x}' for a in args]
                try:
                    ref = reference(entry, args, high, low, 0x13579bdf, symbols.values(), result['call_specs'], result.get('indirect_counts', []))
                    alternate = reference(entry, args, high, low, 0x2468ace0, symbols.values(), result['call_specs'], result.get('indirect_counts', []))
                except (sh2emu.MemoryError_, RuntimeError, ValueError, IndexError) as e:
                    raise Unsupported('reference cannot run: ' + str(e)) from e
                if difference(ref, alternate, result['r0_checked']):
                    raise Unsupported('depends on non-argument registers / undefined r0')
                # C only models addressable RAM, never host pointers or hardware.
                import struct
                state.write_bytes(struct.pack('=%dI' % len(args), *args) + high + low)
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
    ap.add_argument('--output-dir', type=pathlib.Path, help='directory for scan JSON and attempt logs (default: work)')
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
    output_dir = args.output_dir or ROOT/'work'
    output_dir.mkdir(parents=True, exist_ok=True)
    symbols = load_symbols()
    tasks = [(a,args.vectors,symbols,str(args.candidate) if args.candidate else None) for a in addresses]
    results = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for result in pool.map(test_one,tasks):
            results.append(result)
            attempt_dir = output_dir/'attempts'
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
    skip_histogram = dict(collections.Counter(r['reason'] for r in results if r['status'] == 'SKIP').most_common())
    report = {'version':VERSION, 'cart_sha256':hashlib.sha256((ROOT/'cart.bin').read_bytes()).hexdigest(), 'vectors':args.vectors, 'seed':'0xd1ff0000 XOR canonical entry',
              'model':'declared/inferred word ABI: r4-r7 then call-time SP; raw indirect targets; big-endian RAM; all byte-normalized stores and final footprint; private stack excluded',
              'counts':counts, 'skip_histogram':skip_histogram, 'results':results}
    destinations = set()
    if args.all:
        destinations.add(output_dir/'difftest_all.json')
    if args.json:
        destinations.add(args.json)
    for path in destinations:
        path.parent.mkdir(parents=True,exist_ok=True)
        path.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(counts,sort_keys=True))
    return 1 if counts.get('FAIL') else 0

if __name__ == '__main__':
    raise SystemExit(main())
