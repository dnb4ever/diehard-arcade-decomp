#!/usr/bin/env python3
"""
Die Hard Arcade (Sega ST-V) -- SCSP SEQUENCE / BGM extractor.

Companion to extract_sound.py (samples). This tool decodes the MUSIC/SEQUENCE
side of the Sega standard SCSP sound driver ("Ver3.0.7|96 4/23|Tit&Sat by Hiro")
that lives in the first 0x4000 of every SND bank.

The driver is a MIDI-based synth: the opcode jump table @0x738 in the 68k driver
dispatches on the MIDI *status high-nibble* (0x8n note-off, 0x9n note-on,
0xBn control, 0xCn program, 0xEn pitch-bend, 0xDn/0xFn ignored). Song data is a
running-status MIDI byte stream. See sequence_format.md for the full reverse.

Song directory = data-block header entry[7] (sound-RAM 0x401c -> e.g. 0x5f02).
Two-level table: level 1 indexed by (SH-2 cmd byte XX - 0x10), level 2 by YY.
Each resolved song:
  * byte0 == 0x00            -> BGM (sequenced); body is a track-pointer table.
  * byte0  & 0x80 (e.g 0xc0) -> SFX (one-shot, driver handler 0x1912); raw dump.

BGM track-pointer table (u32 BE, from the song pointer):
  +0  0x000000FF-ish flags word (bank0 songs: 0x00000041)
  +4  0x80a00002            control word A (bit31 set; global, constant)
  +8  0x80a007YY            control word B (bit31 set; YY = song variant id)
  .. 0x0000xxxx             one or more TRACK pointers (sound-RAM offset of a
                            MIDI stream); first track is usually channel setup
                            (program/volume/bend), later track(s) are notes.
  .. 0xfffffff1             LOOP marker
  .. 0x0000xxxx             loop target = a slot inside this same table (replays
                            from the note track, skipping the setup track)
  .. 0xffffffff             END

MIDI stream decode: running status. Data-byte counts follow the driver's
real-time parser (0x2de8: 0xC0-0xDF = 1 byte) EXCEPT that the ROM track data
empirically carries a 2nd byte after every program change (0xCn pp bb) -- so we
read program change as 2 bytes, which makes every setup block and note stream
parse without spurious events. A standalone 0xff in status position ends a track
(driver treats 0xff as reset). See sequence_format.md "Open / uncertain".

Usage:
  extract_sequence.py cart.bin --list                 # all songs, all banks
  extract_sequence.py cart.bin --index out.json       # JSON directory dump
  extract_sequence.py cart.bin --dump BANK XX YY       # event dump of one song
  extract_sequence.py cart.bin --midi BANK [--out DIR] # SMF for each BGM in bank
"""
import sys, os, struct, argparse, json

SND_BASE = 0x1000000
STRIDE   = 0x80000
HDR      = 0x4000
DATA_END = 0x6c000        # data block size (ptrs beyond this are not tracks)

def read_bank(cart, bank):
    with open(cart, "rb") as f:
        f.seek(SND_BASE + bank*STRIDE)
        return f.read(STRIDE)

def u16(d,o): return struct.unpack(">H", d[o:o+2])[0]
def u32(d,o): return struct.unpack(">I", d[o:o+4])[0]

def hdr_entry(d, i):
    v = u32(d, HDR + i*4)
    return None if v == 0xffffffff else (v & 0xffffff)

# ---- song directory (entry[7]) ---------------------------------------------
def song_dir(d):
    """Yield dicts: group (XX-0x10), sub (YY), cmd byte XX, cmd YY, ptr, type."""
    base = hdr_entry(d, 7)
    ngroup = u16(d, base)
    out = []
    for g in range(ngroup + 1):
        sub = base + u16(d, base + 2 + g*2)
        nsub = u16(d, sub)
        for j in range(nsub + 1):
            sp = base + u16(d, sub + 2 + j*2)
            b0 = d[sp]
            typ = "BGM" if b0 == 0 else ("SFX" if (b0 & 0x80) else "?")
            out.append(dict(XX=0x10+g, YY=j, ptr=sp, type=typ,
                            cmd=0xA0000000 | ((0x10+g) << 16) | (j << 8) | 0xFF))
    return base, out

