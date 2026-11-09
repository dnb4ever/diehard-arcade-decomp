#!/usr/bin/env python3
"""v1.04: bounded, reproducible SH-2 / host-C differential tests.

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

VERSION = '1.04'
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
TYPES['char'] = (1, True)  # SHC's plain char is signed.

RUNTIME = (0x0604e000, 0x0604f000)
STUB_OBJECTS = 0x00210000  # 64 KiB region for objects returned by pointer stubs
ROM_BASE = 0x02000000  # cart.bin offset 0; offset 0x200000 is the 0x06004000 image
PRIVATE = (STACK - 0x8000, STACK + 0x100)  # ABI-private stack window, never compared
FRAME = (STACK - 0x7f00, STACK - 0x100)    # candidate stack-backed locals
# Compiler-runtime helpers (divide: dividend r1, divisor r0; variable shift:
# value r0, count r1). C calls name them (dividend, divisor) / (value, count). The
# reference executes their original bytes inline, without a stub or call event;
# an explicit C call uses the host model below, which the regression tests check
# against those original bytes. Other runtime entries remain SKIP.
RUNTIME_MODELS = {0x0604e06c:'sdiv32', 0x0604e120:'udiv32', 0x0604e1c8:'umod32',
                  0x0604e360:'ashl_var', 0x0604e420:'ashr_var'}
HOST_WIDE = {  # 64-bit host values are exact; they never reach guest memory/ABI
   'long long', 'signed long long', 'long long int', 'unsigned long long',
               'unsigned long long int', 's64', 'u64'}


class Unsupported(Exception):
    pass


_IMAGE = []


def image_word(address):
    """Big-endian word of the immutable original program image at HWRAM address."""
    if not _IMAGE:
        with (ROOT / 'cart.bin').open('rb') as image:
            image.seek(0x200000)
            _IMAGE.append(image.read(0x8de20))
    at = address - 0x06004000
    return int.from_bytes(_IMAGE[0][at:at+4], 'big')


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
        command = ['gcc', '-E', '-P', '-undef', '-nostdinc', '-D__attribute__(x)=', '-I', str(tmp),
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


def guest_layout(t, aliases):
    """SH-2 word ABI layout, independent of the host's pointer/long widths."""
    t = resolve_type(t, aliases)
    if isinstance(t, (A.Struct, A.Union)):
        if t.decls is None:
            t = aliases.get(('union ' if isinstance(t,A.Union) else 'struct ') + str(t.name), t)
        if t.decls is None:
            raise Unsupported('incomplete guest aggregate')
        offset, alignment, fields = 0, 1, {}
        for field in t.decls:
            if field.bitsize or not field.name:
                raise Unsupported('guest bitfield/anonymous member')
            size, align, _ = guest_layout(field.type, aliases)
            alignment = max(alignment, align)
            at = 0 if isinstance(t,A.Union) else (offset+align-1)//align*align
            fields[field.name] = (at, field.type)
            offset = max(offset, at+size)
        return (offset+alignment-1)//alignment*alignment, alignment, fields
    if isinstance(t, A.ArrayDecl):
        if not isinstance(t.dim,A.Constant) or t.dim.type != 'int':
            raise Unsupported('variable/incomplete guest array layout')
        size, align, _ = guest_layout(t.type,aliases)
        return size*int(re.sub('[uUlL]+$','',t.dim.value),0), align, {}
    size, _ = type_size(t,aliases)
    return size, min(size,4), {}


