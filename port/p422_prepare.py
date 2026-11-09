#!/usr/bin/env python3
"""Deploy the pinned qualified 216-adapter program without modifying stock generation.

The text payload is qualified output retained by P4.21, rebased onto the stock
P4.19 continuation/idle-loop program. Fail closed when any patched input changes.
No work/p421 or work/converge paths are used by builds.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parent.parent


def prepare(source, output):
    source, output = source.resolve(), output.resolve()
    if source == output or source in output.parents or output in source.parents:
        raise SystemExit('Qualified output must be separate from stock inputs')
    payload = ROOT / 'port/mods/qualified-base.patch'
    pin = json.loads((payload.with_suffix('.json')).read_text())

    def qualified_hash(path):
        data = path.read_bytes()
        if path.name == 'CMakeLists.txt':
            # The generator embeds its checkout's fallback runtime path. Every
            # shipping wrapper supplies SATURN_RUNTIME explicitly; relocating a
            # checkout must not invalidate otherwise identical guest inputs.
            data, count = re.subn(rb'^  set\(SATURN_RUNTIME "[^"\r\n]*"\)$',
                                  pin['cmake_runtime_default'].encode(), data,
                                  flags=re.MULTILINE)
            if count != 1:
                raise SystemExit('Unreviewed CMake runtime default layout')
        return hashlib.sha256(data).hexdigest()

    for name, digest in pin['before'].items():
        if qualified_hash(source / name) != digest:
            raise SystemExit('Unreviewed stock input changed: ' + name)
    inputs = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
              for p in sorted(source.iterdir()) if p.is_file()}
    for p in [payload, ROOT / 'port/runtime-patches/p421-mod-convergence.patch']:
        inputs[p.name] = hashlib.sha256(p.read_bytes()).hexdigest()
    marker = output / 'p422-deployment.json'
    if marker.exists():
        previous = json.loads(marker.read_text())
        actual = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in output.iterdir() if p.is_file() and p != marker}
        if previous.get('inputs') == inputs and previous.get('outputs') == actual:
            print('Qualified 216-adapter deployment is current:', output.relative_to(ROOT))
            return
    if output.exists():
        # Only the explicitly generated destination is replaced.
        shutil.rmtree(output)
    shutil.copytree(source, output, ignore=shutil.ignore_patterns('__pycache__', '*.orig', '*.rej'))
    for patch in [payload, ROOT / 'port/runtime-patches/p421-mod-convergence.patch']:
        result = subprocess.run(['patch', '--batch', '--fuzz=0', '-p2', '-d', str(output)],
                                input=patch.read_bytes(), capture_output=True, check=True)
        log = result.stdout.decode() + result.stderr.decode()
        if 'offset' in log or 'fuzz' in log:
            raise SystemExit(log)
        if patch == payload:
            for name, digest in pin['after'].items():
                assert qualified_hash(output / name) == digest, name
    outputs = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
               for p in output.iterdir() if p.is_file()}
    marker.write_text(json.dumps(dict(inputs=inputs, outputs=outputs), indent=2) + '\n')
    print('Deployed qualified 216-adapter program:', output.relative_to(ROOT))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'port/run/P41/recomp')
    parser.add_argument('--output', type=Path, default=ROOT / 'port/run/P422/recomp')
    args = parser.parse_args()
    prepare(args.source, args.output)
