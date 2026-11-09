#!/usr/bin/env python3
"""Build only in work/converge, using a frozen repo snapshot; default port unchanged."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT=Path(__file__).resolve().parents[2]
HERE=Path(__file__).resolve().parent


def freeze(snapshot):
    if not snapshot.exists():
        snapshot.mkdir(parents=True)
        # Copy each tree once. Never hardlink concurrently edited source files.
        for name in ('tools/saturn-recomp','port','project/src_functional','tools/decomp'):
            shutil.copytree(ROOT/name,snapshot/name,
                ignore=shutil.ignore_patterns('.git','__pycache__'))
        (snapshot/'notes').mkdir()
        for name in ('symbols.txt','floors.tsv'):
            shutil.copyfile(ROOT/'notes'/name,snapshot/'notes'/name)
        shutil.copyfile(ROOT/'cart.bin',snapshot/'cart.bin')
    manifest=snapshot.parent/'snapshot.sha256.json'
    if not manifest.exists():
        files={str(p.relative_to(snapshot)):hashlib.sha256(p.read_bytes()).hexdigest()
               for top in ('port','project/src_functional','tools/saturn-recomp','tools/decomp','notes')
               for p in sorted((snapshot/top).rglob('*'))
               if p.is_file() and '/run/' not in str(p.relative_to(snapshot))
               and '/venv/' not in str(p.relative_to(snapshot)) and '__pycache__' not in str(p)
               and '/.git/' not in str(p)}
        for p in (snapshot/'cart.bin',snapshot/'port/run/P41/recomp/CMakeLists.txt'):
            files[str(p.relative_to(snapshot))]=hashlib.sha256(p.read_bytes()).hexdigest()
        for p in sorted((snapshot/'port/run/P41/recomp').glob('*.cpp')):
            files[str(p.relative_to(snapshot))]=hashlib.sha256(p.read_bytes()).hexdigest()
        manifest.write_text(json.dumps(files,indent=2)+'\n')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--variant',choices=['stock','pilot10','pilot','mod'],default='pilot')
    p.add_argument('--overrides',type=Path)
    p.add_argument('--jobs',type=int,choices=range(1,5),default=4)
    p.add_argument('--generate-only',action='store_true')
    a=p.parse_args();os.chdir(ROOT)
    work=ROOT/'work/converge';snapshot=work/'snapshot'
    freeze(snapshot)
    source=snapshot/'port/run/P41/recomp'
    started=time.monotonic()
    if a.variant!='stock':
        python=snapshot/'tools/decomp/venv/bin/python'
        source=work/f'generated-{a.variant}'
        overrides=a.overrides or HERE/('pilot10.txt' if a.variant=='pilot10' else 'overrides.txt')
        subprocess.run([str(python),str(HERE/'generate.py'),'--snapshot',str(snapshot),
            '--output',str(source),'--overrides',str(overrides),
            '--mod','sin-half' if a.variant=='mod' else 'off'],check=True)
    generated=time.monotonic()
    if a.generate_only:return
    build=work/f'build-{a.variant}'
    subprocess.run(['cmake','-S',str(source),'-B',str(build),
        f'-DSATURN_RUNTIME={snapshot}/tools/saturn-recomp/runtime',
        f'-DSATURN_EXTRA={snapshot}/port/stv.cmake','-DSATURN_NO_SDL=ON'],check=True)
    subprocess.run(['cmake','--build',str(build),'--target','saturn_stv','-j'+str(a.jobs)],check=True)
    binary=build/'saturn_stv'
    (work/f'build-{a.variant}-metrics.json').write_text(json.dumps(dict(
        generation_seconds=generated-started,build_seconds=time.monotonic()-generated,
        bytes=binary.stat().st_size,binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest()),indent=2)+'\n')


if __name__=='__main__':main()
