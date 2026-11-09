# Die Hard Arcade (Sega ST-V) — BGM / SEQUENCE format

Reverse of the **music / sequence** side of the SCSP sound system: the M68000
driver's sequencer and the song-data format in `cart.bin`. Companion to
`README.md` (samples, already decoded). Disassembly: capstone `CS_ARCH_M68K`,
`CS_MODE_BIG_ENDIAN` over the first `0x4000` of each SND bank (the 68k driver).
No Ghidra.

Confidence: **HIGH** = proven by code or exact structural math; **MED** = strong
structural evidence; **GUESS** = plausible, unverified.

**Headline result:** the driver is a Sega *standard SCSP MIDI sound module*
(build string `Ver3.0.7|96 4/23|Tit&Sat by Hiro`). The "8-entry opcode jump
table @0x738" is a **MIDI status-byte dispatcher** — the song data is a
**running-status MIDI byte stream**. 21 BGM songs + ~500 SFX across the 8 banks
are now enumerated, located, and decoded to MIDI events. `extract_sequence.py`
emits an event dump, a JSON index, and Standard MIDI Files.

---

## 1. The engine is MIDI — opcode table @0x738 — HIGH

The driver's main loop (`0x18e`) calls the event processor `0x6f4` every
iteration. `0x6f4` pulls a 4-byte event from a 1 KiB ring at `0x800(a6)` (cursor
`$ec6(a6)`, pending count `$ecc(a6)`) and dispatches on the first byte = a MIDI
**status byte**:

```
0x722  d0 = event[0]                 ; MIDI status byte
0x728  d5 = d0 & 0x0f                ; low nibble = MIDI CHANNEL (handler arg)
0x72c  d0 = (d0 >> 2) & 0x1c         ; = (status_high_nibble - 8) * 4
0x732  lea 0x738(pc,d0.w),a1 ; jmp (a1)
```

`(status>>2)&0x1c` is simply the MIDI status **high nibble** scaled ×4, so the
8-entry table at `0x738` is indexed by `status_hi - 8`. Table (8× `bra.w`, the
8th slot is an inline `rts`):

| idx | status | jump → | handler meaning | data bytes |
|-----|--------|--------|-----------------|------------|
| 0 | `0x8n` | `0xe54` | **NOTE OFF** (key-off + release) | 2 |
| 1 | `0x9n` | `0x756` | **NOTE ON** (voice alloc + slot program) | 2 |
| 2 | `0xAn` | `0x181e`| **poly-AT → repurposed as SONG CONTROL** (the SH-2 `0xA0` command) | 2 |
| 3 | `0xBn` | `0x1012`| **CONTROL CHANGE** (meta: sub-table @`0x102c`, ≤0x50 selectors) | 2 |
| 4 | `0xCn` | `0x1798`| **PROGRAM / TONE change** | 1 (port) / 2 (ROM, see §6) |
| 5 | `0xDn` | `0x754` | channel-AT — **NOP** (`rts`) | 1 |
| 6 | `0xEn` | `0xefc` | **PITCH BEND** (14-bit, via FNS table) | 2 |
| 7 | `0xFn` | `0x754` | system — **NOP** (`rts`) | — |

Proven by matching each handler's SCSP actions to MIDI semantics:

* **Note off `0xe54`:** scans 20 voice slots (`0xc40(a6)`, stride `0x20`) for the
  channel; `move.b #0x10,(a5,slot)` writes **key-off** (KYONB) to the SCSP slot.
* **Note on `0x756`:** reads note+velocity, scans the 20 slots to allocate a
  voice, raises IRQ to `0x2200`, programs the SCSP slot (pitch from the tone +
  FNS table). `$c40(a6)` = 20× `0x20`-byte voice structs (matches `README §1`).
* **Pitch bend `0xefc`:** builds a 14-bit value `(b2<<7)|b1`, scales by the
  per-channel bend range `$7(a0)`, converts to SCSP OCT/FNS via the semitone
  table at `0x2b68` (0x180 entries) and writes slot reg `+0x10` (OCT/FNS).
* **Program `0x1798`:** indexes the tone offset table (header entry[14]) and
  binds the channel's tone/instrument.
* **Control `0x1012`:** `d0*2` → word jump-table at `0x102c` (selectors 0–0x50,
  most point at the default `rts`); the live ones are the meta events
  (volume/pan/tempo/loop etc. — individual selectors not each byte-decoded).

---

## 2. The MIDI parser — `0x2de8` / `0x2e92` — HIGH

Two routines feed the `0x800(a6)` event ring, both **standard MIDI running-status
state machines**:

* `0x2de8` — timer-IRQ driven; reads **SCSP MIDI-in** register `$405(a5)`
  (`a5 = 0x100000`, so SCSP `0x100405` = MIBUF low byte).
* `0x2e92` — main-loop driven (`0x552`); reads a second serial port (`0xd00001`).

Parser logic (identical in both):

