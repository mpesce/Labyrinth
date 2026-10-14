# Print, per function, the string literals it references in instruction order.
import sys, struct, pefile, capstone, re
pe = pefile.PE(r'D:\laby\LABYRINTH.EXE', fast_load=True); img = pe.get_memory_mapped_image()
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
def cstr(va):
    o = va - 0x400000
    try: e = img.index(b'\0', o)
    except ValueError: return None
    s = img[o:e]
    if len(s) >= 2 and all(32 <= c < 127 or c in (9, 10, 13) for c in s): return s.decode('latin1')
    return None
lo, hi = int(sys.argv[1], 16), int(sys.argv[2], 16)
cur = None
for ins in md.disasm(img[lo - 0x400000:hi - 0x400000], lo):
    for m in re.finditer(r'0x([0-9a-f]{6,8})', ins.op_str):
        v = int(m.group(1), 16)
        if 0x4b3000 <= v < 0x4c6000:
            s = cstr(v)
            if s is not None: print('  %x: %s %r' % (ins.address, ins.mnemonic, s))
