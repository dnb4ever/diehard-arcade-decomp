import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;

public class DisasmFn extends GhidraScript {
  @Override public void run() throws Exception {
    String[] args = getScriptArgs();
    long a = Long.parseLong(args[0].replace("0x",""),16);
    PrintWriter pw = new PrintWriter(args.length>1?args[1]:"disasm.txt");
    Function f = getFunctionContaining(toAddr(a));
    if(f==null){pw.println("no fn");pw.close();println("RESULT");return;}
    Listing lst = currentProgram.getListing();
    InstructionIterator it = lst.getInstructions(f.getBody(), true);
    while(it.hasNext()){
      Instruction in = it.next();
      StringBuilder sb=new StringBuilder();
      sb.append(in.getAddress()).append(":  ").append(in.toString());
      // annotate referenced data values
      Reference[] rs = in.getReferencesFrom();
      for(Reference r: rs){
        if(r.getReferenceType().isData()){
          Address t=r.getToAddress();
          try{ long v=((long)currentProgram.getMemory().getInt(t))&0xffffffffL;
            sb.append("   ; [").append(t).append("]=0x").append(Long.toHexString(v)); }catch(Exception e){}
        }
      }
      pw.println(sb.toString());
    }
    pw.close(); println("RESULT wrote disasm");
  }
}
