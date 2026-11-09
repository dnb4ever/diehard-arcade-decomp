"""v1.04 regression checks: every retained v1.03 contract plus the new coverage.
Run: tools/decomp/venv/bin/python -m unittest discover -s tools/decomp -p test_difftest_v104.py
(DIFFTEST_MODULE selects the implementation; default difftest_v104.)
"""
import os
os.environ.setdefault('DIFFTEST_MODULE', 'difftest_v104')
import pathlib
import struct
import subprocess
import tempfile
import unittest
from test_difftest import *  # noqa: F401,F403  retained v1.03 contract tests
from test_difftest import d, Asm

M32 = 0xffffffff


def image_high():
    high = bytearray(0x100000)
    high[0x4000:0x91e20] = d.cart_rom()[0x200000:0x28de20]
    return high


def native(source, args=(), initial=(), symbols=None, pointer_targets=(), rom=False, check=True, entry=None):
    """Compile and run one candidate exactly as test_one does (harness + defines)."""
    metadata = {}
    generated, _ = d.prepare(source, symbols if symbols is not None else {'callee':0x06005000}, entry, metadata)
    high, low = bytearray(0x100000), bytearray(0x100000)
    for address, size, value in initial:
        region, base = (high, d.RAM) if address >= d.RAM else (low, 0x200000)
        region[address-base:address-base+size] = (value & ((1 << (size*8))-1)).to_bytes(size, 'big')
    args = [a & M32 for a in args] + [0]*max(0, 4-len(args))
    defines = '#define DT_ARG_WORDS %d\n#define DT_PTR_RET %s\n#define DT_FRAME %d\n' % (
        len(args), ','.join(['0'] + ['0x%x' % t for t in pointer_targets]), bool(metadata.get('stack_locals')))
    with tempfile.TemporaryDirectory() as tmp:
        tmp = pathlib.Path(tmp)
        (tmp/'test.c').write_text(defines + d.PRELUDE + d.HARNESS + generated + d.MAIN)
        subprocess.run(['gcc','-std=c11','-O1','-fwrapv',str(tmp/'test.c'),'-o',str(tmp/'test')], check=True, capture_output=True)
        (tmp/'state').write_bytes(struct.pack('=%dI' % len(args), *args) + high + low)
        command = [str(tmp/'test'), str(tmp/'state')] + ([str(d.ROOT/'cart.bin')] if rom else [])
        proc = subprocess.run(command, capture_output=True, text=True)
    if not check:
        return proc, metadata
    assert proc.returncode == 0, proc.stderr
    calls, log, ret = [], [], None
    for line in proc.stdout.splitlines():
        fields = line.split()
        if fields[0] == 'R': ret = int(fields[1], 16)
        elif fields[0] == 'C': calls.append([int(x, 16) for x in fields[1:]])
        elif fields[0] == 'W': log.append(('w', int(fields[1], 16), int(fields[2]), int(fields[3], 16)))
    return (ret, d.writes(log), calls, d.write_trace(log)), metadata


def must_fail(test, reference, mutated, check_r0=True):
    test.assertIsNotNone(d.difference(reference, mutated, check_r0), 'mutation must FAIL')


def candidate_result(address, text, symbols=None, vectors=4):
    with tempfile.TemporaryDirectory() as tmp:
        path = pathlib.Path(tmp)/'candidate.c'
        path.write_text(text)
        return d.test_one((address, vectors, symbols or {}, str(path)))


