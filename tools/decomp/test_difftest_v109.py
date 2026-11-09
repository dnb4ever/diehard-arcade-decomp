"""v1.09: corrected source bindings, mutable data and exact block-copy contracts."""
import importlib,json,os,pathlib,random,subprocess,sys,tempfile,unittest
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parent))
d=importlib.import_module(os.environ.get('DIFFTEST_MODULE','difftest_v109'))

class V109Contracts(unittest.TestCase):
    def test_source_bindings_name_real_entries_and_leave_canonical_addresses_exact(self):
        cases={0x06013c16:0x06013c18,0x06017570:0x060175a0,0x060208f4:0x060208f2,0x0602dbce:0x0602dbca,0x0604a3c8:0x0604a3b8}
        for old,new in cases.items():
            self.assertEqual(d.source_entry(old),new)
            self.assertEqual(d.canonical(old),old)
            self.assertEqual(d.function_address('func_%08x'%old,{}),new)
        self.assertEqual(d.source_entry(0x02001b8e),0x02001b8e)
        self.assertEqual(d.source_entry(0x06044444),0x06044444)

    def test_actor_is_outside_the_inline_runtime_range(self):
        self.assertLess(0x0604e000,d.RUNTIME[0])
        self.assertEqual(d.RUNTIME[0],0x0604e06c)
        hi=bytearray(0x100000);hi[0x4000:0x91e20]=d.cart_rom()[0x200000:0x28de20]
        # Ordinary actor entry calls a service at 0604c9d0 before its animation boundary.
        lo=bytearray(0x100000);lo[0x1000+0x4c]=1
        ref=d.reference(0x0604e000,[0x00200000,0x00201000,0,0],hi,lo,0x13579bdf,call_specs={0x0604c9d0:1,0x06031c2c:2})
        self.assertEqual([c[0] for c in ref[2]],[0x0604c9d0,0x06031c2c])

    def test_image_input_permission_is_entry_scoped_and_bounded(self):
        def apply(entry,address,size=4):
            hi=bytearray(0x100000);lo=bytearray(0x100000)
            d.apply_hints({'globals':[{'address':'%08x'%address,'size':size,'value':7}]},[0]*4,hi,lo,random.Random(0),[],entry)
            return hi
        self.assertEqual(apply(0x060183bc,0x06091e14)[0x91e14:0x91e18],b'\0\0\0\7')
        self.assertEqual(apply(0x0600dd70,0x2609029c)[0x9029c:0x902a0],b'\0\0\0\7')
        for entry,address,size in [(None,0x06091e14,4),(0x060183bc,0x06091e10,4),(0x060183bc,0x06091e1e,4),(0x0600dd70,0x06090298,4),(0x060183bc,0x060143bc,4)]:
            # A crossing store out of the allowed span into nonimage RAM is also rejected.
            with self.assertRaises(d.Unsupported):apply(entry,address,size)

    def test_action_out_word_and_side_input_mutations_are_observed(self):
        spec=d.entry_abi(0x060494e4)['calls']['06049632'];p=d.FRAME[0]
        a=d.payload_args(0x06049632,[0x00281000,p,p+4],lambda a,n:0,spec)
        b=d.payload_args(0x06049632,[0x00281000,p,p+4],lambda a,n:1,spec)
        self.assertNotEqual(a,b)
        self.assertNotEqual(d.stub(0x06049632,a),d.stub(0x06049632,b))
        for entry in [0x06043fcc,0x06044054,0x060441d6]:
            self.assertEqual(d.entry_abi(entry)['calls']['060442c0']['out'],{'1':4})
        # Caller consumption must differ if the returned output word changes.
        ref=(5,{0x00281000:5},[],[(0x00281000,5)])
        self.assertIsNotNone(d.difference(ref,(6,{0x00281000:6},[],[(0x00281000,6)])))

    def test_runtime_copy_matches_original_order_with_overlap_and_mutation(self):
        for n,delta in [(0,4),(4,4),(16,4),(64,4),(68,4),(108,4),(108,-4)]:
            src=0x00281000;dst=src+delta
            hi=bytearray(0x100000);hi[0x4000:0x91e20]=d.cart_rom()[0x200000:0x28de20]
            lo=bytearray((i*13+3)&255 for i in range(0x100000))
            ref=d.reference(d.COPY_RUNTIME,[n,dst,src,0],hi,lo,0x13579bdf,call_specs={})
            generated,_=d.prepare('extern void shc_copy_word_block(u32 n,void*d,void*s);void f(u32 n,void*d,void*s){shc_copy_word_block(n,d,s);}',{'shc_copy_word_block':d.COPY_RUNTIME})
            with tempfile.TemporaryDirectory(prefix='dt109-copy-') as tmp:
                p=pathlib.Path(tmp);body='for(u32 i=0;i<0x100000;i++)low[i]=(i*13+3)&255;u32 a[]={%d,0x%x,0x%x};dt_entry(a);'%(n,dst,src)
                (p/'t.c').write_text(d.PRELUDE+d.HARNESS+generated+'int main(void){'+body+'}')
                subprocess.run(['gcc','-std=c11','-O1',str(p/'t.c'),'-o',str(p/'t')],check=True,capture_output=True)
                lines=subprocess.run([str(p/'t')],check=True,text=True,capture_output=True).stdout.splitlines()
            log=[('w',int(z[1],16),int(z[2]),int(z[3],16)) for z in (line.split() for line in lines) if z[0]=='W']
            self.assertEqual(d.write_trace(log),ref[3])
            self.assertEqual(d.writes(log),ref[1])
            if n:
                changed=list(log);kind,a,size,v=changed[0];changed[0]=(kind,a,size,v^1)
                self.assertNotEqual(d.write_trace(changed),ref[3])

    def test_purge_leaf_sets_zero_r5_even_when_incoming_length_is_nonzero(self):
        hi=bytearray(0x100000);hi[0x4000:0x91e20]=d.cart_rom()[0x200000:0x28de20]
        mem=d.GuestMemory();mem.add(d.RAM,len(hi),hi);mem.log=[]
        cpu=d.Oracle(mem,call_specs={});cpu.r[15]=d.STACK
        # The wrapped endpoint skips stores but executes the branch delay slot.
        cpu.call(0x060188ac,r4=0x06099584,r5=0xffffffff)
        self.assertEqual(cpu.r[5],0)
        self.assertEqual(d.writes(mem.log),{})
        self.assertEqual(d.entry_abi(0x0602bca8)['args'],[4,5])
        self.assertEqual(d.entry_abi(0x0602bca8)['calls']['060188ac']['constant_results'],{'5':0})

    def test_runtime_copy_rejects_undeclared_arity(self):
        with self.assertRaisesRegex(d.Unsupported,'requires count'):
            d.prepare('extern void shc_copy_word_block(u32 n,void*d);void f(void){shc_copy_word_block(4,(void*)0x280000);}',{'shc_copy_word_block':d.COPY_RUNTIME})

if __name__=='__main__':unittest.main()
