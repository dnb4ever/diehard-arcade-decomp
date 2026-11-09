"""v1.07 audited ABI, private-payload, static-data and device contracts.
Run with the inherited v1.04 and v1.06 suites. Mutations must remain observable.
"""
import json
import pathlib
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
import difftest_v107 as d


def host(body, definitions='', generated=''):
    with tempfile.TemporaryDirectory(prefix='difftest107-test-') as tmp:
        p=pathlib.Path(tmp)
        (p/'x.c').write_text(definitions+d.PRELUDE+d.HARNESS+generated+'\nint main(void){'+body+'}')
        subprocess.run(['gcc','-std=c11','-O1','-fwrapv',str(p/'x.c'),'-o',str(p/'x')],check=True,capture_output=True)
        return subprocess.run([str(p/'x')],check=True,capture_output=True,text=True).stdout.splitlines()


def high_image():
    high=bytearray(0x100000)
    high[0x4000:0x91e20]=d.cart_rom()[0x200000:0x28de20]
    return high


class V107Contracts(unittest.TestCase):
    def test_integer_entry_contract_relocates_args_without_noise_dependence(self):
        hi=high_image();lo=bytearray(0x100000)
        for noise in (0x13579bdf,0x2468ace0):
            ref=d.reference(0x0604e06c,[0xfffffff9,2,0,0],hi,lo,noise,call_specs={})
            self.assertEqual(ref[0],0xfffffffd)
            changed=d.reference(0x0604e06c,[0xfffffff9,3,0,0],hi,lo,noise,call_specs={})
            self.assertIsNotNone(d.difference(ref,changed))

    def test_sincos_two_results_and_output_mutation(self):
        hi=high_image();lo=bytearray(0x100000)
        args=[0x1234,0x00280000,0x00281000,0]
        ref=d.reference(d.SINCOS,args,hi,lo,0x13579bdf,call_specs={})
        expected={}
        index=((args[0]+8)&0xffff)>>4
        for base,table in [(args[1],0x75718),(args[2],0x75f18)]:
            value=int.from_bytes(hi[table+2*index:table+2*index+2],'big',signed=True)*2
            expected.update({base+i:b for i,b in enumerate((value&d.M32).to_bytes(4,'big'))})
        self.assertEqual(ref[1],expected)
        changed=dict(expected);changed[args[2]]^=1
        self.assertIsNotNone(d.difference(ref,(0,changed,[],ref[3]),False))

    def test_custom_dispatcher_callee_reads_r9_r8_and_returns_r1(self):
        from saturnrecomp.recomp.selftest import Asm
        a=Asm(0x060a0000)
        a.op('sts.l pr,@-Rn',n=15)
        a.op('mov.l @T,Rn',n=2,to='target')
        a.op('jsr @Rn',n=2);a.op('nop')
        a.op('mov Rm,Rn',m=1,n=0)
        a.op('lds.l @Rm+,pr',m=15);a.op('rts');a.op('nop')
        a.align();a.label('target');a.long(0x06017d90)
        hi=high_image();code=a.assemble();hi[0xa0000:0xa0000+len(code)]=code
        mem=d.GuestMemory();mem.add(d.RAM,len(hi),hi);mem.add(0x00200000,0x100000)
        cpu=d.Oracle(mem,entry=0x060a0000,call_specs={0x06017d90:2})
        cpu.r[15]=d.STACK
        cpu.call(0x060a0000,r8=0x00280000,r9=3,r4=99,r5=100)
        self.assertEqual(cpu.calls,[[0x06017d90,3,0x00280000]])
        self.assertEqual(cpu.r[0],d.stub(0x06017d90,[3,0x00280000]))

    def test_private_payload_relocation_and_mutation(self):
        target=0x06017354
        def read(a,n):return (a&0xff)^0x33
        args=[d.FRAME[0],d.FRAME[0]+0x100]
        py=d.payload_args(target,args,read)
        body='for(unsigned i=0;i<12;i++){dt_ptr(0x%x+i,1)[0]=(i^0x33);dt_ptr(0x%x+i,1)[0]=(i^0x33);} dt_callv(0x%x,2,(u32[]){0x%x,0x%x});' % (args[0],args[1],target,*args)
        lines=host(body,'#define DT_PAYLOADS {0x06017354,0,12,0},{0x06017354,1,12,0}\n')
        got=[int(x,16) for x in lines[0].split()[1:]]
        self.assertEqual(got,[target]+py)
        changed=d.payload_args(target,args,lambda a,n: read(a,n)^1)
        self.assertNotEqual(py,changed)
        self.assertNotEqual(d.stub(target,py),d.stub(target,changed))
        self.assertFalse(d.private_argument([[target]+py]))
        self.assertTrue(d.private_argument([[0x06009999]+args]))

    def test_output_pointer_is_filled_and_result_is_consumed(self):
        target=0x060147dc;args=[1,2,3,4,d.FRAME[0]]
        packed=d.payload_args(target,args,lambda a,n: self.fail('out-only argument was read'))
        expected=d.stub(target,packed)&0xffff
        lines=host('u32 a[]={1,2,3,4,0x%x};dt_callv(0x%x,5,a);printf("R %%08x\\n",dt_read(a[4],2));' % (args[4],target),'#define DT_PAYLOADS {0x060147dc,4,2,1}\n')
        self.assertEqual(int(lines[-1].split()[1],16),expected)

    def test_static_multidimensional_initializer_and_mutation(self):
        source='static const s32 table[2][2]={{3,5},{7,11}}; s32 f(u32 i){return table[i&1][1];}'
        values=[]
        for text in (source,source.replace('7,11','7,13')):
            generated,_=d.prepare(text,{})
            lines=host('u32 args[4]={1};printf("R %08x\\n",dt_entry(args));',generated=generated)
            values.append(int(lines[-1].split()[1],16))
        self.assertEqual(values,[11,13])

    def test_static_local_array_is_guest_backed(self):
        generated,_=d.prepare('u32 f(u32 i){static const u32 a[2]={7,9};return a[i&1];}',{})
        lines=host('u32 args[4]={1};printf("R %08x\\n",dt_entry(args));',generated=generated)
        self.assertEqual(int(lines[-1].split()[1],16),9)

    def test_divider_signed64_and_saturation_matches_native(self):
        cases=[(3,0,10),(-3,0,10),(1,0x7fffffff,0xffffffff),(0,0,7),(5,0xffffffff,0xfffffffa)]
        expected=[];body=''
        for divisor,hi,lo in cases:
            mem=d.DeviceMemory();mem.log=[]
            for at,v in [(0xffffff00,divisor),(0xffffff10,hi),(0xffffff14,lo)]:
                mem.write(at,4,v)
                body+='dt_write(0x%x,4,0x%x);'%(at,v&d.M32)
            expected.append(mem.read(0xffffff1c,4))
            body+='printf("R %08x\\n",dt_read(0xffffff1c,4));'
        lines=host(body,'#define DT_DEVICES 1\n')
        self.assertEqual([int(x.split()[1],16) for x in lines if x.startswith('R ')],expected)
        self.assertEqual(expected[:4],[3,0xfffffffd,0x7fffffff,0x7fffffff])

    def test_dma_busy_then_ready_and_unknown_device_stays_unmapped(self):
        mem=d.DeviceMemory();mem.log=[]
        self.assertEqual([mem.read(0x25fe007c,4) for _ in range(3)],[0x10010,0,0])
        self.assertEqual([mem.read(0xffffff8c,4) for _ in range(3)],[0,2,2])
        lines=host('for(int i=0;i<3;i++)dt_read(0x25fe007c,4);for(int i=0;i<3;i++)dt_read(0xffffff8c,4);','#define DT_DEVICES 1\n')
        got=[[int(x.split()[1],16),int(x.split()[2]),int(x.split()[3],16)] for x in lines]
        self.assertEqual(got,mem.reads)
        with self.assertRaises(d.sh2emu.MemoryError_):mem.read(0x25f80000,2)
        with self.assertRaises(d.sh2emu.MemoryError_):d.GuestMemory().read(0x25fe007c,4)
        ref=(0,{},[],[],mem.reads)
        self.assertEqual(d.difference(ref,(0,{},[],[],mem.reads[1:]),False)['kind'],'device read trace')

    def test_contract_files_are_explicit_and_bounded(self):
        for spec in d.CALLEE_ABIS.values():
            for kind in ('read','out'):
                for index,size in spec.get(kind,{}).items():
                    self.assertLess(int(index),32);self.assertGreater(size,0);self.assertLessEqual(size,256)
        self.assertNotIn(0x0604e518,d.ENTRY_ABIS)  # soft-float ABI still unmodelled