class V104RuntimeHelpers(unittest.TestCase):
    CASES = [(7, 2), (-7, 2), (7, -2), (0x80000000, M32), (5, 0), (0, 0), (0x12345678, 0x1234),
             (M32, 1), (1, 31), (1, 32), (0x80000000, 31), (0x80000000, 40), (9, M32), (0x7fffffff, 3)]

    def test_runtime_models_match_original_bytes(self):
        program = '\n'.join('printf("%%08x\\n", dt_rt_%s(0x%xu,0x%xu));' % (name, a & M32, b & M32)
                            for name in sorted(set(d.RUNTIME_MODELS.values())) for a, b in self.CASES)
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            (tmp/'m.c').write_text(d.PRELUDE + d.HARNESS + 'int main(void){' + program + 'return 0;}')
            subprocess.run(['gcc','-std=c11','-O1','-fwrapv',str(tmp/'m.c'),'-o',str(tmp/'m')], check=True, capture_output=True)
            lines = subprocess.run([str(tmp/'m')], check=True, capture_output=True, text=True).stdout.splitlines()
        host = [int(x, 16) for x in lines if not x.startswith('W ')]
        expected = []
        for target, name in sorted(d.RUNTIME_MODELS.items(), key=lambda kv: kv[1]):
            for a, b in self.CASES:
                mem = d.sh2emu.Memory()
                mem.add(d.RAM, 0x100000, image_high())
                mem.log = []
                cpu = d.Oracle(mem, call_specs={})
                cpu.r[15] = d.STACK
                if name.startswith('ash'):
                    cpu.call(target, r0=a, r1=b)  # shift: value in r0, count in r1
                else:
                    cpu.call(target, r0=b, r1=a)  # divide: divisor in r0, dividend in r1
                expected.append(cpu.r[0])
                if name.endswith(('div32', 'mod32')) and b == 0:
                    self.assertEqual(d.writes(mem.log), {0x060e132c:0, 0x060e132d:0, 0x060e132e:4, 0x060e132f:0x4e})
        self.assertEqual(host, expected)

    def caller(self, helper):
        a = Asm(0x060a0000)
        a.op('sts.l pr,@-Rn', n=15)
        a.op('mov Rm,Rn', m=4, n=1)
        a.op('mov.l @T,Rn', n=2, to='helper')
        a.op('jsr @Rn', n=2)
        a.op('mov Rm,Rn', m=5, n=0)
        a.op('lds.l @Rm+,pr', m=15)
        a.op('rts'); a.op('nop')
        a.align(); a.label('helper'); a.long(helper)
        high = image_high()
        code = a.assemble()
        high[0xa0000:0xa0000+len(code)] = code
        return high

    def test_runtime_call_runs_inline_and_argument_order_mutation(self):
        high = self.caller(0x0604e06c)
        source = 'extern s32 sdiv32(s32 dividend, s32 divisor); s32 f(s32 a, s32 b){return sdiv32(a,b);}'
        symbols = {'sdiv32':0x0604e06c}
        for args in ([100, 7], [-100, 7], [5, 0]):
            with self.subTest(args=args):
                ref = d.reference(0x060a0000, args+[0, 0], high, bytearray(0x100000), 0, call_specs={})
                self.assertEqual(ref[2], [])  # inline helper: no stub event
                got, _ = native(source, args, symbols=symbols)
                self.assertIsNone(d.difference(ref, got))
        ref = d.reference(0x060a0000, [100, 7, 0, 0], high, bytearray(0x100000), 0, call_specs={})
        must_fail(self, ref, native(source.replace('sdiv32(a,b)', 'sdiv32(b,a)'), [100, 7], symbols=symbols)[0])

    def test_unmodelled_runtime_entry_stays_skip(self):
        with self.assertRaisesRegex(d.Unsupported, 'nonstandard callee ABI'):
            d.prepare('extern s32 shr15(s32); s32 f(s32 a){return shr15(a);}', {'shr15':0x0604eb5c})


