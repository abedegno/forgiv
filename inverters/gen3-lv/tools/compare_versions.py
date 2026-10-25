"""Initial Gen3 LV version survey; no source matches are accepted here.

Run after fetching gen3-lv-316, gen3-lv-318 and gen3-lv-319. Whole mapped
function-row byte strings of at least 16 bytes are searched on aligned image
positions. Rows can overlap; byte sums are not source-coverage measurements.
Changed relocations make this conservative. An exact hit is a mapping candidate,
not proof of entry boundaries, call targets, RAM bindings or compiled source.
Results go to the repository's ignored build/gen3-lv-version-survey directory.
"""
from pathlib import Path
import csv,json,hashlib
R=Path(__file__).resolve().parents[3];S=R/'firmware/gen3-lv';W=R/'build/gen3-lv-version-survey';W.mkdir(parents=True,exist_ok=True);out={'pairs':{},'baseline_raw_function_candidates':{}}
for kind,prefix,file,base,width in [('ARM','A','ARMStore.bin',0x08014000,1),('DSP','D','DSPStore.bin',0x3e8000,2)]:
 images={v:(S/(f'{prefix}{v}_{file}')).read_bytes() for v in (316,318,319)}
 if kind=='ARM':images={v:b[4:] for v,b in images.items()}
 for a,b in ((316,318),(318,319),(316,319)):
  x,y=images[a],images[b];diff=sum(x[i:i+width]!=y[i:i+width] for i in range(0,min(len(x),len(y)),width))+abs(len(x)-len(y))//width
  row={'lengths_bytes':[len(x),len(y)],'different_at_same_offset':diff,'unit':'bytes' if width==1 else 'words'}
  if kind=='DSP':row['boot_sector_A_identical']=x[(0x3f4000-base)*2:]==y[(0x3f4000-base)*2:]
  out['pairs'][f'{kind}_{a}_{b}']=row
 rows=list(csv.DictReader((R/f'inverters/gen3-lv/{prefix.lower()}316/map/functions.tsv').open(),delimiter='\t'))
 # A conservative starting inventory: whole row byte-for-byte, no masking.
 for v in (318,319):
  counts={'rows':0,'unique_exact':0,'ambiguous_exact':0,'not_found_exact':0,'short_rows_skipped':0,'unique_exact_bytes':0};matches=[]
  for row in rows:
   at=int(row['addr'],16);n=int(row['size'],16 if kind=='ARM' else 10)*width;off=(at-base)*width;counts['rows']+=1
   if n<16:counts['short_rows_skipped']+=1;continue
   raw=images[316][off:off+n]
   if len(raw)!=n:continue
   hits=[];pos=0
   while True:
    p=images[v].find(raw,pos)
    if p<0:break
    if p%2==0:hits.append(p)
    if len(hits)>1:break
    pos=p+1
   if len(hits)==1:
    counts['unique_exact']+=1;counts['unique_exact_bytes']+=n;matches.append({'name':row['name'],'baseline_addr':row['addr'],'candidate_addr':f'{base+hits[0]//width:08X}' if kind=='ARM' else f'{base+hits[0]//width:06X}','size_bytes':n})
   elif hits:counts['ambiguous_exact']+=1
   else:counts['not_found_exact']+=1
  out['baseline_raw_function_candidates'][f'{kind}_{v}']=counts;(W/f'{kind}{v}-exact-candidates.json').write_text(json.dumps(matches,indent=2)+'\n')
(W/'comparison.json').write_text(json.dumps(out,indent=2)+'\n');print(json.dumps(out,indent=2))
