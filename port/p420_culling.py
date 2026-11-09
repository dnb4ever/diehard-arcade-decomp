#!/usr/bin/env python3
"""Apply the narrow, REJECTED gameplay-changing culling experiment to a snapshot only."""
import argparse,hashlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('generated',type=Path);a=p.parse_args()
f=a.generated/'p_dh_004.cpp';s=f.read_text()
if 'p420_options.h' in s: raise SystemExit('Already patched; restore snapshot file before reapplying')
base=Path('work/p420/port-base/run/P41/recomp/p_dh_004.cpp')
if s!=base.read_text(): raise SystemExit('Unfamiliar generated input; refuse')
s='#include "p420_options.h"\nstatic uint32_t p420_cull_x(uint32_t v) { return p420_culling() ? uint32_t(int64_t(int32_t(v))*704/p420_width()) : v; }\n'+s
for old,new in [
 ('c.r[0] = 0x160u;  // 06017F5A', 'c.r[0] = p420_culling() ? p420_width()/2 : 0x160u;  // 06017F5A'),
 ('c.r[1] = 0xFFFFFEA0u;  // 06017F5C','c.r[1] = p420_culling() ? uint32_t(-p420_width()/2) : 0xFFFFFEA0u;  // 06017F5C'),
 ('c.r[2] += c.r[7];  // 06019A5C','c.r[2] += p420_cull_x(c.r[7]);  // 06019A5C'),
 ('c.r[2] -= c.r[7];  // 06019A64','c.r[2] -= p420_cull_x(c.r[7]);  // 06019A64')]:
 assert s.count(old)==1,old
 s=s.replace(old,new)
f.write_text(s);print('Patched two horizontal culling tests only; polls and continuation IDs unchanged')
