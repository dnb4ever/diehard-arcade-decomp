#!/usr/bin/env python3
"""Fresh source snapshots for convergence, excluding historical run payloads.

Run once from the repo root before building. Existing snapshots are never
silently overwritten, and live port/tools files are never modified.
"""
import datetime,hashlib,json,shutil
from pathlib import Path
w=Path('work/converge/round4');s=w/'snapshot';s.mkdir(parents=True,exist_ok=False)
start=datetime.datetime.now(datetime.timezone.utc).isoformat()
for name in ('tools/saturn-recomp','tools/decomp','project/src_functional'):
 shutil.copytree(name,s/name,symlinks=True,ignore=shutil.ignore_patterns('.git','__pycache__'))
shutil.copytree('port',s/'port',symlinks=True,ignore=shutil.ignore_patterns('run*','recomp*','__pycache__','converge'))
shutil.copytree('port/run/P41/recomp',s/'port/run/P41/recomp',symlinks=True)
(s/'notes').mkdir()
for name in ('symbols.txt','floors.tsv'):shutil.copyfile(Path('notes')/name,s/'notes'/name)
shutil.copyfile('cart.bin',s/'cart.bin')
files={str(p.relative_to(s)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(s.rglob('*')) if p.is_file() and not p.is_symlink() and '/venv/' not in str(p)}
(w/'snapshot.sha256.json').write_text(json.dumps(files,indent=2)+'\n')
(w/'snapshot-time.json').write_text(json.dumps(dict(started_utc=start,completed_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),files=len(files)),indent=2)+'\n')
# A diagnostic copy changes only the observer's RAM sampling rate. Stock,
# override and profiler all use this same copy. Snapshot inputs stay untouched.
shutil.copytree(s/'tools/saturn-recomp/runtime',w/'capture-runtime')
p=w/'capture-runtime/src/video/video.cpp';text=p.read_text();old='return !value || std::strcmp(value, "0");'
assert text.count(old)==1
p.write_text(text.replace(old,'''const char* interval = std::getenv("CONVERGE_RAM_INTERVAL");
    unsigned every = interval ? std::strtoul(interval, nullptr, 10) : 1;
    return (!value || std::strcmp(value, "0")) && (!every || g_vblanks % every == 0);'''))
print((w/'snapshot-time.json').read_text())
