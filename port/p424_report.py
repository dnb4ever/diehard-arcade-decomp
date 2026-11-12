#!/usr/bin/env python3
"""Require final on/off/release oracle equality and summarize retained P4.24 evidence."""
from pathlib import Path
import hashlib, json, re, statistics
root=Path(__file__).resolve().parent.parent
w=root/'work/p424'
read=lambda p:json.loads(p.read_text())
sha=lambda b:hashlib.sha256(b).hexdigest()
refs=[root/'work/p411/regression/after/manifest.json']+[root/f'work/{m}/regression/manifest.json' for m in ['p413','p414','p415','p416']]+[root/'work/p419/regression/final-window/manifest.json',root/'work/p422/regression/linux/manifest.json']
fields=['png','trace_sha256','guest_trace_sha256','wav_sha256']
reports={tag:read(w/tag/'report.json') for tag in ['window-on','window-off','appimage']}
result={'oracle_references':[str(p.relative_to(root)) for p in refs], 'oracles':{}, 'pacing':{}}
for tag,report in reports.items():
 for refpath in refs:
  for name,ref in read(refpath).items():
   for key in fields:assert report[name][key]==ref[key],(tag,str(refpath),name,key)
 result['oracles'][tag]={'pngs':sum(len(report[n]['png']) for n in ['nocoin','gameover','combat','legacy-combat']),'trace_pairs':4,'wavs':4,'smoke_md5':{n:report[n]['md5'] for n in ['smoke-1x','smoke-2x']}}
for tag in ['window-on','window-off']:
 result['pacing'][tag]={}
 for name in ['nocoin','gameover','combat','legacy-combat']:
  r=reports[tag][name];rates=r['rates'];lag=r['lag_ms']
  result['pacing'][tag][name]=dict(rate_min=min(rates),rate_max=max(rates),rate_median=statistics.median(rates),lag_min_ms=min(lag),lag_max_ms=max(lag),lag_first_ms=lag[0],lag_last_ms=lag[-1],omitted=sum(r['skipped']))
on=(w/'window-on/state/state.quickstate').read_bytes();off=(w/'window-off/state/state.quickstate').read_bytes();before=(w/'baseline/state/state.quickstate').read_bytes()
assert on==off,'Complete on/off quick-state files differ'
assert on[48:]==before[48:],'Baseline machine-state payload differs'
# The exact-build fingerprint changes with source; the option does not change it.
assert on[:16]+on[24:]==before[:16]+before[24:]
names=re.findall(r'\{"([\w-]+)",', (root/'tools/saturn-recomp/runtime/src/machine/state.cpp').read_text().split('kQuickParts[] = {')[1].split('};')[0])
at=48;parts={}
for name in names:
 assert on[at:at+len(name)]==name.encode(),name
 at+=len(name);n=int.from_bytes(on[at:at+8],'little');at+=8
 parts[name]=sha(on[at:at+n]);at+=n
assert at==len(on)
for key in ['png','wav_sha256']:
 assert reports['window-on']['state'][key]==reports['window-off']['state'][key]==read(w/'baseline/report.json')['state'][key],key
result['state']={'bytes':len(on),'whole_file_on_off_sha256':sha(on),'payload_baseline_on_off_sha256':sha(on[48:]),'parts':parts,'only_baseline_header_difference':'exact-build identity bytes 16..23'}
result['release']=read(w/'release/report.json')
for path,value in result['release']['artifacts'].items():assert sha((root/path).read_bytes())==value['sha256'],path
result['patch']=read(w/'patch-verification.json')
result['presentation']=read(w/'present-verification.json')
for case,path in [('proxy','netplay-proxy/report.json'),('audio','audio/report.json'),('appimage','netplay-appimage/report.json'),('stall','netplay-stall/report.json')]:
 report=read(w/path);result[case+'_cases']=list(report['cases'])
(w/'verification-report.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps({k:v for k,v in result.items() if k in ['oracles','pacing','proxy_cases','audio_cases','appimage_cases','stall_cases']},indent=2))
print('PASS: every reference, complete on/off state, baseline payload, package hashes and all case reports')
