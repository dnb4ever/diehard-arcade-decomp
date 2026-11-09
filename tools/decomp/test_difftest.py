"""Regression checks for the differential oracle's observable contract.
Run: tools/decomp/venv/bin/python -m unittest discover -s tools/decomp -p test_difftest.py
"""
import unittest
import pathlib
import struct
import subprocess
import tempfile
import os
import importlib
d = importlib.import_module(os.environ.get('DIFFTEST_MODULE', 'difftest'))
from saturnrecomp.recomp.selftest import Asm


class DifferentialContract(unittest.TestCase):
    def memory(self, code):
        mem = d.sh2emu.Memory()
        mem.add(0x06004000, len(code), code)
        mem.add(0x00280000, 0x1000)
        mem.add(d.STACK-0x1000, 0x2000)
        mem.log = []
        return mem

    def test_big_endian_store_and_same_value_footprint(self):
        mem = self.memory(bytes.fromhex('2402 000b 0009'))  # mov.l r0,@r4; rts; nop
        cpu = d.Oracle(mem)
        cpu.r[0] = 0x12345678
        cpu.call(0x06004000, r4=0x00280000)
        self.assertEqual(d.writes(mem.log), {0x280000:0x12,0x280001:0x34,0x280002:0x56,0x280003:0x78})
        mem.log = []
        mem.write(0x280000,4,0x12345678)
        self.assertEqual(len(d.writes(mem.log)),4)
        self.assertIsNotNone(d.difference((0,d.writes(mem.log),[]),(0,{},[])))

    def test_call_records_arguments_after_delay_slot(self):
        a = Asm(0x06004000)
        a.op('sts.l pr,@-Rn',n=15)
        a.op('bsr @T',to='callee')
        a.op('add #imm,Rn',n=4,low=1)
        a.op('lds.l @Rm+,pr',m=15)
        a.op('rts')
        a.op('nop')
        a.label('callee')
        # This body must NOT run: the stub substitutes its return value.
        a.op('mov #imm,Rn',n=0,low=0)
        a.op('rts')
        a.op('nop')
        cpu = d.Oracle(self.memory(a.assemble()))
        cpu.r[15] = d.STACK
        cpu.call(0x06004000,r4=10,r5=20,r6=30,r7=40)
        target = a.labels['callee']
        self.assertEqual(cpu.calls, [[target,11,20,30,40]])
        self.assertEqual(cpu.r[0], d.stub(target,[11,20,30,40]))

    def test_known_tail_call_is_stubbed(self):
        a = Asm(0x06004000)
        a.op('bra @T',to='callee')
        a.op('add #imm,Rn',n=4,low=2)
        a.label('callee')
        a.op('rts')
        a.op('nop')
        target = a.labels['callee']
        cpu = d.Oracle(self.memory(a.assemble()),[target],0x06004000)
        cpu.call(0x06004000,r4=1,r5=2,r6=3,r7=4)
        self.assertEqual(cpu.calls,[[target,3,2,3,4]])
        self.assertEqual(cpu.r[0],d.stub(target,[3,2,3,4]))

    def test_unmapped_hardware_is_not_zero_filled(self):
        mem = self.memory(bytes.fromhex('000b 0009'))
        with self.assertRaises(d.sh2emu.MemoryError_):
            mem.read(0x25fe0000,4)

    def test_intermediate_stores_are_compared(self):
        a = [('w',0x280000,1,1),('w',0x280000,1,2)]
        b = [('w',0x280000,1,2)]
        self.assertEqual(d.writes(a),d.writes(b))
        diff = d.difference((0,d.writes(a),[],d.write_trace(a)),(0,d.writes(b),[],d.write_trace(b)))
        self.assertEqual(diff['kind'],'memory write trace')

    def test_private_stack_is_excluded(self):
        self.assertEqual(d.writes([('w',d.STACK-4,4,0)]),{})

    def test_unsupported_source_is_rejected(self):
        for source in ['u32 f(u32 *p) { return (u32)&p; }',
                       'u32 f(void) { return missing; }',
                       'u32 f(void) { return *((u32 *)(u32[]){1}); }',
                       'u32 f(void) { return unknown_callee(1); }']:
            with self.subTest(source=source), self.assertRaises(d.Unsupported):
                d.prepare(source,{'callee':0x06005000})

    def test_void_observables_and_mutation(self):
        source = 'void f(void) { MEM32(0x00280000) = 7; }'
        metadata = {}
        generated, _ = d.prepare(source, {}, metadata=metadata)
        self.assertFalse(metadata['r0_checked'])
        self.assertIn('return 0;', generated)
        def execute(body):
            generated, _ = d.prepare(body, {})
            with tempfile.TemporaryDirectory(dir=None) as tmp:
                tmp = pathlib.Path(tmp)
                (tmp/'test.c').write_text(d.PRELUDE+d.HARNESS+generated+d.MAIN)
                subprocess.run(['gcc',str(tmp/'test.c'),'-o',str(tmp/'test')], check=True, capture_output=True)
                (tmp/'state').write_bytes(bytes(16+0x200000))
                run = subprocess.run([str(tmp/'test'),str(tmp/'state')],check=True,capture_output=True,text=True)
                fields = run.stdout.splitlines()[0].split()
                log = [('w', int(fields[1],16),int(fields[2]),int(fields[3],16))]
                return (0,d.writes(log),[],d.write_trace(log))
        good = execute(source)
        ref = (123,good[1],[],good[3])
        self.assertIsNone(d.difference(ref,good,False))
        self.assertEqual(d.difference(ref,execute(source.replace('= 7','= 8')),False)['kind'],'memory write')
        self.assertIsNotNone(d.difference(ref,(0,{},[],[]),False))
        self.assertIsNotNone(d.difference(ref,(0,good[1],[[1]],good[3]),False))

    def test_mutated_void_entry_must_fail(self):
        # Real cart no-op entry: a new externally visible store must FAIL even
        # though its undefined final r0 is deliberately ignored.
        baseline = d.test_one((0x02201b8e,1,{},None))
        self.assertEqual(baseline['status'],'PASS-void',baseline)
        self.assertFalse(baseline['r0_checked'])
        with tempfile.TemporaryDirectory(dir=None) as tmp:
            candidate = pathlib.Path(tmp)/'mutated.c'
            candidate.write_text('void func_02201b8e(void) { MEM32(0x00280000) = 7; }')
            result = d.test_one((0x02201b8e,1,{},str(candidate)))
        self.assertEqual(result['status'],'FAIL',result)
        self.assertFalse(result['r0_checked'])
        self.assertEqual(result['diff']['kind'],'memory write')

    def test_preprocessor_and_signed_markers(self):
        source = '#include "dh_band21.h"\n#define VALUE 7\n#if VALUE == 7\ns32 f(void) { MEM16(0x280000) = VALUE; return MEM16(0x280000); }\n#else\n#error wrong branch\n#endif'
        generated, _ = d.prepare(source,{})
        self.assertIn('(s16)',generated)
        self.assertIn('dt_write',generated)
        self.assertIn('dt_read',generated)

    def test_multi_function_entry_and_stub(self):
        source = 'u32 func_02210024(u32 a,u32 b,u32 c,u32 e) { return 99; } u32 func_02210020(u32 a,u32 b,u32 c,u32 e) { return func_02210024(a,b,c,e); }'
        generated, _ = d.prepare(source,{},0x02210020)
        self.assertIn('dt_callv((u32)(0x6014024)',generated)
        self.assertNotIn('return 99',generated)
        with self.assertRaises(d.Unsupported):
            d.prepare(source,{},0x02210028)

    def test_host_stub_signed_return_and_store_footprint(self):
        source = """extern s32 callee(u32 a,u32 b,u32 c,u32 e);
        u32 f(u32 a,u32 b,u32 c,u32 e) {
            MEM16(0x00280000) = a;
            return callee(a,b,c,e) < 0;
        }"""
        generated, _ = d.prepare(source,{'callee':0x06005000})
        args = [0x12345678,2,3,4]
        with tempfile.TemporaryDirectory(dir=None) as tmp:
            tmp = pathlib.Path(tmp)
            (tmp/'test.c').write_text(d.PRELUDE+d.HARNESS+generated+d.MAIN)
            subprocess.run(['gcc','-std=c11','-O1',str(tmp/'test.c'),'-o',str(tmp/'test')],check=True,capture_output=True)
            (tmp/'state').write_bytes(struct.pack('=4I',*args)+bytes(0x200000))
            run = subprocess.run([str(tmp/'test'),str(tmp/'state')],check=True,capture_output=True,text=True)
        lines = run.stdout.splitlines()
        self.assertEqual(lines[0],'W 00280000 2 12345678')
        self.assertEqual(lines[1],'C 06005000 12345678 00000002 00000003 00000004')
        self.assertEqual(lines[2],'R %08x' % (d.stub(0x06005000,args) >> 31))

    def test_memory_store_is_instrumented_and_narrowed(self):
        source = 's32 f(void *p) { M16(p,2) = -1; return M16(p,2); }'
        generated, pointers = d.prepare(source,{})
        self.assertIn('dt_write',generated)
        self.assertIn('dt_read',generated)
        self.assertIn('(s16)',generated)
        self.assertEqual(pointers,[True])


