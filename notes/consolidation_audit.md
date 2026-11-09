# Consolidation naming audit - Die Hard Arcade (ST-V, SH-2) decomp

Generated 2026-10-21 by the consolidation pass (READ-ONLY audit; no source files were modified).
Scope: 934 translation units in `project/src_functional/*.c`.

**Method.** Each file's canonical ROM entry is its filename as a full SH-2 address (`0x02xxxxxx`; census low-24 / missing-leading-zero filenames normalised by `+0x02000000`). A file's *public entry symbol* is its sole non-`static` function definition (body-bearing). References are address-form `func_<hex>(` call sites. A reference is a link defect when no definition of that exact symbol name exists but a definition DOES exist at the resolved address under a different name.

## Summary

| Category | Count |
|---|---|
| (a) `func_<hex>.c` filenames | 0 |
| (b) non-canonical filenames (census/low-24/short) | 134 |
| (b) canonical filename but truncated internal `func_`/`f_` entry | 31 |
| (c) address-form calls that do not resolve to their definition's name | 210 |
|   - definition is a SEMANTIC name | 115 |
|   - definition is `func_`/`f_` (zero-pad/width only) | 95 |
| (d) duplicate entry-address pairs (2 TUs, same ROM entry) | 39 |
| (d) duplicate public symbol name (hard link collision) | 3 |

## (a) Files named `func_<hex>.c` instead of `<hex>.c`

**None.** All source files follow the bare `<hex>.c` convention. No fix needed.

## (b) Filename / internal-name convention defects

### (b.1) Canonical 8-hex filename, but the defined function is a truncated placeholder
Fix: rename the function body to the canonical name (safe, local rename) OR add a one-line alias.

| file | defines | canonical | fix |
|---|---|---|---|
| `02001b8e.c` | `func_0201b8e` | `func_02001b8e` | rename def `func_0201b8e`->`func_02001b8e` |
| `02215f08.c` | `func_0215f08` | `func_02215f08` | rename def `func_0215f08`->`func_02215f08` |
| `02216c88.c` | `func_0216c88` | `func_02216c88` | rename def `func_0216c88`->`func_02216c88` |
| `02216f7a.c` | `func_0216f7a` | `func_02216f7a` | rename def `func_0216f7a`->`func_02216f7a` |
| `02242cb8.c` | `func_242cb8` | `func_02242cb8` | rename def `func_242cb8`->`func_02242cb8` |
| `0224806a.c` | `f_24806a` | `func_0224806a` | rename def `f_24806a`->`func_0224806a` |
| `02248104.c` | `f_248104` | `func_02248104` | rename def `f_248104`->`func_02248104` |
| `022482dc.c` | `f_2482dc` | `func_022482dc` | rename def `f_2482dc`->`func_022482dc` |
| `02248436.c` | `f_248436` | `func_02248436` | rename def `f_248436`->`func_02248436` |
| `0224852c.c` | `f_24852c` | `func_0224852c` | rename def `f_24852c`->`func_0224852c` |
| `022485b8.c` | `f_2485b8` | `func_022485b8` | rename def `f_2485b8`->`func_022485b8` |
| `0224886e.c` | `f_24886e` | `func_0224886e` | rename def `f_24886e`->`func_0224886e` |
| `022489d0.c` | `f_2489d0` | `func_022489d0` | rename def `f_2489d0`->`func_022489d0` |
| `02248bb8.c` | `f_248bb8` | `func_02248bb8` | rename def `f_248bb8`->`func_02248bb8` |
| `02248f88.c` | `f_248f88` | `func_02248f88` | rename def `f_248f88`->`func_02248f88` |
| `022490a0.c` | `f_2490a0` | `func_022490a0` | rename def `f_2490a0`->`func_022490a0` |
| `022490e4.c` | `f_2490e4` | `func_022490e4` | rename def `f_2490e4`->`func_022490e4` |
| `0224910e.c` | `f_24910e` | `func_0224910e` | rename def `f_24910e`->`func_0224910e` |
| `02249240.c` | `f_249240` | `func_02249240` | rename def `f_249240`->`func_02249240` |
| `022492f8.c` | `f_2492f8` | `func_022492f8` | rename def `f_2492f8`->`func_022492f8` |
| `02249836.c` | `f_249836` | `func_02249836` | rename def `f_249836`->`func_02249836` |
| `02249898.c` | `f_249898` | `func_02249898` | rename def `f_249898`->`func_02249898` |
| `022499a6.c` | `f_2499a6` | `func_022499a6` | rename def `f_2499a6`->`func_022499a6` |
| `02249a04.c` | `f_249a04` | `func_02249a04` | rename def `f_249a04`->`func_02249a04` |
| `02249a82.c` | `f_249a82` | `func_02249a82` | rename def `f_249a82`->`func_02249a82` |
| `02249b80.c` | `f_249b80` | `func_02249b80` | rename def `f_249b80`->`func_02249b80` |
| `02249bba.c` | `f_249bba` | `func_02249bba` | rename def `f_249bba`->`func_02249bba` |
| `02249cda.c` | `f_249cda` | `func_02249cda` | rename def `f_249cda`->`func_02249cda` |
| `02249da8.c` | `f_249da8` | `func_02249da8` | rename def `f_249da8`->`func_02249da8` |
| `02249f52.c` | `func_0249f52` | `func_02249f52` | rename def `func_0249f52`->`func_02249f52` |
| `0224a000.c` | `f_24a000` | `func_0224a000` | rename def `f_24a000`->`func_0224a000` |

### (b.2) Non-canonical filenames (and thus non-canonical entry symbols)
134 files are named with a census/low-24-bit or missing-leading-zero address instead of the full 8-hex SH-2 entry. Each should be renamed to `<8hexSH2>.c` with its entry symbol set to `func_<8hexSH2>` (or a consistent semantic name). These are the main reason address-form callers in (c) fail to resolve. Full list:

