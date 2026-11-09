#!/usr/bin/env python3
"""Local STUN/PCP/NAT-PMP/SSDP+SOAP fixtures and a complete code-based game pair.

File mode runs production socket I/O through the explicit test shim. UDP mode
uses kernel UDP plus local HTTP; it must be repeated outside this sandbox.
"""
import argparse, hashlib, json, os, re, shutil, socket, struct, subprocess, threading, time
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--transport',choices=['files','udp'],default='files')
p.add_argument('--binary',default='port/run/P46/build-sdl/saturn_stv')
p.add_argument('--frames',type=int,default=900)
p.add_argument('--tag',default='local')
a=p.parse_args();root=Path(__file__).resolve().parent.parent;os.chdir(root)
out=Path('work/p419/traversal')/a.tag;out.mkdir(parents=True,exist_ok=True)
if a.transport=='udp':
 try:
  with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as probe:probe.bind(('127.0.0.1',0))
 except OSError as e:raise SystemExit('Kernel sockets unavailable; run this command outside the sandbox: '+str(e))
shim=Path('work/p419/traversal/datagram-shim.so').resolve()
subprocess.run(['cc','-shared','-fPIC','-O2','port/p419_datagram_shim.c','-ldl','-o',str(shim)],check=True)
include='tools/saturn-recomp/runtime/src/machine'
probe=Path('work/p419/traversal/discovery-probe')
subprocess.run(['c++','-std=c++20','-O2','-pthread','-I'+include,'port/p419_discovery_probe.cpp',include+'/net_discovery.cpp',include+'/net_transport.cpp','-o',str(probe)],check=True)
setup_probe=Path('work/p419/traversal/setup-probe')
subprocess.run(['c++','-std=c++20','-O2','-pthread','-I'+include,'-Itools/saturn-recomp/runtime/include','port/p419_setup_probe.cpp',include+'/net_setup.cpp',include+'/net_discovery.cpp',include+'/net_transport.cpp','-o',str(setup_probe)],check=True)
env={k:v for k,v in os.environ.items() if not k.startswith(('STV_','SATURN_','P416_'))}
report={}
class Fixture:
 def __init__(self,name):
  self.name=name;self.stop=False;self.errors=[];self.events=[];self.serial=0;self.handled=set();self.sockets={};self.tcp=None
  self.d=out/name;self.d.mkdir(exist_ok=True);self.wire=(self.d/'datagrams').resolve()
  if self.wire.exists():shutil.rmtree(self.wire)
  self.wire.mkdir()
  self.ports=[20001,20002,20004,20005]
  if a.transport=='files':
   for port in self.ports:(self.wire/str(port)).mkdir()
  else:
   for port in self.ports:
    s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);s.bind(('127.0.0.1',port));s.setblocking(False);self.sockets[port]=s
   self.tcp=socket.socket(socket.AF_INET,socket.SOCK_STREAM);self.tcp.setsockopt(socket.SOL_SOCKET,socket.SO_REUSEADDR,1);self.tcp.bind(('127.0.0.1',20003));self.tcp.listen();self.tcp.setblocking(False)
  self.thread=threading.Thread(target=self.loop);self.thread.start()
 def environment(self):
  e=dict(env,STV_NETPLAY_GATEWAY='127.0.0.1:20001',STV_NETPLAY_SSDP='127.0.0.1:20002',STV_NETPLAY_LAN='127.0.0.1')
  if a.transport=='files':e.update(LD_PRELOAD=str(shim),P416_DATAGRAM_DIR=str(self.wire))
  return e
 def reply(self,port,source,data):
  if a.transport=='udp':self.sockets[port].sendto(data,source);return
  folder=self.wire/str(source[1])
  if not folder.exists():return
  self.serial+=1
  header=struct.pack('<Q',time.monotonic_ns())+struct.pack('<H',socket.AF_INET)+struct.pack('!H4s',port,socket.inet_aton('127.0.0.1'))+bytes(8)+struct.pack('<I',len(data))+bytes(4)
  file=folder/f'{time.monotonic_ns():020}-{os.getpid()}-fixture-{self.serial}.pkt';tmp=file.with_suffix('.tmp');tmp.write_bytes(header+data);tmp.replace(file)
 def datagram(self,port,source,data):
  if port in (20004,20005):
   assert len(data)==20 and data[:8]==bytes.fromhex('000100002112a442'),data.hex()
   mapped=source[1]+(1 if self.name=='symmetric' and port==20005 else 0)
   attr=struct.pack('!HHBBHI',0x20,8,0,1,mapped^0x2112,0xcb007109^0x2112a442)
   response=bytes.fromhex('0101000c2112a442')+data[8:20]+attr
   self.reply(port,source,response);self.events.append('stun');return
  if port==20002:
   assert data.startswith(b'M-SEARCH * HTTP/1.1\r\n')
   self.reply(port,source,b'HTTP/1.1 200 OK\r\nLOCATION: http://127.0.0.1:20003/root.xml\r\nST: urn:schemas-upnp-org:device:InternetGatewayDevice:1\r\n\r\n');self.events.append('ssdp');return
  if data and data[0]==2:
   if self.name not in ['pcp','lifecycle-pcp','direct-pcp']:self.reply(port,source,bytes.fromhex('02810001')+bytes(20));return
   assert len(data)==60 and data[36]==17
   lifetime=struct.unpack_from('!I',data,4)[0];response=bytearray(data);response[1]=129;response[8:24]=bytes(16);struct.pack_into('!HHI',response,40,7845,40000,0);response[44:56]=bytes(10)+b'\xff\xff';struct.pack_into('!I',response,56,0xcb007109)
   if self.name=='lifecycle-pcp' and lifetime:struct.pack_into('!I',response,4,2)
   self.reply(port,source,bytes(response));self.events.append('delete' if lifetime==0 else 'add');return
  if self.name=='pmp' and data==b'\0\0':self.reply(port,source,bytes.fromhex('008000000000002acb007109'));return
  if self.name=='pmp' and len(data)==12 and data[:2]==b'\0\1':
   inside,external,lifetime=struct.unpack_from('!HHI',data,4);self.reply(port,source,struct.pack('!BBHIHHI',0,129,0,42,inside,0 if lifetime==0 else 40000,lifetime));self.events.append('delete' if lifetime==0 else 'add');return
  self.reply(port,source,bytes.fromhex('008000010000000000000000'))
 def http(self,request):
  assert b'\r\n\r\n' in request
  header,body=request.split(b'\r\n\r\n',1)
  service='urn:schemas-upnp-org:service:WANIPConnection:1'
  if header.startswith(b'GET '):
   self.events.append('description');body=f'<root><device><serviceList><service><serviceType>{service}</serviceType><controlURL>/control</controlURL></service></serviceList></device></root>'.encode()
  elif b'#GetExternalIPAddress' in header:body=b'<u:GetExternalIPAddressResponse><NewExternalIPAddress>203.0.113.9</NewExternalIPAddress></u:GetExternalIPAddressResponse>'
  elif b'#AddPortMapping' in header:
   assert b'<NewProtocol>UDP</NewProtocol>' in body and b'<NewInternalPort>7845</NewInternalPort>' in body and b'<NewLeaseDuration>600</NewLeaseDuration>' in body
   self.events.append('add');body=b'<u:AddPortMappingResponse/>'
  elif b'#DeletePortMapping' in header:self.events.append('delete');body=b'<u:DeletePortMappingResponse/>'
  else:raise AssertionError(header)
  if self.name=='igd-chunked':return b'HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\nConnection: close\r\n\r\n'+f'{len(body):x}\r\n'.encode()+body+b'\r\n0\r\n\r\n'
  return b'HTTP/1.1 200 OK\r\nContent-Type: text/xml\r\nContent-Length: '+str(len(body)).encode()+b'\r\nConnection: close\r\n\r\n'+body
 def loop(self):
  try:
   while not self.stop:
    for port in self.ports:
     if a.transport=='files':
      for file in sorted((self.wire/str(port)).glob('*.pkt')):
       b=file.read_bytes()
       if struct.unpack_from('<Q',b)[0]>time.monotonic_ns():continue
       file.unlink();source=(socket.inet_ntoa(b[12:16]),struct.unpack_from('!H',b,10)[0]);n=struct.unpack_from('<I',b,24)[0];self.datagram(port,source,b[32:32+n])
     else:
      while True:
       try:data,source=self.sockets[port].recvfrom(65535)
       except BlockingIOError:break
       self.datagram(port,source,data)
    if a.transport=='files':
     for file in self.wire.glob('tcp-*.request'):
      if file.name in self.handled:continue
      data=file.read_bytes();dest,request=data.split(b'\n',1)
      if b'\r\n\r\n' not in request:continue
      header,body=request.split(b'\r\n\r\n',1);match=re.search(rb'Content-Length: (\d+)',header,re.I)
      if match and len(body)<int(match[1]):continue
      assert dest==b'20003';response=self.http(request);target=file.with_suffix('.response');tmp=target.with_suffix('.tmp');tmp.write_bytes(response);tmp.replace(target);self.handled.add(file.name)
    elif self.tcp:
     try:connection,_=self.tcp.accept()
     except BlockingIOError:connection=None
     if connection:
      with connection:
       connection.settimeout(2);request=b''
       while True:
        request+=connection.recv(16384)
        if b'\r\n\r\n' not in request:continue
        h,b=request.split(b'\r\n\r\n',1);match=re.search(rb'Content-Length: (\d+)',h,re.I)
        if not match or len(b)>=int(match[1]):break
       connection.sendall(self.http(request))
    time.sleep(.001)
  except BaseException as e:self.errors.append(repr(e))
 def close(self):
  self.stop=True;self.thread.join(timeout=5);assert not self.thread.is_alive()
  for s in self.sockets.values():s.close()
  if self.tcp:self.tcp.close()
  assert not self.errors,self.errors
