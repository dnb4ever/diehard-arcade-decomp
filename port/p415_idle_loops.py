#!/usr/bin/env python3
"""Batch proven empty slave-loop iterations between existing SH-2 polls.

Fail closed if the generated/continuation function changes. No poll, continuation
label, scheduling point or command handler is removed. Run after P4.10's pass.
"""
from pathlib import Path
import hashlib,re,sys
p=Path(sys.argv[1])/'p_dh_000.cpp';s=p.read_text()
marker='    sh2_stv_idle_batch(c); // P415_EMPTY_SLAVE_V1\n'
m=re.search(r'void f_060051B0\(SH2Context& c\) \{.*?\n\}',s,re.S)
if not m or hashlib.sha256(m[0].replace(marker,'').encode()).hexdigest()!='316d5b485d3945d4f71af2cd64274a194eecfda78a03781a4d6798e1693e7ba5':
 raise SystemExit('P4.15 idle loop changed: review the guest loop before updating its fingerprint')
if marker in m[0]:raise SystemExit(0)
s=s[:m.start()]+m[0].replace('L_060051EE:\n','L_060051EE:\n'+marker)+s[m.end():]
p.write_text(s)
