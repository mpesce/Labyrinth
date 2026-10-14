// Create functions at (a) pointers in data sections that target .text,
// (b) 16-byte aligned addresses following int3/nop padding in undefined gaps.
// @category Labyrinth
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.function.CreateFunctionCmd;
import ghidra.app.cmd.disassemble.DisassembleCommand;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.mem.*;
import java.util.*;

public class FindMoreFuncs extends GhidraScript {
    Memory mem;
    Listing listing;
    AddressSpace sp;
    long textStart = 0x401000L, textEnd = 0x492d62L;

    boolean inFunc(Address a) {
        return currentProgram.getFunctionManager().getFunctionContaining(a) != null;
    }

    boolean tryCreate(long va) throws Exception {
        Address a = sp.getAddress(va);
        if (inFunc(a)) return false;
        if (listing.getInstructionAt(a) == null) {
            if (listing.getDefinedDataContaining(a) != null) return false;
            DisassembleCommand dc = new DisassembleCommand(a, null, true);
            if (!dc.applyTo(currentProgram, monitor)) return false;
            if (listing.getInstructionAt(a) == null) return false;
        }
        CreateFunctionCmd cf = new CreateFunctionCmd(a);
        return cf.applyTo(currentProgram, monitor);
    }

    @Override
    public void run() throws Exception {
        mem = currentProgram.getMemory();
        listing = currentProgram.getListing();
        sp = currentProgram.getAddressFactory().getDefaultAddressSpace();
        int created = 0;
        for (int pass = 0; pass < 3; pass++) {
            int before = created;
            // (a) data pointers
            for (MemoryBlock b : mem.getBlocks()) {
                String n = b.getName();
                if (!(n.equals(".data") || n.equals(".rdata"))) continue;
                if (!b.isInitialized()) continue;
                long s = b.getStart().getOffset(), e = b.getEnd().getOffset();
                for (long va = s; va + 4 <= e; va += 4) {
                    int v = mem.getInt(sp.getAddress(va));
                    long uv = v & 0xffffffffL;
                    if (uv >= textStart && uv < textEnd) {
                        if (tryCreate(uv)) created++;
                    }
                }
            }
            // (b) padding-aligned starts
            for (long va = textStart; va < textEnd; va += 16) {
                Address a = sp.getAddress(va);
                if (inFunc(a)) continue;
                byte prev = mem.getByte(sp.getAddress(va - 1));
                byte cur = mem.getByte(a);
                if ((prev == (byte) 0xcc || prev == (byte) 0x90 || prev == (byte) 0xc3)
                        && cur != (byte) 0xcc && cur != 0) {
                    if (tryCreate(va)) created++;
                }
            }
            println("pass " + pass + " created " + (created - before));
            if (created == before) break;
        }
        println("FindMoreFuncs total created " + created);
    }
}
