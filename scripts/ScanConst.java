import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.listing.*;
import java.io.PrintWriter;

public class ScanConst extends GhidraScript {
  @Override public void run() throws Exception {
    PrintWriter pw = new PrintWriter("scan_const.txt");
    Memory mem = currentProgram.getMemory();
    long[] consts = {
      0x25f80000L,0x25f800bcL,0x25f800b2L,0x05f80000L,0x05f800bcL,
      0x25e00000L,0x25e5f000L,0x05e00000L,0x05e5f000L,
      0x0005f000L,0x0002f800L,0x25e5f048L,
      0x25c00000L,0x25d00000L, // vdp1 vram / regs
      0x05c00000L,0x05d00000L
    };
    MemoryBlock blk = mem.getBlock("ram");
    Address s = blk.getStart(), e = blk.getEnd();
    byte[] buf = new byte[(int)(e.getOffset()-s.getOffset()+1)];
    mem.getBytes(s, buf);
    for (long c : consts) {
      byte b0=(byte)((c>>24)&0xff), b1=(byte)((c>>16)&0xff), b2=(byte)((c>>8)&0xff), b3=(byte)(c&0xff);
      int hits=0;
      for (int i=0;i+3<buf.length;i++) {
        if (buf[i]==b0&&buf[i+1]==b1&&buf[i+2]==b2&&buf[i+3]==b3) {
          long at = s.getOffset()+i;
          pw.print("CONST 0x"+Long.toHexString(c)+" @ 0x"+Long.toHexString(at));
          // find references to this literal location (PC-rel loads)
          Address la = toAddr(at);
          ghidra.program.model.symbol.ReferenceIterator it = currentProgram.getReferenceManager().getReferencesTo(la);
          boolean any=false;
          while (it.hasNext()) {
            ghidra.program.model.symbol.Reference r = it.next();
            Function f = getFunctionContaining(r.getFromAddress());
            pw.print("  <- "+r.getFromAddress()+" fn="+(f!=null?f.getEntryPoint()+"/"+f.getName():"?"));
            any=true;
          }
          if(!any) pw.print("  (no xref)");
          pw.println();
          hits++;
        }
      }
      if (hits==0) pw.println("CONST 0x"+Long.toHexString(c)+" : NONE");
    }
    pw.close();
    println("RESULT wrote scan_const.txt");
  }
}