```
byte >= 0x80 (status):
    == 0xFF           -> RESET: sp=0x4000, jmp 0x120 (driver restart / track end)
    >= 0xF0           -> ignore (system real-time)
    else store status $ec0; data_count = 2, but 1 if 0xC0..0xDF  (program/chan-AT)
byte <  0x80 (data):
    accumulate; when `data_count` bytes collected -> push {status,d1,d2,0}
                into ring 0x800(a6), $ec4 += 4, $ecc += 1
```

So `0xC0`/`0xD0` take **1** data byte, everything else **2** — textbook MIDI.
`0xFF` doubles as the **end-of-track / reset** marker.

---

## 3. SH-2 → driver command = a MIDI `0xA0` message — HIGH

`sound_queue_push(req)` (SH-2 `0x02215f6c`) pushes a 32-bit request into a
16-entry ring (`0x060e0df0`). The request word is always `0xA0 XX YY FF`:

```
0xA0  = MIDI status: poly after-touch, channel 0  -> dispatcher idx 2 (0x181e)
XX    = command / song group byte  (first MIDI data byte)
YY    = sub-song / argument         (second MIDI data byte)
0xFF  = MIDI reset / message terminator
```

The host injects these bytes into the same MIDI stream; handler `0x181e`
interprets a `0xA0` message as a **song-control command** rather than a note.
`XX` ranges (handler `0x181e`):

| XX range | meaning | handler |
|----------|---------|---------|
| `0x00`–`0x0f` | **system commands** (master vol / all-off / fade / bank etc.) | sub-table @`0x198c` (12 live entries), arg = `YY` |
| `0x10`–`0x5f` | **play song** `(XX-0x10, YY)` via the song directory (§4) | `0x1834`+ |
| `0x60`–`0xff` | **direct per-channel SFX / voice control** | `0x2594` (8 slots @`0x380(a6)`, 8-way sub-dispatch @`0x25b6`) |

Examples seen in the SH-2 code: `0xA00200FF`, `0xA00338FF` (system);
`0xA01000FF`..`0xA01004FF` (**BGM**, group 0); `0xA012xxFF`, `0xA013xxFF`,
`0xA014xxFF` (SFX groups). The command space is global; each loaded bank resolves
it against *its own* directory, and out-of-range `(XX,YY)` is silently ignored
(`bgt → rts`).

---

## 4. Song directory — header entry[7] — HIGH

The data-block header (sound-RAM `0x4000`, see `README §2`) entry **[7]**
(`@0x401c`, e.g. bank0 → `0x5f02`) is a **two-level offset table**. All offsets
are u16, relative to the entry[7] base; every table is `count` followed by
`count+1` entries.

```
L1  base:   u16 group_count
            u16 off[g]   -> L2 sub-table              (indexed by XX-0x10)
L2  base+off: u16 sub_count
            u16 off[j]   -> song pointer              (indexed by YY)
song = base + off[j]
```

First song byte classifies it (matches handler `0x181e`: `(a2)+ ; bmi SFX ; beq
BGM`):

* `0x00`  → **BGM** — body is a track-pointer table (§5).
* `0x80`-set (observed `0xc0`) → **SFX** — handled by `0x1912`, a different
  one-shot format (§7).

Per-bank census (`extract_sequence.py --list`):

| bank | dir@ | groups | songs | BGM | SFX |
|------|------|--------|-------|-----|-----|
| 0 | 0x5f02 | 5 | 104 | 4 | 100 |
| 1 | 0x5f84 | 5 | 119 | 4 | 115 |
| 2 | 0x5d58 | 5 | 109 | 3 | 106 |
| 3 | 0x5e8e | 5 | 119 | 5 | 114 |
| 4 | 0x5452 | 3 | 12  | 1 | 11  |
| 5 | 0x56da | 3 | 18  | 1 | 17  |
| 6 | 0x5760 | 5 | 24  | 2 | 22  |
| 7 | 0x56c2 | 5 | 16  | 1 | 15  |

**21 BGM songs** total (group 0, cmd `0xA010YYFF`); the rest SFX.

---

## 5. BGM song = track-pointer table — HIGH

A group-0 song body is a list of u32 BE words:

```
+0x00  flags              e.g. 0x00000041  (bit0/bit6 → player flags, handler 0x186e)
+0x04  0x80a00002         control word A  (bit31 set; constant across all songs)
+0x08  0x80a007YY         control word B  (bit31 set; YY = song variant id)
+..    0x0000xxxx         TRACK pointer  (sound-RAM offset of a MIDI stream)
       0x0000xxxx         …more tracks (1–2 observed; first = channel setup,
                          later = note data)
+..    0xfffffff1         LOOP marker
+..    0x0000xxxx         loop target = a slot *inside this table* (replays from
                          the note track, skipping the setup track)
+..    0xffffffff         END
```

Rule used by the extractor: words with bit31 set and value `0x80a0xxxx` are
**control words**; words `0 < (v & 0xffffff) < 0x6c000` are **track pointers**;
`0xfffffff1` = loop (next word = target); `0xffffffff` = end.

