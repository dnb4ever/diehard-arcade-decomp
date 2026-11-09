# Die Hard Arcade (Sega ST-V) — SOUND decode

Reverse of the **SCSP sound system**: the M68000 sound driver + the PCM sample
bank format in the SND region of `cart.bin`. This is the ST-V **sound CPU**
(Motorola 68000 running inside the SCSP / YMF292), decoded with capstone
(`CS_ARCH_M68K`, `CS_MODE_BIG_ENDIAN`) + byte analysis. No Ghidra.

Confidence: **HIGH** = proven by code or exact structural math; **MED** = strong
structural evidence; **GUESS** = plausible, unverified.

SND region recap (from `../README.md`): 4 MiB at cart `0x1000000`, 8 SCSP images,
`0x80000` stride. `sound_init` @ SH2 `0x02215da8` copies a `0x4000` 68k driver to
SCSP RAM `0x25a00000` and `0x6c000` of data to `0x25a04000`. In 68k address space
the driver sits at `0x000000`, the data at `0x004000`. **A sound-RAM address X is
simply byte X of the bank image** (image base cart `0x1000000 + bank*0x80000`).

---

## 1. M68000 driver layout (first 0x4000 of a bank) — HIGH

```
0x0000  u32  initial SP  = 0x00004000
0x0004  u32  reset PC    = 0x00000120
0x0008..0x00ff   68k exception vector table (autovectors; most point at a
                 default handler 0x0508; level-irq vectors at 0x60..0x7c)
0x0100  ASCII  "Ver3.0.7|96 4/23|Tit&Sat by Hiro"   (driver build string)
0x0120  CODE   reset entry; real 68k program starts here
0x0120..~0x2fff  driver code (sequencer + SCSP slot programming)
~0x3000..0x3fff  driver RAM / work area (a6 base = 0x3000): track buffers,
                 20 voice slots, command/flag bytes. SH2 writes here to
                 request sound (sound_queue_push).
```

NOTE: the "increasing-offset table" `0x120, 0x500, 0x508…` that the asset README
flagged as a possible sequence table is the **68k exception vector table**
(`SP, PC, bus-error, …`), not sound data.

Reset code (proven): `move #$2700,sr` (mask IRQs) → clear D0–D7/A0–A4 →
`a5 = 0x100000` (**SCSP register base** in 68k space) → `a6 = 0x3000` (work RAM)
→ zero loop → `move.w #$300,$400(a5)` (SCSP ctrl, matches the SH2 write to
`0x25b00400`) → init subroutines → main loop polling the command byte.

Key proven facts:
* `a5 = 0x100000` = SCSP slot/control registers; slot N regs at `a5 + N*0x20`.
* Routine @ `0x216` zero-inits **32 SCSP slots** (loop count 0x1f, stride 0x20).
* `a6 = 0x3000` work RAM; voice/channel structs are **0x20 bytes**, up to **20**
  of them (scanned with `moveq #$13,d7` @ `0x76c`, stride `0x20`).
* The data header is read via absolute-short addressing: `movea.l $4018.w,a2`,
  `movea.l $4024.w,a2`, `movea.l $403c.w,a3` — i.e. the driver indexes the data
  block through the typed pointer table at sound-RAM `0x4000` (below).

---

## 2. Data block header @ sound-RAM 0x4000 (area-control block) — HIGH

A table of u32 **sound-RAM pointers** (`0xffffffff` = empty), followed by a
signature. Bank 0:

```
0x4000  [0] 0x6454   -> SAMPLE DIRECTORY                 (HIGH)
0x4004  [1..4] 0xffffffff (empty)
0x4014  [5] 0x5338   -> sequence/control data            (MED)
0x4018  [6] 0x6704   -> voice/DSP param block (used by sub 0x288)   (MED)
0x401c  [7] 0x5f02   -> offset table (sequence pointers?) (MED/GUESS)
0x4020  [8] 0x6452   -> sample directory - 2             (MED)
0x4024  [9] 0x6a3c   -> tone index list (used by sub 0x358)        (MED)
0x4028  [10] 0x572e  -> monotonic lookup/curve table      (GUESS)
0x4030  [12] 0x6a46  -> tone parameter data               (MED)
0x4038  [14] 0x57b2  -> offset table                      (GUESS)
0x403c  [15] 0x6a44  -> tone parameter data (used by sub 0x358)    (MED)
0x4040  0x00360000, 0x00001fff   (config words)
0x4050  0x0006c000  = data block allocation size (0x6c000)
0x4054  "SEGA" 0x96051608 "1.00"   (signature | date 1996-05-16 | version)
```