# ---- BGM track-pointer table -----------------------------------------------
def bgm_tracks(d, song_ptr):
    """Parse the track-pointer table. Return (flags, controls, tracks, loop)."""
    flags = u32(d, song_ptr)
    controls = []
    tracks = []
    loop = None
    o = song_ptr + 4
    while o + 4 <= len(d):
        v = u32(d, o); o += 4
        if v == 0xffffffff:
            break
        if v == 0xfffffff1:              # loop marker; next long = loop target
            loop = u32(d, o); o += 4
            break
        if v & 0x80000000:               # control word (e.g. 0x80a007YY)
            controls.append(v)
        elif 0 < (v & 0xffffff) < DATA_END:
            tracks.append(v & 0xffffff)
        # else: 0 / junk -> skip
    return flags, controls, tracks, loop

# ---- MIDI stream decode -----------------------------------------------------
NAMES = {0x80:"note_off", 0x90:"note_on", 0xa0:"poly_at",
         0xb0:"control", 0xc0:"program", 0xd0:"chan_at", 0xe0:"bend"}

def nbytes(status):
    hi = status & 0xf0
    # driver real-time parser uses 1 for 0xC/0xD; ROM tracks carry 2 after
    # program change (pp bb). channel-pressure 0xD assumed 1 (no ROM example).
    if hi == 0xc0: return 2
    if hi == 0xd0: return 1
    return 2

def decode_midi(d, off, limit=20000):
    """Decode a running-status MIDI stream. Return list of event dicts."""
    ev = []
    status = 0
    i = off
    while i < len(d) and len(ev) < limit:
        b = d[i]
        if b >= 0x80:
            if b == 0xff:
                ev.append(dict(off=i, kind="end"))
                break
            if b >= 0xf0:
                ev.append(dict(off=i, kind="sys", status=b)); i += 1; status = 0
                continue
            status = b; i += 1
        if status < 0x80:                 # data byte with no status yet -> stop
            ev.append(dict(off=i, kind="stop_nostatus")); break
        nb = nbytes(status)
        if i + nb > len(d): break
        data = list(d[i:i+nb]); i += nb
        hi = status & 0xf0; ch = status & 0x0f
        ev.append(dict(kind=NAMES.get(hi, "?%02x"%hi), hi=hi, ch=ch, data=data))
    return ev

# ---- minimal Standard MIDI File writer -------------------------------------
def _vlq(n):
    out = bytearray([n & 0x7f]); n >>= 7
    while n:
        out.insert(0, (n & 0x7f) | 0x80); n >>= 7
    return bytes(out)

def write_smf(path, tracks_events, ppqn=48):
    """tracks_events: list of event-lists (one per BGM track). Timing is
    APPROXIMATE: 1 tick advance per event group (see sequence_format.md)."""
    def chunk(tag, body):
        return tag + struct.pack(">I", len(body)) + body
    hdr = struct.pack(">HHH", 1, len(tracks_events), ppqn)
    out = bytearray(chunk(b"MThd", hdr))
    for events in tracks_events:
        trk = bytearray()
        # tempo ~120bpm on first track
        trk += _vlq(0) + bytes([0xff, 0x51, 0x03]) + struct.pack(">I", 500000)[1:]
        for e in events:
            if e["kind"] in ("end", "sys", "stop_nostatus"): continue
            hi = e.get("hi"); ch = e.get("ch", 0); data = e.get("data", [])
            if hi is None: continue
            trk += _vlq(1)                 # 1 tick between events (approximate)
            if hi == 0xc0:                 # program change -> 1 data byte in SMF
                trk += bytes([0xc0 | ch, data[0] & 0x7f])
            elif hi == 0xd0:
                trk += bytes([0xd0 | ch, data[0] & 0x7f])
            else:
                d0 = data[0] & 0x7f
                d1 = (data[1] & 0x7f) if len(data) > 1 else 0
                trk += bytes([hi | ch, d0, d1])
        trk += _vlq(0) + bytes([0xff, 0x2f, 0x00])   # end of track
        out += chunk(b"MTrk", bytes(trk))
    with open(path, "wb") as f:
        f.write(out)

# ---- commands ---------------------------------------------------------------
def cmd_list(cart):
    for bank in range(8):
        d = read_bank(cart, bank)
        base, songs = song_dir(d)
        bgm = [s for s in songs if s["type"] == "BGM"]
        sfx = [s for s in songs if s["type"] == "SFX"]
        print("BANK %d  dir@0x%04x  %d songs (%d BGM, %d SFX)  sig=%s"
              % (bank, base, len(songs), len(bgm), len(sfx),
                 d[0x4054:0x4058].decode("latin1")))
        for s in bgm:
            fl, ct, tr, lp = bgm_tracks(d, s["ptr"])
            print("   BGM  cmd=0x%08X  @0x%04x  tracks=%s  ctl=%s  loop=%s"
                  % (s["cmd"], s["ptr"],
                     ",".join("0x%05x"%t for t in tr),
                     ",".join("0x%08x"%c for c in ct),
                     ("0x%06x"%(lp&0xffffff)) if lp else "-"))