class V107AdditionalContracts(unittest.TestCase):
    def test_format_arity_comes_from_original_bytes(self):
        mem=d.GuestMemory();mem.add(d.RAM,0x100000)
        mem.write(d.STACK,4,0x06004000)
        for i,b in enumerate(b'%02x %% %d\0'):mem.write(0x06004000+i,1,b)
        self.assertEqual(d.format_words(mem,d.STACK,[1,2,3]),3)
        with self.assertRaises(d.Unsupported):d.format_words(mem,d.STACK,[1,2])
        mem.write(d.STACK,4,0x00280000)
        with self.assertRaises(d.Unsupported):d.format_words(mem,d.STACK,[3])

    def test_mutable_static_table_is_not_reinitialized_as_const(self):
        with self.assertRaisesRegex(d.Unsupported,'persistent state'):
            d.prepare('u32 f(void){static u32 a[2]={1,2};return ++a[0];}',{})

    def test_dispatcher_result_store_is_native_observable(self):
        target=0x06017d90;args=[2,0x00280000]
        expected=d.stub(target,args)
        lines=host('dt_callv(0x06017d90,2,(u32[]){2,0x00280000});printf("R %08x\\n",dt_read(0x00280020,4));', '#define DT_RESULT_STORES {0x06017d90,1,32,4}\n')
        self.assertEqual(int(lines[-1].split()[1],16),expected)