| current file | canonical file | canonical symbol |
|---|---|---|
| `0201b92.c` | `02201b92.c` | `func_02201b92` |
| `0201c9c.c` | `02201c9c.c` | `func_02201c9c` |
| `0201f20.c` | `02201f20.c` | `func_02201f20` |
| `0204370.c` | `02204370.c` | `func_02204370` |
| `0207360.c` | `02207360.c` | `func_02207360` |
| `0207542.c` | `02207542.c` | `func_02207542` |
| `02079d0.c` | `022079d0.c` | `func_022079d0` |
| `02080ac.c` | `022080ac.c` | `func_022080ac` |
| `0208a52.c` | `02208a52.c` | `func_02208a52` |
| `020d58c.c` | `0220d58c.c` | `func_0220d58c` |
| `020d8d0.c` | `0220d8d0.c` | `func_0220d8d0` |
| `020e6d4.c` | `0220e6d4.c` | `func_0220e6d4` |
| `0214e2e.c` | `02214e2e.c` | `func_02214e2e` |
| `0216c38.c` | `02216c38.c` | `func_02216c38` |
| `0219c44.c` | `02219c44.c` | `func_02219c44` |
| `0219f26.c` | `02219f26.c` | `func_02219f26` |
| `0226eb8.c` | `02226eb8.c` | `func_02226eb8` |
| `02275e4.c` | `022275e4.c` | `func_022275e4` |
| `0227602.c` | `02227602.c` | `func_02227602` |
| `0228168.c` | `02228168.c` | `func_02228168` |
| `02286cc.c` | `022286cc.c` | `func_022286cc` |
| `0228738.c` | `02228738.c` | `func_02228738` |
| `0228768.c` | `02228768.c` | `func_02228768` |
| `0228bb6.c` | `02228bb6.c` | `func_02228bb6` |
| `0228c70.c` | `02228c70.c` | `func_02228c70` |
| `0228ca2.c` | `02228ca2.c` | `func_02228ca2` |
| `0228f08.c` | `02228f08.c` | `func_02228f08` |
| `0229074.c` | `02229074.c` | `func_02229074` |
| `022934a.c` | `0222934a.c` | `func_0222934a` |
| `02293b8.c` | `022293b8.c` | `func_022293b8` |
| `0229488.c` | `02229488.c` | `func_02229488` |
| `0229640.c` | `02229640.c` | `func_02229640` |
| `022988e.c` | `0222988e.c` | `func_0222988e` |
| `022992c.c` | `0222992c.c` | `func_0222992c` |
| `0229af4.c` | `02229af4.c` | `func_02229af4` |
| `0229bb0.c` | `02229bb0.c` | `func_02229bb0` |
| `0229bce.c` | `02229bce.c` | `func_02229bce` |
| `022a120.c` | `0222a120.c` | `func_0222a120` |
| `022ab18.c` | `0222ab18.c` | `func_0222ab18` |
| `022ae40.c` | `0222ae40.c` | `func_0222ae40` |
| `022afba.c` | `0222afba.c` | `func_0222afba` |
| `022ece4.c` | `0222ece4.c` | `func_0222ece4` |
| `0239fc6.c` | `02239fc6.c` | `func_02239fc6` |
| `023c95c.c` | `0223c95c.c` | `func_0223c95c` |
| `023df30.c` | `0223df30.c` | `func_0223df30` |
| `02402c0.c` | `022402c0.c` | `func_022402c0` |
| `0242272.c` | `02242272.c` | `func_02242272` |
| `02425d0.c` | `022425d0.c` | `func_022425d0` |
| `0243066.c` | `02243066.c` | `func_02243066` |
| `02473ee.c` | `022473ee.c` | `func_022473ee` |
| `0247508.c` | `02247508.c` | `func_02247508` |
| `02475a8.c` | `022475a8.c` | `func_022475a8` |
| `0247924.c` | `02247924.c` | `func_02247924` |
| `2203de8.c` | `02203de8.c` | `func_02203de8` |
| `2203e0a.c` | `02203e0a.c` | `func_02203e0a` |
| `2203e5e.c` | `02203e5e.c` | `func_02203e5e` |
| `2203ec2.c` | `02203ec2.c` | `func_02203ec2` |
| `2205812.c` | `02205812.c` | `func_02205812` |
| `220581a.c` | `0220581a.c` | `func_0220581a` |
| `220686a.c` | `0220686a.c` | `func_0220686a` |
| `22072ce.c` | `022072ce.c` | `func_022072ce` |
| `2207342.c` | `02207342.c` | `func_02207342` |
| `220903e.c` | `0220903e.c` | `func_0220903e` |
| `2209100.c` | `02209100.c` | `func_02209100` |
| `220a962.c` | `0220a962.c` | `func_0220a962` |
| `220a992.c` | `0220a992.c` | `func_0220a992` |
| `220cab0.c` | `0220cab0.c` | `func_0220cab0` |
| `220d188.c` | `0220d188.c` | `func_0220d188` |
| `220d204.c` | `0220d204.c` | `func_0220d204` |
| `220d98e.c` | `0220d98e.c` | `func_0220d98e` |
| `220e000.c` | `0220e000.c` | `func_0220e000` |
| `220e0aa.c` | `0220e0aa.c` | `func_0220e0aa` |
| `220f0ee.c` | `0220f0ee.c` | `func_0220f0ee` |
| `220f15e.c` | `0220f15e.c` | `func_0220f15e` |
| `220f19e.c` | `0220f19e.c` | `func_0220f19e` |
| `220f202.c` | `0220f202.c` | `func_0220f202` |
| `220f26a.c` | `0220f26a.c` | `func_0220f26a` |
| `220fb58.c` | `0220fb58.c` | `func_0220fb58` |
| `220fc16.c` | `0220fc16.c` | `func_0220fc16` |
| `220fc3c.c` | `0220fc3c.c` | `func_0220fc3c` |
| `220fc82.c` | `0220fc82.c` | `func_0220fc82` |
| `220fcc4.c` | `0220fcc4.c` | `func_0220fcc4` |
| `220fd52.c` | `0220fd52.c` | `func_0220fd52` |
| `220fd90.c` | `0220fd90.c` | `func_0220fd90` |
| `220fdc8.c` | `0220fdc8.c` | `func_0220fdc8` |
| `220fe48.c` | `0220fe48.c` | `func_0220fe48` |
| `220fe98.c` | `0220fe98.c` | `func_0220fe98` |
| `220fece.c` | `0220fece.c` | `func_0220fece` |
| `220feec.c` | `0220feec.c` | `func_0220feec` |
| `220ff0a.c` | `0220ff0a.c` | `func_0220ff0a` |
| `220ff28.c` | `0220ff28.c` | `func_0220ff28` |
| `220ff54.c` | `0220ff54.c` | `func_0220ff54` |
| `220ff72.c` | `0220ff72.c` | `func_0220ff72` |
| `220ff90.c` | `0220ff90.c` | `func_0220ff90` |
| `220ffae.c` | `0220ffae.c` | `func_0220ffae` |
| `22107dc.c` | `022107dc.c` | `func_022107dc` |
| `2213190.c` | `02213190.c` | `func_02213190` |
| `22131a8.c` | `022131a8.c` | `func_022131a8` |
| `2213234.c` | `02213234.c` | `func_02213234` |
| `2213248.c` | `02213248.c` | `func_02213248` |
| `221326c.c` | `0221326c.c` | `func_0221326c` |
| `221327c.c` | `0221327c.c` | `func_0221327c` |
| `2213290.c` | `02213290.c` | `func_02213290` |
| `22132ac.c` | `022132ac.c` | `func_022132ac` |
| `22132d4.c` | `022132d4.c` | `func_022132d4` |
| `221330c.c` | `0221330c.c` | `func_0221330c` |
| `2213354.c` | `02213354.c` | `func_02213354` |
| `2213390.c` | `02213390.c` | `func_02213390` |
| `2213404.c` | `02213404.c` | `func_02213404` |
| `2213434.c` | `02213434.c` | `func_02213434` |
| `2213474.c` | `02213474.c` | `func_02213474` |
| `22134d8.c` | `022134d8.c` | `func_022134d8` |
| `2213694.c` | `02213694.c` | `func_02213694` |
| `2213d18.c` | `02213d18.c` | `func_02213d18` |
| `2213dd0.c` | `02213dd0.c` | `func_02213dd0` |
| `2213dec.c` | `02213dec.c` | `func_02213dec` |
| `2213e00.c` | `02213e00.c` | `func_02213e00` |
| `2213f90.c` | `02213f90.c` | `func_02213f90` |
| `2213fb4.c` | `02213fb4.c` | `func_02213fb4` |
| `2215630.c` | `02215630.c` | `func_02215630` |
| `221566e.c` | `0221566e.c` | `func_0221566e` |
| `22157ba.c` | `022157ba.c` | `func_022157ba` |
| `22158dc.c` | `022158dc.c` | `func_022158dc` |
| `2215f24.c` | `02215f24.c` | `func_02215f24` |
| `2215f6c.c` | `02215f6c.c` | `func_02215f6c` |
| `22162fc.c` | `022162fc.c` | `func_022162fc` |
| `2216310.c` | `02216310.c` | `func_02216310` |
| `22164ba.c` | `022164ba.c` | `func_022164ba` |
| `22170b4.c` | `022170b4.c` | `func_022170b4` |
| `2219984.c` | `02219984.c` | `func_02219984` |
| `2219c22.c` | `02219c22.c` | `func_02219c22` |
| `232610.c` | `02232610.c` | `func_02232610` |
| `233250.c` | `02233250.c` | `func_02233250` |
| `2463c8.c` | `022463c8.c` | `func_022463c8` |