Example — bank0 `0xA01001FF` @`0x6004`:
`00000041 80a00002 80a00701 0000e086 0000b632 fffffff1 00006014 ffffffff`
→ flags `0x41`; tracks `0xe086` (setup) + `0xb632` (notes); loop target `0x6014`
(= the `0x0000b632` slot, i.e. loop replays the note track).

**Each track pointer → a running-status MIDI stream** (§1/§2 semantics),
terminated by a standalone `0xFF` in status position. Decoded example (track 0 =
per-channel init, track 1 = notes):

```
track0 @0xe086:  ch0 bend 40 00 | ch3 cc 07 ff | ch3 bend 49 00 |
                 ch0 program 01 00 | ch0 cc 07 ff | ... (one block per channel) | 0xFF
track1 @0xb632:  ch2 program 11 00 | ch2 cc 07 f6 |
                 ch2 note_on 26 d9 | ch0 note_on 2c ff | ch0 note_on 41 ff | ...
```

Bank0 `0xA01001FF` track1 decodes to **2793 note-ons across 11 channels**, note
range `0x09..0x6d` — real, coherent multi-channel music.

---

## 6. Running status, data counts, and the one discrepancy — MED

The real-time port parser (`0x2de8`, §2) uses **1** data byte for `0xC0`–`0xDF`.
But the ROM track streams empirically carry a **trailing byte after every program
change** (`Cn pp bb`, `bb` usually `0x00`): reading program change as **2 bytes**
is what makes every setup block and note stream parse with *zero* spurious
events. The extractor therefore decodes `0xCn` as 2 bytes for ROM tracks (and
writes a standard 1-byte program change into the exported SMF). `bb` is most
likely a tone-bank / variant selector. This is the only confirmed divergence
between the port parser and the ROM-track reader; the ROM-track reader routine
itself was not fully isolated (the driver code ends ~`0x3022`).

---

## 7. SFX format (groups ≥1) — MED, not fully decoded

SFX songs (first byte bit7 set, e.g. `c0 df XX NN 80 ff 2f 00`) go through driver
handler `0x1912`, which allocates one of 7 SFX slots (`0x310(a6)`, stride `0x10`)
and stores the pointer for a one-shot. The 8-byte record is a tone/sample
trigger + envelope, **not** a MIDI stream. `extract_sequence.py` dumps these raw
(`--index` records the 8 bytes); their field layout is not byte-decoded.

---

## 8. Tone / instrument binding — MED (from code)

A note's actual sample + envelope comes from the tone tables (header entries, see
`README §2`):

* entry[9] `@0x6a3c` — tone **index list** (`0xff`-terminated); sub `0x358`
  reads it, ×4, indexes…
* entry[12]/[13] `@0x6a46`, entry[15] `@0x6a44` — tone **parameter** records
  (become the SCSP slot register image: envelope / LFO / loop + the sample's
  start/loop from the sample directory, entry[0]).
* entry[10] `@0x572e`, entry[14] `@0x57b2` — velocity / curve and tone **offset**
  tables used by note-on (`0x756`) and program change (`0x1798`).
* Pitch: note+bend → OCT/FNS via the `0x2b68` semitone→FNS table (§1).

Mapping a MIDI `program`/`note` to a *specific extracted WAV* (so a rendered-WAV
song becomes exact) needs these tone records walked end-to-end; identified and
role-assigned here, not yet fully tabulated.

---

## 9. Deliverables

* `extract_sequence.py` — decode the sequence side of any bank:
  * `--list` — every song, all banks (type, tracks, control words, loop).
  * `--index out.json` — full JSON directory (header entries, per-song tracks,
    event counts, SFX raw bytes).
  * `--dump BANK XX YY` — human-readable MIDI event dump of one song.
  * `--midi BANK [--out DIR]` — a **Standard MIDI File** per BGM song.
* `seq/songs_index.json` — the full decoded directory (all 8 banks).
* `seq/sample_dump_bank0_A01001.txt` — a representative event dump (truncated).
* `seq/mid/bank0_cmdA01000FF.mid`, `bank0_cmdA01001FF.mid` — two proof SMFs
  (format-1, loadable in any DAW/synth).

---

## 10. Open / uncertain

1. **Timing (the main gap).** The MIDI streams contain **no inline delta-time** —
   events are back-to-back; the real-time parser reads them one-per-arrival and
   derives timing from *when* bytes are fed. The ROM-track player that paces the
   streams (and would supply tempo/wait) was not fully isolated. The exported SMF
   therefore uses an **approximate fixed 1-tick step per event** (documented in
   code): note *content/pitch/instrument is exact*, but **rhythm is not
   authoritative**. Candidates for the real timing: the `0x80a00002` / `0x80a007YY`
   control words (likely tempo/division + song id), and/or a conductor track.
2. Program-change trailing byte `bb` (§6) — tone-bank select vs filler: unconfirmed.
3. SFX 8-byte record fields (§7) — not byte-decoded.
4. Tone-record → exact-WAV mapping (§8) — identified, not tabulated; needed for a
   faithful rendered-WAV of a song.
5. Control-change meta selectors (`0x1012` sub-table @`0x102c`) — volume/pan/etc.
   located but not each byte-decoded.