class V107WaitContracts(unittest.TestCase):
    def test_scripted_shared_ram_reads_match_native_without_mapping_devices(self):
        schedule={'060986af':{'size':1,'values':[0,0,1,1,0]}}
        mem=d.DeviceMemory(schedule,False);mem.add(d.RAM,0x100000);mem.log=[]
        expected=[mem.read(0x260986af,1) for _ in range(7)]
        lines=host('for(int i=0;i<7;i++)dt_read(0x260986af,1);', '#define DT_READ_SEQUENCES {0x060986af,1,5,{0,0,1,1,0},0}\n')
        self.assertEqual([int(x.split()[3],16) for x in lines],expected)
        with self.assertRaises(d.sh2emu.MemoryError_):mem.read(0x25fe007c,4)

    def test_dma_constant_r7_result_matches_original_bytes(self):
        mem=d.DeviceMemory();mem.add(d.RAM,0x100000,high_image());mem.add(0x00200000,0x100000)
        cpu=d.Oracle(mem,entry=0x06018678,call_specs={})
        cpu.r[15]=d.STACK
        cpu.call(0x06018678,r4=0,r5=0,r6=0,r7=0x12345678)
        self.assertEqual(cpu.r[7],d.CALLEE_ABIS[0x06018678]['constant_results']['7'])

class V107ScopeContracts(unittest.TestCase):
    def test_optional_particle_position_requires_flag_one(self):
        for flag in (0,2):
            args=[13,flag]
            self.assertEqual(d.payload_args(0x06040bb0,args,lambda a,n:self.fail('inactive position was read')),args)
        with self.assertRaisesRegex(d.Unsupported,'arity'):
            d.payload_args(0x06040bb0,[13,1],lambda a,n:0)

    def test_formatter_dummy_register_adapter_has_explicit_fifth_stack_word(self):
        spec=d.ENTRY_ABIS[0x06037122]['calls']['06027460']
        self.assertEqual(spec['args'],[4,5,6,7,'sp:0'])

class V107MemoryAddressContracts(unittest.TestCase):
    def test_memory_accessor_address_taking_keeps_guest_offset(self):
        generated,_=d.prepare('#include "dh.h"\nu32 f(void){return (u32)&M16(0x00280000,2);}',{})
        lines=host('u32 args[4]={0};printf("R %08x\\n",dt_entry(args));',generated=generated)
        self.assertEqual(int(lines[-1].split()[1],16),0x00280002)

if __name__=='__main__':unittest.main()
