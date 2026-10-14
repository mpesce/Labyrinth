// Apply "addr name" lines from a file as function names (creating functions if needed).
// @category Labyrinth
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.address.*;
import ghidra.program.model.symbol.*;
import java.io.*;

public class ApplyNames extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        BufferedReader br = new BufferedReader(new FileReader(args[0]));
        String line; int n = 0, bad = 0;
        FunctionManager fm = currentProgram.getFunctionManager();
        while ((line = br.readLine()) != null) {
            line = line.trim();
            if (line.isEmpty() || line.startsWith("#")) continue;
            String[] p = line.split("\s+");
            Address a = toAddr(Long.parseLong(p[0], 16));
            Function f = fm.getFunctionAt(a);
            if (f == null) f = createFunction(a, null);
            if (f == null) { bad++; continue; }
            f.setName(p[1], SourceType.USER_DEFINED);
            n++;
        }
        br.close();
        println("ApplyNames: named " + n + ", failed " + bad);
    }
}
