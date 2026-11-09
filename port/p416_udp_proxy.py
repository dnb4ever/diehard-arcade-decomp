#!/usr/bin/env python3
"""Test-only portable UDP impairment proxy; no dependency beyond Python.

Host listens on 7845; guest joins this proxy's 7846. Run outside the sandbox
(which denies sockets). Both directions get the selected ONE-WAY latency/loss.
This is a verification utility, not a deployed netplay relay service.
"""
import argparse,heapq,random,select,socket,time
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--listen',default='127.0.0.1:7846')
p.add_argument('--host',default='127.0.0.1:7845')
p.add_argument('--latency',type=float,default=0,help='one-way milliseconds')
p.add_argument('--jitter',type=float,default=0,help='uniform +/- milliseconds')
p.add_argument('--loss',type=float,default=0,help='loss percent')
p.add_argument('--seed',type=int,default=416)
a=p.parse_args()
def endpoint(text):
 ip,port=text.rsplit(':',1);socket.inet_aton(ip);n=int(port)
 if not 1<=n<=65535:raise ValueError('port out of range')
 return ip,n
if a.latency<0 or a.jitter<0 or not 0<=a.loss<=100:p.error('invalid impairment settings')
host=endpoint(a.host);guest=None;rng=random.Random(a.seed);pending=[];seq=0;received=0;dropped=0
with socket.socket(socket.AF_INET,socket.SOCK_DGRAM) as sock:
 sock.bind(endpoint(a.listen));sock.setblocking(False)
 print(f'UDP proxy {a.listen} -> {a.host}; one-way {a.latency} +/- {a.jitter} ms, loss {a.loss}%',flush=True)
 try:
  while True:
   now=time.monotonic()
   while pending and pending[0][0]<=now:
    _,_,dest,data=heapq.heappop(pending)
    try:sock.sendto(data,dest)
    except BlockingIOError:heapq.heappush(pending,(now+.001,seq,dest,data));seq+=1;break
   timeout=min(.05,max(0,pending[0][0]-now)) if pending else .05
   if not select.select([sock],[],[],timeout)[0]:continue
   data,source=sock.recvfrom(65535);received+=1
   if source==host:dest=guest
   else:
    if guest is None:guest=source
    if source!=guest:continue
    dest=host
   if dest is None:continue
   if rng.random()*100<a.loss:dropped+=1;continue
   delay=max(0,a.latency+rng.uniform(-a.jitter,a.jitter))/1000
   heapq.heappush(pending,(time.monotonic()+delay,seq,dest,data));seq+=1
 except KeyboardInterrupt:
  print(f'{received} received, {dropped} dropped, {len(pending)} pending',flush=True)
