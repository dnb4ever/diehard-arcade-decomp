import struct
import os
ROM = open(os.path.expanduser("~/decomp/diehard-stv/cart.bin"),"rb").read()
# find runs of 0xff >= 0x400 bytes (gaps/padding) in 0x400000..0x1400000
runs=[]
o=0x400000
while o<len(ROM):
    if ROM[o]==0xff:
        s=o
        while o<len(ROM) and ROM[o]==0xff: o+=1
        if o-s>=0x400: runs.append((s,o))
    else:
        o+=1
print("### FF-padding gaps >=0x400 (segment delimiters) in 0x400000..end")
prev=0x400000
for s,e in runs:
    print("  seg %08x..%08x (len %#9x) | gap %08x..%08x (%#x)"%(prev,s,s-prev,s,e,e-s))
    prev=e