This is Sega's **standard Saturn/ST-V SCSP sound driver** data format. Entry [0]
is the only one needed for sample extraction and is fully decoded; the tone/
sequence entries are identified structurally but not byte-for-byte decoded.

---

## 3. SAMPLE FORMAT — HIGH

**16-bit signed PCM, big-endian, mono, uncompressed.** The SCSP (YMF292) supports
8-bit and 16-bit linear PCM only — **there is no Yamaha ADPCM in SCSP**; the
ADPCM the task mentioned does not apply here. Every sample in these banks is
16-bit (the PCM8B per-slot bit is clear for these tones).

Proof: decoded as 16-bit BE the data is smooth/zero-centred (DC offset ~tens of
LSB on ±32768 scale); decoded as 8-bit it is noise. Extracted samples show a
healthy spread of statistics — tonal (zero-cross rate 0.04) through percussive
(0.56), RMS 1k–13k, peaks near full-scale — i.e. real audio, not a misread.

**Sample rate is not stored.** SCSP replays a sample at a rate set per *note* by
the slot OCT/FNS pitch registers (driven from the tone + sequence). There is no
canonical rate in the directory; WAVs are written at a **nominal 22050 Hz**
(override with `--rate`). Pitch/tuning of a given note needs the tone+sequence
decode (section 5).

---

## 4. SAMPLE DIRECTORY — HIGH

Header entry [0] -> directory. Layout:

```
dir+0   u32  N  = sample count          (bank 0: 0x38)
dir+4   N (+1 trailing) records, 12 bytes each:
  +0  u32   flag(bit31) | start_addr     (sound-RAM = image offset)
  +4  u32   flag(bit31) | length_bytes
  +8  u32   flag(bit31) | loop_start_addr (absolute)
```
Low 24 bits = value; **bit31** is a driver loop/format flag (set on most
start/length words). Records **chain**: `start + length == next.start`
(samples are stored contiguously), which is how the record boundaries were
proven. `loop_start == start` → loop whole sample; `loop_start >= end` →
one-shot; otherwise a mid-sample loop point. All 57 bank-0 loop fields fall
inside `[start, end)` (valid).

The trailing record ends exactly at the known data-end marker (bank 0:
`0x6f988`). The count word is one less than the number of chained records, so the
last record is either a guard/sentinel or a real final sample (ambiguous by ±1);
the extractor keeps it by default so no audio is dropped.

Counts per bank (all 8 parse with the identical format — HIGH):

| bank | dir @  | count | records |
|------|--------|-------|---------|
| 0 | 0x6454 | 56 | 57 |
| 1 | 0x6592 | 60 | 61 |
| 2 | 0x628a | 54 | 55 |
| 3 | 0x6416 | 49 | 50 |
| 4 | 0x54e6 | 29 | 30 |
| 5 | 0x57aa | 27 | 28 |
| 6 | 0x5884 | 24 | 25 |
| 7 | 0x578e | 27 | 28 |

~**334 records total** across the 8 banks (likely one sound set per stage/scene).

Note: the PCM region *below* the first directory entry (bank 0: `0x4080..0xe0e6`)
also contains 16-bit PCM but is not covered by entry [0]; it is intermixed with
the sequence/tone control structures (entries [5]/[7]) and the low samples they
reference — decoding those tables (section 5) would enumerate them. Not yet done.

---

## 5. MUSIC / SEQUENCE FORMAT — DECODED (see sequence_format.md)

**UPDATE — fully decoded, see `sequence_format.md`.** The driver is a Sega
*standard SCSP MIDI sound module* (`Ver3.0.7|96 4/23|Tit&Sat by Hiro`). The
8-entry jump table @`0x738` is a **MIDI status-byte dispatcher** (0x8n note-off,
0x9n note-on, 0xBn control, 0xCn program, 0xEn pitch-bend); song data is a
**running-status MIDI byte stream**. Songs live in header **entry[7]** (`@0x401c`),
a 2-level table indexed by the SH-2 command `0xA0 XX YY FF` (`XX-0x10`, `YY`).
Group 0 = **BGM** (track-pointer tables -> MIDI streams); groups 1+ = **SFX**.
21 BGM songs + ~500 SFX enumerated across the 8 banks; `extract_sequence.py`
emits event dumps, a JSON index and Standard MIDI Files. Remaining gap: inline
delta-time/tempo (exported MIDI rhythm is approximate; note content is exact).

