import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.Address;
import java.io.PrintWriter;

public class DecompileFn extends GhidraScript {
    @Override
    public void run() throws Exception {
        DecompInterface di = new DecompInterface();
        di.openProgram(currentProgram);
        long[] addrs = {0x0220feecL, 0x02214a06L, 0x0220ff0aL};
        PrintWriter pw = new PrintWriter("decomp_sample.c");
        for (long a : addrs) {
            Address e = toAddr(a);
            Function f = getFunctionAt(e);
            pw.println("/* ===== " + e + "  " + (f!=null?f.getName():"<none>") + " ===== */");
            if (f == null) { pw.println("// no function here\n"); continue; }
            DecompileResults r = di.decompileFunction(f, 60, monitor);
            if (r != null && r.decompileCompleted())
                pw.println(r.getDecompiledFunction().getC());
            else
                pw.println("// decompile failed: " + (r!=null?r.getErrorMessage():"null"));
            pw.println();
        }
        pw.close();
        println("RESULT wrote decomp_sample.c");
    }
}