def cmd_index(cart, path):
    doc = {"banks": []}
    for bank in range(8):
        d = read_bank(cart, bank)
        base, songs = song_dir(d)
        bd = {"bank": bank, "dir": base,
              "sig": d[0x4054:0x4058].decode("latin1"),
              "header": {i: hdr_entry(d, i) for i in range(16)},
              "songs": []}
        for s in songs:
            rec = {"cmd": "0x%08X" % s["cmd"], "XX": s["XX"], "YY": s["YY"],
                   "ptr": s["ptr"], "type": s["type"]}
            if s["type"] == "BGM":
                fl, ct, tr, lp = bgm_tracks(d, s["ptr"])
                rec["flags"] = fl
                rec["controls"] = ["0x%08x" % c for c in ct]
                rec["tracks"] = tr
                rec["loop"] = (lp & 0xffffff) if lp else None
                rec["events"] = sum(len(decode_midi(d, t)) for t in tr)
            else:
                rec["raw"] = " ".join("%02x" % b for b in d[s["ptr"]:s["ptr"]+8])
            bd["songs"].append(rec)
        doc["banks"].append(bd)
    with open(path, "w") as f:
        json.dump(doc, f, indent=1)
    print("wrote", path)

def cmd_dump(cart, bank, XX, YY):
    d = read_bank(cart, bank)
    base, songs = song_dir(d)
    hit = [s for s in songs if s["XX"] == XX and s["YY"] == YY]
    if not hit:
        print("no song XX=0x%02x YY=%d in bank %d" % (XX, YY, bank)); return
    s = hit[0]
    print("bank %d  cmd=0x%08X  @0x%04x  type=%s" % (bank, s["cmd"], s["ptr"], s["type"]))
    if s["type"] != "BGM":
        print("  SFX raw:", " ".join("%02x" % b for b in d[s["ptr"]:s["ptr"]+16]))
        return
    fl, ct, tr, lp = bgm_tracks(d, s["ptr"])
    print("  flags=0x%08x controls=%s loop=%s"
          % (fl, [hex(c) for c in ct], hex(lp & 0xffffff) if lp else None))
    for ti, t in enumerate(tr):
        print("  --- track %d @0x%05x ---" % (ti, t))
        for e in decode_midi(d, t):
            if e["kind"] in ("end", "stop_nostatus"):
                print("      [%s @0x%05x]" % (e["kind"], e.get("off", 0))); break
            if e["kind"] == "sys":
                print("      sys %02x" % e["status"]); continue
            dd = " ".join("%02x" % x for x in e["data"])
            print("      ch%-2d %-9s %s" % (e["ch"], e["kind"], dd))

def cmd_midi(cart, bank, outdir):
    d = read_bank(cart, bank)
    base, songs = song_dir(d)
    os.makedirs(outdir, exist_ok=True)
    n = 0
    for s in songs:
        if s["type"] != "BGM": continue
        fl, ct, tr, lp = bgm_tracks(d, s["ptr"])
        tev = [decode_midi(d, t) for t in tr]
        p = os.path.join(outdir, "bank%d_cmd%08X.mid" % (bank, s["cmd"]))
        write_smf(p, tev)
        n += 1
        print("wrote %s  (%d tracks, %d events)" % (p, len(tev), sum(len(e) for e in tev)))
    print("%d BGM MIDI files for bank %d" % (n, bank))

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("cart")
    ap.add_argument("--list", action="store_true")
    ap.add_argument("--index", metavar="JSON")
    ap.add_argument("--dump", nargs=3, type=lambda x: int(x, 0), metavar=("BANK","XX","YY"))
    ap.add_argument("--midi", type=int, metavar="BANK")
    ap.add_argument("--out", default="mid")
    a = ap.parse_args()
    if a.list:   cmd_list(a.cart)
    if a.index:  cmd_index(a.cart, a.index)
    if a.dump:   cmd_dump(a.cart, *a.dump)
    if a.midi is not None: cmd_midi(a.cart, a.midi, a.out)
    if not (a.list or a.index or a.dump or a.midi is not None):
        ap.print_help()

if __name__ == "__main__":
    main()