class Generator(c_generator.CGenerator):
    """Instrument typed guest loads/stores and calls using the pycparser AST.

    Guest pointers stay 32-bit words. No guest dereference or function pointer
    reaches native execution; unsupported types and ambiguous ABIs remain SKIP.
    """
    def __init__(self, fn, globals_, symbols, signatures, metadata, aliases, vartypes, cursor=None):
        super().__init__()
        self.aliases = aliases
        self.vartypes = dict(vartypes)
        self.call_specs = {}
        self.inferred = {}
        self.indirect_counts = set()
        self.globals = dict(globals_)
        self.symbols = symbols
        self.signatures = signatures
        self.locals = set()
        self.fn = fn
        self.calls = set()
        self.used_globals = set()
        self.temporary = 0
        self.helpers = set()
        self.narrow_args = {}
        self.frame = {}         # stack-backed local name -> guest address
        self.frame_decls = set()
        local_decls, taken = [], set()
        class Locals(A.NodeVisitor):
            def visit_Decl(inner, n):
                if n is fn.decl:
                    for param in fn.decl.type.args.params if fn.decl.type.args else []:
                        if isinstance(param, A.Decl):
                            inner.visit(param)
                    return
                if isinstance(n.type,A.FuncDecl):
                    self.signatures[n.name] = n.type
                    return
                if 'extern' in n.storage:
                    # Block-scope extern: the same object as a file-scope global.
                    self.vartypes[n.name] = n.type
                    match = re.fullmatch(r'g_([0-9a-fA-F]{8})', n.name or '')
                    if match:
                        try:
                            self.globals[n.name] = ('0x' + match[1],) + type_size(n.type, aliases)
                        except Unsupported:
                            pass
                    return
                local_decls.append(n)
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

            def visit_UnaryOp(inner, n):
                if n.op == '&' and isinstance(n.expr, A.ID):
                    taken.add(n.expr.name)
                inner.generic_visit(n)
        Locals().visit(fn)
        params = {p.name for p in (fn.decl.type.args.params if fn.decl.type.args else [])
                  if isinstance(p, A.Decl)}
        # Local arrays, aggregates and address-taken scalars live in guest memory
        # inside the private stack window, so every access is instrumented and
        # no write there is externally observable on either side.
        cursor = cursor if cursor is not None else [FRAME[0]]  # shared by helpers
        for n in local_decls:
            if n.name in params:
                continue
            t = self.resolved(n.type)
            if not (isinstance(t, (A.ArrayDecl, A.Struct, A.Union)) or n.name in taken):
                continue
            if n.name in self.frame:
                raise Unsupported('repeated stack-backed local')
            if isinstance(t, A.ArrayDecl) and t.dim is None and isinstance(n.init, A.InitList):
                t.dim = A.Constant('int', str(len(n.init.exprs)))
            size, align, _ = guest_layout(n.type, aliases)
            at = (cursor[0] + 3) // 4 * 4
            if at + size > FRAME[1]:
                raise Unsupported('stack-backed locals exceed the private frame window')
            self.frame[n.name] = at
            self.frame_decls.add(id(n))
            cursor[0] = at + size

    def resolved(self, t):
        return resolve_type(t, self.aliases) if t is not None else None

    def expr_type(self, n):
        if isinstance(n,A.StructRef):
            t = self.resolved(self.expr_type(n.name))
            if n.type == '->' and isinstance(t,A.PtrDecl):
                t = t.type
            fields = guest_layout(t,self.aliases)[2]
            if n.field.name not in fields:
                raise Unsupported('unknown guest struct field')
            return fields[n.field.name][1]
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
            word = A.TypeDecl(name,[],None,A.IdentifierType(['u32']))
            return super()._generate_type(word,modifiers,emit_declname,emit_base_type)
        return super()._generate_type(n, modifiers, emit_declname, emit_base_type)

    def pointer_stride(self, t):
        t = self.resolved(t)
        if not isinstance(t, (A.PtrDecl, A.ArrayDecl)):
            raise Unsupported('untyped guest pointer')
        return guest_layout(t.type, self.aliases)[0]

    def memory_size(self, t):
        if isinstance(self.resolved(t),(A.Struct,A.Union,A.ArrayDecl)):
            return guest_layout(t,self.aliases)[0],False
        return type_size(t,self.aliases)

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

    def frame_name(self, name):
        return 'dt_frame_' + name

    def memory(self, n):
        if isinstance(n, A.ID) and n.name in self.frame and not isinstance(
                self.resolved(self.vartypes[n.name]), A.ArrayDecl):
            size, signed = self.memory_size(self.vartypes[n.name])
            return self.frame_name(n.name), size, signed
        if isinstance(n,A.StructRef):
            t = self.resolved(self.expr_type(n.name))
            if n.type == '->' and isinstance(t,A.PtrDecl):
                base, t = self.visit(n.name), t.type
            elif n.type == '.':
                parent = self.memory(n.name)
                if parent is None:
                    raise Unsupported('local aggregate access')
                base = parent[0]
            else:
                raise Unsupported('untyped guest struct pointer')
            fields = guest_layout(t,self.aliases)[2]
            if n.field.name not in fields:
                raise Unsupported('unknown guest struct field')
            offset, field = fields[n.field.name]
            size, signed = self.memory_size(field)
            return '((u32)(%s)+%d)' % (base,offset), size, signed
        if isinstance(n, A.UnaryOp) and n.op == '*':
            t = self.resolved(self.expr_type(n.expr))
            if not isinstance(t, A.PtrDecl):
                raise Unsupported('untyped raw pointer dereference')
            size, signed = self.memory_size(t.type)
            return self.visit(n.expr), size, signed
        if isinstance(n, A.ArrayRef):
            size, signed = self.memory_size(self.expr_type(n))
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
            self.used_globals.add(n.name)
            return self.globals[n.name]
        return None

    def load(self, mem):
        addr, size, signed = mem
        if size not in (1,2,4):
            raise Unsupported('non-scalar guest load')
        cast = ('s' if signed else 'u') + str(size * 8)
        return '((%s)dt_read((u32)(%s),%d))' % (cast, addr, size)

    def visit_ID(self, n):
        if n.name in self.frame and isinstance(self.resolved(self.vartypes[n.name]), A.ArrayDecl):
            return self.frame_name(n.name)  # array decays to its guest address
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
        if name in self.helpers and name not in self.locals:
            # A static helper without a known address is part of this candidate:
            # it is generated with the same instrumentation and runs natively.
            return '%s(%s)' % (name, ','.join('(%s)' % self.visit(a) for a in args))
        callee_type = self.resolved(self.expr_type(n.name))
        indirect = isinstance(callee_type, A.PtrDecl) or (isinstance(callee_type, A.FuncDecl) and (name is None or name in self.locals))
        if indirect:
            signature = self.resolved(callee_type.type) if isinstance(callee_type, A.PtrDecl) else callee_type
            if not isinstance(signature, A.FuncDecl):
                raise Unsupported('indirect call lacks function-pointer type')
            cell = n.name
            if isinstance(cell, A.UnaryOp) and cell.op == '*':
                inner = self.resolved(self.expr_type(cell.expr))
                # (*fp)(...) calls fp itself; (*(fn **)cell)(...) loads the pointer.
                if isinstance(inner, A.PtrDecl) and isinstance(self.resolved(inner.type), A.FuncDecl):
                    cell = cell.expr
            target_expr = self.visit(cell)
            fixed = self.image_pointer(cell)
            if fixed is not None:
                # The pointer cell lies in the immutable original image, so its
                # target is known before execution and gets a direct arity.
                if fixed in self.call_specs and self.call_specs[fixed] != len(args):
                    raise Unsupported('conflicting callee argument counts for target')
                self.call_specs[fixed] = len(args)
                target = fixed
            else:
                self.indirect_counts.add(len(args))
                target = None
            label = 'indirect@' + str(n.coord)
        else:
            if name is None:
                raise Unsupported('indirect call lacks function-pointer type')
            target = function_address(name, self.symbols)
            if target is None:
                raise Unsupported('unresolved callee: ' + name)
            signature = self.signatures.get(name)
            if RUNTIME[0] <= canonical(target) < RUNTIME[1]:
                return self.runtime_call(canonical(target), signature, args)
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
        ret = self.return_cast(signature)
        values = []
        for i, a in enumerate(args):
            value = self.visit(a)
            size = type_size(params[i].type, self.aliases) if params else (4, False)
            if size[0] != 4:
                # SHC callees extend narrow parameters themselves; the C value is
                # converted, and an upper-bit-only mismatch is reported as SKIP.
                value = '(%s%d)(%s)' % ('s' if size[1] else 'u', size[0]*8, value)
                self.narrow_args.setdefault(canonical(target) if target is not None else None, {})[i] = size[0]
            values.append('(u32)(%s)' % value)
        call = 'dt_callv((u32)(%s),%d,(u32[]){%s})' % (target_expr, len(args), ','.join(values) or '0')
        return '((%s)%s)' % (ret,call)

    def return_cast(self, signature):
        ret_type = self.resolved(signature.type) if signature is not None else A.IdentifierType(['int'])
        if isinstance(ret_type, A.PtrDecl):
            return 'u32'
        ret = typename(ret_type)
        if ret != 'void' and TYPES.get(ret, (0,False))[0] != 4:
            raise Unsupported('callee prototype has unsupported return type')
        return ret

    def runtime_call(self, target, signature, args):
        model = RUNTIME_MODELS.get(target)
        params = word_params(signature, self.aliases) if signature is not None else None
        if model is None or len(args) != 2 or (params is not None and any(
                type_size(p.type, self.aliases)[0] != 4 for p in params)):
            raise Unsupported('compiler runtime/nonstandard callee ABI')
        return '((%s)dt_rt_%s((u32)(%s),(u32)(%s)))' % (self.return_cast(signature), model,
                                                       self.visit(args[0]), self.visit(args[1]))

    def image_pointer(self, cell):
        if not (isinstance(cell, A.ID) and cell.name in self.globals and cell.name not in self.locals):
            return None
        address, size, _ = self.globals[cell.name]
        address = int(address, 16)
        if size != 4 or not 0x06004000 <= address <= 0x06091e1c:
            return None
        return canonical(image_word(address))

    def visit_Assignment(self, n):
        mem = self.memory(n.lvalue)
        if not mem:
            t = self.resolved(self.expr_type(n.lvalue))
            if isinstance(t, A.PtrDecl) and n.op in ('+=', '-='):
                return '(%s %s %d*(u32)(%s))' % (self.visit(n.lvalue),n.op,self.pointer_stride(t),self.visit(n.rvalue))
            return super().visit_Assignment(n)
        addr, size, signed = mem
        if isinstance(self.resolved(self.expr_type(n.lvalue)),(A.Struct,A.Union,A.ArrayDecl)):
            raise Unsupported('aggregate value store')
        if n.op != '=':
            return self.update_memory(mem, n.op[:-1], n.rvalue, self.expr_type(n.lvalue))
        return '((%s)dt_write((u32)(%s),%d,(u32)(%s)))' % (('s' if signed else 'u') + str(size*8), addr, size, self.visit(n.rvalue))

    def update_memory(self, mem, op, rhs, value_type=None, postfix=False):
        addr, size, signed = mem
        if size not in (1,2,4) or isinstance(self.resolved(value_type),(A.Struct,A.Union,A.ArrayDecl)):
            raise Unsupported('aggregate memory update')
        self.temporary += 1
        prefix = 'dt_tmp_%d_' % self.temporary
        while any(name.startswith(prefix) for name in self.locals):
            prefix += '_'
        a, old, new = (prefix+x for x in ('addr','old','new'))
        cast = ('s' if signed else 'u') + str(size*8)
        value = self.visit(rhs) if rhs is not None else '1'
        if isinstance(self.resolved(value_type),A.PtrDecl) and op in ('+','-'):
            value = '%d*(u32)(%s)' % (self.pointer_stride(value_type),value)
        # A statement expression gives nested assignments independent temporaries
        # and evaluates a side-effecting lvalue address exactly once.
        return ('({u32 %s=(u32)(%s); %s %s=%s; %s %s=(%s)(%s %s (%s)); '
                'dt_write(%s,%d,(u32)%s); %s;})' %
                (a,addr,cast,old,self.load((a,size,signed)),cast,new,cast,old,op,value,
                 a,size,new,old if postfix else new))

    def visit_UnaryOp(self, n):
        if n.op == '*':
            if isinstance(self.resolved(self.expr_type(n)),(A.Struct,A.Union,A.ArrayDecl)):
                raise Unsupported('aggregate value load')
            return self.load(self.memory(n))
        if n.op == '&':
            if isinstance(n.expr, A.UnaryOp) and n.expr.op == '*':
                return self.visit(n.expr.expr)
            if isinstance(n.expr, A.ArrayRef):
                return self.array_address(n.expr)
            if isinstance(n.expr,A.StructRef):
                return self.memory(n.expr)[0]
            if isinstance(n.expr, A.ID) and n.expr.name in self.frame:
                return self.frame_name(n.expr.name)
            if isinstance(n.expr, A.ID) and n.expr.name not in self.locals:
                mem = self.memory(n.expr)
                if mem:
                    return mem[0]
                if n.expr.name in self.signatures:
                    return self.visit(n.expr)
            raise Unsupported('local/unsupported address taking')
        if n.op == 'sizeof':
            t = n.expr.type if isinstance(n.expr,A.Typename) else self.expr_type(n.expr)
            if t is None:
                raise Unsupported('untyped sizeof expression')
            return str(guest_layout(t,self.aliases)[0])
        if n.op in ('p++','p--','++','--'):
            mem = self.memory(n.expr)
            if mem:
                return self.update_memory(mem,'+' if '+' in n.op else '-',None,
                                          self.expr_type(n.expr),n.op.startswith('p'))
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
        # long long suffixes have the same width on host and SHC; plain L does not.
        if n.type in ('float', 'double', 'string') or re.search(r'[lL]', re.sub(r'(?:ll|LL)[uU]?$', '', n.value)):
            raise Unsupported('floating/string/wide constant')
        return super().visit_Constant(n)

    def visit_ArrayRef(self, n):
        if isinstance(self.resolved(self.expr_type(n)),(A.Struct,A.Union)):
            raise Unsupported('aggregate value load')
        return self.load(self.memory(n))

    def visit_CompoundLiteral(self, n):
        raise Unsupported('local aggregate literal')

    def visit_StructRef(self, n):
        t = self.resolved(self.expr_type(n))
        if isinstance(t,A.ArrayDecl):
            return self.memory(n)[0]
        if isinstance(t,(A.Struct,A.Union)):
            raise Unsupported('aggregate value load')
        return self.load(self.memory(n))

    def visit_Decl(self, n, no_type=False):
        if isinstance(n.type,A.FuncDecl) and n is not self.fn.decl:
            return ''  # Calls use the retained declaration's ABI and symbol.
        if 'extern' in n.storage and n is not self.fn.decl:
            return ''  # Block-scope extern refers to the global object.
        if id(n) in self.frame_decls:
            if no_type or 'static' in n.storage:
                raise Unsupported('static/multi-declarator stack-backed local')
            writes = self.frame_init(self.frame[n.name], n.type, n.init)
            address = '0x%x' % self.frame[n.name]
            return 'const u32 %s = %s' % (self.frame_name(n.name),
                                          '(%s, %s)' % (', '.join(writes), address) if writes else address)
        if isinstance(n.type, A.ArrayDecl):
            raise Unsupported('local array/function declaration')
        if ('static' in n.storage and n is not self.fn.decl) or n.bitsize:
            raise Unsupported('static local or bitfield')
        return super().visit_Decl(n, no_type)

    def frame_init(self, address, t, init):
        """Instrumented stores that initialize a stack-backed local (C zero-fills
        array elements without an initializer). Uninitialized objects get none."""
        if init is None:
            return []
        out = []
        def emit(address, t, init):
            t = self.resolved(t)
            if isinstance(t, A.ArrayDecl):
                if not isinstance(init, (A.InitList, type(None))):
                    raise Unsupported('local array initializer')
                stride = guest_layout(t.type, self.aliases)[0]
                count = guest_layout(t, self.aliases)[0] // stride
                items = init.exprs if init is not None else []
                if len(items) > count:
                    raise Unsupported('local array initializer')
                for k in range(count):
                    emit(address + k*stride, t.type, items[k] if k < len(items) else None)
                return
            if isinstance(t, (A.Struct, A.Union)):
                raise Unsupported('local aggregate initializer')
            if isinstance(init, A.InitList) and len(init.exprs) == 1:
                init = init.exprs[0]
            if isinstance(init, (A.InitList, A.NamedInitializer)):
                raise Unsupported('local array initializer')
            if len(out) >= 1024:
                raise Unsupported('local initializer too large')
            size, _ = type_size(t, self.aliases)
            out.append('dt_write(0x%x,%d,(u32)(%s))' % (address, size, '0' if init is None else self.visit(init)))
        emit(address, t, init)
        return out

    def visit_IdentifierType(self, n):
        resolved = self.resolved(n)
        name = typename(resolved)
        if name not in TYPES and name != 'void' and name not in HOST_WIDE:
            raise Unsupported('unsupported host type: ' + name)
        return name


