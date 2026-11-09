import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.mem.Memory;

public class FindInit extends GhidraScript {
    @Override public void run() throws Exception {
        FunctionManager fm=currentProgram.getFunctionManager();
        Memory mem=currentProgram.getMemory();
        Listing l=currentProgram.getListing();
        // Which function (if any) contains 0x02201080? and nearest function starts around here
        for(long a=0x02200f00L;a<0x02201140L;a+=2){
            Function f=fm.getFunctionAt(toAddr(a));
            if(f!=null) println("FN start "+toAddr(a)+" "+f.getName()+" sz="+f.getBody().getNumAddresses());
        }
        Function c=fm.getFunctionContaining(toAddr(0x02201080L));
        println("contains(0x02201080)= "+(c!=null?c.getEntryPoint()+" "+c.getName():"<none>"));
        // callers of 0x02201080 region / who references into 0x022010xx
        for(long t=0x02200fa0L; t<0x02201080L; t+=2){
            for(Reference r:getReferencesTo(toAddr(t))){
                if(r.getReferenceType().isCall()||r.getReferenceType().isJump())
                    println("REF -> "+toAddr(t)+" from "+r.getFromAddress()+" "+r.getReferenceType());
            }
        }
        // disasm 0x02200fa0..0x02201086 to locate rts boundary / entry
        println("--- disasm 0x02200fc0..0x02201088 ---");
        Address a=toAddr(0x02200fc0L);
        while(a.getOffset()<0x02201088L){
            Instruction ins=l.getInstructionAt(a);
            if(ins==null){ println(a+" (data)"); a=a.add(2); continue; }
            println(a+"  "+ins);
            a=a.add(ins.getLength());
        }
        println("RESULT done");
    }
}
