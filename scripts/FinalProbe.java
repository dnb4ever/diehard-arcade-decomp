import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.Memory;
import java.io.PrintWriter;
import java.util.*;

public class FinalProbe extends GhidraScript {
    @Override public void run() throws Exception {
        FunctionManager fm = currentProgram.getFunctionManager();
        Memory mem = currentProgram.getMemory();
        Listing l = currentProgram.getListing();

        // (1) disassembly around frame_sync_loop caller 0x022010fe
        println("=== (1) disasm 0x02201080..0x02201140 ===");
        Address a=toAddr(0x02201080L);
        while(a.getOffset()<0x02201140L){
            Instruction ins=l.getInstructionAt(a);
            if(ins==null){ println("1 "+a+" (no insn)"); a=a.add(2); continue; }
            String pool="";
            for(Reference r:ins.getReferencesFrom()){ Address to=r.getToAddress();
                if(to!=null&&mem.contains(to)){ try{ pool=" ; ->"+String.format("%08x",((long)mem.getInt(to))&0xffffffffL);}catch(Exception e){} } }
            println("1 "+a+"  "+ins+pool);
            a=a.add(ins.getLength());
        }

        // (2) rte stub characterization: raw bytes of 3 stubs + whether identical
        println("=== (2) rte stubs ===");
        long[] stubs={0x022848f8L,0x02284a34L,0x02284b70L,0x02286c5cL};
        for(long s:stubs){
            StringBuilder sb=new StringBuilder("2 "+String.format("%08x",s)+":");
            for(int i=0;i<28;i++){ try{ sb.append(String.format(" %02x",mem.getByte(toAddr(s+i))&0xff)); }catch(Exception e){} }
            println(sb.toString());
        }
        println("2 -- disasm of 0x022848f8 --");
        a=toAddr(0x022848f8L);
        for(int i=0;i<14 && a.getOffset()<0x02284914L;i++){
            Instruction ins=l.getInstructionAt(a); if(ins==null)break;
            String pool="";
            for(Reference r:ins.getReferencesFrom()){ Address to=r.getToAddress();
                if(to!=null&&mem.contains(to)){ try{ pool=" ; ->"+String.format("%08x",((long)mem.getInt(to))&0xffffffffL);}catch(Exception e){} } }
            println("2 "+a+"  "+ins+pool); a=a.add(ins.getLength());
        }

        // (3) dispatch_table_a/b jump-table bytes + handler targets
        println("=== (3) dispatch tables ===");
        // dispatch_table_a: base DAT_0224a37c, byte table DAT_0224a380. Pool consts live right after fn.
        // Read the two pool longs after the fn code to get base+table addrs.
        for(long poolscan=0x0224a398L; poolscan<0x0224a3c0L; poolscan+=4){
            try{ long v=((long)mem.getInt(toAddr(poolscan)))&0xffffffffL; println("3 A pool@"+String.format("%08x",poolscan)+" = "+String.format("%08x",v)); }catch(Exception e){}
        }
        for(long poolscan=0x0224a460L; poolscan<0x0224a490L; poolscan+=4){
            try{ long v=((long)mem.getInt(toAddr(poolscan)))&0xffffffffL; println("3 B pool@"+String.format("%08x",poolscan)+" = "+String.format("%08x",v)); }catch(Exception e){}
        }

        // (4) decompile batch: caller region fn, per-frame callees, dispatch callers
        long[] decs={0x022227c4L,0x022225f6L,0x0220fde4L,0x0222ece4L,0x0221279cL,0x0222156cL};
        DecompInterface di=new DecompInterface(); di.openProgram(currentProgram);
        PrintWriter pw=new PrintWriter("work/decomp_probe.c");
        for(long adr:decs){
            Address e=toAddr(adr); Function f=fm.getFunctionAt(e);
            pw.println("/* ===== 0x"+Long.toHexString(adr)+"  "+(f!=null?f.getName():"<none>")+" ===== */");
            if(f==null){ pw.println("// none\n"); continue; }
            DecompileResults r=di.decompileFunction(f,120,monitor);
            pw.println(r!=null&&r.decompileCompleted()? r.getDecompiledFunction().getC():"// fail");
            pw.println();
        }
        pw.close();
        println("RESULT wrote work/decomp_probe.c");
    }
}