def is_void_type(t, aliases):
    t = resolve_type(t.type if isinstance(t, A.Typename) else t, aliases)
    return isinstance(t, A.IdentifierType) and t.names == ['void']


def word_params(signature, aliases):
    """None means a missing/old-style prototype, [] means explicit void."""
    if signature.args is None:
        return None
    params = signature.args.params
    if len(params) == 1 and is_void_type(params[0], aliases):
        return []
    for p in params:
        if not isinstance(p, (A.Decl, A.Typename)):
            raise Unsupported('variadic/non-word callee ABI')
        if type_size(p.type, aliases)[0] not in (1, 2, 4):
            raise Unsupported('callee prototype has unsupported parameter type')
    return params


def prepare(source, symbols, entry=None, metadata=None):
    tree = parse(source)
    aliases = {n.name:n.type for n in tree.ext if isinstance(n, A.Typedef)}
    class Aggregates(A.NodeVisitor):
        def visit_Pragma(self,n):
            if 'pack' in n.string:
                raise Unsupported('packed guest aggregate layout')
        def visit_Struct(self,n):
            if n.name and n.decls is not None:
                aliases['struct '+n.name] = n
            self.generic_visit(n)
        def visit_Union(self,n):
            if n.name and n.decls is not None:
                aliases['union '+n.name] = n
            self.generic_visit(n)
    Aggregates().visit(tree)
    vartypes = {n.name:n.type for n in tree.ext if isinstance(n, A.Decl)}
    fn = select_function(tree, symbols, entry)
    return_type = resolve_type(fn.decl.type.type,aliases)
    is_pointer_return = isinstance(return_type,A.PtrDecl)
    is_void = not is_pointer_return and typename(return_type) == 'void'
    if not is_void and not is_pointer_return and typename(return_type) not in TYPES:
        raise Unsupported('void/non-integer return: r0 has no faithful C representation')
    if metadata is not None:
        metadata['r0_checked'] = not is_void
    params = fn.decl.type.args.params if fn.decl.type.args else []
    if len(params) == 1 and is_void_type(params[0], aliases):
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
    cursor = [FRAME[0]]
    gen = Generator(fn, globals_, symbols, signatures, metadata, aliases, vartypes, cursor)
    helpers = [n for n in tree.ext if isinstance(n, A.FuncDef) and n is not fn
               and function_address(n.decl.name, symbols) is None]
    gen.helpers = {h.decl.name for h in helpers}
    prototypes, bodies, helper_gens = [], [], []
    for h in helpers:
        hgen = Generator(h, globals_, symbols, signatures, metadata, aliases, vartypes, cursor)
        helper_gens.append(hgen)
        for shared in ('call_specs', 'inferred', 'indirect_counts', 'calls', 'used_globals',
                       'helpers', 'narrow_args'):
            setattr(hgen, shared, getattr(gen, shared))
        prototypes.append(hgen.visit(h.decl) + ';\n')
        bodies.append(hgen.visit(h) + '\n')
    generated = ''.join(prototypes) + ''.join(bodies) + gen.visit(fn)
    if metadata is not None:
        metadata['call_specs'] = gen.call_specs
        metadata['indirect_counts'] = sorted(gen.indirect_counts)
        metadata['inferred_prototypes'] = dict(sorted(gen.inferred.items()))
        metadata['narrow_args'] = gen.narrow_args
        if helpers:
            metadata['native_helpers'] = sorted(gen.helpers)
        metadata['stack_locals'] = sorted(set(gen.frame).union(*(g.frame for g in helper_gens)))
    casts = []
    pointers = []
    narrow = {}
    for p in params:
        pointer = isinstance(resolve_type(p.type, aliases), A.PtrDecl)
        pointers.append(pointer)
        if pointer:
            cast = 'u32'
        else:
            cast = typename(resolve_type(p.type, aliases))
            if cast not in TYPES:
                raise Unsupported('non-word parameter ABI')
            if TYPES[cast][0] != 4:
                # Inputs are ABI-valid extended words for narrow parameters.
                narrow[len(casts)] = TYPES[cast]
        casts.append('(%s)(uintptr_t)args[%d]' % (cast, len(casts)))
    wrapper = '\nu32 dt_entry(u32 *args) { return (u32)%s(%s); }\n' % (fn.decl.name, ','.join(casts))
    if is_void:
        wrapper = '\nu32 dt_entry(u32 *args) { %s(%s); return 0; }\n' % (fn.decl.name, ','.join(casts))
    if metadata is not None:
        metadata['entry_arg_count'] = len(params)
        metadata['narrow_params'] = narrow
        metadata['typed_pointer_globals'] = sorted(
            int(mem[0],16) for name,mem in globals_.items() if name in gen.used_globals
            if isinstance(resolve_type(vartypes[name],aliases),A.PtrDecl)
            and not isinstance(resolve_type(resolve_type(vartypes[name],aliases).type,aliases),A.FuncDecl))
    return generated + wrapper, pointers


