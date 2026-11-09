#!/usr/bin/env python3
"""Audit rebuilt owner archives, matching payloads, old-loader smoke and identities."""
import hashlib,json,os,re,subprocess,tarfile,zipfile
from pathlib import Path
root=Path(__file__).resolve().parent.parent;os.chdir(root)
out=Path('work/p419/release');out.mkdir(parents=True,exist_ok=True)
app=Path('work/p417/DieHardArcade-x86_64.AppImage').resolve();appdir=Path('work/p417/DieHardArcade.AppDir')
env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','P416_','APPIMAGE_'))}
# Independently extract both archives; compare regular files and symbolic links.
extracted=out/'appimage';extracted.mkdir(exist_ok=True)
with (out/'extract.log').open('w') as log:subprocess.run([str(app),'--appimage-extract'],cwd=extracted,env=env,stdout=log,stderr=log,check=True,timeout=60)
tarroot=out/'tar';tarroot.mkdir(exist_ok=True)
with tarfile.open('work/p417/DieHardArcade-x86_64.tar.gz') as archive:archive.extractall(tarroot,filter='data')
def inventory(base):
 return {str(p.relative_to(base)):('link:'+os.readlink(p) if p.is_symlink() else hashlib.sha256(p.read_bytes()).hexdigest()) for p in base.rglob('*') if p.is_file() or p.is_symlink()}
expected=inventory(appdir)
assert inventory(extracted/'squashfs-root')==expected
assert inventory(tarroot/'DieHardArcade.AppDir')==expected
assert not any(re.search(r'cart\.bin|diehard_main\.bin|\.wav$|\.inputs$|\.quickstate$|shim|probe',s,re.I) for s in expected)
with zipfile.ZipFile('work/win/DieHardArcade-windows-x64.zip') as archive:
 assert archive.testzip() is None
 names=archive.namelist();assert not any(re.search(r'cart\.bin|diehard_main\.bin|\.wav$|\.inputs$|\.quickstate$|shim|probe',s,re.I) for s in names)
 readme=next(s for s in names if Path(s).name=='README-windows.txt');assert b'Connect by code' in archive.read(readme)
assert 'Connect by code' in (appdir/'README-linux.txt').read_text()
# Actual glibc 2.31 loader and libraries, not just a symbol-version estimate.
sysroot=Path('work/p417/opt/sysroot').resolve();package=appdir.resolve();d=out/'glibc231';d.mkdir(exist_ok=True)
command=[str(sysroot/'lib64/ld-linux-x86-64.so.2'),'--library-path',f'{package}/usr/lib:{sysroot}/lib/x86_64-linux-gnu:{sysroot}/usr/lib/x86_64-linux-gnu',str(package/'usr/bin/saturn_stv'),'--cart','port/diehard_main.bin','--cartrom','cart.bin','--vblanks','2000','--shots','1900','--out',str(d)]
with (out/'glibc231.log').open('w') as log:subprocess.run(command,env=env,stdout=log,stderr=log,check=True,timeout=180)
assert hashlib.md5((d/'shot-1900.png').read_bytes()).hexdigest()=='9bd5386ea7439621c19674381086e8fd'
manifest=json.loads((appdir/'package-manifest.json').read_text());assert max(tuple(map(int,s.split('.'))) for s in manifest['glibc_max'].values())<=(2,31)
identities={}
for name in ['port/run/P46/build-sdl/p410_identity.h','work/win/build-windows/p410_identity.h','work/p417/build-linux/p410_identity.h']:
 identities[name]=Path(name).read_text()
assert len({re.search(r'STV_NETPLAY_CODE 0x([0-9a-f]+)',text)[1] for text in identities.values()})==1
artifacts={}
for name in ['work/p417/DieHardArcade-x86_64.AppImage','work/p417/DieHardArcade-x86_64.tar.gz','work/win/DieHardArcade-windows-x64.zip']:
 p=Path(name);artifacts[name]={'bytes':p.stat().st_size,'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}
report={'artifacts':artifacts,'appdir_entries':len(expected),'windows_zip_entries':len(names),'glibc_max':manifest['glibc_max'],'glibc231_md5':'9bd5386ea7439621c19674381086e8fd','identities':identities,'payload':expected}
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
Path('work/p419/artifact-report.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: archives / payload / no ROMs or shims / glibc 2.31 loader / cross-platform source identity')
print(json.dumps(artifacts,indent=2))
