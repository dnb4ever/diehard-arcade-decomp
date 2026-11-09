"""v1.08 contracts: physical provenance, lexical bindings and observable spans.
All fixtures are local; negative controls must retain a detectable difference.
"""
import importlib
import json
import os
import pathlib
import subprocess
import sys
import tempfile
import unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
d=importlib.import_module(os.environ.get('DIFFTEST_MODULE','difftest_v108'))


def host(source,body,defines=''):
    generated,_=d.prepare(source,{})
    with tempfile.TemporaryDirectory(prefix='dt108-test-') as tmp:
        p=pathlib.Path(tmp)
        (p/'x.c').write_text(defines+d.PRELUDE+d.HARNESS+generated+'\nint main(void){'+body+'}')
        subprocess.run(['gcc','-std=c11','-O1','-fwrapv',str(p/'x.c'),'-o',str(p/'x')],check=True,capture_output=True)
        return subprocess.run([str(p/'x')],check=True,capture_output=True,text=True).stdout.splitlines()


def high_image():
    high=bytearray(0x100000);high[0x4000:0x91e20]=d.cart_rom()[0x200000:0x28de20];return high


class V108Contracts(unittest.TestCase):
    def test_retry_bus_alias_matches_physical_fault_and_keeps_initial_source(self):
        log=[('r',0x06099000,4,0x286ff0c0)]
        self.assertEqual(d.retry_source(0x086ff110,[0,0,0,0],log,set()),('mem',0x06099000))
        self.assertEqual(d.retry_source(0x086ff110,[0x286ff0c0,0,0,0],[],set()),('arg',0))
        self.assertIsNone(d.retry_source(0x086ff110,[0,0,0,0],[('w',0x06099000,4,1)]+log,set()))
        self.assertIsNone(d.retry_source(0x086ff110,[0,0,0,0],[('r',0x06090294,4,0x286ff0c0)],set()))

    def test_retry_cannot_remap_mapped_pointer_or_program_bytes(self):
        self.assertIsNone(d.retry_source(0x06004020,[0x26004000,0,0,0],[],set()))
        self.assertFalse(d.seedable(0x26090294))
        self.assertFalse(d.seedable(d.STACK-4))

    def test_lexical_shadow_types_and_outer_pointer_mutation(self):
        source='u32 f(u32 *p){{u16 *p=(u16*)0x00281000;*p=9;}return *p;}'
        lines=host(source,'dt_write(0x00280000,4,0x12345678);u32 a[]={0x00280000};printf("R %08x\\n",dt_entry(a));')
        self.assertEqual(lines[-1],'R 12345678')
        changed=source.replace('return *p','return *(p+1)')
        lines=host(changed,'dt_write(0x00280000,4,0x12345678);u32 a[]={0x00280000};printf("R %08x\\n",dt_entry(a));')
        self.assertNotEqual(lines[-1],'R 12345678')

    def test_same_named_arrays_are_distinct_objects(self):
        source='u32 f(void){u32 out=0;{u32 a[2]={7,9};out=a[0];}{u16 a[2]={3,4};out+=a[1];}return out;}'
        self.assertEqual(host(source,'u32 a[4]={0};printf("R %08x\\n",dt_entry(a));')[-1],'R 0000000b')

    def test_array_constant_expression_uses_guest_extent(self):
        source='u32 f(void){u32 a[0x14/4]={1,2,3,4,5};return sizeof(a)+a[4];}'
        self.assertEqual(host(source,'u32 a[4]={0};printf("R %08x\\n",dt_entry(a));')[-1],'R 00000019')
        with self.assertRaisesRegex(d.Unsupported,'variable'):
            d.prepare('u32 f(u32 n){u32 a[n];return sizeof a;}',{})

    def test_address_taken_parameter_keeps_value_and_private_escape_rejected(self):
        source='u32 f(u32 p){u32 *saved=&p;*saved+=3;return p;}'
        self.assertEqual(host(source,'u32 a[4]={7};printf("R %08x\\n",dt_entry(a));')[-1],'R 0000000a')
        with self.assertRaisesRegex(d.Unsupported,'private stack address returned'):
            d.prepare('u32 f(u32 p){return (u32)&p;}',{})

    def test_long_is_guest32_and_long_long_stays64(self):
        source='u32 f(void){unsigned long v=0xffffffffUL;long n=-1L;return sizeof(v)*100+sizeof(n)*10+(u32)((1LL<<32)>>32)+(v+1);}'
        self.assertEqual(host(source,'u32 a[4]={0};printf("R %08x\\n",dt_entry(a));')[-1],'R 000001b9')
        self.assertEqual(host('u32 f(void){return 2147483648L > -1;}','u32 a[4]={0};printf("R %08x\\n",dt_entry(a));')[-1],'R 00000001')

    def test_sparse_bounds_output_preserves_middle_word(self):
        target=0x060119ec;p=d.FRAME[0]
        packed=d.payload_args(target,[1,p,p+16],lambda a,n:self.fail('output-only span was read'))
        self.assertEqual(packed,[1,0xf1000001,0xf1000002,12,12])
        source='u32 f(void){return 0;}'
        defines='#define DT_PAYLOADS {0x060119ec,1,12,2},{0x060119ec,2,12,2}\n#define DT_RESULT_STORES {0x060119ec,1,0,4},{0x060119ec,1,8,4},{0x060119ec,2,0,4},{0x060119ec,2,8,4}\n'
        lines=host(source,'dt_write(0x%x,4,0x12345678);dt_callv(0x060119ec,3,(u32[]){1,0x%x,0x%x});printf("R %%08x\\n",dt_read(0x%x,4));'%(p+4,p,p+16,p+4),defines)
        self.assertEqual(lines[-1],'R 12345678')

    def test_vector_output_fills_distinct_words(self):
        source='u32 f(void){return 0;}';p=d.FRAME[0]
        target=0x06018194;packed=d.payload_args(target,[1,2,p],lambda a,n:0);h=d.stub(target,packed)
        lines=host(source,'dt_callv(0x06018194,3,(u32[]){1,2,0x%x});for(int i=0;i<12;i+=4)printf("R %%08x\\n",dt_read(0x%x+i,4));'%(p,p),'#define DT_PAYLOADS {0x06018194,2,12,1}\n')
        expected=[(h ^ (2*0x9e3779b9) ^ (k*0x85ebca6b))&d.M32 for k in [0,4,8]]
        self.assertEqual([int(x.split()[1],16) for x in lines if x.startswith('R ')],expected)
        self.assertEqual(len(set(expected)),3)

    def test_caller_owned_stack_includes_crossing_write_bytes(self):
        at=d.STACK+32
        log=[('w',at-2,4,0x12345678),('w',d.STACK-4,4,7)]
        self.assertEqual(d.writes(log,[(at,4)]),{at:0x56,at+1:0x78})
        self.assertEqual(d.writes(log),{})
        self.assertEqual(d.write_trace(log,[(at,4)]),[(at,0x56),(at+1,0x78)])

    def test_caller_stack_snapshot_detects_unwritten_byte_mutation(self):
        a=(0,{},[],[],[],[[d.STACK+32,[1,2,3,4]]])
        b=(0,{},[],[],[],[[d.STACK+32,[1,2,3,5]]])
        self.assertEqual(d.difference(a,b)['kind'],'caller stack span')

    def test_real_transform_compares_writes_above_entry_stack(self):
        high=high_image();low=bytearray(0x100000);at=d.STACK+32
        ref=d.reference(0x06018194,[0x00280000,0,at,0],high,low,0x13579bdf,call_specs={},caller_spans=[(at,12)])
        self.assertEqual(set(ref[1]),set(range(at,at+12)))
        self.assertEqual(ref[5],[[at,[0]*12]])
        mutant=list(ref);mutant[1]=dict(ref[1]);mutant[1][at+8]^=1
        self.assertEqual(d.difference(ref,tuple(mutant),False)['kind'],'memory write')

    def test_image_contract_allows_only_callback_cells_and_compares_values(self):
        # Use prepared metadata for the real source's boundaries.
        source=(d.ROOT/'project/src_functional/02222e44.c').read_text()
        # Full integration mutation below does not require symbol discovery.
        symbols=json.loads((d.ROOT/'work/decomp_round2_20261108/symbols.json').read_text())
        options=dict(hints=d.load_hints(d.HINTS),retry_pointers=True,vary_stubs=True)
        result=d.test_one((0x02222e44,1,symbols,None,options));self.assertEqual(result['status'],'PASS-void',result)
        with tempfile.TemporaryDirectory() as tmp:
            p=pathlib.Path(tmp)/'bad.c';p.write_text(source.replace('0x06026ed6','0x06026ed8'))
            bad=d.test_one((0x02222e44,1,symbols,str(p),options))
        self.assertEqual(bad['status'],'FAIL',bad)
        self.assertEqual(bad['diff']['kind'],'memory write')
        hi=high_image();hi[0xa0000:0xa0006]=bytes.fromhex('2412000b0009')
        # Direct image store outside a declared entry remains unsupported.
        with self.assertRaisesRegex(d.Unsupported,'outside explicit contract'):
            d.reference(0x060a0000,[0x06090294,0x1234,0,0],hi,bytearray(0x100000),0,call_specs={})

    def test_entry_specific_payload_does_not_change_other_callers(self):
        self.assertEqual(d.active_callee_contracts(0x06008370)[0x06027080]['read'],{'0':3})
        self.assertNotIn(0x06027080,d.active_callee_contracts(0x06008374))
        a=[d.FRAME[0]]
        self.assertEqual(d.payload_args(0x06027080,a,lambda a,n:0),a)
        packed=d.payload_args(0x06027080,a,lambda a,n:9,{'read':{'0':3}})
        changed=d.payload_args(0x06027080,a,lambda a,n:8,{'read':{'0':3}})
        self.assertNotEqual(packed,changed)
        self.assertNotEqual(d.stub(0x06027080,packed),d.stub(0x06027080,changed))

    def test_captured_bios_words_and_manifest_are_reproducible(self):
        j=json.loads((d.ROOT/'work/decomp4/live_slots.json').read_text())
        b=(d.ROOT/j['source']).read_bytes()
        import hashlib
        self.assertEqual(hashlib.sha256(b).hexdigest(),j['sha256'])
        for a,value in j['slots'].items():
            at=136+int(a,16)-d.RAM
            self.assertEqual(int.from_bytes(b[at:at+4],'big'),int(value,16))

if __name__=='__main__':unittest.main()
