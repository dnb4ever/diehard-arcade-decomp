import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.Memory;
import java.io.PrintWriter;

public class ResolveHandlers extends GhidraScript {
    long rd(long a) throws Exception { return ((long)currentProgram.getMemory().getInt(toAddr(a)))&0xffffffffL; }
    @Override public void run() throws Exception {
        FunctionManager fm=currentProgram.getFunctionManager();
        // pool values
        long[] pools={0x02201144L,0x02201148L,0x0220114cL,0x02201150L,0x02201154L,0x02201158L,0x0220115cL,0x02201160L,0x02201164L,0x02201168L,0x0220116cL,0x02201170L,0x02201174L,0x02201178L,0x0220117cL,0x02201180L};
        for(long p:pools) println("POOL "+toAddr(p)+" = "+String.format("%08x",rd(p)));
        // handler addresses = pool + mova-anchor
        long h40 = rd(0x02201154L) + 0x02201050L;
        long h41 = rd(0x02201158L) + 0x0220105cL;
        long h43 = rd(0x0220115cL) + 0x02201068L;
        println("HANDLER vec0x40 VBlankIN  = "+String.format("%08x",h40&0xffffffffL));
        println("HANDLER vec0x41 VBlankOUT = "+String.format("%08x",h41&0xffffffffL));
        println("HANDLER vec0x43 Timer0    = "+String.format("%08x",h43&0xffffffffL));
        // BIOS set-func pointer
        println("BIOS setfunc tbl ptr @ "+String.format("%08x",rd(0x02201150L)));

        // ensure functions exist at handlers, then decompile
        DecompInterface di=new DecompInterface(); di.openProgram(currentProgram);
        PrintWriter pw=new PrintWriter("work/decomp_handlers.c");
        long[] hs={h40&0xffffffffL,h41&0xffffffffL,h43&0xffffffffL,0x02201020L};
        for(long a:hs){
            Address e=toAddr(a);
            Function f=fm.getFunctionAt(e);
            if(f==null){ try{ f=createFunction(e,null);}catch(Exception ex){} }
            pw.println("/* ===== 0x"+Long.toHexString(a)+"  "+(f!=null?f.getName():"<none>")+" ===== */");
            if(f==null){ pw.println("// could not create fn\n"); continue; }
            DecompileResults r=di.decompileFunction(f,120,monitor);
            pw.println(r!=null&&r.decompileCompleted()? r.getDecompiledFunction().getC():"// fail");
            pw.println();
        }
        pw.close();
        println("RESULT wrote work/decomp_handlers.c");
    }
}