HARNESS = r'''
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
static unsigned char high[0x100000], low[0x100000];
static const unsigned char *rom; static uint64_t romlen;  /* read-only cart ROM */
#ifndef DT_PTR_RET
#define DT_PTR_RET 0
#endif
#ifndef DT_FRAME
#define DT_FRAME 0
#endif
static const u32 dt_ptr_ret[] = {DT_PTR_RET};
static unsigned events;
static void dt_event(void) { if(++events>100000) exit(76); }
static unsigned char *dt_ptr(u32 a, int n) {
 if((a>>28)==2) a &= 0x1fffffff;
 if(a>=0x06000000 && (uint64_t)a+n<=0x06100000) return high+a-0x06000000;
 if(a>=0x00200000 && (uint64_t)a+n<=0x00300000) return low+a-0x00200000;
 fprintf(stderr,"unmapped C memory %08x/%d\n",a,n); exit(73);
}
static u32 dt_read(u32 a,int n) {
 u32 c=(a>>28)==2 ? a&0x1fffffff : a;
 const unsigned char *p=(c>=0x02000000u && (uint64_t)c+n<=0x02000000u+romlen) ? rom+c-0x02000000u : dt_ptr(a,n);
 u32 v=0;
 for(int i=0;i<n;i++) v=(v<<8)|p[i]; return v;
}
static int dt_frame(u32 a) { if((a>>28)==2) a &= 0x1fffffff; return a>=0x060f7100u && a<0x060fef00u; }
static u32 dt_write(u32 a,int n,u32 v) {
 dt_event(); unsigned char *p=dt_ptr(a,n); u32 result=v;
 if(DT_FRAME && n==4 && dt_frame(v) && !dt_frame(a)) { fprintf(stderr,"stack-backed local address stored to guest memory\n"); exit(77); }
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
 u32 c=t; if((c>>28)==2) c&=0x1fffffff;
 if(c>=0x02200000u && c<0x0228de20u) c=c-0x02200000u+0x06004000u;
 for(unsigned i=0;i<sizeof dt_ptr_ret/sizeof *dt_ptr_ret;i++) if(dt_ptr_ret[i]==c) h=0x00210000u|(h&0xfffcu);
 return h;
}
'''
HARNESS += r'''
static u32 dt_rt_divzero(void) { dt_write(0x060e132cu,4,0x44eu); return 0; }
static u32 dt_rt_sdiv32(u32 a,u32 b) {
 if(!b) return dt_rt_divzero();
 if(b==0xffffffffu) return 0u-a;
 return (u32)((int32_t)a/(int32_t)b);
}
static u32 dt_rt_udiv32(u32 a,u32 b) { return b ? a/b : dt_rt_divzero(); }
static u32 dt_rt_umod32(u32 a,u32 b) { return b ? a%b : dt_rt_divzero(); }
static u32 dt_rt_ashl_var(u32 v,u32 c) { int32_t n=(int32_t)c; return n<0 ? v : n>=32 ? 0 : v<<n; }
static u32 dt_rt_ashr_var(u32 v,u32 c) {
 int32_t n=(int32_t)c, x=(int32_t)v; return n<0 ? v : (u32)(x >> (n>=32 ? 31 : n));
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
 fclose(f);
 if(argc>2) { int fd=open(argv[2],O_RDONLY); struct stat st;
  if(fd<0 || fstat(fd,&st)) return 74;
  rom=mmap(0,st.st_size,PROT_READ,MAP_PRIVATE,fd,0); if(rom==MAP_FAILED) return 74; romlen=st.st_size; }
 u32 out=dt_entry(args); printf("R %08x\n",out); return 0;
}
'''


