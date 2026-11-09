import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.scalar.Scalar;
import java.io.PrintWriter;
import java.util.*;

public class RotRecon extends GhidraScript {
  PrintWriter pw;
  void refsTo(long addr, String tag) throws Exception {
    Address t = toAddr(addr);
    ReferenceManager rm = currentProgram.getReferenceManager();
    ReferenceIterator it = rm.getReferencesTo(t);
    int n=0;
    while (it.hasNext()) {
      Reference r = it.next();
      Address from = r.getFromAddress();
      Function f = getFunctionContaining(from);
      pw.println(tag+" 0x"+Long.toHexString(addr)+" <- "+from+"  "+r.getReferenceType()+"  fn="+(f!=null?f.getEntryPoint()+" "+f.getName():"<none>"));
      n++;
    }
    if (n==0) pw.println(tag+" 0x"+Long.toHexString(addr)+" : no refs");
  }
  @Override public void run() throws Exception {
    pw = new PrintWriter("rot_recon.txt");
    pw.println("=== MEMORY BLOCKS ===");
    for (MemoryBlock b : currentProgram.getMemory().getBlocks())
      pw.println(b.getName()+"  "+b.getStart()+" - "+b.getEnd()+"  r="+b.isRead()+" w="+b.isWrite()+" x="+b.isExecute()+" init="+b.isInitialized());
    pw.println();
    pw.println("=== REFS TO VDP2 ROT REGISTERS / VRAM ===");
    // VDP2 regs @0x25F80000 ; RPTAU=0xBC RPTAL=0xBE ; also try 0x05F80000 mirror + 0x25E00000 VRAM
    long[] regs = {0x25f800bcL,0x25f800beL,0x05f800bcL,0x05f800beL,0x25f800b2L,0x25f800b4L};
    for (long a: regs) refsTo(a,"REG");
    // rot table VRAM 0x5F000 region, SH2 0x25E5F000, + mirror 0x05E5F000, + the My field 0x5F048
    long[] vram = {0x25e5f000L,0x25e5f048L,0x05e5f000L,0x05e5f048L,0x25e00000L};
    for (long a: vram) refsTo(a,"VRAM");
    pw.close();
    println("RESULT wrote rot_recon.txt");
  }
}
