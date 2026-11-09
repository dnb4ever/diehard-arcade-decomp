#!/usr/bin/env python3
"""Qualify every round-3 entry against the unchanged, refreshed oracle."""
from pathlib import Path
import datetime,json
import round3_generate as generate
WORK=Path('work/converge/round3')
def main():
 snapshot=(WORK/'snapshot').resolve();dt=generate.load_oracle(snapshot)
 cache=WORK/'symbols.json'
 if not cache.exists():cache.write_text(json.dumps(dt.load_symbols(),sort_keys=True)+'\n')
 symbols=json.loads(cache.read_text());dt.load_symbols=lambda:symbols
 addresses=Path(__file__).with_name('round3_overrides.txt').read_text().split()+['02213164','02228106','02223c74']
 dest=(WORK/'qualification').resolve();dest.mkdir(exist_ok=True)
 code=dt.main(addresses+['--vectors','64','--jobs','4','--output-dir',str(dest),'--json',str(dest/'report.json')])
 data=json.loads((dest/'report.json').read_text())
 if code or any(r['status'] not in ('PASS','PASS-void') for r in data['results']):raise SystemExit('Qualification failed; no certificate published')
 cert=dict(version=data['version'],vectors=64,harness_sha256=data['harness_sha256'],cart_sha256=data['cart_sha256'],functions={r['address']:{k:r[k] for k in ('status','vectors','source_sha256','branch_coverage')} for r in data['results']})
 (WORK/'qualification.json').write_text(json.dumps(cert,indent=2)+'\n')
 polls=WORK/'qualification-polls';polls.mkdir(exist_ok=True)
 (polls/'report.json').write_text(json.dumps(dict(results=[r for r in data['results'] if r['address'] in ('02213164','02228106')]),indent=2)+'\n')
 for row in data['results']:
  p=WORK/'attempts'/f"{row['address']}.jsonl";p.parent.mkdir(exist_ok=True)
  with p.open('a') as f:f.write(json.dumps(dict(ts=datetime.datetime.now(datetime.timezone.utc).isoformat(),agent='converge-round3',result=row['status'],note='64-vector frozen qualification',report=str(dest/'report.json').replace(str(Path.cwd())+'/','')))+'\n')
 print(data['counts'])
if __name__=='__main__':main()