def pointer_returns(constraints):
    """Callee targets whose result the original code uses as an address."""
    out = set()
    for expr in constraints:
        while expr[0] in ('add', 'load', 'loadn', 'index', 'count', 'idx'):
            expr = expr[1]
        if expr[0] == 'ret':
            out.add(expr[1])
    return sorted(out)


def stub(t, args, pointer_targets=()):
    a, b, c, d = (list(args) + [0]*4)[:4]
    h = (t ^ a ^ ((b << 7) | (b >> 25)) ^ ((c << 13) | (c >> 19)) ^ d ^ 0x9e3779b9) & M32
    for i, value in enumerate(args[4:], 4):
        h = (((h << 5) | (h >> 27)) ^ value ^ (i * 0x9e3779b9)) & M32
    if canonical(t) in pointer_targets:
        h = STUB_OBJECTS | (h & 0xfffc)  # mapped low RAM, see test_one
    return h


class Oracle(sh2emu.CPU):
    def __init__(self, mem, entries=(), entry=None, call_specs=None, indirect_counts=(), pointer_targets=()):
        super().__init__(mem)
        self.pointer_targets = pointer_targets
        self.calls = []
        self.call_specs = call_specs
        self.indirect_counts = set(indirect_counts)
        self.entries = set(entries)
        self.entry = entry
        # A computed jump inside this function's own body (up to the next known
        # entry) is a switch over original bytes, not a call boundary.
        self.limit = min((e for e in self.entries if entry is not None and e > entry), default=None)

    def step(self):
        ins = self.fetch(self.pc)
        if ins.op in ('rte', 'trapa', 'sleep'):
            raise Unsupported('system control flow')
        if RUNTIME[0] <= canonical(self.pc) < RUNTIME[1]:
            super().step()  # inside an inline runtime helper (original bytes)
            return
        call = ins.op in ('bsr', 'jsr', 'bsrf')
        computed = ins.op in ('jmp', 'braf')
        super().step()  # includes the delay slot, before observing arguments
        tail = ins.op in ('bra', 'jmp', 'braf') and self.pc in self.entries and self.pc != self.entry
        internal = self.limit is not None and self.entry < self.pc < self.limit
        if computed and not tail and not internal:
            raise Unsupported('computed tail/switch target not a known callee')
        if call or tail:
            target = self.pc & M32
            if RUNTIME[0] <= canonical(target) < RUNTIME[1]:
                if canonical(target) in RUNTIME_MODELS:
                    return  # execute the helper's original bytes; no event
                raise Unsupported('compiler runtime/nonstandard callee ABI')
            if self.call_specs is None:  # low-level harness tests use the legacy four-word default
                count = 4
            elif canonical(target) in self.call_specs:
                # A known direct target keeps its declared arity; if an indirect
                # call of another arity reaches it, the length mismatch is SKIP.
                count = self.call_specs[canonical(target)]
            else:
                counts = set(self.indirect_counts)
                if len(counts) != 1:
                    raise Unsupported('unknown/ambiguous callee argument count: %08x' % target)
                count = counts.pop()
            args = self.r[4:4+min(count,4)]
            # SHC word ABI: fifth and later arguments start at the call-time SP.
            # Observe after the delay slot, before any callee prologue/spills.
            args += [self.mem.read((self.r[15]+4*i)&M32,4) for i in range(max(0,count-4))]
            self.calls.append([target] + args)
            self.r[0] = stub(target, args, self.pointer_targets)
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
        if PRIVATE[0] <= addr < PRIVATE[1]:
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


_ROM = []


def cart_rom():
    """The complete immutable cart image, mapped read-only at 0x02000000."""
    if not _ROM:
        _ROM.append((ROOT / 'cart.bin').read_bytes())
    return _ROM[0]


class GuestMemory(sh2emu.Memory):
    """HWRAM, low RAM and the read-only cart ROM (A-bus CS0); nothing else."""
    def write(self, addr, size, value):
        a = addr & 0x1fffffff if (addr & M32) >> 28 == 2 else addr & M32
        if ROM_BASE <= a < ROM_BASE + len(cart_rom()):
            raise sh2emu.MemoryError_('write to read-only cart ROM at %08X' % (addr & M32))
        super().write(addr, size, value)


def reference(entry, args, high, low, noise, entries=(), call_specs=None, indirect_counts=(), pointer_targets=()):
    mem = GuestMemory()
    mem.add(RAM, len(high), high)
    mem.add(0x00200000, len(low), low)
    rom = cart_rom()
    mem.regions.append((ROM_BASE, ROM_BASE + len(rom), rom))  # shared, never copied
    mem.log = []
    cpu = Oracle(mem, entries, entry, call_specs, indirect_counts, pointer_targets)
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
    for line in (ROOT/'notes/symbols.txt').read_text().splitlines():
        match = re.match(r'\s*0x([0-9a-fA-F]{8})\s+([A-Za-z_]\w*)\s*(?:#|$)',line)
        if match:
            addr, name = canonical(int(match[1],16)), match[2]
            if name in out and out[name] != addr:
                ambiguous.add(name)
            out.setdefault(name,addr)
    return {name:addr for name,addr in out.items() if name not in ambiguous}


