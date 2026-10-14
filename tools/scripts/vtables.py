"""Find vtables: addresses stored as '*this = &PTR_...' in constructors; dump entries."""
import re, sys, struct, csv, pefile
d = sys.argv[1]
pe = pefile.PE(r'D:\laby\LABYRINTH.EXE', fast_load=True); img = pe.get_memory_mapped_image()
def u32(va): return struct.unpack_from('<I', img, va - 0x400000)[0]
names = {}
for row in csv.reader(open(d + '/functions.tsv'), delimiter='\t'):
    names[int(row[0], 16)] = row[3]
src = open(d + '/decomp.c', encoding='latin1').read()
vts = set()
for m in re.finditer(r'= &PTR_(?:FUN|PTR|s|DAT|LAB)?_?[A-Za-z0-9_]*?([0-9a-f]{8});', src):
    va = int(m.group(1), 16)
    if 0x4ae000 <= va < 0x4c6000 and 0x401000 <= u32(va) < 0x492d62:
        vts.add(va)
lo = int(sys.argv[2], 16) if len(sys.argv) > 2 else 0
hi = int(sys.argv[3], 16) if len(sys.argv) > 3 else 1 << 32
for va in sorted(vts):
    ents = []
    p = va
    while True:
        t = u32(p)
        if not (0x401000 <= t < 0x492d62): break
        if p != va and p in vts: break
        ents.append(t); p += 4
    if not any(lo <= e < hi for e in ents): continue
    print('vtable %x (%d entries)' % (va, len(ents)))
    for i, e in enumerate(ents):
        print('   [%02x] %x %s' % (i * 4, e, names.get(e, '?')))