class WiderCoverage(unittest.TestCase):
    memory = DifferentialContract.memory
    def native(self, source, args=(), initial=()):
        metadata = {}
        generated, _ = d.prepare(source, {'callee':0x06005000}, metadata=metadata)
        high, low = bytearray(0x100000), bytearray(0x100000)
        for address,size,value in initial:
            region,base = (high,d.RAM) if address >= d.RAM else (low,0x200000)
            region[address-base:address-base+size] = (value & ((1<<(size*8))-1)).to_bytes(size,'big')
        with tempfile.TemporaryDirectory() as tmp:
            tmp = pathlib.Path(tmp)
            args = list(args) + [0]*max(0,4-len(args))
            (tmp/'test.c').write_text('#define DT_ARG_WORDS %d\n' % len(args)+d.PRELUDE+d.HARNESS+generated+d.MAIN)
            subprocess.run(['gcc','-std=c11','-O1','-fwrapv',str(tmp/'test.c'),'-o',str(tmp/'test')],check=True,capture_output=True)
            (tmp/'state').write_bytes(struct.pack('=%dI' % len(args),*args)+high+low)
            proc = subprocess.run([str(tmp/'test'),str(tmp/'state')],check=True,capture_output=True,text=True)
        calls,log,ret = [],[],None
        for line in proc.stdout.splitlines():
            fields = line.split()
            if fields[0] == 'R': ret = int(fields[1],16)
            elif fields[0] == 'C': calls.append([int(x,16) for x in fields[1:]])
            elif fields[0] == 'W': log.append(('w',int(fields[1],16),int(fields[2]),int(fields[3],16)))
        return (ret,d.writes(log),calls,d.write_trace(log)),metadata

    def assert_mutation_fails(self, reference, mutated, check_r0=True):
        diff = d.difference(reference,mutated,check_r0)
        verdict = 'FAIL' if diff else 'PASS'
        self.assertEqual(verdict,'FAIL',diff)
        return diff

    def test_declared_callee_arities_and_stack_words(self):
        for count in (0,1,2,3,4,5,7):
            with self.subTest(count=count):
                a = Asm(0x06004000)
                a.op('sts.l pr,@-Rn',n=15)
                a.op('add #imm,Rn',n=15,low=(-12)&255)
                a.labels['callee'] = 0x06005000
                a.op('bsr @T',to='callee')
                a.op('add #imm,Rn',n=4,low=1)
                a.op('add #imm,Rn',n=15,low=12)
                a.op('lds.l @Rm+,pr',m=15)
                a.op('rts'); a.op('nop')
                mem = self.memory(a.assemble())
                for i,v in enumerate((50,60,70)): mem.write(d.STACK-16+4*i,4,v)
                mem.log = []
                cpu = d.Oracle(mem,call_specs={0x06005000:count})
                cpu.r[15] = d.STACK
                cpu.call(0x06004000,r4=10,r5=20,r6=30,r7=40)
                values = ['a+1','b','c','e','50','60','70'][:count]
                params = ','.join('u32 p%d' % i for i in range(count)) or 'void'
                source = 'extern u32 callee(%s); u32 f(u32 a,u32 b,u32 c,u32 e){return callee(%s);}' % (params,','.join(values))
                got,_ = self.native(source,[10,20,30,40])
                ref = (cpu.r[0],d.writes(mem.log),cpu.calls,d.write_trace(mem.log))
                self.assertIsNone(d.difference(ref,got))
                self.assertEqual(cpu.calls,[[0x06005000]+[11,20,30,40,50,60,70][:count]])
                mutated = source.replace('return callee(', 'return 1 ^ callee(')
                self.assert_mutation_fails(ref,self.native(mutated,[10,20,30,40])[0])
                if count > 4:
                    mutated = source.replace('50','51')
                    bad,_ = self.native(mutated,[10,20,30,40])
                    self.assert_mutation_fails(ref,bad)
                    self.assertNotEqual(ref[2],bad[2])

    def test_stack_arg_is_observed_after_delay_slot(self):
        a = Asm(0x06004000)
        a.op('sts.l pr,@-Rn',n=15)
        a.op('add #imm,Rn',n=15,low=252)
        a.labels['callee'] = 0x06005000
        a.op('bsr @T',to='callee')
        a.op('mov.l Rm,@Rn',m=0,n=15)
        a.op('add #imm,Rn',n=15,low=4)
        a.op('lds.l @Rm+,pr',m=15)
        a.op('rts'); a.op('nop')
        cpu = d.Oracle(self.memory(a.assemble()),call_specs={0x06005000:5})
        cpu.r[0],cpu.r[15] = 0x89abcdef,d.STACK
        cpu.call(0x06004000,r4=1,r5=2,r6=3,r7=4)
        self.assertEqual(cpu.calls,[[0x06005000,1,2,3,4,0x89abcdef]])

    def test_missing_and_old_style_prototypes_are_noted(self):
        for declaration in ('','extern s32 callee();'):
            source = declaration+'s32 f(void){return callee(1,2,3,4,5);}'
            got,meta = self.native(source)
            self.assertEqual(meta['inferred_prototypes']['callee']['args'],5)
            self.assertEqual(got[2],[[0x06005000,1,2,3,4,5]])
        with self.assertRaises(d.Unsupported):
            d.prepare('u32 f(void){callee(1); return callee(1,2);}',{'callee':0x06005000})
        with self.assertRaises(d.Unsupported):
            d.prepare('extern u32 callee(u32 a);u32 f(void){return callee(1,2);}',{'callee':0x06005000})

    def test_mem32_signedness_raw_words_and_mutations(self):
        for include in ('','#include "dh_band21.h"\n'):
            source = include+'u32 f(void){MEMU32(0x280000)=0x80000001u;return (MEM32(0x280000)<0) + (MEMU32(0x280000)>0x7fffffffu);}'
            got,_ = self.native(source)
            ref = (2,{0x280000:128,0x280001:0,0x280002:0,0x280003:1},[],[(0x280000,128),(0x280001,0),(0x280002,0),(0x280003,1)])
            self.assertIsNone(d.difference(ref,got))
            bad,_ = self.native(source.replace('MEM32(0x280000)<0','MEMU32(0x280000)<0'))
            self.assert_mutation_fails(ref,bad)
        self.assertIsNone(d.difference((-1,{},[]),(0xffffffff,{},[])))
        self.assert_mutation_fails((0x80000000,{},[]),(0x80000001,{},[]))

    def test_indirect_target_args_and_no_follow(self):
        # Neither target has backing executable bytes; only values are observed.
        for expression in ('cb','(*cb)','((s32 (*)(u32))cb)'):
            source = 's32 f(s32 (*cb)(u32),u32 a){return %s(a);}' % expression
            got,meta = self.native(source,[0xdeadbeef,0x80000000])
            self.assertEqual(meta['indirect_counts'],[1])
            mem = self.memory(bytes.fromhex('4f22 400b 0009 4f26 000b 0009'))
            # jsr @r0 after saving PR; the callee argument is r4.
            cpu = d.Oracle(mem,call_specs={},indirect_counts=[1])
            cpu.r[0],cpu.r[15] = 0xdeadbeef,d.STACK
            cpu.call(0x06004000,r4=0x80000000)
            ref = (cpu.r[0],d.writes(mem.log),cpu.calls,d.write_trace(mem.log))
            self.assertIsNone(d.difference(ref,got))
            bad,_ = self.native(source,[0xdeadbeed,0x80000000])
            self.assert_mutation_fails(ref,bad)
            self.assertNotEqual(ref[2],bad[2])
        with self.assertRaises(d.Unsupported):
            cpu = d.Oracle(self.memory(bytes.fromhex('400b 0009')),call_specs={},indirect_counts=[1,2])
            cpu.r[0] = 0xdeadbeef
            cpu.call(0x06004000)

    def test_indirect_compiler_helper_keeps_nonstandard_abi_skip(self):
        cpu = d.Oracle(self.memory(bytes.fromhex('400b 0009')),call_specs={},indirect_counts=[3])
        cpu.r[0] = d.canonical(0x0224a7a8)  # dtoi: no runtime model in any version
        with self.assertRaisesRegex(d.Unsupported,'nonstandard callee ABI'):
            cpu.call(0x06004000)

    def test_callback_table_and_stack_args(self):
        source = 'extern void (*g_06095000[])(u32,u32,u32,u32,u32); void f(void){g_06095000[2](1,2,3,4,5);}'
        got,_ = self.native(source,initial=[(0x06095008,4,0xcafebabe)])
        self.assertEqual(got[2],[[0xcafebabe,1,2,3,4,5]])
        bad,_ = self.native(source.replace('(1,2,3,4,5)','(1,2,3,4,6)'),initial=[(0x06095008,4,0xcafebabe)])
        self.assert_mutation_fails(got,bad,False)

    def test_raw_pointer_width_endian_arithmetic_and_mutations(self):
        source = 'u32 f(u16 *p){p[1]=0x8001; p+=1; return *(s16 *)p;}'
        got,_ = self.native(source,[d.SCRATCH])
        ref = (0xffff8001,{d.SCRATCH+2:128,d.SCRATCH+3:1},[],[(d.SCRATCH+2,128),(d.SCRATCH+3,1)])
        self.assertIsNone(d.difference(ref,got))
        bad,_ = self.native(source.replace('p[1]','p[2]'),[d.SCRATCH])
        self.assert_mutation_fails(ref,bad)
        got,_ = self.native('u32 f(u32 **p){return **p;}',[d.SCRATCH],[(d.SCRATCH,4,d.SCRATCH+8),(d.SCRATCH+8,4,0xabcdef01)])
        self.assertEqual(got[0],0xabcdef01)
        got,_ = self.native('u32 f(u16 *p){u16 *q=p++;return (u32)p-(u32)q;}',[0xfffffffe])
        self.assertEqual(got[0],2)

    def test_real_cart_indirect_argument_mutation_must_fail(self):
        address = 0x0222c322
        baseline = d.test_one((address,1,{},None))
        self.assertEqual(baseline['status'],'PASS-void',baseline)
        source = (d.ROOT/'project/src_functional/0222c322.c').read_text()
        original = 'g_actor_pose_handlers[g_actor_pose_set](arg0, 0xE);'
        self.assertIn(original,source)
        with tempfile.TemporaryDirectory() as tmp:
            candidate = pathlib.Path(tmp)/'mutated.c'
            candidate.write_text(source.replace(original,'g_actor_pose_handlers[g_actor_pose_set](arg0, 0xF);'))
            mutated = d.test_one((address,1,{},str(candidate)))
        self.assertEqual(mutated['status'],'FAIL',mutated)
        self.assertEqual(mutated['diff']['kind'],'calls')
        self.assertFalse(mutated['r0_checked'])

    def test_array_decay_and_pointer_expression_types(self):
        source = 'extern u16 g_06095000[];u32 f(void){return *(g_06095000+2);}'
        got,_ = self.native(source,initial=[(0x06095004,2,0x8765)])
        self.assertEqual(got[0],0x8765)
        source = 'u32 f(u32 *p){return (!p)+1;}'
        self.assertEqual(self.native(source,[0])[0][0],2)
        self.assertEqual(self.native(source,[d.SCRATCH])[0][0],1)
        source = 'u32 f(u16 *p,u32 flag){return *(flag?p:NULL);}'
        self.assertEqual(self.native(source,[d.SCRATCH,1],[(d.SCRATCH,2,0xabcd)])[0][0],0xabcd)
        if tuple(map(int,d.VERSION.split('.'))) >= (1,8):
            # v1.08 has lexical bindings; an inner pointer must not change the
            # type or address of the outer pointer used after the block.
            source='u32 f(u32 *p){{u16 *p=0;}return *p;}'
            self.assertEqual(self.native(source,[d.SCRATCH],[(d.SCRATCH,4,0x12345678)])[0][0],0x12345678)
        else:
            with self.assertRaisesRegex(d.Unsupported,'shadowed'):
                d.prepare('u32 f(u32 *p){{u16 *p=0;}return *p;}',{})

    def test_wider_entry_stack_words(self):
        source = 'u32 f(u32 a,u32 b,u32 c,u32 e,u32 fifth){return fifth;}'
        got,meta = self.native(source,[1,2,3,4,0x87654321])
        self.assertEqual(meta['entry_arg_count'],5)
        self.assertEqual(got[0],0x87654321)
        high = bytearray(0x100000)
        high[0x4000:0x4006] = bytes.fromhex('60f2 000b 0009')
        ref = d.reference(0x06004000,[1,2,3,4,0x87654321],high,bytes(0x100000),0,call_specs={})
        self.assertIsNone(d.difference(ref,got))


