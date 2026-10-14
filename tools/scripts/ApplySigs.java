// Apply C prototypes: lines "addr|prototype" e.g. 00425b20|int RLFrameSetPosition(int f,int r,double x,double y,double z)
// @category Labyrinth
import ghidra.app.script.GhidraScript;
import ghidra.app.cmd.function.ApplyFunctionSignatureCmd;
import ghidra.app.util.parser.FunctionSignatureParser;
import ghidra.program.model.listing.*;
import ghidra.program.model.data.*;
import ghidra.program.model.symbol.SourceType;
import java.io.*;

public class ApplySigs extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        BufferedReader br = new BufferedReader(new FileReader(args[0]));
        DataTypeManager dtm = currentProgram.getDataTypeManager();
        FunctionSignatureParser parser = new FunctionSignatureParser(dtm, null);
        String line; int n = 0, bad = 0;
        while ((line = br.readLine()) != null) {
            line = line.trim();
            if (line.isEmpty() || line.startsWith("#")) continue;
            int bar = line.indexOf('|');
            Function f = currentProgram.getFunctionManager().getFunctionAt(toAddr(Long.parseLong(line.substring(0, bar), 16)));
            if (f == null) { bad++; println("no func " + line); continue; }
            try {
                FunctionDefinitionDataType sig = parser.parse(f.getSignature(), line.substring(bar + 1));
                ApplyFunctionSignatureCmd cmd = new ApplyFunctionSignatureCmd(f.getEntryPoint(), sig, SourceType.USER_DEFINED);
                if (cmd.applyTo(currentProgram, monitor)) n++; else { bad++; println("fail " + line + " " + cmd.getStatusMsg()); }
            } catch (Exception e) { bad++; println("parse fail " + line + " : " + e.getMessage()); }
        }
        br.close();
        println("ApplySigs: applied " + n + ", failed " + bad);
    }
}
