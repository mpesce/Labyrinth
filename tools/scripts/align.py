"""Align binary functions in an address range with source functions (in order).
usage: python align.py <ghidra_export_dir> <lo> <hi> <src files...>
Prints proposed names: addr  name  (score / method)
"""
import csv, re, sys, collections, os
sys.path.insert(0, os.path.dirname(__file__))
from srcfuncs import functions

d, lo, hi = sys.argv[1], int(sys.argv[2], 16), int(sys.argv[3], 16)
srcs = sys.argv[4:]

def norm(s):
    s = s.strip()
    if s.startswith('u"'): s = s[1:]
    if s.startswith('"') and s.endswith('"'): s = s[1:-1]
    return s

bfun = []
for row in csv.reader(open(d + '/functions.tsv'), delimiter='\t'):
    a = int(row[0], 16)
    if lo <= a < hi: bfun.append((a, int(row[2]), row[3]))
bfun.sort()
bstr = collections.defaultdict(set)
for row in csv.reader(open(d + '/strrefs.tsv'), delimiter='\t', quoting=csv.QUOTE_NONE):
    a = int(row[0], 16)
    if lo <= a < hi: bstr[a].add(norm(row[2]))

sfun = []  # (file, name, set(strings))
for fn in srcs:
    for name, strs in functions(open(fn, encoding='latin1').read()):
        ss = set(s for s in strs if len(s) >= 3)
        sfun.append((os.path.basename(fn), name, ss))

def score(bs, ss):
    if not bs or not ss: return 0.0
    inter = len(bs & ss)
    # partial match: binary string contained in a source string (e.g. after merging)
    if inter == 0:
        for b in bs:
            for s in ss:
                if len(b) >= 6 and (b in s or s in b): inter += 0.5; break
    return inter / max(len(bs), 1)

# DP alignment (Needleman-Wunsch, gaps free-ish)
B, S = len(bfun), len(sfun)
NEG = -1e9
dp = [[0.0] * (S + 1) for _ in range(B + 1)]
bt = [[0] * (S + 1) for _ in range(B + 1)]
GAP = -0.02
for i in range(B + 1):
    for j in range(S + 1):
        if i == 0 and j == 0: continue
        best, how = NEG, 0
        if i > 0 and j > 0:
            sc = score(bstr[bfun[i - 1][0]], sfun[j - 1][2])
            # no-string pairs get a small positive score to encourage diagonal
            if not bstr[bfun[i - 1][0]] and not sfun[j - 1][2]: sc = 0.05
            elif sc == 0: sc = -0.3
            v = dp[i - 1][j - 1] + sc
            if v > best: best, how = v, 1
        if i > 0:
            v = dp[i - 1][j] + GAP
            if v > best: best, how = v, 2
        if j > 0:
            v = dp[i][j - 1] + GAP
            if v > best: best, how = v, 3
        dp[i][j], bt[i][j] = best, how
i, j = B, S
pairs = []
while i > 0 or j > 0:
    h = bt[i][j]
    if h == 1:
        pairs.append((i - 1, j - 1)); i -= 1; j -= 1
    elif h == 2:
        pairs.append((i - 1, None)); i -= 1
    else:
        pairs.append((None, j - 1)); j -= 1
pairs.reverse()
for bi, sj in pairs:
    if bi is None:
        print('%-8s %-6s %-28s %s' % ('', '', '(src only)', sfun[sj][0] + ':' + sfun[sj][1]))
        continue
    a, sz, gname = bfun[bi]
    if sj is None:
        print('%08x %6d %-28s %s' % (a, sz, '(bin only)', ' ; '.join(list(bstr[a])[:3])[:80]))
        continue
    sc = score(bstr[a], sfun[sj][2])
    print('%08x %6d %-28s %s  [%.2f]' % (a, sz, sfun[sj][1], sfun[sj][0], sc))
