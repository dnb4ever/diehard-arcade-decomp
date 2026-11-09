#!/usr/bin/env python3
"""Original geometric skyline badge; no game art, names or third-party logos."""
from io import BytesIO
from pathlib import Path
import struct
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parent.parent;out=root/'port/assets';out.mkdir(exist_ok=True)
# Supersampled primitives are deliberately simple enough to remain distinct at 16px.
scale=3
im=Image.new('RGBA',(1024*scale,1024*scale));draw=ImageDraw.Draw(im)
def polygon(points,fill):draw.polygon([(x*scale,y*scale) for x,y in points],fill=fill)
def rect(box,fill):draw.rectangle(tuple(x*scale for x in box),fill=fill)
draw.rounded_rectangle((48*scale,48*scale,976*scale,976*scale),radius=200*scale,fill='#102c40')
draw.ellipse((682*scale,154*scale,874*scale,346*scale),fill='#ffc66a')
polygon([(208,252),(512,144),(816,252),(792,610),(718,762),(512,892),(306,762),(232,610)],'#bdf5e9')
polygon([(252,282),(512,190),(772,282),(748,598),(682,728),(512,838),(342,728),(276,598)],'#1a6570')
polygon([(294,606),(294,440),(368,440),(368,350),(444,350),(444,486),(484,486),(484,294),(564,294),(564,430),(610,430),(610,368),(694,368),(694,606)],'#edfff8')
rect((292,606,696,644),'#ffc66a')
polygon([(326,672),(698,672),(666,732),(512,826),(358,732)],'#102c40')
for box in [(506,328,542,372),(506,400,542,444),(390,382,422,426),(634,404,670,450)]:rect(box,'#1a6570')
im=im.resize((1024,1024),Image.Resampling.LANCZOS)
im.save(out/'p413-icon.png',optimize=True)
im.save(out/'p413-icon.ico',format='ICO',sizes=[(n,n) for n in [16,24,32,48,64,128,256]])
chunks=[]
for tag,size in [('icp4',16),('icp5',32),('icp6',64),('ic07',128),('ic08',256),('ic09',512),('ic10',1024),('ic11',32),('ic12',64),('ic13',256),('ic14',512)]:
 b=BytesIO();im.resize((size,size),Image.Resampling.LANCZOS).save(b,format='PNG',optimize=True)
 data=b.getvalue();chunks.append(tag.encode()+struct.pack('>I',len(data)+8)+data)
payload=b''.join(chunks);(out/'p413-icon.icns').write_bytes(b'icns'+struct.pack('>I',len(payload)+8)+payload)
print('Generated original P4.13 icon: PNG, 7-size ICO, 11-representation ICNS')