Original proven structure (code @ `0x70e`):

* Each track has a read cursor `$ec6(a6)` into a track buffer at `0x800(a6)`
  (masked `& 0x3ff` → 1 KiB ring per track).
* A command byte is fetched; `opcode = (byte>>2) & 0x1c` selects one of **8
  handlers** via a PC-relative jump table at `0x738`
  (targets: `0xe54, 0x756, 0x181e, 0x1012, 0x1798, 0x754(nop/rts), 0xefc`).
  The low nibble (`byte & 0x0f`) is the handler's argument (e.g. channel).
* Handler `0x756` reads note/params and scans the **20 voice slots**
  (`0xc40(a6)`, stride `0x20`) to allocate a voice, then (with IRQs raised to
  level `0x2200`) programs the SCSP slot via the tone data.
* **Tone data** (header entries [9]/[12]/[15], @ `~0x6a3c`): sub `0x358` reads a
  byte index list (`0x6a3c`, `0xff`-terminated), multiplies by 4, and indexes a
  table at `0x6a44` to fetch per-tone bytes — these become the SCSP slot register
  image (envelope/LFO/pitch + the sample's start/loop from the directory).
* Sequence song data is reached through the offset tables at entries [5]/[7]
  (`0x5338`, `0x5f02` = arrays of small relative offsets).

Full opcode semantics (note on/off, tempo, loop, pitch-bend, etc.) are **not**
decoded — that is a substantial additional disassembly effort. What is proven is
enough to locate songs and tones and to understand how a note maps to a sample.

---

## 6. Deliverables

* `extract_sound.py` — parse any bank's sample directory and dump samples to WAV.
  * `--list` : print the directory (start/len/loop/type).
  * `--bank N` (0–7), `--rate HZ` (default 22050), `--only i,j,k`, `--out DIR`.
  * Example: `python extract_sound.py cart.bin --bank 0 --out wav`
* `extract_sequence.py` — decode the BGM/SFX sequence side (see
  `sequence_format.md`): `--list`, `--index out.json`, `--dump BANK XX YY`,
  `--midi BANK`. Emits event dumps, a song directory JSON, and Standard MIDI Files.
* `seq/` — `songs_index.json` (full decoded directory), a sample event dump,
  and two proof `.mid` files (`seq/mid/`).
* `wav/` — 7 representative bank-0 WAVs (16-bit PCM): a looped tone (s000), a
  long one-shot (s001), a mid-loop sample (s005), the largest sample (s009,
  26 KiB), a tiny one (s017), a mid-bank sample (s040), and the trailing record (s056).

### Plausibility (why the WAVs are real audio)
| sample | bytes | dur @22k | RMS | DC | peak | zero-cross |
|--------|-------|----------|-----|----|------|-----------|
| s000 | 6032  | 0.137s | 12876 | 186 | 32757 | 0.110 (tonal) |
| s001 | 15940 | 0.361s |  9547 | -30 | 32639 | 0.036 (bass/tonal) |
| s005 | 10669 | 0.242s |  9713 | 167 | 32626 | 0.147 |
| s009 | 26051 | 0.591s |  4397 | 404 | 31971 | 0.529 (noisy/perc) |
| s017 |   842 | 0.019s |  1166 | 122 |  3336 | 0.565 |
| s040 |  8460 | 0.191s |  8313 |  37 | 30079 | 0.160 |

Tiny DC offsets confirm correct 16-bit-signed decoding; near-full-scale peaks and
the wide zero-cross spread confirm genuine, varied waveforms. Durations
(0.02–0.6 s) are right for game SFX / voice / short instrument loops. True
musical pitch depends on the per-note SCSP rate (section 3/5).

---

## 7. Caveats / open items

* Sample rate is nominal (SCSP sets pitch per note) — WAVs are correct PCM but
  not necessarily at authored pitch. HIGH that PCM bytes are exact; the 22050 Hz
  playback rate is a GUESS.
* `count` vs `records` off-by-one: last record kept by default (section 4).
* ~~The full sequence/tone opcode set~~ — DECODED (`sequence_format.md`): the
  opcode table is MIDI; songs are in entry[7]. The low PCM region below entry[0]
  overlaps the MIDI track data (BGM tracks point there). Remaining: inline
  timing/tempo, SFX record fields, tone-record->exact-WAV mapping.
* Header entries [5]/[6]/[7]/[9]/[12]/[15] are role-identified (MED), not fully
  parsed.
