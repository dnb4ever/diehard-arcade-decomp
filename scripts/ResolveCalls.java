import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.lang.Register;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.Memory;

public class ResolveCalls extends GhidraScript {
    @Override public void run() throws Exception {
        Listing l = currentProgram.getListing();
        Memory mem = currentProgram.getMemory();
        ReferenceManager rm = currentProgram.getReferenceManager();
        AddressSet range = new AddressSet(toAddr(0x02200200L), toAddr(0x0229ffffL));
        InstructionIterator it = l.getInstructions(range, true);
        int resolved = 0, created = 0;
        while (it.hasNext() && !monitor.isCancelled()) {
            Instruction insn = it.next();
            String m = insn.getMnemonicString();
            boolean isFar = m.equals("bsrf") || m.equals("braf");   // target = pc+4+disp
            boolean isInd = m.equals("jsr")  || m.equals("jmp");    // target = absolute value
            if (!(isFar || isInd)) continue;
            Register reg = null;
            for (Object o : insn.getOpObjects(0)) if (o instanceof Register) reg = (Register)o;
            if (reg == null) continue;
            Instruction p = insn.getPrevious();
            Long poolVal = null;
            for (int k=0; k<10 && p!=null; k++, p=p.getPrevious()) {
                boolean writes=false;
                for (Object r : p.getResultObjects()) if (r instanceof Register && ((Register)r).equals(reg)) writes=true;
                if (writes) {
                    if (p.getMnemonicString().equals("mov.l")) {
                        for (Reference ref : p.getReferencesFrom()) {
                            Address to = ref.getToAddress();
                            if (to!=null && mem.contains(to)) { try { poolVal=(long)mem.getInt(to); } catch(Exception e){} }
                        }
                    }
                    break;
                }
            }
            if (poolVal == null) continue;
            long tgt = isFar ? (insn.getAddress().getOffset() + 4 + (int)(long)poolVal) : (poolVal & 0xffffffffL);
            Address ta;
            try { ta = toAddr(tgt & 0xffffffffL); } catch(Exception e){ continue; }
            if (!range.contains(ta)) continue;
            RefType rt = isInd || m.equals("bsrf") ? RefType.COMPUTED_CALL : RefType.COMPUTED_JUMP;
            if (m.equals("jsr")||m.equals("bsrf")) rt=RefType.COMPUTED_CALL; else rt=RefType.COMPUTED_JUMP;
            try { rm.addMemoryReference(insn.getAddress(), ta, rt, SourceType.ANALYSIS, CodeUnit.MNEMONIC); resolved++; } catch(Exception e){}
            if (rt==RefType.COMPUTED_CALL && getFunctionAt(ta)==null) { try { if (createFunction(ta,null)!=null) created++; } catch(Exception e){} }
        }
        println("RESULT resolved_refs="+resolved+" new_functions="+created+" total="+currentProgram.getFunctionManager().getFunctionCount());
    }
}
