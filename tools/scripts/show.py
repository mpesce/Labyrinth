import re,sys
src=open(sys.argv[1],encoding='latin1').read()
idx={}
for m in re.finditer(r'^//==== ([0-9a-f]+) ', src, re.M):
    idx[int(m.group(1),16)]=m.start()
keys=sorted(idx)
def get(a):
    i=keys.index(a); s=idx[a]; e=idx[keys[i+1]] if i+1<len(keys) else len(src)
    return src[s:e]
lo=int(sys.argv[2],16); hi=int(sys.argv[3],16) if len(sys.argv)>3 else lo+1
for k in keys:
    if lo<=k<hi: sys.stdout.write(get(k))
