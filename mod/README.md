# Die Hard Arcade (Sega ST-V) — RECOLOR mod

First runnable mod of the `diehard` ST-V romset. It hue-shifts the game's 3D-model
master colour palette so the recolor is visible in-game (the orange/brown Die Hard
look becomes teal/blue).

## What gets patched — the one fact that matters

The flat-shaded 3D models pick colours by a global index (`cidx`, face record
`+0x0e`) into one contiguous **Saturn BGR555** master palette: **6316 entries
(0x18ac), 16-bit big-endian**, bit15 = white sentinel.

| | value |
|---|---|
| Palette in reconstructed `cart.bin` | offset **0x9bb438** (SH-2 cart base 0x02000000 → 0x029bb438; A-bus mirror 0x229bb438) |
| Romset file it actually lives in | **`mpr19116.3`** |
| Offset inside that file | **0x1bb438** |
| Length | 0x3158 bytes (6316 × 2) |

### Why that file / offset (reverse of `build_cart.py`)
`build_cart.py` places `mpr19116.3` at cart `0x800000` with
`ROM_LOAD16_WORD_SWAP`, i.e. `cart[0x800000:] = word_swap(mpr19116.3)`
(`word_swap` swaps the two bytes of every 16-bit word).
So the palette file-offset = `0x9bb438 − 0x800000 = 0x1bb438`.
`word_swap` is its own inverse, so to write edited colours back:
`mpr_region = word_swap(new_cart_region)`. The rest of the ROM is untouched.

## Files here
- `recolor.py` — the pipeline: reads the original romset, recolors the palette inside
  `mpr19116.3`, repackages a new romset `.zip`. Size-preserving.
- `verify.py` — emulator-free proof: re-runs `build_cart.py`'s word-swap+placement on
  the modified romset and confirms cart `0x9bb438` holds the new colours and nothing
  else moved. Never touches the real `cart.bin`.
- `diehard_recolor.zip` — the proof romset (180° hue rotation).
- `diehard_orig_copy.zip` — untouched copy of the source romset (original never modified).

## Run it in an emulator

You need a **ST-V BIOS**. MAME treats `diehard` as a clone of the `stvbios` parent, so
it needs the BIOS ROMs. This romset already bundles them (`epr-*`, `stv*.bin`), but the
safe setup is to also have a standard `stvbios.zip` in your roms folder.

### MAME
1. Put `stvbios.zip` in your MAME `roms/` folder (standard ST-V BIOS set).
2. Copy `diehard_recolor.zip` into `roms/` and **rename it to `diehard.zip`**
   (MAME loads sets by filename; the set/driver name is `diehard`).
3. Launch:  `mame diehard -bios us`   (or your preferred region bios).

**Checksum warning (expected):** we changed `mpr19116.3`'s data, so its CRC no longer
matches MAME's database (CRC `af9e627b` → `35735363`; **size is unchanged at 4194304
bytes**, which is what lets MAME still load it). MAME will show a red
"one or more ROMs are incorrect" screen at boot — **press any key / click OK / Enter to
dismiss it** and the game runs with the recolor. There is no clean "ignore CRC" flag in
modern MAME; the warning screen is simply dismissible because the file size is correct.
(`-skip_gameinfo` skips the info screen but not the bad-dump warning.)

### Other ST-V emulators (Mednafen/SSF-style)
Same idea: supply a ST-V BIOS and point the emulator at the recolored set. Anything that
loads `mpr19116.3` by name/size will accept it; CRC-strict loaders behave like MAME above.

## Make your own recolor
```
python3 recolor.py --in ../diehard.zip --out diehard_recolor.zip --mode hue --hue 0.5
python3 recolor.py --in ../diehard.zip --out diehard_recolor.zip --mode swap_rb
python3 recolor.py --in ../diehard.zip --out diehard_recolor.zip --mode tint --tint 1.4,0.6,0.6
python3 recolor.py --in ../diehard.zip --out diehard_recolor.zip --mode invert
```
`--mode hue --hue 0.5` = 180° rotation (the shipped proof). Or `import recolor` and pass
your own `u16 -> u16` function to `recolor_palette()`. White/sentinel entries (bit15 set)
are left untouched so the engine's white markers survive.

## Verification (run `python3 verify.py` from the project root)
- Original reconstruction matches the existing `cart.bin` palette → **the mapping is correct**.
- Patch changed **5214 / 6316** entries (greys/blacks/whites unchanged by a hue rotate).
- **All 10420 changed bytes fall inside 0x1bb438..0x1be590 — zero bytes changed anywhere else.**
- Every other romset file is byte-identical (same CRC); `mpr19116.3` size preserved,
  only its CRC changed (expected).

Example decoded entries (RGB, old → new): cidx 10 `[173,99,49]`→`[49,123,173]`,
cidx 23 `[222,156,16]`→`[16,82,222]` — orange/amber model colours become blue.
