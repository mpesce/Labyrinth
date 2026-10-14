// Headless export: functions, call graph, string refs, decompiled C.
// @category Labyrinth
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.data.*;
import ghidra.util.task.ConsoleTaskMonitor;
import java.io.*;
import java.util.*;

public class ExportAll extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        File outDir = new File(args.length > 0 ? args[0] : "out");
        boolean doDecomp = args.length < 2 || !args[1].equals("nodecomp");
        outDir.mkdirs();
        Listing listing = currentProgram.getListing();
        FunctionManager fm = currentProgram.getFunctionManager();
        ReferenceManager rm = currentProgram.getReferenceManager();

        PrintWriter fpw = new PrintWriter(new FileWriter(new File(outDir, "functions.tsv")));
        PrintWriter cpw = new PrintWriter(new FileWriter(new File(outDir, "calls.tsv")));
        PrintWriter spw = new PrintWriter(new FileWriter(new File(outDir, "strrefs.tsv")));
        PrintWriter dpw = new PrintWriter(new FileWriter(new File(outDir, "datarefs.tsv")));

        DecompInterface di = null;
        PrintWriter cw = null;
        if (doDecomp) {
            di = new DecompInterface();
            DecompileOptions opts = new DecompileOptions();
            di.setOptions(opts);
            di.toggleCCode(true);
            di.toggleSyntaxTree(false);
            di.setSimplificationStyle("decompile");
            di.openProgram(currentProgram);
            cw = new PrintWriter(new FileWriter(new File(outDir, "decomp.c")));
        }

        FunctionIterator it = fm.getFunctions(true);
        int n = 0;
        while (it.hasNext() && !monitor.isCancelled()) {
            Function f = it.next();
            Address ep = f.getEntryPoint();
            AddressSetView body = f.getBody();
            long size = body.getNumAddresses();
            fpw.printf("%s\t%s\t%d\t%s\t%s\t%s%n", ep, body.getMaxAddress(), size, f.getName(),
                f.getCallingConventionName(), f.getSignature().getPrototypeString());
            // calls + refs
            Set<String> seen = new HashSet<>();
            InstructionIterator ii = listing.getInstructions(body, true);
            while (ii.hasNext()) {
                Instruction ins = ii.next();
                for (Reference r : ins.getReferencesFrom()) {
                    Address to = r.getToAddress();
                    RefType rt = r.getReferenceType();
                    if (rt.isCall()) {
                        Function g = fm.getFunctionAt(to);
                        String key = "C" + to;
                        if (seen.add(key))
                            cpw.printf("%s\t%s\t%s\t%s%n", ep, to, g != null ? g.getName() : "?", ins.getAddress());
                    } else if (to.isMemoryAddress()) {
                        Data d = listing.getDataContaining(to);
                        if (d != null && d.hasStringValue()) {
                            String s = d.getDefaultValueRepresentation();
                            if (seen.add("S" + to))
                                spw.printf("%s\t%s\t%s%n", ep, to, s.replace("\t", "\\t"));
                        } else if (!rt.isFlow()) {
                            if (seen.add("D" + to))
                                dpw.printf("%s\t%s\t%s%n", ep, to, rt.toString());
                        }
                    }
                }
            }
            if (doDecomp) {
                DecompileResults res = di.decompileFunction(f, 60, monitor);
                cw.printf("//==== %s %s size=%d%n", ep, f.getName(), size);
                if (res != null && res.decompileCompleted()) {
                    cw.println(res.getDecompiledFunction().getC());
                } else {
                    cw.println("// DECOMPILE FAILED: " + (res != null ? res.getErrorMessage() : "null"));
                }
            }
            n++;
        }
        fpw.close(); cpw.close(); spw.close(); dpw.close();
        if (cw != null) cw.close();
        if (di != null) di.dispose();
        println("Exported " + n + " functions");
    }
}
