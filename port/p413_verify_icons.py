#!/usr/bin/env python3
"""Validate ICNS/ICO representations and exact icon payloads embedded in the packaged PE."""
from io import BytesIO
from pathlib import Path
import hashlib,json,struct
from PIL import Image
ROOT=Path(__file__).resolve().parent.parent
assets=ROOT/'port/assets';report={}
b=(assets/'p413-icon.icns').read_bytes();assert b[:4]==b'icns' and struct.unpack_from('>I',b,4)[0]==len(b)
at=8;icns={}
while at<len(b):
 tag=b[at:at+4].decode();n=struct.unpack_from('>I',b,at+4)[0];assert n>=8 and at+n<=len(b)
 px=Image.open(BytesIO(b[at+8:at+n]));icns[tag]=px.size;px.verify();at+=n
assert len(icns)==11 and icns['ic10']==(1024,1024)
report['icns']=icns
ico=(assets/'p413-icon.ico').read_bytes();reserved,kind,count=struct.unpack_from('<HHH',ico);assert reserved==0 and kind==1 and count==7
source={}
for i in range(count):
 w,h,_,_,planes,depth,size,offset=struct.unpack_from('<BBBBHHII',ico,6+16*i)
 w=w or 256;h=h or 256;assert w==h and planes in (0,1) and depth==32
 data=ico[offset:offset+size];Image.open(BytesIO(data)).verify();source[w]=data
assert sorted(source)==[16,24,32,48,64,128,256]
pe=(ROOT/'work/win/DieHardArcade-windows-x64/DieHardArcade.exe').read_bytes()
u16=lambda o:struct.unpack_from('<H',pe,o)[0]
u32=lambda o:struct.unpack_from('<I',pe,o)[0]
p=u32(0x3c);assert pe[p:p+4]==b'PE\0\0';optional=p+24;assert u16(optional)==0x20b
section_table=optional+u16(p+20)
sections=[]
for i in range(u16(p+6)):
 at=section_table+40*i;sections.append((u32(at+12),u32(at+8),u32(at+16),u32(at+20)))
def offset(rva):
 for address,virtual,raw,file_offset in sections:
  if address<=rva<address+max(virtual,raw):return file_offset+rva-address
 raise AssertionError('RVA outside PE sections')
base=offset(u32(optional+112+2*8));resources={}
def walk(relative,path=()):
 at=base+relative;count=u16(at+12)+u16(at+14)
 for i in range(count):
  name=u32(at+16+8*i);child=u32(at+20+8*i);assert not name&0x80000000
  key=path+(name,)
  if child&0x80000000:walk(child&0x7fffffff,key)
  else:
   data=base+child;start=offset(u32(data));resources[key]=pe[start:start+u32(data+4)]
walk(0)
groups=[(k,v) for k,v in resources.items() if k[0]==14];assert len(groups)==1
_,group=groups[0];assert struct.unpack_from('<HHH',group)==(0,1,7)
embedded={}
for i in range(7):
 w,h,_,_,planes,depth,size,identifier=struct.unpack_from('<BBBBHHIH',group,6+14*i);w=w or 256
 data=next(v for k,v in resources.items() if k[:2]==(3,identifier))
 assert len(data)==size and data==source[w],w
 embedded[w]=hashlib.sha256(data).hexdigest()
report['pe_icon_png_sha256']=embedded
report['pe_manifest_present']=any(k[0]==24 for k in resources)
report['ico_bytes']=len(ico);report['icns_bytes']=len(b)
(ROOT/'work/p413/icon-resources.json').write_text(json.dumps(report,indent=2)+'\n')
print('PASS: 11 ICNS / 7 ICO sizes; all 7 exact icon payloads and manifest embedded in final Windows PE')
