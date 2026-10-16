// EmuDiff.java: runs functions of a D316 image in Ghidra's p-code emulator, with the C28x
// processor module (ghidra-tms320c28x), for tools/emudiff.py. Not a simulator of the device:
// no peripherals, no interrupts; only the CPU and memory, which is enough for pure functions.
//
//   analyzeHeadless PROJ p -import ANY.bin -processor TMS320C28x:LE:32:default -noanalysis
//       -scriptPath tools/emu -postScript EmuDiff.java SPEC OUT
//
// SPEC (written by emudiff.py), one directive per line:
//   IMAGE label path.bin ramload ramrun ramsize   load an image (big-endian words at 0x3E8000)
//                                                 and copy its ramfuncs to their run address
//   CASE name entry                                start a call of the function at entry
//   REG name value                                 a register (AL, ACC, XAR4, R0H, ...)
//   MEM addr w,w,...                               words in memory
//   OUT reg | OUT mem addr n                       what to read back when it returns
//   END                                            run it (until it returns to STOP)
// OUT gets one line per case: label name steps result...
//
// Addresses are word addresses; Ghidra's space is word-addressed with byte offsets twice the
// word address (as the module's own Emu*Test scripts use it).
//@category D316
import ghidra.app.emulator.EmulatorHelper;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.AddressSpace;
import java.io.*;
import java.nio.file.*;
import java.util.*;

public class EmuDiff extends GhidraScript {
    static final long STOP = 0x3FFFF0L;     // the return address: outside every image
    static final long STACK = 0x0500L;      // M1 RAM
    static final int MAX_STEPS = 200000;

    AddressSpace sp;

    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        List<String> spec = Files.readAllLines(Paths.get(args[0]));
        PrintWriter out = new PrintWriter(new FileWriter(args[1]));
        sp = currentProgram.getAddressFactory().getDefaultAddressSpace();
        EmulatorHelper emu = null;
        String label = "";
        String name = "";
        long entry = 0;
        List<String[]> regs = new ArrayList<>(), mems = new ArrayList<>(), outs = new ArrayList<>();
        byte[] image = null;
        long[] ram = null;
        for (String line : spec) {
            String[] f = line.trim().split("\\s+");
            if (f.length == 0 || f[0].isEmpty()) continue;
            switch (f[0]) {
                case "IMAGE":
                    label = f[1];
                    image = Files.readAllBytes(Paths.get(f[2]));
                    ram = new long[] { Long.decode(f[3]), Long.decode(f[4]), Long.decode(f[5]) };
                    break;
                case "CASE":
                    name = f[1]; entry = Long.decode(f[2]);
                    regs.clear(); mems.clear(); outs.clear();
                    break;
                case "REG": regs.add(f); break;
                case "MEM": mems.add(f); break;
                case "OUT": outs.add(f); break;
                case "END":
                    // a fresh emulator per call, so no call sees another's memory
                    if (emu != null) emu.dispose();
                    emu = new EmulatorHelper(currentProgram);
                    load(emu, image, ram);
                    out.println(label + " " + name + " " + call(emu, entry, regs, mems, outs));
                    out.flush();
                    break;
            }
        }
        if (emu != null) emu.dispose();
        out.close();
        println("EmuDiff.java> done");
    }

    void load(EmulatorHelper emu, byte[] img, long[] ram) throws Exception {
        // the file is big-endian words; the module's memory is little-endian words
        int n = img.length / 2;
        byte[] le = new byte[img.length];
        for (int i = 0; i < n; i++) { le[2 * i] = img[2 * i + 1]; le[2 * i + 1] = img[2 * i]; }
        emu.writeMemory(sp.getAddress(0x3E8000L * 2), le);
        // init_system's memcpy of the ramfuncs
        long from = ram[0] - 0x3E8000L, size = ram[2];
        byte[] r = Arrays.copyOfRange(le, (int) (2 * from), (int) (2 * (from + size)));
        emu.writeMemory(sp.getAddress(ram[1] * 2), r);
        // RAM the functions may touch: zeroed (the CPU's RAM is cleared by boot_main too)
        emu.writeMemory(sp.getAddress(0), new byte[2 * 0x800]);
        emu.writeMemory(sp.getAddress(0xC000L * 2), new byte[2 * 0x8000]);
    }

    String call(EmulatorHelper emu, long entry, List<String[]> regs, List<String[]> mems, List<String[]> outs) throws Exception {
        // the C environment c_int00 sets up (SPRU514: OBJMODE 1, AMODE 0, PAGE0 0, OVM 0, PM 0)
        String[][] st = { {"OBJMODE", "1"}, {"AMODE", "0"}, {"PAGE0", "0"}, {"OVM", "0"}, {"PM", "0"},
                          {"SXM", "0"}, {"M0M1MAP", "1"}, {"SPA", "0"}, {"INTM", "1"}, {"EALLOW", "0"} };
        for (String[] s : st) { try { emu.writeRegister(s[0], Long.decode(s[1])); } catch (Exception e) { } }
        emu.writeRegister("DP", 0);
        emu.writeRegister("SP", STACK);
        emu.writeRegister("RPC", STOP);
        for (String[] m : mems) {
            long a = Long.decode(m[1]);
            for (String w : m[2].split(",")) emu.writeMemoryValue(sp.getAddress(2 * a++), 2, Long.decode(w));
        }
        for (String[] r : regs) emu.writeRegister(r[1], Long.decode(r[2]));
        emu.writeRegister("PC", entry);
        int steps = 0;
        while ((emu.readRegister("PC").longValue() & 0x3FFFFFL) != STOP) {
            if (!emu.step(monitor)) return "ERROR step " + steps + " at 0x" + Long.toHexString(emu.readRegister("PC").longValue()) + ": " + emu.getLastError();
            if (++steps > MAX_STEPS) return "ERROR no return in " + MAX_STEPS + " steps";
        }
        StringBuilder b = new StringBuilder("steps=" + steps);
        // LRETR pops the caller's RPC that an LCR would have pushed: SP ends 2 below STACK
        if ((emu.readRegister("SP").longValue() & 0xFFFF) != STACK - 2) b.append(" SP=0x" + Long.toHexString(emu.readRegister("SP").longValue()));
        for (String[] o : outs) {
            if (o[1].equals("mem")) {
                long a = Long.decode(o[2]); int n = Integer.parseInt(o[3]);
                b.append(" mem" + Long.toHexString(a) + "=");
                for (int i = 0; i < n; i++) {
                    byte[] w = emu.readMemory(sp.getAddress(2 * (a + i)), 2);
                    b.append((i > 0 ? "," : "") + Integer.toHexString((w[0] & 0xFF) | (w[1] & 0xFF) << 8));
                }
            } else {
                b.append(" " + o[1] + "=" + Long.toHexString(emu.readRegister(o[1]).longValue() & 0xFFFFFFFFL));
            }
        }
        return b.toString();
    }
}
