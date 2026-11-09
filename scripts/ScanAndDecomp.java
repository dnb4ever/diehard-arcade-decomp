import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.Memory;
import java.io.PrintWriter;
import java.util.*;

public class ScanAndDecomp extends GhidraScript {
    long norm(long v){ return v & 0x07FFFFFFL; }

    @Override public void run() throws Exception {
        FunctionManager fm = currentProgram.getFunctionManager();
        Memory mem = currentProgram.getMemory();
        Listing l = currentProgram.getListing();

        // (1) functions containing rte / rts-after-sr (interrupt/exception handler markers)
        println("=== (1) functions with rte (handler candidates) ===");
        for (Function f : fm.getFunctions(true)) {
            if (monitor.isCancelled()) break;
            boolean rte=false, vbr=false;
            InstructionIterator it = l.getInstructions(f.getBody(), true);
            while (it.hasNext()){
                Instruction insn=it.next();
                String m=insn.getMnemonicString();
                if (m.equals("rte")) rte=true;
                if (m.startsWith("ldc") && insn.toString().contains("VBR")) vbr=true;
                if (m.startsWith("stc") && insn.toString().contains("VBR")) vbr=true;
            }
            if (rte||vbr) println("1 "+f.getEntryPoint()+" "+f.getName()+" sz="+f.getBody().getNumAddresses()+(rte?" RTE":"")+(vbr?" VBR":""));
        }

        // (2) raw scan of whole program memory for SCU-mask / VBR-base / vector constants
        println("=== (2) raw pool-constant scan (whole image) ===");
        for (ghidra.program.model.mem.MemoryBlock blk : mem.getBlocks()){
            if(!blk.isInitialized()) continue;
            long start=blk.getStart().getOffset(), end=blk.getEnd().getOffset();
            for(long a=start; a+3<=end; a+=4){
                long val;
                try{ val=((long)mem.getInt(toAddr(a)))&0xffffffffL; }catch(Exception e){continue;}
                long p=norm(val);
                boolean interesting=false; String tag=null;
                if(p>=0x05fe0000L&&p<0x05ff0000L){ long off=val&0xffffL; if(off==0xa0||off==0xa4||off==0xa8){interesting=true;tag="SCU_MASK";} }
                if(val==0x06000000L||val==0x06000400L||val==0x00000000L){/*too common*/}
                if(p>=0x06000200L&&p<0x06000300L){interesting=true;tag="VEC06000_2xx";}
                if(interesting){
                    Function cf=fm.getFunctionContaining(toAddr(a));
                    println("2 pool@"+toAddr(a)+" = "+String.format("%08x",val)+" "+tag+" in="+(cf!=null?cf.getEntryPoint()+" "+cf.getName():"<none>"));
                }
            }
        }

        // (3) caller chain of frame_sync_loop: resolve the function containing 0x022010fe
        println("=== (3) context around frame_sync_loop caller 0x022010fe ===");
        Function cfn = fm.getFunctionContaining(toAddr(0x022010feL));
        println("3 containing = "+(cfn!=null?cfn.getEntryPoint()+" "+cfn.getName()+" sz="+cfn.getBody().getNumAddresses():"<none>"));

        // (4) callers of giant FUN_02223ee0 and dispatch tables
        println("=== (4) callers of key candidates ===");
        long[] keys={0x02223ee0L,0x022339f0L,0x0224a360L,0x0224a420L,0x02209188L};
        for(long k:keys){
            Address ka=toAddr(k);
            Function kf=fm.getFunctionAt(ka);
            StringBuilder sb=new StringBuilder("4 "+ka+" "+(kf!=null?kf.getName():"?")+" callers:");
            for(Reference r:getReferencesTo(ka)){
                Function c=fm.getFunctionContaining(r.getFromAddress());
                sb.append(" ["+r.getReferenceType()+" "+(c!=null?c.getEntryPoint():r.getFromAddress())+"]");
            }
            println(sb.toString());
        }

        // (5) decompile batch -> file
        long[] decs={
            0x022010a0L, // guess start of fn containing 0x022010fe (will fix if wrong)
            0x022013c4L, // frame_sync_loop
            0x02223ee0L, // giant braf - master state machine candidate
            0x0224a360L, // dispatch_table_a
            0x0224a420L, // dispatch_table_b
            0x02201958L, // 06000248 cluster
            0x0221a1a0L, // 06000248 cluster
            0x02209188L  // reads 06000248 + many HW
        };
        DecompInterface di=new DecompInterface();
        di.openProgram(currentProgram);
        PrintWriter pw=new PrintWriter("work/decomp_engine.c");
        // also include the real containing function of 0x022010fe
        TreeSet<Long> toDec=new TreeSet<>();
        for(long d:decs) toDec.add(d);
        if(cfn!=null) toDec.add(cfn.getEntryPoint().getOffset());
        for(long a:toDec){
            Address e=toAddr(a);
            Function f=fm.getFunctionAt(e);
            pw.println("/* ===== 0x"+Long.toHexString(a)+"  "+(f!=null?f.getName():"<none>")+" ===== */");
            if(f==null){ pw.println("// no function here\n"); continue; }
            DecompileResults r=di.decompileFunction(f,120,monitor);
            if(r!=null&&r.decompileCompleted()) pw.println(r.getDecompiledFunction().getC());
            else pw.println("// decompile failed");
            pw.println();
        }
        pw.close();
        println("RESULT wrote work/decomp_engine.c");
    }
}
