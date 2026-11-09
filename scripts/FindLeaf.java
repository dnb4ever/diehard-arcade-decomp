import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.Address;
import java.io.PrintWriter; import java.util.*;

public class FindLeaf extends GhidraScript {
    @Override public void run() throws Exception {
        DecompInterface di=new DecompInterface(); di.openProgram(currentProgram);
        FunctionManager fm=currentProgram.getFunctionManager();
        List<Function> cand=new ArrayList<>();
        for(Function f: fm.getFunctions(true)){
            long sz=f.getBody().getNumAddresses();
            if(sz<8||sz>48) continue;
            boolean hasCall=false;
            InstructionIterator it=currentProgram.getListing().getInstructions(f.getBody(),true);
            while(it.hasNext()){ if(it.next().getFlowType().isCall()){hasCall=true;break;} }
            if(!hasCall) cand.add(f);
        }
        cand.sort(Comparator.comparingLong(f->f.getBody().getNumAddresses()));
        PrintWriter pw=new PrintWriter("leaf_candidates.c");
        int n=0;
        for(Function f:cand){
            Address e=f.getEntryPoint(); long sz=f.getBody().getNumAddresses();
            byte[] b=getBytes(e,(int)sz);
            StringBuilder hx=new StringBuilder();
            for(byte x:b) hx.append(String.format("%02x",x&0xff));
            println(String.format("RESULT %s size=%d bytes=%s", e, sz, hx));
            pw.println("/* ==== "+e+"  size="+sz+"  bytes="+hx+" ==== */");
            DecompileResults r=di.decompileFunction(f,60,monitor);
            pw.println((r!=null&&r.decompileCompleted())? r.getDecompiledFunction().getC() : "// decompile failed");
            pw.println();
            if(++n>=10) break;
        }
        pw.close(); println("RESULT wrote leaf_candidates.c count="+n);
    }
}
