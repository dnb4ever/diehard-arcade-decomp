import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.symbol.Reference;
import java.io.PrintWriter; import java.util.*;

public class FindLeaf2 extends GhidraScript {
    @Override public void run() throws Exception {
        DecompInterface di=new DecompInterface(); di.openProgram(currentProgram);
        FunctionManager fm=currentProgram.getFunctionManager();
        List<Function> cand=new ArrayList<>();
        for(Function f: fm.getFunctions(true)){
            long sz=f.getBody().getNumAddresses();
            if(sz<10||sz>52) continue;
            int inc=0;
            for(Reference r: getReferencesTo(f.getEntryPoint())) if(r.getReferenceType().isCall()) inc++;
            if(inc<1) continue;                              // must be really called
            boolean hasCall=false;
            InstructionIterator it=currentProgram.getListing().getInstructions(f.getBody(),true);
            while(it.hasNext()){ if(it.next().getFlowType().isCall()){hasCall=true;break;} }
            if(hasCall) continue;                            // leaf only (no relocs from calls)
            cand.add(f);
        }
        cand.sort(Comparator.comparingLong(f->f.getBody().getNumAddresses()));
        PrintWriter pw=new PrintWriter("leaf2.c");
        int n=0;
        for(Function f:cand){
            Address e=f.getEntryPoint(); long sz=f.getBody().getNumAddresses();
            byte[] b=getBytes(e,(int)sz); StringBuilder hx=new StringBuilder();
            for(byte x:b) hx.append(String.format("%02x",x&0xff));
            int inc=0; for(Reference r: getReferencesTo(e)) if(r.getReferenceType().isCall()) inc++;
            println(String.format("RESULT %s size=%d callers=%d bytes=%s", e, sz, inc, hx));
            pw.println("/* ==== "+e+"  size="+sz+"  callers="+inc+"  bytes="+hx+" ==== */");
            DecompileResults r=di.decompileFunction(f,60,monitor);
            pw.println((r!=null&&r.decompileCompleted())? r.getDecompiledFunction().getC():"// decompile failed");
            pw.println();
            if(++n>=12) break;
        }
        pw.close(); println("RESULT wrote leaf2.c count="+n);
    }
}