for name in ['stun','symmetric','pcp','pmp','igd','igd-chunked']:
 fixture=Fixture(name)
 try:
  mode='stun' if name in ['stun','symmetric'] else 'map';endpoint='127.0.0.1:20004' if mode=='stun' else '127.0.0.1:20001'
  r=subprocess.run([str(probe),mode,endpoint],env=fixture.environment(),capture_output=True,text=True,timeout=20)
  (fixture.d/'run.log').write_text(r.stdout+r.stderr);assert r.returncode==0,(name,r.stdout,r.stderr)
  if mode=='stun':assert f'symmetric={int(name=="symmetric")}' in r.stdout
  else:assert fixture.events.count('add')==2 and fixture.events.count('delete')==1,(name,fixture.events)
  report[name]={'stdout':r.stdout,'events':fixture.events};print(name,'PASS',flush=True)
 finally:fixture.close()
fixture=Fixture('lifecycle-pcp')
try:
 r=subprocess.run([str(setup_probe)],env=fixture.environment(),capture_output=True,text=True,timeout=15)
 (fixture.d/'run.log').write_text(r.stdout+r.stderr);assert r.returncode==0,(r.stdout,r.stderr)
 assert fixture.events.count('add')>=2 and fixture.events.count('delete')==1,fixture.events
 report['lifecycle-pcp']={'stdout':r.stdout,'events':fixture.events};print('lease lifecycle / timed renewal / exit removal PASS',flush=True)
