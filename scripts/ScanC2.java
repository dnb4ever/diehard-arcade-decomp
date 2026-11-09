import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import java.io.*;

public class ScanC2 extends GhidraScript {
  @Override public void run() throws Exception {
    String[] args=getScriptArgs();
    PrintWriter pw=new PrintWriter(args.length>0?args[0]:"scan2.txt");
    Memory mem=currentProgram.getMemory();
    long[] consts={0x060b8b90L,0x060b89d0L,0x060b8960L,0x060b8b94L,0x060b8ab0L,0x060b8938L,0x060b89a8L};
    MemoryBlock blk=mem.getBlock("ram");
    byte[] buf=new byte[(int)(blk.getEnd().getOffset()-blk.getStart().getOffset()+1)];
    mem.getBytes(blk.getStart(),buf);
    long base=blk.getStart().getOffset();
    for(long c: consts){
      byte b0=(byte)((c>>24)&0xff),b1=(byte)((c>>16)&0xff),b2=(byte)((c>>8)&0xff),b3=(byte)(c&0xff);
      for(int i=0;i+3<buf.length;i++){
        if(buf[i]==b0&&buf[i+1]==b1&&buf[i+2]==b2&&buf[i+3]==b3){
          long at=base+i;
          pw.print("CONST 0x"+Long.toHexString(c)+" @ 0x"+Long.toHexString(at));
          ReferenceIterator it=currentProgram.getReferenceManager().getReferencesTo(toAddr(at));
          while(it.hasNext()){Reference r=it.next();Function f=getFunctionContaining(r.getFromAddress());
            pw.print("  <- "+r.getFromAddress()+" fn="+(f!=null?f.getEntryPoint()+"/"+f.getName():"?"));}
          pw.println();
        }
      }
    }
    pw.close();println("RESULT wrote scan2");
  }
}
