import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.Memory;
import java.util.*;

// Broad scan:
//  (A) functions that touch SCU interrupt-mask/status (0x25fe00a0/a4/a8) or set VBR (ldc ...,VBR)
//      or load a pointer into the low vector-table area (0x06000000..0x06000400 / 0x00000000..0x400)
//  (B) callers + callees of frame_sync_loop (0x022013c4)
//  (C) functions containing braf (computed-goto / big switch) with size + globals touched
public class InvestigateEngine extends GhidraScript {

    long norm(long v){ return v & 0x07FFFFFFL; }

    @Override public void run() throws Exception {
        FunctionManager fm = currentProgram.getFunctionManager();
        Memory mem = currentProgram.getMemory();
        Listing l = currentProgram.getListing();

        println("=== (A) INTERRUPT / VBR / VECTOR scan ===");
        for (Function f : fm.getFunctions(true)) {
            if (monitor.isCancelled()) break;
            TreeSet<String> hits = new TreeSet<>();
            InstructionIterator it = l.getInstructions(f.getBody(), true);
            while (it.hasNext()) {
                Instruction insn = it.next();
                String m = insn.getMnemonicString();
                // VBR / SR control reg writes
                String rep = insn.toString();
                if (m.startsWith("ldc") && rep.contains("VBR")) hits.add("SET_VBR");
                if (m.startsWith("stc") && rep.contains("VBR")) hits.add("GET_VBR");
                // pool constants
                if (m.equals("mov.l")) {
                    for (Reference ref : insn.getReferencesFrom()) {
                        Address to = ref.getToAddress();
                        if (to != null && mem.contains(to)) {
                            try {
                                long val = ((long)mem.getInt(to)) & 0xffffffffL;
                                long p = norm(val);
                                if (p>=0x05fe0000L && p<0x05ff0000L) {
                                    long off = val & 0xffffL;
                                    if (off==0xa0 || off==0xa4 || off==0xa8 || off==0x80 || off==0x84 || off==0x88 || off==0x8c)
                                        hits.add("SCU_INTR@"+String.format("%08x",val));
                                }
                                // low vector table / BIOS vector region
                                if (p < 0x00001000L && val!=0) hits.add("VEC_LOW@"+String.format("%08x",val));
                                if (p>=0x06000000L && p<0x06000400L) hits.add("HWRAM_VEC@"+String.format("%08x",val));
                                if (p>=0x03000000L && p<0x03000010L) hits.add("BIOS_DISP@"+String.format("%08x",val));
                            } catch (Exception ex) {}
                        }
                    }
                }
            }
            if (!hits.isEmpty())
                println("A "+f.getEntryPoint()+" "+f.getName()+" sz="+f.getBody().getNumAddresses()+"  "+String.join(",",hits));
        }

        println("=== (B) frame_sync_loop 0x022013c4 call graph ===");
        Address fsl = toAddr(0x022013c4L);
        Function fslf = fm.getFunctionAt(fsl);
        if (fslf!=null){
            println("B CALLERS:");
            for (Reference r : getReferencesTo(fsl))
                if (r.getReferenceType().isCall()){
                    Function c = fm.getFunctionContaining(r.getFromAddress());
                    println("B   caller "+(c!=null?c.getEntryPoint()+" "+c.getName():"?")+" from "+r.getFromAddress());
                }
            println("B CALLEES:");
            for (Function c : fslf.getCalledFunctions(monitor))
                println("B   callee "+c.getEntryPoint()+" "+c.getName()+" sz="+c.getBody().getNumAddresses());
        } else println("B frame_sync_loop not a function!");

        println("=== (C) functions with braf (jump tables), size>=200 ===");
        List<Object[]> brafs = new ArrayList<>();
        for (Function f : fm.getFunctions(true)) {
            if (monitor.isCancelled()) break;
            int brafc=0;
            InstructionIterator it = l.getInstructions(f.getBody(), true);
            while (it.hasNext()){
                Instruction insn=it.next();
                if (insn.getMnemonicString().equals("braf")) brafc++;
            }
            long sz=f.getBody().getNumAddresses();
            if (brafc>0 && sz>=200) brafs.add(new Object[]{f,sz,brafc});
        }
        brafs.sort((a,b)->Long.compare((Long)b[1],(Long)a[1]));
        for (Object[] o : brafs){
            Function f=(Function)o[0];
            int indeg=0; for(Reference r:getReferencesTo(f.getEntryPoint())) if(r.getReferenceType().isCall()) indeg++;
            // HWRAM globals read (candidates for state var)
            TreeSet<String> globs=new TreeSet<>();
            InstructionIterator it=l.getInstructions(f.getBody(),true);
            while(it.hasNext()){
                Instruction insn=it.next();
                if(insn.getMnemonicString().equals("mov.l")){
                    for(Reference ref:insn.getReferencesFrom()){
                        Address to=ref.getToAddress();
                        if(to!=null&&mem.contains(to)){
                            try{ long val=((long)mem.getInt(to))&0xffffffffL; long p=norm(val);
                                if(p>=0x06000000L&&p<0x06100000L) globs.add(String.format("%08x",val));
                            }catch(Exception ex){}
                        }
                    }
                }
            }
            println("C "+f.getEntryPoint()+" "+f.getName()+" sz="+o[1]+" braf="+o[2]+" indeg="+indeg+" globals="+String.join(",",globs));
        }
        println("RESULT done");
    }
}
