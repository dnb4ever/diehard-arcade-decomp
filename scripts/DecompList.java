import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.Address;
import java.io.*;
import java.util.*;

public class DecompList extends GhidraScript {
  @Override public void run() throws Exception {
    String[] args = getScriptArgs();
    String inFile = args.length>0?args[0]:"decomp_targets.txt";
    String outFile = args.length>1?args[1]:"decomp_out.c";
    List<Long> addrs = new ArrayList<>();
    BufferedReader br = new BufferedReader(new FileReader(inFile));
    String line;
    while ((line=br.readLine())!=null){ line=line.trim(); if(line.isEmpty()||line.startsWith("#"))continue; addrs.add(Long.parseLong(line.replace("0x",""),16)); }
    br.close();
    DecompInterface di = new DecompInterface();
    di.openProgram(currentProgram);
    PrintWriter pw = new PrintWriter(outFile);
    for (long a: addrs){
      Address e = toAddr(a);
      Function f = getFunctionAt(e);
      if (f==null) f = getFunctionContaining(e);
      pw.println("/* ===================== 0x"+Long.toHexString(a)+"  "+(f!=null?f.getName():"<none>")+" ===================== */");
      if (f==null){ pw.println("// no function\n"); continue; }
      DecompileResults r = di.decompileFunction(f,120,monitor);
      if (r!=null && r.decompileCompleted()) pw.println(r.getDecompiledFunction().getC());
      else pw.println("// decompile failed: "+(r!=null?r.getErrorMessage():"null"));
      pw.println();
    }
    pw.close();
    println("RESULT wrote "+outFile);
  }
}
