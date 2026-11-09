#!/usr/bin/env python3
"""Exercise the shipping Linux loader's default data paths and explicit override."""
import argparse
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent.parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary', default='port/run/P46/build-sdl/saturn_stv')
args = parser.parse_args()
os.chdir(ROOT)
work = ROOT / 'work/p422/paths'
work.mkdir(parents=True, exist_ok=True)
env = {k: v for k, v in os.environ.items()
       if not k.startswith(('STV_', 'SATURN_', 'P416_')) and k != 'XDG_CONFIG_HOME'}
env['HOME'] = str(work / 'home')
report = {}
for name, extra, options, expected in [
    ('xdg', {'XDG_CONFIG_HOME': str(work / 'xdg config')}, [],
     work / 'xdg config/saturn-recomp/mods'),
    ('home', {}, [], work / 'home/.config/saturn-recomp/mods'),
    ('custom-settings', {'XDG_CONFIG_HOME': str(work / 'xdg config')},
     ['--settings', str(work / 'custom/settings.ini')],
     work / 'xdg config/saturn-recomp/mods'),
    ('override', {'XDG_CONFIG_HOME': str(work / 'xdg config')},
     ['--mods-dir', str(work / 'override mods')], work / 'override mods'),
]:
    command = [args.binary, '--cart', 'port/diehard_main.bin', '--mods', 'none',
               '--mods-list'] + options
    result = subprocess.run(command, env=dict(env, **extra), capture_output=True,
                            text=True, check=True, timeout=15)
    actual = next(line[5:] for line in result.stdout.splitlines() if line.startswith('mods='))
    assert Path(actual) == expected and expected.is_dir(), (name, actual, expected)
    assert not (expected / 'enabled.txt').exists(), name
    report[name] = {'directory': str(expected.relative_to(ROOT)), 'disabled': True}
    print(name, 'PASS', expected.relative_to(ROOT), flush=True)
(work / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
