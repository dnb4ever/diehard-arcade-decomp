import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.app.cmd.disassemble.DisassembleCommand;

public class SweepFns extends GhidraScript {
    @Override
    public void run() throws Exception {
        Address lo = toAddr(0x02200200L), hi = toAddr(0x0229ffffL);
        AddressSet set = new AddressSet(lo, hi);
        monitor.setMessage("disassembling code region");
        new DisassembleCommand(set, null, true).applyTo(currentProgram, monitor);

        Listing listing = currentProgram.getListing();
        java.util.LinkedHashSet<Address> entries = new java.util.LinkedHashSet<>();
        InstructionIterator it = listing.getInstructions(set, true);
        boolean afterTerminal = true;              // start of range counts as an entry
        while (it.hasNext() && !monitor.isCancelled()) {
            Instruction insn = it.next();
            if (afterTerminal && !insn.isInDelaySlot()) {
                entries.add(insn.getAddress());
                afterTerminal = false;
            }
            if (insn.getFlowType().isCall()) {
                for (Address t : insn.getFlows()) if (t != null) entries.add(t);
            }
            if (insn.getFlowType().isTerminal()) afterTerminal = true;
        }
        int created = 0;
        for (Address e : entries) {
            if (monitor.isCancelled()) break;
            if (getFunctionAt(e) != null) continue;
            if (createFunction(e, null) != null) created++;
        }
        println("RESULT candidate_entries=" + entries.size() + " created=" + created +
                " TOTAL_FUNCTIONS=" + currentProgram.getFunctionManager().getFunctionCount());
    }
}
