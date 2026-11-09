import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.Address;
import java.io.PrintWriter;

public class DecompBatch2 extends GhidraScript {
    @Override public void run() throws Exception {
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        long[] addrs = {
            0x02213290L,0x0224840aL,0x02245fc4L,0x0224518eL,0x022132acL,0x02246208L,
            0x022132c4L,0x022148acL,0x022473daL,0x022470a2L,0x02244e2eL,0x0220f816L,
            0x0222a5d0L,0x02223e70L,0x0222a708L,0x022227c4L,0x02214a60L,0x022149b6L
        };
        PrintWriter pw = new PrintWriter("decomp_batch2.c");
        for (long a : addrs) {
            Address e = toAddr(a);
            Function f = getFunctionAt(e);
            pw.println("/* ===================== 0x"+Long.toHexString(a)+"  "+(f!=null?f.getName():"<none>")+" ===================== */");
            if (f == null) { pw.println("// no function here\n"); continue; }
            DecompileResults r = di.decompileFunction(f, 90, monitor);
            if (r != null && r.decompileCompleted())
                pw.println(r.getDecompiledFunction().getC());
            else
                pw.println("// decompile failed");
            pw.println();
        }
        pw.close();
        println("RESULT wrote decomp_batch2.c");
    }
}
