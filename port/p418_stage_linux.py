#!/usr/bin/env python3
"""Stage current integrated Linux sources; refresh whenever any input changes."""
import hashlib
import json
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
WORK = ROOT / 'work/p417'
BASE = WORK / 'baseline'
NAMES = ['main_stv.cpp', 'stv.cmake', 'p410_continuations.py', 'p415_idle_loops.py',
         'p413_roms.c', 'p413_roms.h', 'p417_linux_frontend.h']


def inventory(directory):
    return {str(p.relative_to(directory)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in sorted(directory.rglob('*')) if p.is_file()
            and '.git' not in p.parts and '__pycache__' not in p.parts
            and p.suffix not in ('.orig', '.rej')}


if not (ROOT / 'port/run/P41/recomp/CMakeLists.txt').is_file():
    raise SystemExit('Generate port/run/P41/recomp separately first.')
files = {'saturn-recomp/' + name: digest
         for name, digest in inventory(ROOT / 'tools/saturn-recomp').items()}
files.update({'saturn-recomp/port/' + name: hashlib.sha256((ROOT / 'port' / name).read_bytes()).hexdigest()
              for name in NAMES})
files.update({'recomp/' + name: digest
              for name, digest in inventory(ROOT / 'port/run/P41/recomp').items()})
record = dict(schema=2, head=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
              files=files)
marker = WORK / 'snapshot.json'
previous = json.loads(marker.read_text()) if marker.exists() else None
# Retain the historical mid-P4.16 snapshot for audit, never use it as build input.
if previous and previous.get('schema') != 2:
    BASE.rename(WORK / 'p417-original-baseline')
    marker.rename(WORK / 'p417-original-snapshot.json')
    previous = None
if previous != record or inventory(BASE) != files:
    for target in [BASE, WORK / 'saturn-recomp', WORK / 'recomp']:
        if target.exists():
            shutil.rmtree(target)
    for name in files:
        if name.startswith('saturn-recomp/port/'):
            source = ROOT / 'port' / Path(name).name
        elif name.startswith('saturn-recomp/'):
            source = ROOT / 'tools' / name
        else:
            source = ROOT / 'port/run/P41' / name
        destination = BASE / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
    if inventory(BASE) != files:
        raise SystemExit('Sources changed while staging; rerun the build to take a fresh snapshot.')
    shutil.copytree(BASE / 'saturn-recomp', WORK / 'saturn-recomp')
    shutil.copytree(BASE / 'recomp', WORK / 'recomp')
    marker.write_text(json.dumps(record, indent=2) + '\n')
    print('Refreshed Linux snapshot from current sources:', record['head'], len(files), 'files')
else:
    # Repair a modified build copy, without accepting it as a new source baseline.
    for directory in ['saturn-recomp', 'recomp']:
        destination = WORK / directory
        if inventory(destination) != inventory(BASE / directory):
            if destination.exists():
                shutil.rmtree(destination)
            shutil.copytree(BASE / directory, destination)
    print('Verified Linux snapshot matches current sources:', record['head'], len(files), 'files')
