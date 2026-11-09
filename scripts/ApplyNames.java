import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.symbol.SourceType;
import java.io.*;
import java.util.*;

// Reads notes/symbols.txt (lines: 0xADDR name  # comment) and renames the function at each
// SH2 address. Safe: skips missing functions, catches per-line errors. New names only.
public class ApplyNames extends GhidraScript {
    @Override public void run() throws Exception {
        File f = new File("notes/symbols.txt");
        if (!f.exists()) { println("RESULT ERROR symbols.txt not found at "+f.getAbsolutePath()); return; }
        BufferedReader br = new BufferedReader(new FileReader(f));
        String line; int renamed=0, missing=0, skipped=0;
        FunctionManager fm = currentProgram.getFunctionManager();
        while ((line = br.readLine()) != null) {
            int h = line.indexOf('#'); if (h>=0) line = line.substring(0,h);
            line = line.trim();
            if (line.isEmpty()) continue;
            String[] p = line.split("\\s+");
            if (p.length < 2 || !p[0].startsWith("0x")) { skipped++; continue; }
            long a;
            try { a = Long.parseLong(p[0].substring(2), 16); } catch(Exception e){ skipped++; continue; }
            Address addr = toAddr(a);
            Function fn = fm.getFunctionAt(addr);
            if (fn == null) { println("RESULT MISSING 0x"+Long.toHexString(a)+" "+p[1]); missing++; continue; }
            try {
                fn.setName(p[1], SourceType.USER_DEFINED);
                renamed++;
            } catch(Exception e) { println("RESULT FAIL 0x"+Long.toHexString(a)+" "+p[1]+" : "+e.getMessage()); skipped++; }
        }
        br.close();
        println("RESULT renamed="+renamed+" missing="+missing+" skipped="+skipped);
    }
}
