import csv,re,collections,sys
d=sys.argv[1]
funcs=[]
for row in csv.reader(open(d+'/functions.tsv'),delimiter='\t'):
    funcs.append((int(row[0],16),int(row[1],16),int(row[2]),row[3],row[4]))
funcs.sort()
srefs=collections.defaultdict(list)
for row in csv.reader(open(d+'/strrefs.tsv'),delimiter='\t',quoting=csv.QUOTE_NONE):
    srefs[int(row[0],16)].append(row[2][:50])
calls=collections.defaultdict(list); callers=collections.defaultdict(int)
ext={}
for row in csv.reader(open(d+'/calls.tsv'),delimiter='\t'):
    a=int(row[0],16)
    if row[1].startswith('EXTERNAL'):
        calls[a].append(row[2]); continue
    calls[a].append(int(row[1],16)); callers[int(row[1],16)]+=1
with open(d+'/funcsum.txt','w') as f:
    for s,e,sz,n,cc in funcs:
        api=[c for c in calls[s] if isinstance(c,str)]
        f.write('%x %5d in%-3d out%-3d %s | %s %s\n'%(s,sz,callers[s],len(calls[s]),cc[2:6],' ; '.join(srefs[s][:6]), ('API:'+','.join(api[:6])) if api else ''))