> Note: many well-formed files intentionally use **semantic** entry names (e.g. `02205c96.c` defines `scene0_attract_select`). That is acceptable *as long as every caller uses the same spelling*; the real problem is callers still using the `func_<addr>` form - see (c).

## (c) Caller / callee name mismatches (address-form calls that will not link)

### (c.1) Call site uses `func_<addr>`, but the entry is defined under a SEMANTIC name
Fix: change the call site(s) and extern to the semantic name (preferred), or give the definition a `func_<8hex>` alias. One row per unresolved symbol.

| called symbol | resolves to addr | actual definition | file | call sites |
|---|---|---|---|---|
| `func_0021080a` | `0x0221080a` | `mtx_push_rotate_apply` | `0221080a.c` | 022119a4 |
| `func_020fece` | `0x0220fece` | `f_220fece` | `220fece.c` | 0223d558 |
| `func_020feec` | `0x0220feec` | `f_220feec` | `220feec.c` | 0223a3a2 |
| `func_020ff0a` | `0x0220ff0a` | `f_220ff0a` | `220ff0a.c` | 0223bf86, 0223d3d0 |
| `func_020ff28` | `0x0220ff28` | `f_220ff28` | `220ff28.c` | 0223bf86, 0223fec4 |
| `func_020ff54` | `0x0220ff54` | `f_220ff54` | `220ff54.c` | 0223d9dc |
| `func_02107dc` | `0x022107dc` | `store_angle_between` | `22107dc.c` | 0222ef38, 0222f1d0 |
| `func_0213190` | `0x02213190` | `sin_lookup` | `2213190.c` | 020d58c, 02239194, 0223b5d8, 0223bcd4, 0223bf86 |
| `func_02131a8` | `0x022131a8` | `cos_lookup` | `22131a8.c` | 02239194, 0223bf86 |
| `func_021326c` | `0x0221326c` | `copy_longs` | `221326c.c` | 02237be4, 02237ea6 |
| `func_02149aa` | `0x022149aa` | `mtx_load_abc` | `022149aa.c` | 02239194, 0223b28a, 0223b5d8, 0223b9b8, 0223bcd4, 0223bf86, 0223d3d0, 0223d558, 0223d9dc |
| `func_02149e4` | `0x022149e4` | `mtx_rotate_zyx` | `022149e4.c` | 02239194 |
| `func_0214a06` | `0x02214a06` | `mtx_rotate_zyx_at24` | `02214a06.c` | 0223b28a, 0223b5d8, 0223b9b8, 0223bcd4, 0223bf86, 0223d3d0, 0223d558, 0223d9dc |
| `func_0214a40` | `0x02214a40` | `mtx_apply_vec_at8` | `02214a40.c` | 0223b28a, 0223b5d8, 0223b9b8, 0223bcd4, 0223bf86, 0223d3d0, 0223d558, 0223d9dc |
| `func_0214a50` | `0x02214a50` | `mtx_apply_vec_at0` | `02214a50.c` | 02239194 |
| `func_0214a7a` | `0x02214a7a` | `mtx_build_fwd_inv` | `02214a7a.c` | 0220b3cc, 0220b544, 0220b6b2, 0220b8d4, 0220ba48, 0220be2c, 0220bfa4 |
| `func_0215f24` | `0x02215f24` | `sound_queue_reset` | `2215f24.c` | 022058a8 |
| `func_0215f6c` | `0x02215f6c` | `sound_queue_push` | `2215f6c.c` | 022203ce, 0222fa20, 0222fc02, 0223bf86, 0223d9dc |
| `func_0220376c` | `0x0220376c` | `hud_status_service_2p` | `0220376c.c` | 0220340c |
| `func_02206e2e` | `0x02206e2e` | `scene81_stage_setup` | `02206e2e.c` | 02233250 |
| `func_022072ce` | `0x022072ce` | `f_22072ce` | `22072ce.c` | 02207224 |
| `func_02207360` | `0x02207360` | `scene_bg_layer_build` | `0207360.c` | 02233250 |
| `func_02207bcc` | `0x02207bcc` | `screen_fx_builder_task` | `02207bcc.c` | 0221bf72, 0221c690 |
| `func_022080ac` | `0x022080ac` | `line_scroll_fx_sm` | `02080ac.c` | 02208030 |
| `func_02208a52` | `0x02208a52` | `disp_shutter_sm` | `0208a52.c` | 022089d0 |
| `func_02209100` | `0x02209100` | `f_2209100` | `2209100.c` | 02205c96 |
| `func_0220a962` | `0x0220a962` | `f_220a962` | `220a962.c` | 0201c9c, 022016ac |
| `func_0220a992` | `0x0220a992` | `f_220a992` | `220a992.c` | 022016ac |
| `func_0220d9ec` | `0x0220d9ec` | `view_bounds_from_actors` | `0220d9ec.c` | 0220b3cc, 0220b544, 0220b8d4, 0220ba48, 0220be2c, 0220bfa4 |
| `func_0221080a` | `0x0221080a` | `mtx_push_rotate_apply` | `0221080a.c` | 02202ba8, 0220ebde, 02211b04, 022203ce, 02231a80, 02231b14, 02232178, 02232294, 02236246 |
| `func_022130b4` | `0x022130b4` | `atan2_fixed` | `022130b4.c` | 0221055c, 022105ca, 02210622, 02210690, 0222e3b0, 022399bc, 02243fe0, 022444b6, 02244df2, 22107dc |
| `func_02213190` | `0x02213190` | `sin_lookup` | `2213190.c` | 0207542, 02079d0, 0220cfac, 0220d262, 0220d70c, 02211108, 02212b20, 02212e00, 022225f6, 0222acf8, 0222f4e8, 0222f6a8, 02234984, 02239082, 0223938c, 0223967a, 0223a888, 02241a10 |
| `func_022131a8` | `0x022131a8` | `cos_lookup` | `22131a8.c` | 02079d0, 0220cfac, 0220d262, 02211108, 022225f6, 0222acf8, 0222f4e8, 0222f6a8, 02239082, 0223a888, 02241a10 |
| `func_02213234` | `0x02213234` | `sh2_hw_divide` | `2213234.c` | 0220a390, 0221039a, 02221460, 022214d2, 0222ef38, 0222f1d0 |
| `func_0221327c` | `0x0221327c` | `zero_longs` | `221327c.c` | 02214bb8 |
| `func_022132ac` | `0x022132ac` | `fill_vec4_array` | `22132ac.c` | 0207360 |
| `func_022132d4` | `0x022132d4` | `vec3_minmax` | `22132d4.c` | 022ece4 |
| `func_0221330c` | `0x0221330c` | `bounds_overlap_margin` | `221330c.c` | 0222ef38 |
| `func_02213390` | `0x02213390` | `interp_shorts_48` | `2213390.c` | 0222dc2c |
| `func_02213404` | `0x02213404` | `clear_record_heads` | `2213404.c` | 02214dae |
| `func_02213434` | `0x02213434` | `matrix_copy_rowswap` | `2213434.c` | 022154fc |
| `func_02213474` | `0x02213474` | `matrix_copy_reverse` | `2213474.c` | 02215964, 02215a2c |
| `func_022134d8` | `0x022134d8` | `matrix_copy_reverse_to` | `22134d8.c` | 02215964, 02215a2c |
| `func_02213ba0` | `0x02213ba0` | `mtx_rotate_x` | `02213ba0.c` | 022399bc |
| `func_02213bf4` | `0x02213bf4` | `mtx_rotate_y` | `02213bf4.c` | 02210690, 0222e3b0, 022399bc |
| `func_02213c4c` | `0x02213c4c` | `mtx_rotate_z` | `02213c4c.c` | 02210690, 0222e3b0, 0222fc02, 022399bc |
| `func_02213fb4` | `0x02213fb4` | `load_matrix_default` | `2213fb4.c` | 02210690, 02227704, 0222dc2c, 0222e3b0, 022399bc |
| `func_022149aa` | `0x022149aa` | `mtx_load_abc` | `022149aa.c` | 02211108, 022114b0, 0221185c, 022119a4, 02211b04, 0221214a, 02212350, 02212c80, 02212e00, 02219916, 0221e8c8, 02227906, 0222d0e8, 0222fa20, 0222fc02, 02231b14, 02232526, 022363e4, 02239082, 022399bc, 0223c6bc, 0223d1b6, 0223d2a0, 022428bc, 022433e8, 02246254 |
| `func_022149e4` | `0x022149e4` | `mtx_rotate_zyx` | `022149e4.c` | 02210690, 02212c80, 02212e00, 02219916, 0221e8c8, 02227704, 0222d0e8, 0222e3b0, 02232526, 02239082, 022399bc, 022428bc, 022433e8, 02246254 |
| `func_02214a06` | `0x02214a06` | `mtx_rotate_zyx_at24` | `02214a06.c` | 02211108, 022114b0, 0221185c, 022119a4, 02211b04, 0221214a, 02212350, 0222d0e8, 0222dc2c, 022363e4, 022399bc, 0223d1b6, 0223d2a0 |
| `func_02214a2e` | `0x02214a2e` | `mtx_stack_push_dup` | `02214a2e.c` | 0222d0e8, 022363e4, 0223967a |
| `func_02214a40` | `0x02214a40` | `mtx_apply_vec_at8` | `02214a40.c` | 02211108, 022114b0, 0221185c, 022119a4, 02211b04, 0221214a, 02212350, 0222d0e8, 022363e4, 0223d1b6, 0223d2a0 |
| `func_02214a50` | `0x02214a50` | `mtx_apply_vec_at0` | `02214a50.c` | 02212c80, 02212e00, 02219916, 0221e8c8, 02227906, 0222d0e8, 0222fa20, 0222fc02, 02231b14, 02232526, 02239082, 022399bc, 022428bc, 022433e8, 02246254 |
| `func_02214a7a` | `0x02214a7a` | `mtx_build_fwd_inv` | `02214a7a.c` | 02206c88, 0221fed4 |
| `func_02215630` | `0x02215630` | `clip_test` | `2215630.c` | 022154fc |
| `func_0221566e` | `0x0221566e` | `emit_sprite_cmd` | `221566e.c` | 02234aae |
| `func_02215f24` | `0x02215f24` | `sound_queue_reset` | `2215f24.c` | 0201b92 |
| `func_02215f6c` | `0x02215f6c` | `sound_queue_push` | `2215f6c.c` | 0201c9c, 0219c44, 0219f26, 02202138, 022027e4, 022114b0, 02211b04, 0221214a, 02212350, 02212c80, 02212e00, 02215da8, 02215f08, 02215fac, 02216018, 0221603e, 02216066, 022160a0, 02216110, 02218038, 0221a1a0, 0221a452, 0221bf72, 0221ce6e, 0221cee8, 0221d6b6, 0221e256, 0221eae2, 0222aadc, 0222b0fe, 0222b1e2, 0222b2a8, 0222dc2c, 0222ef38, 022301c4, 02230434, 022304b4, 0223101c, 0223144a, 0223152c, 02231b14, 02231c30, 02231db8, 02231fd4, 02232178, 02232610, 02232bc4, 0223f054, 02240054, 02240622, 022408a0, 02240d1c, 02241822, 02241868, 022424c4, 022425d0, 02243066, 02248bb8, 022492f8, 02425d0, 0243066, 232610 |
| `func_022162fc` | `0x022162fc` | `reset_displist_state` | `22162fc.c` | 02216154 |
| `func_02216310` | `0x02216310` | `clear_displist_slots` | `2216310.c` | 02216154 |
| `func_022164ba` | `0x022164ba` | `flush_dirty_dma_records` | `22164ba.c` | 02216530 |
| `func_022170b4` | `0x022170b4` | `release_slot_entry` | `22170b4.c` | 02211108, 02211b04, 0221214a, 02212350, 02229b60, 02229df4, 02249cda, 02249da8, 0229bce |
| `func_02217838` | `0x02217838` | `dispatch_cmd` | `02217838.c` | 02243fe0, 022444b6, 022452be |
| `func_02219984` | `0x02219984` | `anim_counter_step` | `2219984.c` | 0221e256 |
| `func_0222d0e8` | `0x0222d0e8` | `draw_actor_hier_model` | `0222d0e8.c` | 02212b20, 0222a9fe |
| `func_0222dc2c` | `0x0222dc2c` | `actor_anim_frame_update` | `0222dc2c.c` | 02212b20, 02228508, 0222881a, 022288e8, 022289b8, 02228a0e, 02228b68, 02228cd6, 02228d44, 02228d9c, 02228e0c, 02228e60, 02228fe2, 02229644, 02229688, 02229714, 02229758, 0222979c, 022297d8, 02229816, 02229854, 02229b60, 02229df4, 0222a300, 0222a418, 0222a970, 0222a9fe, 0222bdf2, 0223346c, 02241eb0, 02242272, 02243066, 02247a8c, 02248104, 02248436, 0224852c, 022485b8, 0224886e, 02248bb8, 02248f88, 022492f8, 02249898, 02249a04, 02249bba, 02249da8, 02249f52, 0224a000, 0228168, 02286cc, 0228738, 0228768, 0228bb6, 0228c70, 0228ca2, 0228f08, 0229074, 02293b8, 0229488, 022988e, 0229bb0 |
| `func_022415ee` | `0x022415ee` | `actor_motion_init` | `022415ee.c` | 02241324, 022413e6 |
| `func_022418a0` | `0x022418a0` | `actor_deactivate_if_active` | `022418a0.c` | 0223ffcc, 022401d6 |
| `func_02241da8` | `0x02241da8` | `actor_init_random` | `02241da8.c` | 02241eb0 |
| `func_02241dde` | `0x02241dde` | `actor_init_rng_dir` | `02241dde.c` | 02241eb0 |
| `func_02244988` | `0x02244988` | `actor_init_from_cfg` | `02244988.c` | 02244670, 022454e4, 02247a8c, 0224806a, 022482dc, 022485b8, 02248bb8, 022490a0 |
| `func_022449b2` | `0x022449b2` | `actor_script_step` | `022449b2.c` | 02247a8c, 02248104, 022492f8, 02249da8, 0224a000 |
| `func_02244aa8` | `0x02244aa8` | `actor_script_begin` | `02244aa8.c` | 02242272, 02243066, 0242272, 0243066 |
| `func_02246208` | `0x02246208` | `actor_flag_rng_1in4` | `02246208.c` | 0224a000 |
| `func_0224806a` | `0x0224806a` | `f_24806a` | `0224806a.c` | 022458ec |
| `func_022489d0` | `0x022489d0` | `f_2489d0` | `022489d0.c` | 0223e5e0, 0223fc06, 02248104, 02248436, 0224852c, 02249898, 02249a04, 02249f52, 0224a000 |
| `func_0224910e` | `0x0224910e` | `f_24910e` | `0224910e.c` | 0224852c, 022490e4 |
| `func_02249240` | `0x02249240` | `f_249240` | `02249240.c` | 02248bb8, 022492f8 |
| `func_022499a6` | `0x022499a6` | `f_2499a6` | `022499a6.c` | 02247a8c, 02248104, 02248436, 022485b8, 0224886e, 022492f8 |
| `func_02249cda` | `0x02249cda` | `f_249cda` | `02249cda.c` | 02248bb8, 022492f8 |
| `func_0224a06c` | `0x0224a06c` | `sdiv32` | `0224a06c.c` | 02236246 |
| `func_0224a120` | `0x0224a120` | `udiv32` | `0224a120.c` | 02223862 |
| `func_0224a1c8` | `0x0224a1c8` | `umod32` | `0224a1c8.c` | 02223862 |
| `func_0227602` | `0x02227602` | `actor_combo_advance` | `0227602.c` | 0226eb8 |
| `func_0228168` | `0x02228168` | `player_join_spawn` | `0228168.c` | 022045f8, 02204afa |
| `func_022ab18` | `0x0222ab18` | `actor_apply_damage` | `022ab18.c` | 0223f054, 0224043c, 02240492, 022404ce, 02240588, 022407b0, 022408a0, 022409a0, 02240a4a, 02240a7a, 02240ae0, 02240b2e, 02240b5a, 02240b8a, 02240bba, 02240c3e, 02240c6a, 02240c9a, 02240cca, 02240d1c, 02240e54, 02240efe, 02240fd8, 02241034, 0224110e, 02241164, 02242226 |
| `func_023c95c` | `0x0223c95c` | `hud_round_data_init` | `023c95c.c` | 02202138 |
| `func_0244e2e` | `0x02244e2e` | `actor_state_dispatch` | `02244e2e.c` | 0223fcb6 |
| `func_0246208` | `0x02246208` | `actor_flag_rng_1in4` | `02246208.c` | 0223e5e0, 0223fc06, 02249f52 |
| `func_024840a` | `0x0224840a` | `actor_set_state_7` | `0224840a.c` | 0223fcb6, 02475a8, 0247924 |
| `func_02489d0` | `0x022489d0` | `f_2489d0` | `022489d0.c` | 02247a8c |
| `func_0249240` | `0x02249240` | `f_249240` | `02249240.c` | 02475a8 |
| `func_0249836` | `0x02249836` | `f_249836` | `02249836.c` | 0223fcb6 |
| `func_0249a82` | `0x02249a82` | `f_249a82` | `02249a82.c` | 0223e5e0, 0223fc06, 02249f52 |
| `func_203de8` | `0x02203de8` | `f_2203de8` | `2203de8.c` | 0220376c |
| `func_203e0a` | `0x02203e0a` | `f_2203e0a` | `2203e0a.c` | 0220376c |
| `func_203e5e` | `0x02203e5e` | `f_2203e5e` | `2203e5e.c` | 0220376c |
| `func_215f6c` | `0x02215f6c` | `sound_queue_push` | `2215f6c.c` | 022413e6, 02242a10, 02402c0 |
| `func_22378c` | `0x0222378c` | `fmt_uint_base2` | `0222378c.c` | 02223142, 02223460 |
| `func_223968` | `0x02223968` | `fmt_uint_base16` | `02223968.c` | 02223142, 02223460 |
| `func_22ab18` | `0x0222ab18` | `actor_apply_damage` | `022ab18.c` | 02402c0 |
| `func_22dc2c` | `0x0222dc2c` | `actor_anim_frame_update` | `0222dc2c.c` | 02246ab8, 02246d0c, 02247106 |
| `func_242f60` | `0x02242f60` | `mode_resolve_0x242f60` | `02242f60.c` | 02242cb8 |
| `func_244988` | `0x02244988` | `actor_init_from_cfg` | `02244988.c` | 02247106 |
| `func_2449b2` | `0x022449b2` | `actor_script_step` | `022449b2.c` | 02247106 |
| `func_244e2e` | `0x02244e2e` | `actor_state_dispatch` | `02244e2e.c` | 02246ab8, 02246dbc, 02246f32 |
| `func_246208` | `0x02246208` | `actor_flag_rng_1in4` | `02246208.c` | 02246ab8, 02246d0c |
| `func_24840a` | `0x0224840a` | `actor_set_state_7` | `0224840a.c` | 02246ab8, 02246cac, 02246dbc, 02246f32 |
| `func_2489d0` | `0x022489d0` | `f_2489d0` | `022489d0.c` | 02246ab8, 02246d0c, 02247106 |
| `func_2490a0` | `0x022490a0` | `f_2490a0` | `022490a0.c` | 022485b8, 0224886e, 02248f88 |
| `func_249240` | `0x02249240` | `f_249240` | `02249240.c` | 0224886e, 02248f88 |
| `func_249836` | `0x02249836` | `f_249836` | `02249836.c` | 02246dbc |
| `func_249a82` | `0x02249a82` | `f_249a82` | `02249a82.c` | 02246ab8, 02246d0c, 0224a000 |
| `func_249b80` | `0x02249b80` | `f_249b80` | `02249b80.c` | 02249bba |
| `func_249cda` | `0x02249cda` | `f_249cda` | `02249cda.c` | 02248f88 |

