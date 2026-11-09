#!/usr/bin/env python3
"""
Die Hard Arcade (Sega ST-V) -- SCSP sound bank extractor.

The SND region of cart.bin (cart 0x1000000) holds 8 SCSP sound-RAM images, one
per 0x80000 bank. Each image = 0x4000 M68000 driver + 0x6c000 data. The data
block begins with an area-control header (typed u32 pointer table) at sound-RAM
0x4000; header entry [0] points to the SAMPLE DIRECTORY. Samples are 16-bit
signed big-endian PCM (SCSP has no ADPCM). Directory addresses are direct
sound-RAM offsets, i.e. offsets straight into the bank image.

Sample-directory layout (header[0] -> dir_ptr):
  dir_ptr+0 : u32  sample count N
  then N (+1 trailing sentinel) records of 12 bytes:
    +0 u32  flag(bit31) | start_addr
    +4 u32  flag(bit31) | length_bytes       (chains: start+len == next start)
    +8 u32  flag(bit31) | loop_start_addr     (== start => loop whole sample;
                                               >= end => one-shot)
Low 24 bits are the value; bit31 is a driver loop/format flag.

Usage:
  extract_sound.py <cart.bin> [--bank N] [--rate HZ] [--out DIR] [--list] [--only i,j,k]
"""
import sys, os, struct, wave, argparse

SND_BASE    = 0x1000000      # cart offset of SND region
BANK_STRIDE = 0x80000        # one SCSP image per bank
HDR         = 0x4000         # area-control header (sound-RAM addr)
REC         = 12             # sample-directory record size (bytes)

def read_bank(cart, bank):
    with open(cart, "rb") as f:
        f.seek(SND_BASE + bank*BANK_STRIDE)
        return f.read(BANK_STRIDE)

def hdr_pointers(d):
    ptrs=[]
    for i in range(HDR, HDR+0x40, 4):
        v=struct.unpack(">I", d[i:i+4])[0]
        ptrs.append(None if v==0xffffffff else v)
    return ptrs

def parse_directory(d):
    dir_ptr = struct.unpack(">I", d[HDR:HDR+4])[0] & 0xffffff
    n_samples = struct.unpack(">I", d[dir_ptr:dir_ptr+4])[0]
    off = dir_ptr + 4
    recs=[]
    while off+REC <= len(d):
        s,l,lp = struct.unpack(">III", d[off:off+REC])
        s&=0xffffff; l&=0xffffff; lp&=0xffffff
        if l==0 or l>len(d) or s>len(d) or s+l>len(d):
            break
        recs.append(dict(idx=len(recs), dir_off=off, start=s, length=l, loop=lp,
                         sentinel=(len(recs) >= n_samples)))
        off+=REC
    return dir_ptr, n_samples, recs

def write_wav(path, pcm_be, rate):
    n=len(pcm_be)//2
    vals=struct.unpack(">%dh"%n, pcm_be[:n*2])
    le=struct.pack("<%dh"%n, *vals)
    with wave.open(path,"wb") as w:
        w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
        w.writeframes(le)

def kind(r):
    if r["sentinel"]: return "SENTINEL"
    if r["loop"]==r["start"]: return "loop"
    if r["loop"]>=r["start"]+r["length"]-2: return "1shot"
    return "loop@%x"%r["loop"]

def main():
    ap=argparse.ArgumentParser()
    ap.add_argument("cart")
    ap.add_argument("--bank", type=int, default=0)
    ap.add_argument("--rate", type=int, default=22050,
                    help="nominal WAV rate (SCSP true pitch is set per-note)")
    ap.add_argument("--out", default="wav")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--only", default=None, help="comma list of sample indices")
    a=ap.parse_args()
    d=read_bank(a.cart, a.bank)
    dir_ptr, n, recs = parse_directory(d)
    print("bank %d: directory @ sound-RAM 0x%x, count=%d (+%d sentinel) = %d records"
          %(a.bank, dir_ptr, n, len(recs)-n, len(recs)))
    if a.list:
        for r in recs:
            print("  [%3d] start=0x%06x len=0x%06x (%6d B) end=0x%06x loop=0x%06x %s"
                  %(r["idx"],r["start"],r["length"],r["length"],
                    r["start"]+r["length"],r["loop"],kind(r)))
        return
    os.makedirs(a.out, exist_ok=True)
    sel = [int(x) for x in a.only.split(",")] if a.only else [r["idx"] for r in recs]
    for i in sel:
        r=recs[i]
        pcm=d[r["start"]:r["start"]+r["length"]]
        p=os.path.join(a.out,"bank%d_s%03d_%06x.wav"%(a.bank,i,r["start"]))
        write_wav(p, pcm, a.rate)
    print("wrote %d WAV(s) to %s/ at %d Hz"%(len(sel), a.out, a.rate))

if __name__=="__main__":
    main()
