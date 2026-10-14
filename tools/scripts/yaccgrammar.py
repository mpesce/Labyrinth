"""Reconstruct the wc (Labyrinth world compiler) grammar from the pcyacc tables
embedded in LABYRINTH.EXE.

Tables (VA):  yyexca 4ae838, yyact 4ae858, yypact 4aed00, yypgo 4aef38,
              yyr1 4aefd8, yyr2 4af0e0, yychk 4af1e8, yydef 4af420
Keywords at 4b7d20: {char *name; int token} pairs, count at 4b7dc8.
"""
import struct, pefile
pe = pefile.PE(r'D:\laby\LABYRINTH.EXE', fast_load=True)
img = pe.get_memory_mapped_image()
def u32(va): return struct.unpack_from('<I', img, va - 0x400000)[0]
def s32(va): return struct.unpack_from('<i', img, va - 0x400000)[0]
def cstr(va):
    o = va - 0x400000; return img[o:img.index(b'\0', o)].decode('latin1')
def arr(va, n): return [s32(va + 4 * i) for i in range(n)]

NSTATES, NACT, NNT, NRULES = 142, 0x129, 40, 66
yyexca = arr(0x4ae838, 8)
yyact = arr(0x4ae858, NACT)
yypact = arr(0x4aed00, NSTATES)
yypgo = arr(0x4aef38, NNT)
yyr1 = arr(0x4aefd8, NRULES)
yyr2 = arr(0x4af0e0, NRULES)
yychk = arr(0x4af1e8, NSTATES)
yydef = arr(0x4af420, NSTATES)

nkw = u32(0x4b7dc8)
tokname = {280: 'IDENTIFIER', 281: 'INTEGER', 282: 'DOUBLE', 283: 'STRING', 0: '$end', 256: 'error'}
for i in range(nkw):
    tokname[u32(0x4b7d20 + 8 * i + 4)] = cstr(u32(0x4b7d20 + 8 * i)).upper()

def sym(s):
    if s >= 0:
        if s in tokname: return tokname[s]
        if 32 <= s < 127: return "'%c'" % s
        return 'T%d' % s
    return 'nt%d' % (-s)

# transitions: shift on token t from state p
def shift(p, t):
    if yypact[p] <= -1000: return None
    i = yypact[p] + t
    if 0 <= i < NACT and yychk[yyact[i]] == t: return yyact[i]
    return None

def goto(p, nt):  # nt positive nonterminal index
    i = yypgo[nt] + p + 1
    if i < NACT:
        g = yyact[i]
        if yychk[g] == -nt: return g
    return yyact[yypgo[nt]]

# predecessor map: exact token shifts first, nonterminal gotos by fixpoint
pred = {s: set() for s in range(NSTATES)}
toks = set(tokname) | set(range(1, 127))
for p in range(NSTATES):
    for t in toks:
        q = shift(p, t)
        if q is not None: pred[q].add(p)

reduce_at = {}
for s in range(NSTATES):
    r = yydef[s]
    if r > 0: reduce_at.setdefault(r, []).append(s)
i = 0
while i < len(yyexca):
    if yyexca[i] == -1:
        st = yyexca[i + 1]; i += 2
        while i < len(yyexca) and yyexca[i] >= 0:
            if yyexca[i + 1] > 0: reduce_at.setdefault(yyexca[i + 1], []).append(st)
            i += 2
        if i < len(yyexca): i += 2
    else:
        i += 1

def back_paths(s, n, limit=200):
    paths = [[s]]
    for _ in range(n):
        np = []
        for pth in paths:
            for p in pred[pth[0]]:
                np.append([p] + pth)
        paths = np[:limit]
    return paths

changed = True
while changed:
    changed = False
    for r in range(1, NRULES):
        A, n = yyr1[r], yyr2[r]
        for s in reduce_at.get(r, []):
            for pth in back_paths(s, n):
                p = pth[0]
                q = goto(p, A)
                if q is not None and 0 <= q < NSTATES and p not in pred[q]:
                    pred[q].add(p); changed = True

def rhs_for(rule):
    n = yyr2[rule]
    if n == 0: return []
    for s in reduce_at.get(rule, []):
        for pth in back_paths(s, n):
            return [sym(yychk[x]) for x in pth[1:]]
    return None

for r in range(1, NRULES):
    rhs = rhs_for(r)
    print('%2d  nt%-3d -> %s' % (r, yyr1[r], ' '.join(rhs) if rhs is not None else '?? (len %d)' % yyr2[r]))
print('keywords:', {v: k for k, v in tokname.items()})
