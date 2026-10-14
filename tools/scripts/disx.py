import sys, pefile, capstone
pe = pefile.PE(r'D:\laby\LABYRINTH.EXE', fast_load=True)
img = pe.get_memory_mapped_image()
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
va = int(sys.argv[1], 16); end = int(sys.argv[2], 16) if len(sys.argv) > 2 else va + 0x100
for ins in md.disasm(img[va - 0x400000:end - 0x400000], va):
    print('  %x: %-28s %s %s' % (ins.address, ins.bytes.hex(), ins.mnemonic, ins.op_str))