### (c.2) Call site uses a short/zero-pad `func_`/`f_` spelling of the same address
Fix: normalise both call sites and the definition to `func_<8hexSH2>`.

| called symbol | resolves to addr | actual definition | file | call sites |
|---|---|---|---|---|
| `func_00210858` | `0x02210858` | `func_02210858` | `02210858.c` | 022119a4 |
| `func_00228106` | `0x02228106` | `func_02228106` | `02228106.c` | 022119a4 |
| `func_0024a420` | `0x0224a420` | `func_0224a420` | `0224a420.c` | 02210876, 022109d4 |
| `func_0207224` | `0x02207224` | `func_02207224` | `02207224.c` | 02202138 |
| `func_0207772` | `0x02207772` | `func_02207772` | `02207772.c` | 02202138 |
| `func_0210020` | `0x02210020` | `func_02210020` | `02210020.c` | 020d58c, 0222fe14, 0223b28a, 0223b5d8, 0224910e, 022499a6 |
| `func_021055c` | `0x0221055c` | `func_0221055c` | `0221055c.c` | 0222ef38, 0222f1d0, 0223d3d0 |
| `func_02131c8` | `0x022131c8` | `func_022131c8` | `022131c8.c` | 0223bf86 |
| `func_0213ce4` | `0x02213ce4` | `func_02213ce4` | `02213ce4.c` | 02239194 |
| `func_0214d26` | `0x02214d26` | `func_02214d26` | `02214d26.c` | 02202138 |
| `func_02150d4` | `0x022150d4` | `func_022150d4` | `022150d4.c` | 02202138 |
| `func_02153f6` | `0x022153f6` | `func_022153f6` | `022153f6.c` | 02239462, 0223d558 |
| `func_02154fc` | `0x022154fc` | `func_022154fc` | `022154fc.c` | 0221b0a2, 02231b14 |
| `func_0215a2c` | `0x02215a2c` | `func_02215a2c` | `02215a2c.c` | 02239194, 0223b28a, 0223b5d8, 0223b9b8, 0223bcd4, 0223bf86, 0223d3d0, 0223d558, 0223d9dc |
| `func_0215e20` | `0x02215e20` | `func_02215e20` | `02215e20.c` | 02202138 |
| `func_0215fac` | `0x02215fac` | `func_02215fac` | `02215fac.c` | 022027e4, 02218038 |
| `func_0216066` | `0x02216066` | `func_02216066` | `02216066.c` | 02205232 |
| `func_021675e` | `0x0221675e` | `func_0221675e` | `0221675e.c` | 02207772, 02208030, 0220825c, 022089d0 |
| `func_0216858` | `0x02216858` | `func_02216858` | `02216858.c` | 02238364 |
| `func_0217ae4` | `0x02217ae4` | `func_02217ae4` | `02217ae4.c` | 0222fb90 |
| `func_0217d22` | `0x02217d22` | `func_02217d22` | `02217d22.c` | 22170b4 |
| `func_021ad58` | `0x0221ad58` | `func_0221ad58` | `0221ad58.c` | 02202138 |
| `func_0220d8d0` | `0x0220d8d0` | `func_020d8d0` | `020d8d0.c` | 0220b354, 0220b85c, 0220bdb4 |
| `func_02213a8` | `0x022213a8` | `func_022213a8` | `022213a8.c` | 02207884 |
| `func_022156c` | `0x0222156c` | `func_0222156c` | `0222156c.c` | 02207772, 02207b3e, 02208030, 0220825c, 022089d0, 02208b98 |
| `func_02215f08` | `0x02215f08` | `func_0215f08` | `02215f08.c` | 02223cf0 |
| `func_02216c38` | `0x02216c38` | `func_0216c38` | `0216c38.c` | 0201f20, 02205c96, 02206876, 02232610, 02232bc4, 0223358c, 232610 |
| `func_02219c44` | `0x02219c44` | `func_0219c44` | `0219c44.c` | 0221c690 |
| `func_0221bd4` | `0x02221bd4` | `func_02221bd4` | `02221bd4.c` | 02207224 |
| `func_0221f68` | `0x02221f68` | `func_02221f68` | `02221f68.c` | 02207224 |
| `func_0222e44` | `0x02222e44` | `func_02222e44` | `02222e44.c` | 02202138 |
| `func_02260c8` | `0x022260c8` | `func_022260c8` | `022260c8.c` | 02239fc6 |
| `func_0227a84` | `0x02227a84` | `func_02227a84` | `02227a84.c` | 02202138 |
| `func_0227c22` | `0x02227c22` | `func_02227c22` | `02227c22.c` | 0222a418 |
| `func_0227c5c` | `0x02227c5c` | `func_02227c5c` | `02227c5c.c` | 02229df4, 0222a418 |
| `func_0228508` | `0x02228508` | `func_02228508` | `02228508.c` | 0222a418 |
| `func_022863e` | `0x0222863e` | `func_0222863e` | `0222863e.c` | 02229df4 |
| `func_0229c00` | `0x02229c00` | `func_02229c00` | `02229c00.c` | 0229bb0 |
| `func_0229df4` | `0x02229df4` | `func_02229df4` | `02229df4.c` | 02229c00 |
| `func_022eea2` | `0x0222eea2` | `func_0222eea2` | `0222eea2.c` | 0223d3d0, 0223d558, 0223d9dc |
| `func_022ef38` | `0x0222ef38` | `func_0222ef38` | `0222ef38.c` | 022ece4 |
| `func_022f1d0` | `0x0222f1d0` | `func_0222f1d0` | `0222f1d0.c` | 022ece4 |
| `func_022f3c0` | `0x0222f3c0` | `func_0222f3c0` | `0222f3c0.c` | 0222f4e8, 0222f6a8 |
| `func_02380fa` | `0x022380fa` | `func_022380fa` | `022380fa.c` | 023df30 |
| `func_0238364` | `0x02238364` | `func_02238364` | `02238364.c` | 02202138 |
| `func_023c6bc` | `0x0223c6bc` | `func_0223c6bc` | `0223c6bc.c` | 0223bf86 |
| `func_023cd0c` | `0x0223cd0c` | `func_0223cd0c` | `0223cd0c.c` | 0223bf86 |
| `func_0242f9c` | `0x02242f9c` | `func_02242f9c` | `02242f9c.c` | 0243066 |
| `func_0242fb0` | `0x02242fb0` | `func_02242fb0` | `02242fb0.c` | 0243066 |
| `func_0242fde` | `0x02242fde` | `func_02242fde` | `02242fde.c` | 0243066 |
| `func_0243fe0` | `0x02243fe0` | `func_02243fe0` | `02243fe0.c` | 0223fcb6 |
| `func_0245d1c` | `0x02245d1c` | `func_02245d1c` | `02245d1c.c` | 0223fcb6 |
| `func_0245dcc` | `0x02245dcc` | `func_02245dcc` | `02245dcc.c` | 0223e5e0, 0223fc06 |
| `func_0245fc4` | `0x02245fc4` | `func_02245fc4` | `02245fc4.c` | 0223fcb6 |
| `func_02460c8` | `0x022460c8` | `func_022460c8` | `022460c8.c` | 02473ee, 0247924 |
| `func_02469f2` | `0x022469f2` | `func_022469f2` | `022469f2.c` | 0223fcb6, 02475a8 |
| `func_0247036` | `0x02247036` | `func_02247036` | `02247036.c` | 0223fcb6 |
| `func_02470a2` | `0x022470a2` | `func_022470a2` | `022470a2.c` | 0223fcb6 |
| `func_0247d6e` | `0x02247d6e` | `func_02247d6e` | `02247d6e.c` | 0223fcb6 |
| `func_0249fb0` | `0x02249fb0` | `func_02249fb0` | `02249fb0.c` | 0223fcb6 |
| `func_024a360` | `0x0224a360` | `func_0224a360` | `0224a360.c` | 0224910e |
| `func_210020` | `0x02210020` | `func_02210020` | `02210020.c` | 022413e6, 02246ab8, 02247106 |
| `func_22131c8` | `0x022131c8` | `func_022131c8` | `022131c8.c` | 02243628 |
| `func_222f34` | `0x02222f34` | `func_02222f34` | `02222f34.c` | 02223460 |
| `func_222fe0` | `0x02222fe0` | `func_02222fe0` | `02222fe0.c` | 02223142 |
| `func_223080` | `0x02223080` | `func_02223080` | `02223080.c` | 02223460 |
| `func_223862` | `0x02223862` | `func_02223862` | `02223862.c` | 02223142, 02223460 |
| `func_223a52` | `0x02223a52` | `func_02223a52` | `02223a52.c` | 02223142, 02223460 |
| `func_225fac` | `0x02225fac` | `func_02225fac` | `02225fac.c` | 02223ee0 |
| `func_227704` | `0x02227704` | `func_02227704` | `02227704.c` | 02240492, 022407b0, 022409a0, 02240a4a, 02240b5a, 02240c6a, 02240e54, 02240efe, 02240fd8, 02241034, 02241164, 02402c0 |
| `func_228106` | `0x02228106` | `func_02228106` | `02228106.c` | 02242a10 |
| `func_23c7f6` | `0x0223c7f6` | `func_0223c7f6` | `0223c7f6.c` | 02223ee0 |
| `func_23e530` | `0x0223e530` | `func_0223e530` | `0223e530.c` | 02243628 |
| `func_24124e` | `0x0224124e` | `func_0224124e` | `0224124e.c` | 02402c0 |
| `func_241556` | `0x02241556` | `func_02241556` | `02241556.c` | 02402c0 |
| `func_24174e` | `0x0224174e` | `func_0224174e` | `0224174e.c` | 02402c0 |
| `func_243590` | `0x02243590` | `func_02243590` | `02243590.c` | 02242a10 |
| `func_243be0` | `0x02243be0` | `func_02243be0` | `02243be0.c` | 02243628 |
| `func_243d78` | `0x02243d78` | `func_02243d78` | `02243d78.c` | 02243628 |
| `func_243db4` | `0x02243db4` | `func_02243db4` | `02243db4.c` | 02243628 |
| `func_243e22` | `0x02243e22` | `func_02243e22` | `02243e22.c` | 02243628 |
| `func_243fe0` | `0x02243fe0` | `func_02243fe0` | `02243fe0.c` | 02246cac, 02246dbc, 02246f32 |
| `func_24492c` | `0x0224492c` | `func_0224492c` | `0224492c.c` | 02246ab8, 02246dbc |
| `func_245382` | `0x02245382` | `func_02245382` | `02245382.c` | 02247106 |
| `func_2453de` | `0x022453de` | `func_022453de` | `022453de.c` | 02247106 |
| `func_245b48` | `0x02245b48` | `func_02245b48` | `02245b48.c` | 02242a10 |
| `func_245d1c` | `0x02245d1c` | `func_02245d1c` | `02245d1c.c` | 02246dbc |
| `func_246180` | `0x02246180` | `func_02246180` | `02246180.c` | 02246ab8 |
| `func_2469f2` | `0x022469f2` | `func_022469f2` | `022469f2.c` | 02246dbc, 02246f32, 02247106 |
| `func_246cac` | `0x02246cac` | `func_02246cac` | `02246cac.c` | 02246ab8 |
| `func_247064` | `0x02247064` | `func_02247064` | `02247064.c` | 02246dbc, 02246f32 |
| `func_2470a2` | `0x022470a2` | `func_022470a2` | `022470a2.c` | 02246ab8, 02246dbc, 02246f32 |
| `func_247d6e` | `0x02247d6e` | `func_02247d6e` | `02247d6e.c` | 02246ab8, 02246dbc, 02246f32 |
| `func_249f08` | `0x02249f08` | `func_02249f08` | `02249f08.c` | 02246ab8, 02246d0c |
| `func_249fb0` | `0x02249fb0` | `func_02249fb0` | `02249fb0.c` | 02246cac, 02246dbc, 02246f32 |

