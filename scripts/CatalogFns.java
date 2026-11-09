import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.Memory;
import java.io.PrintWriter;
import java.util.*;

public class CatalogFns extends GhidraScript {

    String region(long v){
        long p = v & 0x07FFFFFFL;
        if(p>=0x00100000L && p<0x00100080L) return "SMPC";
        if(p>=0x05a00000L && p<0x05b00000L) return "SNDRAM";
        if(p>=0x05b00000L && p<0x05c00000L) return "SCSP";
        if(p>=0x05c00000L && p<0x05c80000L) return "VDP1VRAM";
        if(p>=0x05c80000L && p<0x05d00000L) return "VDP1FB";
        if(p>=0x05d00000L && p<0x05e00000L) return "VDP1REG";
        if(p>=0x05e00000L && p<0x05f00000L) return "VDP2VRAM";
        if(p>=0x05f00000L && p<0x05f80000L) return "VDP2CRAM";
        if(p>=0x05f80000L && p<0x05fe0000L) return "VDP2REG";
        if(p>=0x05fe0000L && p<0x05ff0000L) return "SCU";
        if(p>=0x06000000L && p<0x06100000L) return "HWRAM";
        return null;
    }

    @Override public void run() throws Exception {
        FunctionManager fm = currentProgram.getFunctionManager();
        Memory mem = currentProgram.getMemory();
        Listing l = currentProgram.getListing();
        PrintWriter pw = new PrintWriter("notes_catalog.tsv");
        pw.println("addr\tsize\tindeg\toutdeg\thasmul\thw\tregs\tname");

        // collect for top-indegree reporting
        List<String> rows = new ArrayList<>();
        List<long[]> indegList = new ArrayList<>(); // [addr, indeg]

        for (Function f : fm.getFunctions(true)) {
            if (monitor.isCancelled()) break;
            Address e = f.getEntryPoint();
            long sz = f.getBody().getNumAddresses();
            int indeg = 0;
            for (Reference r : getReferencesTo(e)) if (r.getReferenceType().isCall()) indeg++;
            int outdeg = f.getCalledFunctions(monitor).size();

            // scan instructions for pool constants + multiply
            TreeSet<String> regs = new TreeSet<>();
            TreeSet<String> hwconsts = new TreeSet<>();
            boolean hasmul = false;
            InstructionIterator it = l.getInstructions(f.getBody(), true);
            while (it.hasNext()) {
                Instruction insn = it.next();
                String m = insn.getMnemonicString();
                if (m.startsWith("mul") || m.startsWith("dmul")) hasmul = true;
                if (m.equals("mov.l")) {
                    for (Reference ref : insn.getReferencesFrom()) {
                        Address to = ref.getToAddress();
                        if (to != null && mem.contains(to)) {
                            try {
                                long val = ((long)mem.getInt(to)) & 0xffffffffL;
                                String rg = region(val);
                                if (rg != null) { regs.add(rg); hwconsts.add(rg+"@"+String.format("%08x",val)); }
                            } catch (Exception ex) {}
                        }
                    }
                }
            }
            String regStr = regs.isEmpty()? "-" : String.join(",", regs);
            String hwStr = hwconsts.isEmpty()? "-" : String.join(";", hwconsts);
            pw.println(e+"\t"+sz+"\t"+indeg+"\t"+outdeg+"\t"+(hasmul?1:0)+"\t"+regStr+"\t"+hwStr+"\t"+f.getName());
            indegList.add(new long[]{e.getOffset(), indeg, sz, outdeg});
        }
        pw.close();

        // print top-40 by indegree
        indegList.sort((a,b)->Long.compare(b[1],a[1]));
        println("RESULT TOP_INDEGREE:");
        for (int i=0;i<Math.min(40,indegList.size());i++){
            long[] r=indegList.get(i);
            println(String.format("RESULT  0x%08x indeg=%d size=%d outdeg=%d", r[0], r[1], r[2], r[3]));
        }
        // print largest functions
        indegList.sort((a,b)->Long.compare(b[2],a[2]));
        println("RESULT LARGEST_FUNCTIONS:");
        for (int i=0;i<Math.min(25,indegList.size());i++){
            long[] r=indegList.get(i);
            println(String.format("RESULT  0x%08x size=%d indeg=%d outdeg=%d", r[0], r[2], r[1], r[3]));
        }
        println("RESULT wrote notes_catalog.tsv functions="+fm.getFunctionCount());
    }
}
