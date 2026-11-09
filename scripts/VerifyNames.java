import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.Address;
public class VerifyNames extends GhidraScript {
  @Override public void run() throws Exception {
    long[] a={0x02201020L,0x02223cf0L,0x02223d98L,0x02223dcaL,0x0220fde4L,0x0222ece4L,0x022225f6L,0x022227c4L,0x02223ee0L,0x0224a360L,0x0224a420L};
    for(long x:a){ Function f=getFunctionAt(toAddr(x)); println("V 0x"+Long.toHexString(x)+" = "+(f!=null?f.getName():"<none>")); }
  }
}
