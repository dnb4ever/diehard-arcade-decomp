import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import java.util.*;

public class DumpFns extends GhidraScript {
    @Override
    public void run() throws Exception {
        FunctionManager fm = currentProgram.getFunctionManager();
        println("RESULT FUNCTION_COUNT=" + fm.getFunctionCount());
        List<Function> fns = new ArrayList<>();
        for (Function f : fm.getFunctions(true)) fns.add(f);
        fns.sort(Comparator.comparingLong(f -> f.getBody().getNumAddresses()));
        println("RESULT SMALLEST_REAL_FUNCTIONS:");
        int c = 0;
        for (Function f : fns) {
            long sz = f.getBody().getNumAddresses();
            if (sz < 6) continue;
            println(String.format("RESULT   %s size=%d calls=%d %s",
                f.getEntryPoint(), sz, f.getCalledFunctions(monitor).size(), f.getName()));
            if (++c >= 15) break;
        }
    }
}
