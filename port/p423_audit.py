#!/usr/bin/env python3
"""Check resumed snapshot/evidence integrity and remove redundant regression dumps."""
from pathlib import Path
import ast,hashlib,json,re,struct,subprocess,tarfile,wave
from PIL import Image
root=Path('work/p423');sha=lambda b:hashlib.sha256(b).hexdigest();report={}
s=json.loads((root/'snapshot.json').read_text())
for n,h in s['files'].items():assert sha((root/n).read_bytes())==h,n
report['pinned_files']=len(s['files'])
for name in ['committed','runtime-committed']:
 with tarfile.open(root/(name+'.tar')) as archive:
  members=[m for m in archive if m.isfile()]
  for m in members:assert archive.extractfile(m).read()==(root/name/m.name).read_bytes(),m.name
 report[name+'_archive_files']=len(members)
for p in Path('port').glob('p423_*.py'):ast.parse(p.read_text())
subprocess.run(['bash','-n','port/p423_build.sh'],check=True)
for name in ['options.h','video.inc','present.inc','vdp2.inc','test_video.cpp']:
 p=Path('port')/('p423_'+name);assert p.read_bytes()==(root/'port'/p.name).read_bytes()
for p in root.rglob('*.json'):json.loads(p.read_text())
pngs=list(root.rglob('*.png'))
for p in pngs:
 with Image.open(p) as im:im.verify()
report['valid_pngs']=len(pngs)
wavs=[]
for p in root.rglob('*.wav'):
 with wave.open(str(p)) as f:
  n=f.getnframes()*f.getnchannels()*f.getsampwidth()
  assert n>0 and len(f.readframes(f.getnframes()))==n,p
 wavs.append(str(p))
report['valid_nonempty_wavs']=wavs
for p in (root/'visual').rglob('report.json'):
 j=json.loads(p.read_text());d=p.parent
 for n,h in j['png'].items():assert sha((d/n).read_bytes())==h
 assert sha((d/'audio.wav').read_bytes())==j['wav']
 for f in d.glob('bg-*.bin'):assert len(f.read_bytes())==2*(0x200+0x1000+0x80000),f
 for f in d.glob('commands-*.bin'):
  b=f.read_bytes();at=0
  for _ in range(2):
   n=struct.unpack_from('<I',b,at)[0];at+=4
   for _ in range(n):
    count=struct.unpack_from('<I',b,at)[0];at+=4+36*count
  assert at==len(b),f

def parts(path):
 b=path.read_bytes();assert b[:8]==b'STVQUICK';at=48;out={}
 for name in ['machine','video','core','mmio','bios','stv','player','scu','scudsp','onchip','clock','smpc','stv-smpc','cdblock','cddrive','cdfs','cdpos','vdp1','vdp2','sound','scsp','scspdsp']:
  assert b[at:at+len(name)]==name.encode();at+=len(name);n=struct.unpack_from('<Q',b,at)[0];at+=8;out[name]=b[at:at+n];at+=n
 assert at==len(b);return out
ad=root/'authority';a=json.loads((ad/'default/report.json').read_text());b=json.loads((ad/'hfr60/report.json').read_text())
assert len(a['rows'])==len(b['rows'])==2100 and a['rows']==b['rows']
assert parts(ad/'default/state.quickstate')==parts(ad/'hfr60/state.quickstate')
for variant in ['record','default','hfr60']:
 j=json.loads((ad/variant/'report.json').read_text());assert len(j['rows'])==2100
 assert sha((ad/variant/'audio.wav').read_bytes())==j['wav']
assert a['wav']==b['wav']
for vb in [1800,2100]:assert (ad/'default'/f'dump-{vb}.bin').read_bytes()==(ad/'hfr60'/f'dump-{vb}.bin').read_bytes()
report['authority']={'equal_fields':2100,'equal_state_parts':22,'equal_hardware_fields':[1800,2100],'wav_equal':True}
reference=json.loads(Path('work/p411/regression/after/manifest.json').read_text())
for name,ref in reference.items():
 d=root/'regression'/name;log=(d/'run.log').read_text()
 trace='\n'.join(line for line in log.splitlines() if re.search(r'SCENETRACE|ST-V coin:|FATAL|stopped after|SCSP:|sound:.*samples',line))+'\n'
 guest='\n'.join(line for line in log.splitlines() if re.search(r'SCENETRACE|ST-V coin:|FATAL|stopped after',line))+'\n'
 assert sha(trace.encode())==ref['trace_sha256'] and sha(guest.encode())==ref['guest_trace_sha256']
 assert sha((d/'audio.wav').read_bytes())==ref['wav_sha256']
 for n,h in ref['png'].items():assert sha((d/n).read_bytes())==h
report['regression']={'pngs':56,'full_traces':4,'guest_traces':4,'wavs':4}
assert hashlib.md5((root/'smoke/shot-1900.png').read_bytes()).hexdigest()=='9bd5386ea7439621c19674381086e8fd'
# Preserve small hashes of redundant, already checked captures before deleting.
cleanpath=root/'removed-intermediates.json';removed=json.loads(cleanpath.read_text()) if cleanpath.exists() else {}
for d in (root/'regression').iterdir():
 if not d.is_dir():continue
 for p in list(d.glob('dump-*.bin'))+list(d.glob('shown-*.bin')):
  data=p.read_bytes();removed[str(p)]={'bytes':len(data),'sha256':sha(data)};p.unlink()
cleanpath.write_text(json.dumps(removed,indent=2)+'\n');report['removed_regression_dump_bytes']=sum(v['bytes'] for v in removed.values())
report['zero_files']=[str(p) for p in root.rglob('*') if p.is_file() and not p.is_symlink() and p.stat().st_size==0]
expected=['work/p423/saturn-recomp/src/saturnrecomp/analysis/__init__.py','work/p423/runtime-committed/src/saturnrecomp/analysis/__init__.py','work/p423/committed/project/src_functional/e']
assert sorted(report['zero_files'])==sorted(expected),report['zero_files']
report['resume_findings']=['157 pinned files and both committed archives intact','gameover hw-log.txt was zero bytes; now discarded via /dev/null','gameover audio.wav had a zero-frame header; complete regression rerun repaired it','original regression.log is superseded by regression-resumed.log','P4.22 files were outside this job scope and were not edited']
(root/'resume-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS snapshot, source, PNG/WAV, replay/state, regression and smoke audit; removed',report['removed_regression_dump_bytes'],'bytes of redundant regression dumps')
