"""v1.06 memory aliases and initial-state setup contracts.
Run alongside test_difftest_v104.py (54 inherited regression checks).
"""
import json
import pathlib
import random
import sys
import unittest
from unittest.mock import patch
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
import difftest_v106 as d

class V106Contracts(unittest.TestCase):
    def test_hwram_mirrors_share_bytes_and_normalized_effects(self):
        mem=d.GuestMemory();mem.add(d.RAM,0x100000);mem.log=[]
        mem.write(0x063abcde,2,0x1234)
        self.assertEqual(mem.read(0x060abcde,2),0x1234)
        self.assertEqual(mem.read(0x27fabcde,2),0x1234)
        self.assertEqual(d.writes(mem.log),{0x060abcde:0x12,0x060abcdf:0x34})
        self.assertEqual(d.write_trace(mem.log),[(0x060abcde,0x12),(0x060abcdf,0x34)])
    def test_hwram_mirror_does_not_map_other_buses(self):
        mem=d.GuestMemory();mem.add(d.RAM,0x100000)
        for address in (0x08000000,0x05ffffff,0x25d00000):
            with self.assertRaises(d.sh2emu.MemoryError_):mem.read(address,2)
    def test_code_image_writes_detected_through_mirror(self):
        self.assertEqual(d.ram_address(0x26304000),0x06004000)
        self.assertFalse(d.seedable(0x26304000))
    def test_nested_fields_and_masked_globals_are_reproducible(self):
        hint={'args':{'0':{'fields':[{'offset':20,'size':4,'pointer':True,'fields':[{'offset':8,'size':4,'pointer':True,'fields':[{'offset':52,'size':1,'choices':[1,2]}]}]}]}},'globals':[{'address':'060987cc','size':4,'value':1,'mask':'00000003'}]}
        outputs=[]
        for _ in range(2):
            args=[0];high=bytearray(b'\xaa'*0x100000);low=bytearray(0x100000);setup=[]
            d.apply_hints(hint,args,high,low,random.Random(17),setup)
            parent=int.from_bytes(low[args[0]+20-0x200000:args[0]+24-0x200000],'big')
            actor=int.from_bytes(low[parent+8-0x200000:parent+12-0x200000],'big')
            self.assertIn(low[actor+52-0x200000],(1,2))
            self.assertEqual(int.from_bytes(high[0x987cc:0x987d0],'big'),0xaaaaaaa9)
            outputs.append((args,setup,high,low))
        self.assertEqual(outputs[0],outputs[1])
    def test_hints_cannot_overwrite_original_code(self):
        with self.assertRaises(d.Unsupported):
            d.apply_hints({'globals':[{'address':'06304000','size':4,'value':0}]},[],bytearray(0x100000),bytearray(0x100000),random.Random(1),[])
    def test_named_global_keeps_declared_width(self):
        code,_=d.prepare('extern short g_curActorObj; void func_02204000(void) { g_curActorObj=7; }',{},d.canonical(0x02204000))
        self.assertIn('0x609882c',code)
        self.assertIn(',2,',code)
    def test_native_reads_and_stores_same_hwram_alias(self):
        # Compile directly with the v1.06 harness (the inherited helper's module
        # is selectable and may intentionally be v1.04).
        import tempfile, subprocess
        with tempfile.TemporaryDirectory() as tmp:
            p=pathlib.Path(tmp);src=d.PRELUDE+d.HARNESS+'int main(void){dt_write(0x063a0000,4,0x11223344); return dt_read(0x27fa0000,4)!=0x11223344;}'
            (p/'x.c').write_text(src)
            subprocess.run(['gcc',str(p/'x.c'),'-o',str(p/'x')],check=True,capture_output=True)
            self.assertEqual(subprocess.run([str(p/'x')],capture_output=True).returncode,0)
    def test_stub_result_domains_match_reference_and_native(self):
        import tempfile, subprocess
        sys.path.insert(0,str(d.ROOT/'tools/saturn-recomp/src'))
        from saturnrecomp.recomp.selftest import Asm
        asm=Asm(0x06004000)
        asm.op('sts.l pr,@-Rn',n=15)
        asm.op('bsr @T',to='callee');asm.op('nop')
        asm.op('lds.l @Rm+,pr',m=15);asm.op('rts');asm.op('nop')
        asm.label('callee');asm.op('rts');asm.op('nop')
        code=asm.assemble();target=asm.labels['callee']
        expected=[]
        for salt in range(4):
            mem=d.GuestMemory();mem.add(0x06004000,len(code),code);mem.add(d.STACK-0x100,0x200)
            cpu=d.Oracle(mem,entry=0x06004000,call_specs={target:0},salt=salt,stub_values={target:[-1,0]})
            cpu.r[15]=d.STACK;cpu.call(0x06004000)
            expected.append(cpu.r[0])
        with tempfile.TemporaryDirectory() as tmp:
            p=pathlib.Path(tmp)
            program=d.PRELUDE+'#define DT_STUB_VALUES {0x%x,2,{0xffffffff,0}}\n' % target+d.HARNESS+'int main(void){for(dt_salt=0;dt_salt<4;dt_salt++) printf("R %%08x\\n",dt_callv(0x%x,0,0));}' % target
            (p/'x.c').write_text(program)
            subprocess.run(['gcc',str(p/'x.c'),'-o',str(p/'x')],check=True,capture_output=True)
            result=subprocess.run([str(p/'x')],check=True,capture_output=True,text=True)
            got=[int(line.split()[1],16) for line in result.stdout.splitlines() if line.startswith('R ')]
            self.assertEqual(got,expected)
            self.assertEqual(set(got),{0xffffffff,0})
if __name__=='__main__':unittest.main()