class V104Calls(unittest.TestCase):
    def test_static_helper_runs_natively_and_mutation(self):
        a = Asm(0x060a0000)
        a.op('add #imm,Rn', n=5, low=1)
        a.op('add Rm,Rn', m=5, n=5)
        a.op('rts')
        a.op('mov.l Rm,@Rn', m=5, n=4)
        high = bytearray(0x100000)
        code = a.assemble()
        high[0xa0000:0xa0000+len(code)] = code
        ref = d.reference(0x060a0000, [d.SCRATCH, 20, 0, 0], high, bytearray(0x100000), 0, call_specs={})
        source = 'static u32 twice(u32 x){return x*2;} void func_060a0000(u32 *p, u32 v){*p = twice(v+1);}'
        got, meta = native(source, [d.SCRATCH, 20], entry=0x060a0000)
        self.assertEqual(meta['native_helpers'], ['twice'])
        self.assertEqual(got[2], [])
        self.assertIsNone(d.difference(ref, got, False))
        must_fail(self, ref, native(source.replace('x*2', 'x*3'), [d.SCRATCH, 20], entry=0x060a0000)[0], False)

    def test_unnamed_pointer_prototype_parameter(self):
        source = 'extern void callee(void *); void f(u32 p){callee((void *)p);}'
        got, _ = native(source, [0x00281234])
        self.assertEqual(got[2], [[0x06005000, 0x00281234]])
        must_fail(self, got, native(source.replace('(void *)p', '(void *)(p+4)'), [0x00281234])[0], False)

    def test_direct_arity_precedence_and_length_mismatch_skip(self):
        a = Asm(0x06004000)
        a.op('sts.l pr,@-Rn', n=15)
        a.labels['callee'] = 0x06005000
        a.op('bsr @T', to='callee')
        a.op('nop')
        a.op('lds.l @Rm+,pr', m=15)
        a.op('rts'); a.op('nop')
        cpu = d.Oracle(DifferentialContract.memory(None, a.assemble()), call_specs={0x06005000:1}, indirect_counts=[2])
        cpu.r[15] = d.STACK
        cpu.call(0x06004000, r4=11, r5=22)
        self.assertEqual(cpu.calls, [[0x06005000, 11]])
        diff = {'kind':'calls', 'index':0, 'reference':[[0x06005000, 11]], 'candidate':[[0x06005000, 11, 22]]}
        with self.assertRaisesRegex(d.Unsupported, 'ambiguous callee argument count'):
            d.abi_call_mismatch(diff, {'call_specs':{0x06005000:1}})

    def test_image_function_pointer_cell_gets_direct_arity(self):
        meta = {}
        d.prepare('extern void (*g_06004010)(u32); extern void (*g_06095000)(u32,u32);'
                  'void f(void){g_06004010(1); g_06095000(1,2);}', {}, metadata=meta)
        self.assertEqual(meta['call_specs'], {d.canonical(d.image_word(0x06004010)):1})
        self.assertEqual(meta['indirect_counts'], [2])

    def test_pointer_to_function_pointer_cell_is_loaded(self):
        a = Asm(0x060a0000)
        a.op('sts.l pr,@-Rn', n=15)
        a.op('mov.l @T,Rn', n=1, to='cell')
        a.op('mov.l @Rm,Rn', m=1, n=1)
        a.op('jsr @Rn', n=1)
        a.op('mov #imm,Rn', n=4, low=7)
        a.op('lds.l @Rm+,pr', m=15)
        a.op('rts'); a.op('nop')
        a.align(); a.label('cell'); a.long(0x06095000)
        high = bytearray(0x100000)
        code = a.assemble()
        high[0xa0000:0xa0000+len(code)] = code
        high[0x95000:0x95004] = (0x0600cafe).to_bytes(4, 'big')
        ref = d.reference(0x060a0000, [0]*4, high, bytearray(0x100000), 0, call_specs={}, indirect_counts=[1])
        source = 'void f(void){(*(void (**)(u32))0x06095000)(7);}'
        got, meta = native(source, initial=[(0x06095000, 4, 0x0600cafe)])
        self.assertEqual(got[2], [[0x0600cafe, 7]])
        self.assertIsNone(d.difference(ref, got, False))
        direct, _ = native('void f(void){(*(void (*)(u32))0x06095000)(7);}', initial=[(0x06095000, 4, 0x0600cafe)])
        self.assertEqual(direct[2], [[0x06095000, 7]])  # a function pointer value is called as is
        must_fail(self, ref, direct, False)

    def test_narrow_callee_argument_upper_bits_skip_but_low_bits_fail(self):
        meta = {}
        generated, _ = d.prepare('extern void callee(s16 v); void f(u32 x){callee(x);}', {'callee':0x06005000}, metadata=meta)
        self.assertIn('(s16)', generated)
        self.assertEqual(meta['narrow_args'], {0x06005000:{0:2}})
        result = {'call_specs':{0x06005000:1}, 'narrow_args':meta['narrow_args']}
        upper = {'kind':'calls', 'index':0, 'reference':[[0x06005000, 0x1234fffe]], 'candidate':[[0x06005000, 0xfffffffe]]}
        with self.assertRaisesRegex(d.Unsupported, 'upper bits'):
            d.abi_call_mismatch(upper, result)
        low = {'kind':'calls', 'index':0, 'reference':[[0x06005000, 0x1234fffe]], 'candidate':[[0x06005000, 0xffffffff]]}
        self.assertIsNone(d.abi_call_mismatch(low, result))  # remains FAIL