## (d) Duplicate definitions of the same ROM entry

### (d.1) Hard symbol collisions (same public symbol defined in two files)
These will fail a whole-tree link with a duplicate-symbol error. Keep one TU, delete the other.

| symbol | files |
|---|---|
| `func_02229640` | `02229640.c`, `0229640.c` |
| `func_02233250` | `02233250.c`, `233250.c` |
| `func_022332d4` | `022332d4.c`, `233250.c` |

### (d.2) Same ROM entry address implemented by two files
39 entry addresses are each defined by two TUs - a zero-padded `0xxxxxxx.c` and an unpadded/census sibling. Even where the two use different symbol names (`func_` vs `f_`), both compile the same ROM routine, so one is redundant and must be dropped (keep the canonical 8-hex file). Pairs marked * also collide on symbol name (see d.1).

| ROM entry | file A | file B | defines A / B |
|---|---|---|---|
| `0x02205812` | `02205812.c` | `2205812.c` | `func_02205812` / `f_2205812` |
| `0x0220581a` | `0220581a.c` | `220581a.c` | `func_0220581a` / `f_220581a` |
| `0x0220d98e` | `0220d98e.c` | `220d98e.c` | `func_0220d98e` / `f_220d98e` |
| `0x0220e000` | `0220e000.c` | `220e000.c` | `func_0220e000` / `f_220e000` |
| `0x0220e0aa` | `0220e0aa.c` | `220e0aa.c` | `func_0220e0aa` / `f_220e0aa` |
| `0x0220f15e` | `0220f15e.c` | `220f15e.c` | `func_0220f15e` / `f_220f15e` |
| `0x0220f19e` | `0220f19e.c` | `220f19e.c` | `func_0220f19e` / `f_220f19e` |
| `0x0220f202` | `0220f202.c` | `220f202.c` | `func_0220f202` / `f_220f202` |
| `0x0220f26a` | `0220f26a.c` | `220f26a.c` | `func_0220f26a` / `f_220f26a` |
| `0x0220fb58` | `0220fb58.c` | `220fb58.c` | `func_0220fb58` / `f_220fb58` |
| `0x0220fc3c` | `0220fc3c.c` | `220fc3c.c` | `func_0220fc3c` / `f_220fc3c` |
| `0x0220fc82` | `0220fc82.c` | `220fc82.c` | `func_0220fc82` / `f_220fc82` |
| `0x0220fcc4` | `0220fcc4.c` | `220fcc4.c` | `func_0220fcc4` / `f_220fcc4` |
| `0x0220fd52` | `0220fd52.c` | `220fd52.c` | `func_0220fd52` / `f_220fd52` |
| `0x0220fd90` | `0220fd90.c` | `220fd90.c` | `func_0220fd90` / `f_220fd90` |
| `0x0220fdc8` | `0220fdc8.c` | `220fdc8.c` | `func_0220fdc8` / `f_220fdc8` |
| `0x0220fe48` | `0220fe48.c` | `220fe48.c` | `func_0220fe48` / `f_220fe48` |
| `0x0220fe98` | `0220fe98.c` | `220fe98.c` | `func_0220fe98` / `f_220fe98` |
| `0x0220fece` | `0220fece.c` | `220fece.c` | `func_0220fece` / `f_220fece` |
| `0x0220feec` | `0220feec.c` | `220feec.c` | `func_0220feec` / `f_220feec` |
| `0x0220ff0a` | `0220ff0a.c` | `220ff0a.c` | `func_0220ff0a` / `f_220ff0a` |
| `0x0220ff28` | `0220ff28.c` | `220ff28.c` | `func_0220ff28` / `f_220ff28` |
| `0x0220ff54` | `0220ff54.c` | `220ff54.c` | `func_0220ff54` / `f_220ff54` |
| `0x0220ff72` | `0220ff72.c` | `220ff72.c` | `func_0220ff72` / `f_220ff72` |
| `0x0220ff90` | `0220ff90.c` | `220ff90.c` | `func_0220ff90` / `f_220ff90` |
| `0x0220ffae` | `0220ffae.c` | `220ffae.c` | `func_0220ffae` / `f_220ffae` |
| `0x02213290` | `02213290.c` | `2213290.c` | `func_02213290` / `clear_vec4_array` |
| `0x022275e4` | `022275e4.c` | `02275e4.c` | `func_022275e4` / `actor_combo_reset` |
| `0x02227602` | `02227602.c` | `0227602.c` | `func_02227602` / `actor_combo_advance` |
| `0x02229640`* | `02229640.c` | `0229640.c` | `func_02229640` / `func_02229640` |
| `0x0222ae40` | `0222ae40.c` | `022ae40.c` | `func_0222ae40` / `actor_pose_resolve` |
| `0x02232610` | `02232610.c` | `232610.c` | `func_02232610` / `f_232610` |
| `0x02233250`* | `02233250.c` | `233250.c` | `func_02233250` / `func_022332d4,func_02233250` |
| `0x02239fc6` | `02239fc6.c` | `0239fc6.c` | `func_02239fc6` / `func_0239fc6` |
| `0x0223c95c` | `0223c95c.c` | `023c95c.c` | `func_0223c95c` / `hud_round_data_init` |
| `0x0223df30` | `0223df30.c` | `023df30.c` | `func_0223df30` / `hud_draw_player_status` |
| `0x02242272` | `02242272.c` | `0242272.c` | `func_02242272` / `actor_pose_hold_state` |
| `0x022425d0` | `022425d0.c` | `02425d0.c` | `func_022425d0` / `actor_throw_sequence` |
| `0x02243066` | `02243066.c` | `0243066.c` | `func_02243066` / `actor_special_move_state` |

---

### Root cause
The defects cluster into one root cause: **inconsistent address notation across worker passes** - full 8-hex `0x02xxxxxx`, census low-24-bit (`0x232610`), and missing-leading-zero (`0x2213190`) forms were all used as filenames and symbol names. A single normalisation pass (rename every file to `<8hexSH2>.c`, every entry symbol to `func_<8hexSH2>` or a consistent semantic name, and dedup the (d) collisions) would make the tree whole-tree-linkable. All fixes are mechanical renames - no decompilation logic changes. This audit is report-only; no renames were applied (other agents are actively editing `src_functional/`).
