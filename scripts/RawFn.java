import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.Address;
import ghidra.app.cmd.function.CreateFunctionCmd;
import java.io.*;

public class RawFn extends GhidraScript {
  @Override public void run() throws Exception {
    String[] args = getScriptArgs();
    long a = Long.parseLong(args[0].replace("0x",""),16);
    PrintWriter pw = new PrintWriter(args.length>1?args[1]:"/tmp/ghwork/raw.c");
    Address e = toAddr(a);
    Function f = getFunctionAt(e);
    if (f==null) f = getFunctionContaining(e);
    if (f==null) {
      // ensure disassembled
      if (getInstructionAt(e)==null) disassemble(e);
      CreateFunctionCmd cmd = new CreateFunctionCmd(e);
      cmd.applyTo(currentProgram, monitor);
      f = getFunctionAt(e);
    }
    if (f==null){ pw.println("// could not create fn at 0x"+Long.toHexString(a)); pw.close(); println("RESULT"); return; }
    pw.println("/* "+f.getName()+" @ "+f.getEntryPoint()+" */");
    DecompInterface di = new DecompInterface();
    di.openProgram(currentProgram);
    DecompileResults r = di.decompileFunction(f,120,monitor);
    if (r!=null && r.decompileCompleted()) pw.println(r.getDecompiledFunction().getC());
    else pw.println("// decompile failed: "+(r!=null?r.getErrorMessage():"null"));
    pw.close(); println("RESULT wrote "+args[1]);
  }
}
