#!/usr/bin/env python3
"""Recreate P4.14 scene fixtures with the final production renderer and compare bytes."""
from pathlib import Path
import hashlib,json,subprocess
p=Path(__file__).resolve().with_name('p414_capture_scenes.py')
s=p.read_text().replace("Path('work/p414/visual')", "Path('work/p415/visual')")
s=s.replace("if scale==1 and scene!='floor':", "if False:") # always execute fresh fields
s=s.replace("'work/p414/probes/p414_render_dump'", "'work/p415/probes/p414_render_dump'")
exec(compile(s,str(p),'exec'))
if a.part in ['scenes','ending']:
 reference=json.loads(Path('work/p414/visual/manifest.json').read_text())
 for name,row in report.items():
  assert row['sha256']==reference[name]['sha256'],('P4.14 pixels changed',name)
 print('PASS: all recreated reference PNGs are byte-identical')