class V103Coverage(unittest.TestCase):
    memory = DifferentialContract.memory
    native = WiderCoverage.native
    assert_mutation_fails = WiderCoverage.assert_mutation_fails
    def test_compound_store_matches_sh2_and_mutation(self):
        a = Asm(0x06004000)
        a.op('mov.l @Rm,Rn',m=4,n=0)
        a.op('add #imm,Rn',n=0,low=1)
        a.op('mov.l Rm,@Rn',m=0,n=4)
        a.op('rts'); a.op('nop')
        mem = self.memory(a.assemble())
        mem.write(d.SCRATCH,4,0xffffffff)
        mem.log = []
        cpu = d.Oracle(mem)
        cpu.call(0x06004000,r4=d.SCRATCH)
        ref = (cpu.r[0],d.writes(mem.log),cpu.calls,d.write_trace(mem.log))
        source = 'u32 f(u32 p){return MEMU32(p)+=1;}'
        got,_ = self.native(source,[d.SCRATCH],[(d.SCRATCH,4,0xffffffff)])
        self.assertIsNone(d.difference(ref,got))
        bad,_ = self.native(source.replace('+=1','+=2'),[d.SCRATCH],[(d.SCRATCH,4,0xffffffff)])
        self.assert_mutation_fails(ref,bad)

    def test_compound_width_signedness_and_operators(self):
        for ctype,size,initial,op,rhs,expected in [
                ('s8',1,0x80,'>>',1,0xffffffc0),
                ('u8',1,0x80,'>>',1,0x40),
                ('s16',2,0x8000,'>>',1,0xffffc000),
                ('u16',2,0xffff,'+',2,1),
                ('s32',4,0x80000000,'>>',1,0xc0000000),
                ('u32',4,0x80000000,'>>',1,0x40000000),
                ('u8',1,15,'&',3,3),('u16',2,15,'|',16,31),
                ('u32',4,15,'^',3,12),('s16',2,0xfff9,'/',2,0xfffffffd),
                ('s16',2,0xfff9,'%',2,0xffffffff),('u32',4,3,'<<',2,12),
                ('u8',1,3,'*',3,9),('u16',2,3,'-',4,0xffff)]:
            with self.subTest(ctype=ctype,op=op):
                source = 'u32 f(%s *p){return *p %s= %d;}' % (ctype,op,rhs)
                got,_ = self.native(source,[d.SCRATCH],[(d.SCRATCH,size,initial)])
                self.assertEqual(got[0],expected)
                self.assertEqual(len(got[1]),size)
                self.assertEqual(len(got[3]),size)
                bad,_ = self.native(source.replace('return ','return 1+(').replace(';}',');}'),[d.SCRATCH],[(d.SCRATCH,size,initial)])
                self.assert_mutation_fails(got,bad)

    def test_compound_address_once_nested_and_pointer_stride(self):
        source = 'u32 f(u16 *p){u32 n=0; p[n++] += 1; return n;}'
        got,_ = self.native(source,[d.SCRATCH],[(d.SCRATCH,2,7)])
        self.assertEqual(got[0],1)
        self.assertEqual(got[1],{d.SCRATCH:0,d.SCRATCH+1:8})
        source = 'u32 f(u32 *p){return p[0] += (p[1] += 3);}'
        got,_ = self.native(source,[d.SCRATCH],[(d.SCRATCH,4,1),(d.SCRATCH+4,4,2)])
        self.assertEqual(got[0],6)
        self.assertEqual(got[3][0][0],d.SCRATCH+4)
        source = 'u32 f(u16 **p){return (u32)(*p += 2);}'
        got,_ = self.native(source,[d.SCRATCH],[(d.SCRATCH,4,0x280100)])
        self.assertEqual(got[0],0x280104)
        self.assertEqual(self.native('u32 f(u8 *p){return (*p)++;}',[d.SCRATCH],[(d.SCRATCH,1,255)])[0][0],255)

    def test_guest_struct_fields_layout_and_mutation(self):
        source = ('typedef struct Node {u8 tag; s16 value; u32 flags; u16 data[2];} Node;'
                  '#define M(a) ((Node *)(a))\n'
                  'u32 f(u32 a){M(a)->value >>= 1; M(a)->flags |= 8;'
                  'M(a)->data[1] += 1; return sizeof(Node);}')
        source = source.replace(';#define',';\n#define')
        initial = [(d.SCRATCH+2,2,0x8000),(d.SCRATCH+4,4,1),(d.SCRATCH+10,2,0xffff)]
        got,_ = self.native(source,[d.SCRATCH],initial)
        stores = [(d.SCRATCH+2,192),(d.SCRATCH+3,0),
                  (d.SCRATCH+4,0),(d.SCRATCH+5,0),(d.SCRATCH+6,0),(d.SCRATCH+7,9),
                  (d.SCRATCH+10,0),(d.SCRATCH+11,0)]
        self.assertIsNone(d.difference((12,dict(stores),[],stores),got))
        bad,_ = self.native(source.replace('flags |= 8','flags |= 16'),[d.SCRATCH],initial)
        self.assert_mutation_fails(got,bad)
        with self.assertRaises(d.Unsupported):
            d.prepare('struct X{u32 a;}; u32 f(struct X x){return x.a;}',{})
        with self.assertRaises(d.Unsupported):
            d.prepare('struct X{u32 a:1;};u32 f(struct X *p){return p->a;}',{})
        for source in [
                '#pragma pack(1)\nstruct X{u8 a;u32 b;};u32 f(struct X *p){return p->b;}',
                'struct X{u32 a;};u32 f(struct X *p){return p->missing;}',
                'struct X{u32 a;};u32 f(struct X *p){return (u32)((*p)++);}',
                'struct X{u32 a;};void f(struct X *p){*p=*p;}']:
            with self.subTest(source=source),self.assertRaises(d.Unsupported):
                d.prepare(source,{})

    def test_static_pointer_argument_copy_and_nested_load(self):
        a = Asm(0x06004000)
        a.op('mov Rm,Rn',m=4,n=8)
        a.op('mov.l @Rm,Rn',m=8,n=1)
        a.op('mov.l @Rm,Rn',m=1,n=0)
        a.op('rts'); a.op('nop')
        code = a.assemble()
        constraints = d.infer_pointer_inputs(0x06004000,code)
        self.assertIn(('arg',0),constraints)
        self.assertIn(('load',('arg',0)),constraints)
        args = [0xdeadbeef,7,0,0]
        high,low = bytearray(0x100000),bytearray(0x100000)
        high[0x4000:0x4000+len(code)] = code
        edits = d.structure_inputs(args,high,low,constraints)
        self.assertTrue(edits)
        self.assertEqual(args[1],7)
        ptr = int.from_bytes(low[args[0]-0x200000:args[0]-0x200000+4],'big')
        low[ptr-0x200000:ptr-0x200000+4] = (0x87654321).to_bytes(4,'big')
        ref = d.reference(0x06004000,args,high,low,0)
        initial = [(args[0],4,ptr),(ptr,4,0x87654321)]
        source = 'u32 f(u32 p){return **(u32 **)p;}'
        got,_ = self.native(source,args,initial)
        self.assertIsNone(d.difference(ref,got))
        bad,_ = self.native(source.replace('return **','return 1+**'),args,initial)
        self.assert_mutation_fails(ref,bad)

    def test_static_global_pointer_chain_and_identical_seed(self):
        a = Asm(0x06004000)
        a.op('mov.l @T,Rn',n=1,to='global')
        a.op('mov.l @Rm,Rn',m=1,n=1)
        a.op('mov.l @Rm,Rn',m=1,n=0)
        a.op('rts'); a.op('nop')
        a.align(); a.label('global'); a.long(0x06095000)
        code = a.assemble()
        constraints = d.infer_pointer_inputs(0x06004000,code)
        self.assertIn(('load',('const',0x06095000)),constraints)
        args = [0]*4
        high,low = bytearray(0x100000),bytearray(0x100000)
        high[0x4000:0x4000+len(code)] = code
        original_code = bytes(high[0x4000:0x91e20])
        first = d.structure_inputs(args,high,low,constraints)
        pointer = int.from_bytes(high[0x95000:0x95004],'big')
        low[pointer-0x200000:pointer-0x200000+4] = (123).to_bytes(4,'big')
        self.assertEqual(bytes(high[0x4000:0x91e20]),original_code)
        ref = d.reference(0x06004000,args,high,low,0)
        source = 'extern u32 *g_06095000; u32 f(void){return *g_06095000;}'
        got,meta = self.native(source,initial=[(0x06095000,4,pointer),(pointer,4,123)])
        self.assertEqual(meta['typed_pointer_globals'],[0x06095000])
        self.assertTrue(first)
        self.assertIsNone(d.difference(ref,got))
        self.assert_mutation_fails(ref,self.native(source.replace('return *','return 1+*'),initial=[(0x06095000,4,pointer),(pointer,4,123)])[0])

    def test_faulting_reference_remains_skip_without_retry(self):
        from unittest.mock import patch
        with patch.object(d,'reference',side_effect=d.sh2emu.MemoryError_('unmapped 4-byte access at DEADBEEF')) as reference:
            result = d.test_one((0x02201b8e,2,{},None))
        self.assertEqual(result['status'],'SKIP',result)
        self.assertEqual(result['vectors'],0)
        self.assertEqual(reference.call_count,1)
        self.assertEqual(result['reason'],'reference cannot run: unmapped 4-byte access at DEADBEEF')
        with patch.object(d,'reference',side_effect=[(0,{},[],[]),(0,{},[],[]),d.sh2emu.MemoryError_('late fault')]) as reference:
            result = d.test_one((0x02201b8e,2,{},None))
        self.assertEqual(result['status'],'SKIP',result)
        self.assertEqual(result['vectors'],1)
        self.assertEqual(reference.call_count,3)

    def test_pointer_setup_never_patches_code_stack_or_hardware(self):
        high,low = bytearray(0x100000),bytearray(0x100000)
        before = bytes(high)
        slots = [0x06003ffe,0x06004000,d.STACK-2,0x25fe0000]
        edits = d.structure_inputs([0]*4,high,low,[('load',('const',a)) for a in slots])
        self.assertEqual(edits,[])
        self.assertEqual(bytes(high),before)

    def test_real_cart_pointer_global_and_compound_mutations(self):
        cases = [
            (0x0220121e,{},'g_060E0DD0 = p + 4;','g_060E0DD0 = p + 8;'),
            (0x0221080a,{'load_matrix_default':d.canonical(0x02213fb4),
                         'mtx_rotate_zyx':d.canonical(0x022149e4)},
             'MEMU32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x30;','MEMU32(DH_MATRIX_STACK_CURSOR_ADDR) += 0x31;')]
        for address,symbols,original,mutated in cases:
            with self.subTest(address=f'{address:08x}'):
                baseline = d.test_one((address,2,symbols,None))
                self.assertEqual(baseline['status'],'PASS-void',baseline)
                source = (d.ROOT/'project/src_functional'/f'{address:08x}.c').read_text()
                self.assertIn(original,source)
                with tempfile.TemporaryDirectory() as tmp:
                    path = pathlib.Path(tmp)/'mutated.c'
                    path.write_text(source.replace(original,mutated))
                    result = d.test_one((address,2,symbols,str(path)))
                self.assertEqual(result['status'],'FAIL',result)
                self.assertIn(result['diff']['kind'],('memory write','memory write trace'))

    def test_symbols_file_stubs_local_prototype_and_pointer_return(self):
        from unittest.mock import patch
        with tempfile.TemporaryDirectory() as tmp:
            root = pathlib.Path(tmp)
            (root/'project/src_functional').mkdir(parents=True)
            (root/'notes').mkdir()
            (root/'notes/symbols.txt').write_text('0x022131c8 fixmul_16_16 # HIGH evidence\n0x022148ac zero_fill_aligned # MED evidence\n')
            with patch.object(d,'ROOT',root):
                symbols = d.load_symbols()
        self.assertEqual(symbols['fixmul_16_16'],d.canonical(0x022131c8))
        generated,_ = d.prepare('u32 f(void){extern u32 fixmul_16_16(u32,u32);return fixmul_16_16(1,2);}',symbols)
        self.assertIn('0x60171c8',generated)
        source = 'u32 f(void){extern u32 callee(u32);return callee(7);}'
        good,_ = self.native(source)
        bad,_ = self.native(source.replace('callee(7)','callee(8)'))
        self.assert_mutation_fails(good,bad)
        self.assertEqual(self.native('u32 *f(u32 *p){return p+2;}',[d.SCRATCH])[0][0],d.SCRATCH+8)


class CoverageDiagnostics(unittest.TestCase):
    def test_definitions_exclude_prototypes_calls_comments_and_strings(self):
        import contextlib
        import io
        import runpy
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            namespace = runpy.run_path(str(d.ROOT/'tools/decomp/coverage_callgraph.py'))
        definitions = namespace['func_definitions']
        source = r'''extern void func_00000001(void);
        /* void func_00000002(void) {} */
        void func_00000003(void (*cb)(int)) { if (func_00000001()) { cb(0); } }
        const char *s = "void func_00000004(void) {}";
        static void func_short(void) { func_00000001(); }
        // void func_00000005(void) {}
        '''
        self.assertEqual(definitions(source),['func_00000003','func_short'])
        text = output.getvalue()
        self.assertIn('defined (files):                        ',text)
        self.assertLess(text.index('TRUE CALL-GRAPH COVERAGE:'),text.index('WARNING: func_ tokens'))
        self.assertIn('files defining more than one func_:',text)


if __name__ == '__main__':
    unittest.main()