finally:fixture.close()
def game_pair(name,by_code,negative=False):
 fixture=Fixture(name);processes=[]
 for who in ['a','b']:(fixture.d/(who+'.code')).unlink(missing_ok=True)
 try:
  for who,other,port in [('a','b',20101),('b','a',20102)]:
   d=fixture.d/who;d.mkdir(exist_ok=True);code=fixture.d/(who+'.code');remote=fixture.d/(other+'.code')
   e=fixture.environment();e.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',SDL_RENDER_DRIVER='software',STV_NETPLAY_DUMP='1',STV_NETPLAY_PLAYED=str(d/'played.s16'))
   net=['--netplay-code',str(port),'--netplay-code-out',str(code),'--netplay-peer-code','@'+str(remote),'--netplay-no-map'] if by_code else ['--netplay-host','7845'] if who=='a' else ['--netplay-join','127.0.0.1:7845']
   cmd=[a.binary,'--cart','port/diehard_main.bin','--cartrom','cart.bin','--netplay-stun-servers','127.0.0.1:20004,127.0.0.1:20005','--input-delay','2' if negative and who=='b' else '3','--vblanks',str(a.frames),'--shots',str(a.frames),'--wav',str(d/'audio.wav'),'--window','--settings','-','--stvinput','100:COIN,130:START1,101:COIN2,131:START2','--out',str(d)]+net
   f=(d/'run.log').open('w');processes.append((subprocess.Popen(cmd,env=e,stdout=f,stderr=f),f,d,cmd))
  for proc,f,d,cmd in processes:
   assert proc.wait(timeout=120)==(2 if negative else 0),(d,(d/'run.log').read_text()[-2500:]);f.close()
   log=(d/'run.log').read_text()
   if negative:assert 'ROM, delay or frame-limit mismatch' in log;continue
   assert 'desync/failure=0' in log
   if by_code:assert 'netplay: code path ' in log
  if negative:
   report[name]={'rejected':True,'commands':[cmd for _,_,_,cmd in processes]};print(name,'PASS',flush=True);return
  sums={}
  for artifact in [f'shot-{a.frames}.png','audio.wav','final-machine.bin']:
   hashes=[hashlib.sha256((d/artifact).read_bytes()).hexdigest() for _,_,d,_ in processes];assert hashes[0]==hashes[1],(artifact,hashes);sums[artifact]=hashes[0]
  checks=[re.findall(r'confirmed field=(\d+) CRC=([0-9a-f]+)',(d/'run.log').read_text()) for _,_,d,_ in processes];assert checks[0]==checks[1]
  if not by_code:assert fixture.events.count('add')==1 and fixture.events.count('delete')==1,fixture.events
  report[name]={'artifacts':sums,'checkpoints':len(checks[0]),'commands':[cmd for _,_,_,cmd in processes],'events':fixture.events};print(name,'PASS',flush=True)
 finally:
  for proc,f,_,_ in processes:
   if proc.poll() is None:proc.terminate();proc.wait()
   f.close()
  fixture.close()
game_pair('code-pair',True)
game_pair('direct-pcp',False)
game_pair('code-delay-mismatch',True,True)
(out/'report.json').write_text(json.dumps({'transport':a.transport,'results':report},indent=2)+'\n')