class V104Values(unittest.TestCase):
    def test_long_long_host_values_and_ll_constants(self):
        source = 's32 f(s32 a, s32 b){return (s32)(((s64)a * b) >> 16) + (s32)1LL;}'
        for a, b in ((0x12345678, 0x7654321), (-5, 0x10000), (0x7fffffff, 0x7fffffff)):
            got, _ = native(source, [a & M32, b & M32])
            self.assertEqual(got[0], ((((a*b) >> 16) + 1) & M32))
        ref = (got[0], {}, [], [])
        must_fail(self, ref, native(source.replace('>> 16', '>> 15'), [0x7fffffff, 0x7fffffff])[0])
        if tuple(map(int,d.VERSION.split('.'))) >= (1,8):
            self.assertEqual(native('s32 f(void){return 1L;}',[])[0][0],1)
        else:
            with self.assertRaisesRegex(d.Unsupported, 'wide constant'):
                d.prepare('s32 f(void){return 1L;}', {})

    def test_narrow_entry_parameter_inputs_are_extended(self):
        self.assertEqual(d.extend(0x1234fffe, 2, True), 0xfffffffe)
        self.assertEqual(d.extend(0x1234fffe, 2, False), 0xfffe)
        self.assertEqual(d.extend(0xffffff7f, 1, True), 0x7f)
        meta = {}
        d.prepare('s32 f(s16 x, u8 y, s32 z){return x+y+z;}', {}, metadata=meta)
        self.assertEqual(meta['narrow_params'], {0:(2, True), 1:(1, False)})
        a = Asm(0x060a0000)
        a.op('rts')
        a.op('mov Rm,Rn', m=4, n=0)
        high = bytearray(0x100000)
        high[0xa0000:0xa0004] = a.assemble()
        arg = d.extend(0x1234fffe, 2, True)
        ref = d.reference(0x060a0000, [arg, 0, 0, 0], high, bytearray(0x100000), 0, call_specs={})
        source = 's32 f(s16 x){return x;}'
        self.assertIsNone(d.difference(ref, native(source, [arg])[0]))
        must_fail(self, ref, native(source.replace('return x', 'return (u16)x'), [arg])[0])


class V104StackLocals(unittest.TestCase):
    def test_local_array_scalar_and_struct_live_in_private_guest_frame(self):
        source = 'u32 f(u32 i){u32 t[4] = {5,6,7}; return t[i & 3];}'
        generated, _ = d.prepare(source, {})
        self.assertEqual(generated.count('dt_write'), 4)  # zero-fills the missing element
        for i, want in enumerate((5, 6, 7, 0)):
            got, meta = native(source, [i])
            self.assertEqual(got[0], want)
            self.assertEqual(got[1], {})  # frame stores are ABI-private
        self.assertEqual(meta['stack_locals'], ['t'])
        must_fail(self, native(source, [1])[0], native(source.replace('{5,6,7}', '{5,9,7}'), [1])[0])
        got, _ = native('u32 f(u32 v){u32 x = v; u32 *p = &x; *p += 1; return x;}', [41])
        self.assertEqual(got[0], 42)
        got, _ = native('struct S {u16 a; u32 b;}; u32 f(u32 v){struct S s; s.b = v; s.a = 3; return s.b + s.a;}', [10])
        self.assertEqual(got[0], 13)
        if tuple(map(int,d.VERSION.split('.'))) >= (1,8):
            source='u32 f(u32 i){{u32 t[2]; t[0]=i;} {u32 t[2]; t[1]=i;} return 0;}'
            got,meta=native(source,[7])
            self.assertEqual(got[0],0)
            self.assertEqual(len(meta['stack_locals']),2)
        else:
            with self.assertRaisesRegex(d.Unsupported, 'repeated stack-backed local'):
                d.prepare('u32 f(u32 i){{u32 t[2]; t[0]=i;} {u32 t[2]; t[1]=i;} return 0;}', {})

    def test_stack_address_escape_is_skip_not_pass(self):
        proc, _ = native('#include "dh_band21.h"\nvoid f(void){u32 x[2]; MEM32(0x00280000) = (u32)x;}', check=False)
        self.assertEqual(proc.returncode, 77)
        got, _ = native('extern void callee(u32 *p); void f(void){u32 x[2]; callee(x);}')
        self.assertTrue(d.private_argument(got[2]))
        result = candidate_result(0x02201b8e, 'extern void callee(u32 *p); void func_02201b8e(void){u32 x[2]; x[0]=1; callee(x);}',
                                  {'callee':0x06005000})
        self.assertEqual((result['status'], result['reason']), ('SKIP', 'private stack address passed to callee'))
        result = candidate_result(0x02201b8e, '#include "dh_band21.h"\nvoid func_02201b8e(void){u32 x[2]; MEM32(0x00280000) = (u32)x;}')
        self.assertEqual(result['status'], 'SKIP')
        self.assertIn('stack-backed local address stored', result['reason'])


