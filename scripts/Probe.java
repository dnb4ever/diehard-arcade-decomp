import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;
import java.util.*;

public class Probe extends GhidraScript {
  Memory mem;
  long rd(long a) throws Exception { return ((long)mem.getInt(toAddr(a)))&0xffffffffL; }
  @Override public void run() throws Exception {
    mem = currentProgram.getMemory();
    String[] args = getScriptArgs();
    PrintWriter pw = new PrintWriter(args.length>0?args[0]:"probe.txt");
    // mode lines in targets file: "V <addr>" dump 32-bit value; "C <fnaddr>" list callers
    BufferedReader br = new BufferedReader(new FileReader(args.length>1?args[1]:"probe_targets.txt"));
    String line;
    while((line=br.readLine())!=null){
      line=line.trim(); if(line.isEmpty()||line.startsWith("#"))continue;
      String[] p=line.split("\\s+");
      long a=Long.parseLong(p[1].replace("0x",""),16);
      if(p[0].equals("V")){
        long v=rd(a);
        pw.println("V 0x"+Long.toHexString(a)+" = 0x"+Long.toHexString(v));
      } else if(p[0].equals("C")){
        Address t=toAddr(a);
        ReferenceIterator it=currentProgram.getReferenceManager().getReferencesTo(t);
        pw.println("CALLERS of 0x"+Long.toHexString(a)+":");
        boolean any=false;
        while(it.hasNext()){Reference r=it.next();Function f=getFunctionContaining(r.getFromAddress());
          pw.println("  <- "+r.getFromAddress()+" "+r.getReferenceType()+" fn="+(f!=null?f.getEntryPoint()+"/"+f.getName():"?"));any=true;}
        if(!any)pw.println("  none");
      }
    }
    br.close(); pw.close();
    println("RESULT wrote probe");
  }
}
