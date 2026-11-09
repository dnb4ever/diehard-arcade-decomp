import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.Address;
import java.io.PrintWriter;

public class DecompBatch extends GhidraScript {
    @Override public void run() throws Exception {
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        long[] addrs = {
            // high in-degree helpers (memcpy/memset/common)
            0x0224a360L,0x02215f6cL,0x022149e4L,0x0224a420L,0x0222dc2cL,0x02214a2eL,0x02214a50L,
            0x02210020L,0x02213190L,0x0220feceL,0x022131c8L,0x022149aaL,0x0220feecL,0x022145d0L,
            0x02213ca4L,0x02213474L,0x0222ec98L,0x0220ff28L,0x02213234L,0x022131a8L,0x02214a40L,
            0x02214a06L,0x022131d4L,0x02214bb8L,
            // SMPC
            0x0220acf6L,0x02214918L,0x02214944L,0x02214970L,0x02214980L,
            // VDP1
            0x02214cdcL,0x02214d26L,0x02214f90L,0x02223cf0L,0x02223d98L,0x02214daeL,0x022150d4L,
            // VDP2
            0x02201c9cL,0x02201f20L,0x02202138L,0x022027e4L,
            // SCU / DMA / timer
            0x0221478aL,0x0221482eL,0x022013c4L,0x02223dcaL,0x0223e5ccL,
            // sound
            0x02215da8L,0x02215e20L,0x02215f36L,
            // large / main-loop candidates
            0x02223ee0L,0x022339f0L,0x0220600eL,0x02211b04L,0x02211108L
        };
        PrintWriter pw = new PrintWriter("decomp_batch.c");
        for (long a : addrs) {
            Address e = toAddr(a);
            Function f = getFunctionAt(e);
            pw.println("/* ===================== 0x"+Long.toHexString(a)+"  "+(f!=null?f.getName():"<none>")+" ===================== */");
            if (f == null) { pw.println("// no function here\n"); continue; }
            DecompileResults r = di.decompileFunction(f, 90, monitor);
            if (r != null && r.decompileCompleted())
                pw.println(r.getDecompiledFunction().getC());
            else
                pw.println("// decompile failed: " + (r!=null?r.getErrorMessage():"null"));
            pw.println();
        }
        pw.close();
        println("RESULT wrote decomp_batch.c");
    }
}