class V104Inputs(unittest.TestCase):
    QUEUE = ('void sound_queue_push(s32 req) { s32 idx = MEM32(0x060e0dec);'
             ' MEM32(0x060e0df0 + idx * 4) = req; MEM32(0x060e0dec) = (idx + 1) & 15; }')

    def test_global_index_is_bounded_and_mutation_fails(self):
        cart = d.cart_rom()[0x200000:0x28de20]
        constraints = d.infer_pointer_inputs(d.canonical(0x02215f6c), cart)
        self.assertIn(('index', ('load', ('const', 0x060e0dec))), constraints)
        source = '#include "dh_band21.h"\n' + self.QUEUE
        result = candidate_result(0x02215f6c, source, vectors=16)
        self.assertEqual(result['status'], 'PASS-void', result)
        self.assertTrue(any('bounded_address' in edit for edit in result['input_setup']))
        mutated = candidate_result(0x02215f6c, source.replace('(idx + 1) & 15', '(idx + 2) & 15'), vectors=16)
        self.assertEqual(mutated['status'], 'FAIL', mutated)

    def test_dt_loop_count_inference(self):
        a = Asm(0x06004000)
        a.op('mov #imm,Rn', n=0, low=0)
        a.label('loop')
        a.op('add #imm,Rn', n=0, low=1)
        a.op('dt Rn', n=4)
        a.op('bf @T', to='loop')
        a.op('rts'); a.op('nop')
        code = a.assemble()
        constraints = d.infer_pointer_inputs(0x06004000, code)
        self.assertIn(('count', ('arg', 0)), constraints)
        args = [0x80000000, 0, 0, 0]
        high, low = bytearray(0x100000), bytearray(0x100000)
        high[0x4000:0x4000+len(code)] = code
        d.structure_inputs(args, high, low, constraints)
        self.assertTrue(1 <= args[0] <= 8)
        ref = d.reference(0x06004000, args, high, low, 0)
        source = 'u32 f(u32 n){u32 c = 0; do c++; while (--n); return c;}'
        self.assertIsNone(d.difference(ref, native(source, args)[0]))
        must_fail(self, ref, native(source.replace('c++', 'c += 2'), args)[0])

    def test_switch_table_targets_are_analysed(self):
        cart = d.cart_rom()[0x200000:0x28de20]
        entries = {d.canonical(0x0221b8e4), d.canonical(0x0221b8e4) + 0x250}
        constraints = d.infer_pointer_inputs(d.canonical(0x0221b8e4), cart, entries)
        self.assertIn(('add', ('load', ('add', ('arg', 0), 8)), 0x30), constraints)

    def test_intra_function_computed_jump_allowed_external_stays_skip(self):
        a = Asm(0x06004000)
        a.op('mova @T,r0', to='table')
        a.op('mov.l @(r0,Rm),Rn', m=4, n=1)  # r4 = 0: first slot
        a.op('jmp @Rn', n=1)
        a.op('nop')
        a.align(); a.label('table'); a.long(0)
        a.label('inside')
        a.op('rts')
        a.op('mov #imm,Rn', n=0, low=9)
        code = bytearray(a.assemble())
        table = a.labels['table'] - 0x06004000
        code[table:table+4] = a.labels['inside'].to_bytes(4, 'big')
        cpu = d.Oracle(DifferentialContract.memory(None, bytes(code)), [0x06004000, 0x06004100], 0x06004000, {}, [])
        cpu.r[15] = d.STACK
        self.assertEqual(cpu.call(0x06004000, r4=0), 9)
        code[table:table+4] = (0x06004200).to_bytes(4, 'big')
        cpu = d.Oracle(DifferentialContract.memory(None, bytes(code)), [0x06004000, 0x06004100], 0x06004000, {}, [])
        with self.assertRaisesRegex(d.Unsupported, 'computed tail/switch'):
            cpu.call(0x06004000, r4=0)

    def test_callee_pointer_result_is_mapped_on_both_sides(self):
        a = Asm(0x06004000)
        a.op('sts.l pr,@-Rn', n=15)
        a.labels['callee'] = 0x06005000
        a.op('bsr @T', to='callee')
        a.op('nop')
        a.op('mov.l @Rm,Rn', m=0, n=0)
        a.op('lds.l @Rm+,pr', m=15)
        a.op('rts'); a.op('nop')
        code = a.assemble()
        constraints = d.infer_pointer_inputs(0x06004000, code, [0x06005000])
        targets = d.pointer_returns(constraints)
        self.assertEqual(targets, [0x06005000])
        returned = d.stub(0x06005000, [0, 0, 0, 0], targets)
        self.assertEqual(returned & 0xffff0000, d.STUB_OBJECTS)
        high, low = bytearray(0x100000), bytearray(0x100000)
        high[0x4000:0x4000+len(code)] = code
        low[returned-0x200000:returned-0x200000+4] = (0x13572468).to_bytes(4, 'big')
        ref = d.reference(0x06004000, [0]*4, high, low, 0, [0x06005000], {0x06005000:0}, (), targets)
        source = 'extern u32 *callee(void); u32 f(void){return *callee();}'
        initial = [(returned, 4, 0x13572468), (returned+4, 4, 0x2468)]
        got, _ = native(source, initial=initial, pointer_targets=targets)
        self.assertIsNone(d.difference(ref, got))
        must_fail(self, ref, native(source.replace('*callee()', 'callee()[1]'), initial=initial, pointer_targets=targets)[0])

    def test_cart_rom_is_mapped_read_only_on_both_sides(self):
        a = Asm(0x060a0000)
        a.op('mov.l @T,Rn', n=1, to='rom')
        a.op('rts')
        a.op('mov.l @Rm,Rn', m=1, n=0)
        a.align(); a.label('rom'); a.long(0x226d1fdc)
        high = bytearray(0x100000)
        code = a.assemble()
        high[0xa0000:0xa0000+len(code)] = code
        ref = d.reference(0x060a0000, [0]*4, high, bytearray(0x100000), 0, call_specs={})
        self.assertEqual(ref[0], int.from_bytes(d.cart_rom()[0x6d1fdc:0x6d1fe0], 'big'))
        source = 'u32 f(void){return MEMU32(0x226d1fdc);}'
        self.assertIsNone(d.difference(ref, native('#include "dh_band21.h"\n' + source, rom=True)[0]))
        proc, _ = native('#include "dh_band21.h"\nvoid f(void){MEMU32(0x226d1fdc) = 1;}', rom=True, check=False)
        self.assertEqual(proc.returncode, 73)
        mem = d.GuestMemory()
        mem.regions.append((d.ROM_BASE, d.ROM_BASE + len(d.cart_rom()), d.cart_rom()))
        with self.assertRaises(d.sh2emu.MemoryError_):
            mem.write(0x226d1fdc, 4, 1)


if __name__ == '__main__':
    unittest.main()