def infer_pointer_inputs(entry, cart, entries=()):
    """Bounded static SH-2 provenance analysis, never a trial reference run.

    Follow both branch successors, register copies, literal pools, constant
    offsets and loaded pointer chains. Unknown arithmetic loses provenance.
    The result constrains initial inputs only; it never maps a faulting access,
    changes instructions, repairs an executing state, or retries a vector.

    v1.04 also tags scaled/extended values and narrow loads used as an index
    with a pointer or constant base ('index' constraints: the source word gets a
    small value), and callee results used as addresses ('ret' roots: that
    target's stub returns a mapped address on both sides).
    """
    from saturnrecomp import sh2
    def decode(pc):
        at = pc-RAM-0x4000
        if at < 0 or at+2 > len(cart):
            return None
        return sh2.decode(int.from_bytes(cart[at:at+2],'big'),pc)
    def literal(address,size):
        at = address-RAM-0x4000
        if 0 <= at and at+size <= len(cart):
            value = int.from_bytes(cart[at:at+size],'big',signed=size<4)
            return ('const',value&M32)
        rom = (address & 0x1fffffff if address >> 28 == 2 else address) - ROM_BASE
        if 0 <= rom and rom+size <= len(cart_rom()):  # read-only cart ROM
            value = int.from_bytes(cart_rom()[rom:rom+size],'big',signed=size<4)
            return ('const',value&M32)
    def add(value, offset):
        if value is None:
            return None
        if not offset or value[0] in ('idx', 'loadn'):
            return value
        if value[0] == 'tbl':
            return ('tbl', (value[1]+offset)&M32, value[2])
        if value[0] == 'const':
            return ('const',(value[1]+offset)&M32)
        if value[0] == 'add':
            return add(value[1],value[2]+offset)
        return ('add',value,offset)
    required = set()
    def mark(value):
        if value is not None and value[0] not in ('const', 'idx', 'loadn', 'tbl'):
            required.add(value)
    def idx(value, shift=0):
        # ('idx', source, log2 scale or None): a small index derived from source.
        if value is not None and value[0] == 'add':
            value = value[1]
        if value is None:
            return None
        if value[0] == 'idx':
            scale = None if value[2] is None or shift is None else value[2] + shift
            return ('idx', value[1], scale)
        return ('idx', value, shift) if value[0] in ('arg', 'load', 'loadn') else None
    def is_index(value):
        return value is not None and value[0] in ('idx', 'loadn')
    def load(address,size):
        mark(address)
        if address is None:
            return None
        if address[0] == 'const':
            a = address[1]
            if size == 4 and STACK <= a < STACK+0x100 and (a-STACK)%4 == 0:
                return ('arg',4+(a-STACK)//4)
            value = literal(a,size)
            if value is not None:
                return value
        if size != 4:
            return ('loadn', address, size)
        if repr(address).count('load') >= 4:
            return None
        return ('load',address)
    def indexed(a,b):
        for base, index in ((a, b), (b, a)):
            if is_index(index) and not is_index(base):
                required.add(('index', index[1] if index[0] == 'idx' else index))
                if base is not None and base[0] == 'const':
                    # A constant table indexed by 0..7 (stride 1 << scale).
                    scale = index[2] if index[0] == 'idx' else 0
                    return ('tbl', base[1], scale) if scale is not None and scale <= 6 else None
                return base
        if a is not None and a[0] == 'const':
            return add(b,a[1])
        if b is not None and b[0] == 'const':
            return add(a,b[1])
        # An unknown index does not establish a field offset. Retain the root
        # only to choose a mapped argument; real out-of-bounds accesses SKIP.
        return a if a is not None else b
    def transfer(ins, regs):
        r = list(regs)
        op,f,n,m = ins.op,ins.fmt,ins.n,ins.m
        if op == 'mov':
            r[n] = ('const',ins.imm&M32) if ins.imm is not None else r[m]
        elif op == 'mova':
            r[0] = ('const',ins.target)
        elif op == 'add':
            if ins.imm is not None:
                r[n] = add(r[n],ins.imm)
            else:
                r[n] = idx(r[n], 1) if n == m else indexed(r[n],r[m])
        elif op in ('shll','shll2','shll8','shll16','shal'):
            r[n] = idx(r[n], {'shll':1,'shal':1,'shll2':2,'shll8':8,'shll16':16}[op])
        elif op in ('extu.b','extu.w','exts.b','exts.w'):
            r[n] = idx(r[m])
        elif op == 'and' and ins.imm is not None:
            r[0] = idx(r[0])
        elif op in ('mul.l','mulu.w','muls.w'):
            r[16] = idx(r[n], None) or idx(r[m], None)
        elif op == 'sts' and 'macl' in f and n is not None:
            r[n] = r[16]
        elif op in ('mov.b','mov.w','mov.l'):
            size = {'mov.b':1,'mov.w':2,'mov.l':4}[op]
            tail = f[len(op)+1:]
            if tail == '@T,Rn':
                r[n] = literal(ins.target,size)
            elif tail in ('@Rm,Rn','@Rm+,Rn','@(disp,Rm),Rn','@(disp,Rm),r0','@(r0,Rm),Rn'):
                address = r[m]
                if 'disp' in tail:
                    address = add(address,ins.disp)
                elif 'r0,Rm' in tail:
                    address = indexed(address,r[0])
                dest = 0 if tail.endswith(',r0') else n
                value = load(address,size)
                if tail == '@Rm+,Rn' and m != dest:
                    r[m] = add(r[m],size)
                r[dest] = value
            elif tail in ('Rm,@Rn','Rm,@-Rn','Rm,@(disp,Rn)','r0,@(disp,Rn)','Rm,@(r0,Rn)'):
                if '@-Rn' in tail:
                    r[n] = add(r[n],-size)
                address = r[n]
                if 'disp' in tail:
                    address = add(address,ins.disp)
                elif 'r0,Rn' in tail:
                    address = indexed(address,r[0])
                mark(address)
            elif n is not None:
                r[n] = None
        elif op in ('sts.l','stc.l') and n is not None:
            r[n] = add(r[n],-4)
        elif op in ('lds.l','ldc.l') and m is not None:
            r[m] = add(r[m],4)
        elif n is not None and not (op.startswith(('cmp','tst','bt','bf','bsr','bra','jsr','jmp','lds','ldc','mac'))):
            # Preserve no stale address provenance through arithmetic/results.
            r[n] = None
        return tuple(r)
    initial = [None]*17  # r0-r15 and macl
    initial[4:8] = [('arg',i) for i in range(4)]
    initial[15] = ('const',STACK)
    pending = collections.deque([(entry,tuple(initial))])
    seen = collections.defaultdict(set)
    entries = set(entries)
    limit = min((e for e in entries if e > entry), default=entry)
    def switch_targets(ins, value):
        # SHC switch: mova table; mov.w/mov.l @(r0,Rm); braf/jmp. Index 0..7
        # slots whose targets stay inside this function are analysed too.
        if value is None or value[0] not in ('load', 'loadn') or value[1][0] != 'tbl':
            return []
        _, base, scale = value[1]
        size = value[2] if value[0] == 'loadn' else 4
        out = []
        for k in range(8):
            word = literal((base + (k << scale)) & M32, size)
            if word is not None:
                target = (ins.pc + 4 + word[1]) & M32 if ins.op == 'braf' else canonical(word[1])
                if entry < target < limit:
                    out.append(target)
        return out
    steps = 0
    while pending and steps < 12000:
        pc,regs = pending.popleft()
        if regs in seen[pc] or len(seen[pc]) >= 8:
            continue
        seen[pc].add(regs)
        steps += 1
        ins = decode(pc)
        if ins is None:
            continue
        follow = decode(pc+2)
        if follow is not None and follow.op in ('bt','bf','bt/s','bf/s') and follow.target <= pc:
            # Loop bounds: a dt counter gets 1..8, an unsigned/signed compare
            # operand 0..7, so bounded loops finish within the step budget.
            if ins.op == 'dt':
                bound = idx(regs[ins.n])
                if bound is not None:
                    required.add(('count', bound[1]))
            elif ins.op in ('cmp/hs','cmp/hi','cmp/ge','cmp/gt') and ins.m is not None:
                for operand in (regs[ins.n], regs[ins.m]):
                    bound = idx(operand)
                    if bound is not None:
                        required.add(('index', bound[1]))
        callee = ins.target if ins.op == 'bsr' else None
        if ins.op in ('jsr', 'jmp', 'braf', 'bsrf'):
            callee = regs[ins.m if ins.m is not None else ins.n]
            if ins.op == 'bsrf' and callee is not None and callee[0] == 'const':
                callee = (pc + 4 + callee[1]) & M32
        regs = transfer(ins,regs)
        if ins.delay:
            slot = decode(pc+2)
            if slot is None or slot.delay:
                continue
            regs = transfer(slot,regs)
        if ins.op in ('jmp','braf'):
            pending.extend((target, regs) for target in switch_targets(ins, callee))
        if ins.op in ('rts','rte','jmp','braf','trapa','sleep'):
            continue
        if ins.op in ('jsr','bsr','bsrf'):
            regs = list(regs)
            # The isolated stub preserves all other registers.
            if isinstance(callee, int):
                regs[0] = ('ret', canonical(callee))
            elif callee is not None and callee[0] == 'const':
                regs[0] = ('ret', canonical(callee[1]))
            else:
                regs[0] = None
            pending.append((pc+4,tuple(regs)))
        elif ins.op == 'bra':
            if ins.target not in entries or ins.target == entry:
                pending.append((ins.target,regs))
        elif ins.op in ('bt','bf','bt/s','bf/s'):
            pending.extend([(ins.target,regs),(pc+(4 if ins.delay else 2),regs)])
        else:
            pending.append((pc+2,regs))
    return sorted(required,key=repr)


def structure_inputs(args, high, low, constraints, typed_globals=()):
    """Seed only statically inferred pointer slots in mutable initial RAM.

    All edits happen before either side runs. Scratch data remains randomized;
    only words known to be used as pointers acquire mapped values. No hardware
    or code/rodata is made writable/mapped by this generator.
    """
    allocations, edits = {}, []
    def location(address):
        address = address & 0x1fffffff if address >> 28 == 2 else address
        if RAM <= address <= RAM+len(high)-4:
            if (address < 0x06091e20 and address+4 > 0x06004000) or (address < STACK+0x100 and address+4 > STACK-0x8000):
                return None
            return high,address-RAM
        if 0x00200000 <= address <= 0x00200000+len(low)-4:
            return low,address-0x00200000
    def allocate(key):
        if key not in allocations:
            if len(allocations) >= 80:
                return None
            allocations[key] = 0x002a0400+0x1000*len(allocations)
        return allocations[key]
    def value(expr):
        if expr is None:
            return None
        kind = expr[0]
        if kind == 'const':
            return expr[1]
        if kind == 'arg':
            return args[expr[1]] if expr[1] < len(args) else None
        if kind == 'add':
            base = value(expr[1])
            return (base+expr[2])&M32 if base is not None else None
        if kind == 'load':
            address = value(expr[1])
            loc = location(address) if address is not None else None
            if loc:
                buf,at = loc
                return int.from_bytes(buf[at:at+4],'big')
            # Immutable literal pools can point to mutable RAM.
            if address is not None and RAM <= address <= RAM+len(high)-4:
                return int.from_bytes(high[address-RAM:address-RAM+4],'big')
    def values(expr):
        # Every concrete value an expression can take; a table yields its slots.
        if expr is not None and expr[0] == 'tbl':
            return [(expr[1] + (k << expr[2])) & M32 for k in range(8)]
        if expr is not None and expr[0] == 'add':
            return [(v + expr[2]) & M32 for v in values(expr[1])]
        if expr is not None and expr[0] == 'load' and repr(expr).count('tbl'):
            out = []
            for address in values(expr[1])[:64]:
                loc = location(address)
                if loc:
                    out.append(int.from_bytes(loc[0][loc[1]:loc[1]+4],'big'))
            return out
        single = value(expr)
        return [] if single is None else [single]
    def ensure(expr):
        if expr[0] in ('ret', 'idx', 'loadn', 'index', 'count', 'tbl'):
            return  # callee results are mapped by the stub, not by input setup
        if expr[0] == 'add':
            ensure(expr[1])
        elif expr[0] == 'arg' and expr[1] < len(args):
            i = expr[1]
            # Preserve existing typed-pointer addresses when already mapped.
            if location(args[i]) is None:
                args[i] = SCRATCH+0x1000*i+0x400
                edits.append({'argument':i,'value':f'{args[i]:08x}'})
        elif expr[0] == 'load':
            ensure(expr[1])
            # A pointer table indexed by a bounded index seeds all 8 slots.
            for address in dict.fromkeys(values(expr[1])):
                loc = location(address) if address is not None else None
                if loc and address not in allocations:
                    buf,at = loc
                    pointer = allocate(address)
                    if pointer is not None:
                        buf[at:at+4] = pointer.to_bytes(4,'big')
                        edits.append({'address':f'{address:08x}','value':f'{pointer:08x}'})
    def bound(expr, low=0):
        # A word/halfword/byte used as a scaled index or loop bound gets a small
        # value (its own random bits masked to 0..7, plus 1 for a dt counter),
        # unless it is a pointer slot.
        if expr[0] == 'arg' and expr[1] < len(args):
            if location(args[expr[1]]) is None:
                args[expr[1]] = (args[expr[1]] & 7) + low
                edits.append({'bounded_argument':expr[1],'value':f'{args[expr[1]]:08x}'})
        elif expr[0] in ('load', 'loadn'):
            size = expr[2] if expr[0] == 'loadn' else 4
            address = value(expr[1])
            loc = location(address) if address is not None else None
            if loc and address not in allocations:
                buf,at = loc
                small = (int.from_bytes(buf[at:at+size],'big') & 7) + low
                buf[at:at+size] = small.to_bytes(size,'big')
                edits.append({'bounded_address':f'{address:08x}','value':f'{small:x}'})
    for address in typed_globals:
        ensure(('load',('const',address)))
    for expr in constraints:
        if expr[0] not in ('index', 'count', 'ret'):
            ensure(expr)
    for expr in constraints:
        if expr[0] in ('index', 'count'):
            bound(expr[1], 1 if expr[0] == 'count' else 0)
    return edits


def extend(value, size, signed):
    mask = (1 << (8*size)) - 1
    value &= mask
    if signed and value >> (8*size - 1):
        value |= M32 ^ mask
    return value


def private_argument(calls):
    return any(PRIVATE[0] <= (v & 0x1fffffff if v >> 28 == 2 else v) < PRIVATE[1]
               for event in calls for v in event[1:])


def abi_call_mismatch(diff, result):
    """Turn two ABI-ambiguous call mismatches into SKIP; never into PASS.

    Same target, different word counts: an indirect call of another arity
    reached a direct callee. Same target, differences confined to the upper bits
    of narrow declared parameters: SHC callees extend those themselves.
    """
    if not diff or diff['kind'] != 'calls' or not diff['reference'] or not diff['candidate']:
        return
    ref, got = diff['reference'][0], diff['candidate'][0]
    if ref[0] != got[0]:
        return
    if len(ref) != len(got):
        raise Unsupported('unknown/ambiguous callee argument count: %08x' % ref[0])
    target = canonical(ref[0])
    narrow = dict(result.get('narrow_args', {}).get(target, {}))
    if target not in result.get('call_specs', {}):
        narrow.update(result.get('narrow_args', {}).get(None, {}))
    differing = [i for i in range(1, len(ref)) if ref[i] != got[i]]
    if differing and all(i-1 in narrow and extend(ref[i], narrow[i-1], False) == extend(got[i], narrow[i-1], False)
                         for i in differing):
        raise Unsupported('narrow callee argument differs only in ABI upper bits: %08x' % ref[0])


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
        constraints = infer_pointer_inputs(entry,cart,symbols.values())
        result['pointer_returns'] = [f'{t:08x}' for t in pointer_returns(constraints)]
        targets = pointer_returns(constraints)
        result['pointer_constraints'] = [repr(expr) for expr in constraints]
        with tempfile.TemporaryDirectory(prefix='difftest-', dir=None) as tmp:
            tmp = pathlib.Path(tmp)
            cfile, exe, state = tmp/'candidate.c', tmp/'candidate', tmp/'state.bin'
            table = '#define DT_PTR_RET %s\n#define DT_FRAME %d\n' % (','.join(['0'] + ['0x%x' % t for t in targets]), bool(result.get('stack_locals')))
            cfile.write_text('#define DT_ARG_WORDS %d\n' % max(4,len(pointers)) + table + PRELUDE + HARNESS + generated + MAIN)
            proc = subprocess.run(['gcc','-std=c11','-O1','-Wall','-Werror=uninitialized','-Werror=maybe-uninitialized','-fwrapv','-fno-strict-aliasing',
                                   '-Werror=implicit-function-declaration','-fsanitize=shift-exponent,integer-divide-by-zero,bounds','-fno-sanitize-recover=all',str(cfile),'-o',str(exe)],
                                  capture_output=True, text=True, timeout=20)
            if proc.returncode:
                raise Unsupported('host compile: ' + proc.stderr.splitlines()[0])
            rng = random.Random(0xd1ff0000 ^ entry)
            for index in range(vectors):
                high = bytearray(rng.randbytes(0x100000))
                low = bytearray(rng.randbytes(0x100000))
                high[0x4000:0x91e20] = cart
                # Mix edge values, bounded counts and full words reproducibly.
                edge = [0,1,2,7,15,31,32,0x7fffffff,0x80000000,0xffffffff]
                args = [rng.choice(edge) if index < 10 else rng.getrandbits(32) for _ in range(max(4,len(pointers)))]
                for i, pointer in enumerate(pointers):
                    if pointer:
                        args[i] = SCRATCH + 0x1000 * i + rng.randrange(0,256,4)
                for i, (size, signed) in result['narrow_params'].items():
                    args[i] = extend(args[i], size, signed)
                result['input_setup'] = structure_inputs(args,high,low,constraints,result['typed_pointer_globals'])
                if targets:
                    # Objects returned by pointer-valued stubs: every word of
                    # their region points back into it, so fields and nested
                    # links stay mapped (identically on both sides).
                    region = low[STUB_OBJECTS-0x00200000:STUB_OBJECTS-0x00200000+0x10000]
                    for at in range(0, 0x10000, 4):
                        word = STUB_OBJECTS | (int.from_bytes(region[at:at+4],'big') & 0xfffc)
                        region[at:at+4] = word.to_bytes(4,'big')
                    low[STUB_OBJECTS-0x00200000:STUB_OBJECTS-0x00200000+0x10000] = region
                    result['input_setup'].append({'stub_object_region':f'{STUB_OBJECTS:08x}'})
                for i,value in enumerate(args[4:]):
                    offset = STACK-RAM+4*i
                    high[offset:offset+4] = value.to_bytes(4, 'big')
                result['vector'] = index
                result['args'] = [f'{a:08x}' for a in args]
                try:
                    ref = reference(entry, args, high, low, 0x13579bdf, symbols.values(), result['call_specs'], result.get('indirect_counts', []), targets)
                    alternate = reference(entry, args, high, low, 0x2468ace0, symbols.values(), result['call_specs'], result.get('indirect_counts', []), targets)
                except (sh2emu.MemoryError_, RuntimeError, ValueError, IndexError) as e:
                    raise Unsupported('reference cannot run: ' + str(e)) from e
                if difference(ref, alternate, result['r0_checked']):
                    raise Unsupported('depends on non-argument registers / undefined r0')
                # C only models addressable RAM, never host pointers or hardware.
                import struct
                state.write_bytes(struct.pack('=%dI' % len(args), *args) + high + low)
                try:
                    proc = subprocess.run([str(exe),str(state),str(ROOT/'cart.bin')], capture_output=True, text=True, timeout=2)
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
                if private_argument(ref[2]) or private_argument(calls):
                    # A callee may fill a caller frame; the stub model cannot.
                    raise Unsupported('private stack address passed to callee')
                diff = difference(ref, (ret, writes(log), calls, write_trace(log)), result['r0_checked'])
                abi_call_mismatch(diff, result)
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
    report = {'version':VERSION, 'cart_sha256':hashlib.sha256((ROOT/'cart.bin').read_bytes()).hexdigest(),
              'harness_sha256':hashlib.sha256(pathlib.Path(__file__).read_bytes()).hexdigest(),
              'reference_sha256':hashlib.sha256(pathlib.Path(sh2emu.__file__).read_bytes()).hexdigest(),
              'vectors':args.vectors, 'seed':'0xd1ff0000 XOR canonical entry',
              'input_model':'C pointer types and bounded static SH-2 provenance; identical initial scratch RAM; no reference retries or runtime repairs',
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
