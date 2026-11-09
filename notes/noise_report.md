# Decomp readability noise report v1.00

Generated: 2026-11-07T04:33:22.298215+00:00

Source: `project/src_functional/*.c`. Parsed **1099/1099 files**, **1,172 function definitions** (including helpers).

Total noise: **29,690 category occurrences**. Zero-noise functions: **55/1,172 (4.69%)**. Lines with noise: **20,021/36,381 (55.03%)**.

## Counting rules and limits

This measures how much source still exposes CPU addresses, object layout, and SH-2 idioms; it is independent of coverage and correctness. Lower is more readable under these rules. A zero score does not prove that a function is well named or understood.

pycparser parses each translation unit with the shared integer typedef vocabulary, retaining macro invocations. Comments, strings, includes, directives, header bodies, prototypes, definition names, and file-scope declarations/initializers do not contribute noise. Function parameter declarations and local initializers do. Named constants, semantic members, and named wrapper macro bodies are clean; replacing an address with a named symbol reduces the metric. Local uintptr_t accessor aliases are recognized. PTR/CALL/va_arg type arguments are masked for parsing with source coordinates preserved. Conditional preprocessing is rejected explicitly.

Categories overlap intentionally: `MEM16(0x25f80000)` has raw-memory and hardware-address noise; `*(u32 *)0x06000000` has raw-memory and pointer-constant noise. Each category/operation/location counts once. Address-named function definitions are excluded; uses as calls or callbacks count. Register variables count once per declaration, including parameters, rather than on every read/write. Gotos and labels count separately; case/default are clean.

The line denominator is nonblank, noncomment physical lines within parsed function definitions (signature through closing brace). The numerator is distinct lines with at least one event, so overlapping categories never multiply line counts. Totals deduplicate physical lines even when two functions share a line. Parse failures are excluded from metrics and listed below; any failure makes the command exit nonzero.

Offset accessors include decimal and zero offsets, computed offsets containing literals, MP and FS/FU/FPTR. Other magic offsets are numeric byte/void-pointer displacements, integer-address displacements explicitly cast to pointers, and unk/field_/pad offset members (hex suffix). Ordinary array indices, typed pointer strides, and general arithmetic constants are excluded. Sign-extension detection is conservative: nested signed 8/16-to-wider casts, equal 16/24-bit left/right shifts (a signedness heuristic), xor/subtract 0x80/0x8000, and sext/sign_extend helpers; plain narrowing casts are excluded. It does not perform dataflow or infer pointer types through arbitrary expressions. Absolute-address detection uses literal-only expressions or literals at least 0x100000 inside address expressions; bare integer address assignments without pointer/access context are outside this metric.

Hardware literals include Saturn physical/P1 peripheral windows 0x05xxxxxx/0x25xxxxxx, SMPC 0x00100000..0x0010007f and its aliases, and SH-2 on-chip 0xfffffe00..0xffffffef. All-ones sentinels are excluded. Function-local named macro implementations are deliberately excluded; audit their definitions separately if measuring hidden implementation complexity.

## Categories

| Category | Occurrences | Share of noise | Rule |
|---|---:|---:|---|
| `raw_memory` | 2,561 | 8.63% | Numeric absolute memory accesses (MEM*, dereferences, local accessor aliases) |
| `offset_access` | 17,657 | 59.47% | Numeric offset accessor calls (M*, MP, FS*/FU*, FPTR) |
| `pointer_constant` | 1,068 | 3.60% | Nonzero numeric integer-to-pointer casts / PTR* constructors |
| `hardware_address` | 264 | 0.89% | Raw Saturn / SH-2 hardware address literals |
| `address_function` | 2,799 | 9.43% | Uses of func_/FUN_ followed by 8 hex digits (not definitions/prototypes) |
| `numeric_call` | 94 | 0.32% | Calls through numeric pointer expressions / numeric callback slots |
| `unnamed_global` | 2,851 | 9.60% | Uses of g_ followed by 8 hex digits (not declarations) |
| `magic_offset` | 2,204 | 7.42% | Numeric byte-pointer arithmetic and offset-named struct members |
| `goto` | 80 | 0.27% | goto statements |
| `label` | 37 | 0.12% | C labels (excluding switch case/default) |
| `register_variable` | 46 | 0.15% | Declarations/parameters named r0..r15, R0..R15, sr/gbr/vbr/pr/mach/macl |
| `sign_extension` | 29 | 0.10% | Signed widening of narrow casts, shift-pair / xor-subtract sign extension, sext helpers |

## Top 50 noisiest functions

Ranked by category occurrences, then source path/name. Counts are unweighted; larger functions naturally rank higher.

| Rank | Function | Source | Noise | Noisy/code lines | Largest categories |
|---:|---|---|---:|---:|---|
| 1 | `func_02223ee0` | [project/src_functional/02223ee0.c:50](../project/src_functional/02223ee0.c#L50) | 897 | 421/866 | offset_access: 618, pointer_constant: 133, raw_memory: 130 |
| 2 | `tieup_task_handler` | [project/src_functional/02226148.c:80](../project/src_functional/02226148.c#L80) | 338 | 299/504 | offset_access: 322, address_function: 7, goto: 6 |
| 3 | `actor_anim_frame_update` | [project/src_functional/0222dc2c.c:57](../project/src_functional/0222dc2c.c#L57) | 245 | 149/226 | offset_access: 182, address_function: 25, pointer_constant: 21 |
| 4 | `func_0223bf86` | [project/src_functional/0223bf86.c:84](../project/src_functional/0223bf86.c#L84) | 228 | 140/220 | offset_access: 183, raw_memory: 27, address_function: 12 |
| 5 | `func_02201020` | [project/src_functional/02201020.c:34](../project/src_functional/02201020.c#L34) | 212 | 88/112 | pointer_constant: 60, offset_access: 57, raw_memory: 55 |
| 6 | `func_02210b58` | [project/src_functional/02210b58.c:24](../project/src_functional/02210b58.c#L24) | 212 | 138/187 | offset_access: 163, address_function: 30, raw_memory: 13 |
| 7 | `func_0222c39c` | [project/src_functional/0222c39c.c:50](../project/src_functional/0222c39c.c#L50) | 187 | 129/225 | offset_access: 168, raw_memory: 11, address_function: 6 |
| 8 | `actor_clash_state` | [project/src_functional/02226eb8.c:84](../project/src_functional/02226eb8.c#L84) | 186 | 145/226 | offset_access: 166, raw_memory: 11, address_function: 7 |
| 9 | `func_02211b04` | [project/src_functional/02211b04.c:82](../project/src_functional/02211b04.c#L82) | 170 | 124/202 | offset_access: 114, unnamed_global: 28, address_function: 25 |
| 10 | `func_02205232` | [project/src_functional/02205232.c:49](../project/src_functional/02205232.c#L49) | 169 | 127/157 | offset_access: 66, raw_memory: 66, address_function: 20 |
| 11 | `func_02232610` | [project/src_functional/02232610.c:81](../project/src_functional/02232610.c#L81) | 169 | 122/155 | offset_access: 74, unnamed_global: 54, address_function: 34 |
| 12 | `func_02232bc4` | [project/src_functional/02232bc4.c:81](../project/src_functional/02232bc4.c#L81) | 166 | 128/171 | offset_access: 75, unnamed_global: 52, address_function: 29 |
| 13 | `func_02202e84` | [project/src_functional/02202e84.c:87](../project/src_functional/02202e84.c#L87) | 165 | 131/167 | offset_access: 74, raw_memory: 54, address_function: 33 |
| 14 | `func_022182c6` | [project/src_functional/022182c6.c:29](../project/src_functional/022182c6.c#L29) | 163 | 101/150 | offset_access: 140, raw_memory: 13, magic_offset: 5 |
| 15 | `func_02202138` | [project/src_functional/02202138.c:65](../project/src_functional/02202138.c#L65) | 162 | 123/176 | offset_access: 90, address_function: 36, raw_memory: 26 |
| 16 | `func_022371c0` | [project/src_functional/022371c0.c:62](../project/src_functional/022371c0.c#L62) | 152 | 89/160 | offset_access: 75, unnamed_global: 66, address_function: 9 |
| 17 | `func_02243fe0` | [project/src_functional/02243fe0.c:50](../project/src_functional/02243fe0.c#L50) | 150 | 79/189 | offset_access: 98, magic_offset: 23, raw_memory: 19 |
| 18 | `func_0221f9c4` | [project/src_functional/0221f9c4.c:17](../project/src_functional/0221f9c4.c#L17) | 144 | 94/146 | offset_access: 91, raw_memory: 24, address_function: 20 |
| 19 | `func_02204afa` | [project/src_functional/02204afa.c:58](../project/src_functional/02204afa.c#L58) | 140 | 112/170 | raw_memory: 71, offset_access: 39, address_function: 15 |
| 20 | `func_0220ebde` | [project/src_functional/0220ebde.c:41](../project/src_functional/0220ebde.c#L41) | 133 | 53/87 | offset_access: 101, unnamed_global: 13, magic_offset: 12 |
| 21 | `func_02209a8c` | [project/src_functional/02209a8c.c:24](../project/src_functional/02209a8c.c#L24) | 130 | 50/87 | unnamed_global: 47, offset_access: 39, raw_memory: 21 |
| 22 | `func_022492f8` | [project/src_functional/022492f8.c:47](../project/src_functional/022492f8.c#L47) | 128 | 115/255 | offset_access: 106, address_function: 17, pointer_constant: 4 |
| 23 | `screen_fx_builder_task` | [project/src_functional/02207bcc.c:43](../project/src_functional/02207bcc.c#L43) | 126 | 71/94 | offset_access: 44, unnamed_global: 26, pointer_constant: 18 |
| 24 | `func_02243628` | [project/src_functional/02243628.c:50](../project/src_functional/02243628.c#L50) | 125 | 90/189 | offset_access: 74, pointer_constant: 25, raw_memory: 12 |
| 25 | `scene81_game_sm` | [project/src_functional/02206876.c:65](../project/src_functional/02206876.c#L65) | 122 | 90/129 | offset_access: 75, raw_memory: 30, address_function: 6 |
| 26 | `func_0221bf72` | [project/src_functional/0221bf72.c:52](../project/src_functional/0221bf72.c#L52) | 114 | 76/135 | magic_offset: 68, raw_memory: 29, address_function: 15 |
| 27 | `func_022363e4` | [project/src_functional/022363e4.c:52](../project/src_functional/022363e4.c#L52) | 114 | 63/84 | offset_access: 80, unnamed_global: 18, address_function: 10 |
| 28 | `func_0223358c` | [project/src_functional/0223358c.c:66](../project/src_functional/0223358c.c#L66) | 113 | 71/105 | offset_access: 61, address_function: 22, pointer_constant: 13 |
| 29 | `func_02211108` | [project/src_functional/02211108.c:55](../project/src_functional/02211108.c#L55) | 110 | 87/135 | offset_access: 88, address_function: 12, unnamed_global: 9 |
| 30 | `func_022114b0` | [project/src_functional/022114b0.c:60](../project/src_functional/022114b0.c#L60) | 110 | 74/108 | offset_access: 78, unnamed_global: 19, address_function: 13 |
| 31 | `func_02239fc6` | [project/src_functional/02239fc6.c:33](../project/src_functional/02239fc6.c#L33) | 109 | 74/105 | offset_access: 79, unnamed_global: 27, address_function: 3 |
| 32 | `func_0220b6b2` | [project/src_functional/0220b6b2.c:32](../project/src_functional/0220b6b2.c#L32) | 105 | 52/60 | offset_access: 75, unnamed_global: 29, address_function: 1 |
| 33 | `func_022475a8` | [project/src_functional/022475a8.c:36](../project/src_functional/022475a8.c#L36) | 103 | 82/144 | offset_access: 88, address_function: 11, unnamed_global: 4 |
| 34 | `credit_render` | [project/src_functional/0220600e.c:118](../project/src_functional/0220600e.c#L118) | 101 | 78/109 | raw_memory: 50, offset_access: 22, pointer_constant: 15 |
| 35 | `func_022095cc` | [project/src_functional/022095cc.c:22](../project/src_functional/022095cc.c#L22) | 101 | 27/35 | unnamed_global: 59, raw_memory: 17, pointer_constant: 16 |
| 36 | `func_02212350` | [project/src_functional/02212350.c:82](../project/src_functional/02212350.c#L82) | 100 | 69/127 | offset_access: 71, address_function: 18, unnamed_global: 6 |
| 37 | `func_0223ac9c` | [project/src_functional/0223ac9c.c:35](../project/src_functional/0223ac9c.c#L35) | 99 | 48/70 | offset_access: 66, pointer_constant: 11, address_function: 10 |
| 38 | `func_0223f95c` | [project/src_functional/0223f95c.c:52](../project/src_functional/0223f95c.c#L52) | 99 | 67/120 | offset_access: 80, unnamed_global: 12, address_function: 7 |
| 39 | `func_02242a10` | [project/src_functional/02242a10.c:39](../project/src_functional/02242a10.c#L39) | 98 | 61/100 | offset_access: 58, pointer_constant: 16, raw_memory: 16 |
| 40 | `func_02243066` | [project/src_functional/02243066.c:54](../project/src_functional/02243066.c#L54) | 97 | 82/136 | offset_access: 73, address_function: 15, unnamed_global: 9 |
| 41 | `func_0220e2d8` | [project/src_functional/0220e2d8.c:47](../project/src_functional/0220e2d8.c#L47) | 96 | 58/80 | offset_access: 70, unnamed_global: 20, address_function: 4 |
| 42 | `scene81_stage_setup` | [project/src_functional/02206e2e.c:49](../project/src_functional/02206e2e.c#L49) | 95 | 65/101 | offset_access: 41, address_function: 23, hardware_address: 15 |
| 43 | `func_0221e256` | [project/src_functional/0221e256.c:43](../project/src_functional/0221e256.c#L43) | 95 | 73/120 | magic_offset: 60, raw_memory: 28, address_function: 7 |
| 44 | `func_022425d0` | [project/src_functional/022425d0.c:41](../project/src_functional/022425d0.c#L41) | 95 | 70/109 | offset_access: 72, unnamed_global: 16, address_function: 5 |
| 45 | `func_0220c3e0` | [project/src_functional/0220c3e0.c:17](../project/src_functional/0220c3e0.c#L17) | 94 | 57/109 | offset_access: 77, unnamed_global: 11, address_function: 4 |
| 46 | `func_02218eac` | [project/src_functional/02218eac.c:43](../project/src_functional/02218eac.c#L43) | 94 | 83/131 | magic_offset: 65, raw_memory: 21, address_function: 5 |
| 47 | `func_0223a3a2` | [project/src_functional/0223a3a2.c:36](../project/src_functional/0223a3a2.c#L36) | 94 | 61/109 | offset_access: 73, unnamed_global: 18, address_function: 3 |
| 48 | `func_0221af30` | [project/src_functional/0221af30.c:20](../project/src_functional/0221af30.c#L20) | 93 | 33/45 | offset_access: 48, unnamed_global: 45 |
| 49 | `func_0221d6b6` | [project/src_functional/0221d6b6.c:33](../project/src_functional/0221d6b6.c#L33) | 93 | 58/90 | magic_offset: 83, raw_memory: 9, address_function: 1 |
| 50 | `camera_pose_init_task` | [project/src_functional/0220b1c0.c:35](../project/src_functional/0220b1c0.c#L35) | 92 | 53/68 | offset_access: 64, raw_memory: 23, pointer_constant: 4 |

## Repeated naming candidates

Occurrences count source sites once per value and kind, not overlapping category events. Addresses include numeric memory/pointer/call sites, hardware literals, and address-named global/function uses. Zero/null pointer casts are clean. Repetition tables show the top 50 of each kind; JSON includes every candidate and example sites. Semantic symbol suggestions come from notes/symbols.txt, never automatic noise exemptions; address-only placeholder names are omitted. Multiple aliases can conflict: confirm them against their evidence. Runtime/cart-offset function aliases use coverage_callgraph.py's canonical mapping for symbol lookup only; address rows remain separate. Offset values are pooled across unrelated object layouts: validate the base type before naming a struct member; counts are opportunities, not guaranteed reductions.

### Raw address candidates

| Value | Occurrences | Functions | Files | Existing symbol suggestions | Example |
|---|---:|---:|---:|---|---|
| `0x06098878` | 169 | 79 | 79 | g_drawPriority, g_matrix_stack_depth, g_mtx_stack_top | [project/src_functional/02210690.c:36](../project/src_functional/02210690.c#L36) |
| `0x060997a8` | 156 | 54 | 48 | g_ui_mode, g_evtWord2, g_diag_line | [project/src_functional/02203500.c:27](../project/src_functional/02203500.c#L27) |
| `0x060997a4` | 154 | 54 | 48 | g_ui_color, g_evtWord1, g_diag_code | [project/src_functional/02203500.c:26](../project/src_functional/02203500.c#L26) |
| `0x06099584` | 133 | 35 | 35 | g_camera_obj_table | [project/src_functional/0220c17c.c:52](../project/src_functional/0220c17c.c#L52) |
| `0x060e0ddc` | 128 | 90 | 89 | g_sound_ready, g_sfxEnableFlag, g_sound_enable, g_combo_enable_flag | [project/src_functional/02201958.c:76](../project/src_functional/02201958.c#L76) |
| `0x02210020` | 116 | 71 | 70 | rng_next | [project/src_functional/02204afa.c:109](../project/src_functional/02204afa.c#L109) |
| `0x0609882c` | 111 | 67 | 67 | g_disp_aux_ptr, g_curActorObj, g_ref_obj_ptr | [project/src_functional/02201f20.c:65](../project/src_functional/02201f20.c#L65) |
| `0x0604ebe4` | 107 | 63 | 63 | g_player_bit_masks | [project/src_functional/0220c17c.c:71](../project/src_functional/0220c17c.c#L71) |
| `0x06098580` | 107 | 54 | 54 | g_scene_flags, g_eventFlags, g_flags580, g_event_flags | [project/src_functional/02201958.c:84](../project/src_functional/02201958.c#L84) |
| `0x060995b0` | 93 | 76 | 76 | — | [project/src_functional/0220cdf6.c:37](../project/src_functional/0220cdf6.c#L37) |
| `0x06098b1c` | 81 | 27 | 27 | g_mtx_vec_out, g_mtx_delta_vec, g_mtx_out_vec | [project/src_functional/02202ba8.c:43](../project/src_functional/02202ba8.c#L43) |
| `0x060987bc` | 79 | 55 | 55 | g_global_phase, g_anim_publish_value | [project/src_functional/02201958.c:39](../project/src_functional/02201958.c#L39) |
| `0x06098f08` | 78 | 48 | 48 | g_game_disp_shadow, g_vdp2Shadow | [project/src_functional/02201020.c:69](../project/src_functional/02201020.c#L69) |
| `0x0609029c` | 76 | 34 | 32 | ui_glyph_thunk | [project/src_functional/02203500.c:16](../project/src_functional/02203500.c#L16) |
| `0x0609866c` | 74 | 40 | 39 | g_scene_desc_ptr, g_sceneStatePtr, g_scene_def_root | [project/src_functional/02201c9c.c:71](../project/src_functional/02201c9c.c#L71) |
| `0x06098830` | 69 | 23 | 22 | — | [project/src_functional/02204afa.c:104](../project/src_functional/02204afa.c#L104) |
| `0x06098670` | 67 | 41 | 41 | g_credit_scene_armed, g_sceneModeFlag, g_player_set_select, g_credit_armed | [project/src_functional/022016ac.c:78](../project/src_functional/022016ac.c#L78) |
| `0x0224a420` | 67 | 22 | 19 | dispatch_table_b, ashr_var_helper, ashr_var | [project/src_functional/0220e5bc.c:35](../project/src_functional/0220e5bc.c#L35) |
| `0x06098674` | 60 | 17 | 17 | g_join_state_p1, g_p1_ready_byte, g_credit_byte_p1 | [project/src_functional/02201958.c:61](../project/src_functional/02201958.c#L61) |
| `0x022131c8` | 58 | 23 | 23 | fixmul_16_16 | [project/src_functional/022083a0.c:65](../project/src_functional/022083a0.c#L65) |
| `0x06099510` | 58 | 9 | 9 | g_cram_cursor, g_disp_cursor | [project/src_functional/022079d0.c:46](../project/src_functional/022079d0.c#L46) |
| `0x0609856c` | 56 | 28 | 28 | g_player_flags | [project/src_functional/02201b92.c:55](../project/src_functional/02201b92.c#L55) |
| `0x060902a0` | 56 | 25 | 24 | ui_printf_thunk, g_eventDispatchSlot, g_diag_logger_fp | [project/src_functional/02204afa.c:55](../project/src_functional/02204afa.c#L55) |
| `0x0220feec` | 55 | 33 | 33 | spawn_obj_a, spawn_obj_variant | [project/src_functional/02202138.c:171](../project/src_functional/02202138.c#L171) |
| `0x060e0e88` | 52 | 14 | 14 | — | [project/src_functional/0223a924.c:13](../project/src_functional/0223a924.c#L13) |
| `0x06098668` | 50 | 20 | 20 | g_scene_index | [project/src_functional/02201958.c:79](../project/src_functional/02201958.c#L79) |
| `0x022153f6` | 48 | 33 | 33 | — | [project/src_functional/02219c44.c:76](../project/src_functional/02219c44.c#L76) |
| `0x060e0f2c` | 48 | 30 | 30 | — | [project/src_functional/02229df4.c:88](../project/src_functional/02229df4.c#L88) |
| `0x06096c74` | 48 | 29 | 29 | g_slot_anim_id | [project/src_functional/02223e70.c:13](../project/src_functional/02223e70.c#L13) |
| `0x022469f2` | 48 | 26 | 26 | — | [project/src_functional/0223e690.c:56](../project/src_functional/0223e690.c#L56) |
| `0x02215a2c` | 47 | 42 | 42 | submit_model_culled | [project/src_functional/02210b58.c:216](../project/src_functional/02210b58.c#L216) |
| `0x02227704` | 44 | 33 | 33 | actor_facing_resolve | [project/src_functional/02228536.c:33](../project/src_functional/02228536.c#L33) |
| `0x06099788` | 44 | 12 | 12 | g_slotCounters | [project/src_functional/02202c5c.c:32](../project/src_functional/02202c5c.c#L32) |
| `0x06099590` | 42 | 37 | 37 | g_slot_flags | [project/src_functional/0220cdf6.c:36](../project/src_functional/0220cdf6.c#L36) |
| `0x0220fece` | 42 | 27 | 27 | spawn_obj_0x6c, spawn_child_task, spawn_child | [project/src_functional/02201c9c.c:73](../project/src_functional/02201c9c.c#L73) |
| `0x06098804` | 40 | 17 | 17 | — | [project/src_functional/02201958.c:65](../project/src_functional/02201958.c#L65) |
| `0x060987ec` | 40 | 14 | 14 | — | [project/src_functional/022016ac.c:69](../project/src_functional/022016ac.c#L69) |
| `0x06099578` | 39 | 24 | 23 | g_edsr_gate | [project/src_functional/02201020.c:91](../project/src_functional/02201020.c#L91) |
| `0x06098734` | 39 | 20 | 20 | g_zone_records, g_ctrl1Word, g_button_records | [project/src_functional/02202d14.c:40](../project/src_functional/02202d14.c#L40) |
| `0x060997ac` | 39 | 20 | 20 | — | [project/src_functional/022093f8.c:29](../project/src_functional/022093f8.c#L29) |
| `0x06098808` | 39 | 14 | 14 | — | [project/src_functional/022016ac.c:68](../project/src_functional/022016ac.c#L68) |
| `0x06096c90` | 38 | 22 | 22 | g_frame_shift | [project/src_functional/02201958.c:49](../project/src_functional/02201958.c#L49) |
| `0x0609979a` | 37 | 24 | 24 | g_mode_flag, g_dispatchIndex | [project/src_functional/02201b92.c:57](../project/src_functional/02201b92.c#L57) |
| `0x0224a360` | 36 | 22 | 22 | dispatch_table_a, ashl_var | [project/src_functional/02210b58.c:32](../project/src_functional/02210b58.c#L32) |
| `0x022145d0` | 36 | 12 | 12 | sh2_dmac_copy, dma_ch0_transfer, dma | [project/src_functional/02207360.c:83](../project/src_functional/02207360.c#L83) |
| `0x0220ff28` | 34 | 28 | 28 | spawn_obj_0x7c, spawn_from_template | [project/src_functional/02211108.c:80](../project/src_functional/02211108.c#L80) |
| `0x06098564` | 33 | 25 | 25 | g_playerReadyFlags | [project/src_functional/02201958.c:78](../project/src_functional/02201958.c#L78) |
| `0x06098794` | 33 | 24 | 24 | g_ctrl_struct, g_syspad | [project/src_functional/02201f20.c:79](../project/src_functional/02201f20.c#L79) |
| `0x06000644` | 33 | 15 | 15 | — | [project/src_functional/02209d70.c:74](../project/src_functional/02209d70.c#L74) |
| `0x06097d58` | 32 | 30 | 30 | — | [project/src_functional/0222ec98.c:13](../project/src_functional/0222ec98.c#L13) |

### Raw offset candidates

| Value | Occurrences | Functions | Files | Existing symbol suggestions | Example |
|---|---:|---:|---:|---|---|
| `0x8` | 2,124 | 607 | 596 | — | [project/src_functional/02201590.c:28](../project/src_functional/02201590.c#L28) |
| `0x10` | 1,746 | 417 | 411 | — | [project/src_functional/02201590.c:30](../project/src_functional/02201590.c#L30) |
| `0x0` | 1,479 | 466 | 455 | — | [project/src_functional/0220074c.c:17](../project/src_functional/0220074c.c#L17) |
| `0xc` | 819 | 325 | 319 | — | [project/src_functional/02202138.c:104](../project/src_functional/02202138.c#L104) |
| `0x4` | 761 | 323 | 316 | — | [project/src_functional/02202138.c:98](../project/src_functional/02202138.c#L98) |
| `0x34` | 619 | 280 | 276 | — | [project/src_functional/02206e2e.c:145](../project/src_functional/02206e2e.c#L145) |
| `0x14` | 481 | 224 | 222 | — | [project/src_functional/02201c9c.c:73](../project/src_functional/02201c9c.c#L73) |
| `0x5c` | 417 | 151 | 151 | — | [project/src_functional/02206d8c.c:29](../project/src_functional/02206d8c.c#L29) |
| `0x18` | 408 | 168 | 167 | — | [project/src_functional/02201f20.c:48](../project/src_functional/02201f20.c#L48) |
| `0x30` | 382 | 191 | 191 | — | [project/src_functional/02202138.c:177](../project/src_functional/02202138.c#L177) |
| `0x44` | 356 | 141 | 141 | — | [project/src_functional/02201c9c.c:47](../project/src_functional/02201c9c.c#L47) |
| `0x84` | 328 | 161 | 161 | — | [project/src_functional/02202138.c:165](../project/src_functional/02202138.c#L165) |
| `0x2` | 320 | 114 | 113 | — | [project/src_functional/02201958.c:52](../project/src_functional/02201958.c#L52) |
| `0x26` | 308 | 146 | 146 | — | [project/src_functional/02202a28.c:33](../project/src_functional/02202a28.c#L33) |
| `0x58` | 299 | 107 | 107 | — | [project/src_functional/02202a28.c:38](../project/src_functional/02202a28.c#L38) |
| `0x64` | 288 | 145 | 145 | — | [project/src_functional/022027e4.c:70](../project/src_functional/022027e4.c#L70) |
| `0x60` | 272 | 114 | 113 | — | [project/src_functional/022029a4.c:16](../project/src_functional/022029a4.c#L16) |
| `0x1c` | 267 | 127 | 126 | — | [project/src_functional/02201f20.c:49](../project/src_functional/02201f20.c#L49) |
| `0x24` | 251 | 139 | 139 | — | [project/src_functional/02201f20.c:70](../project/src_functional/02201f20.c#L70) |
| `0x68` | 242 | 112 | 112 | — | [project/src_functional/02202e84.c:196](../project/src_functional/02202e84.c#L196) |
| `0xe0` | 237 | 108 | 108 | — | [project/src_functional/02201f20.c:56](../project/src_functional/02201f20.c#L56) |
| `0x40` | 224 | 81 | 80 | — | [project/src_functional/02201c9c.c:46](../project/src_functional/02201c9c.c#L46) |
| `0x28` | 220 | 127 | 127 | — | [project/src_functional/02202a28.c:34](../project/src_functional/02202a28.c#L34) |
| `0x88` | 207 | 78 | 77 | — | [project/src_functional/02209d70.c:89](../project/src_functional/02209d70.c#L89) |
| `0x41` | 200 | 79 | 79 | — | [project/src_functional/02212b20.c:50](../project/src_functional/02212b20.c#L50) |
| `0x54` | 184 | 103 | 103 | — | [project/src_functional/022094c4.c:41](../project/src_functional/022094c4.c#L41) |
| `0xc4` | 178 | 75 | 75 | — | [project/src_functional/0220b1c0.c:50](../project/src_functional/0220b1c0.c#L50) |
| `0x38` | 168 | 96 | 96 | — | [project/src_functional/02206e2e.c:61](../project/src_functional/02206e2e.c#L61) |
| `0x6c` | 166 | 59 | 58 | — | [project/src_functional/02203ed4.c:53](../project/src_functional/02203ed4.c#L53) |
| `0x20` | 159 | 87 | 87 | — | [project/src_functional/02201f20.c:70](../project/src_functional/02201f20.c#L70) |
| `0x3c` | 156 | 69 | 68 | — | [project/src_functional/02201590.c:58](../project/src_functional/02201590.c#L58) |
| `0x48` | 153 | 83 | 83 | — | [project/src_functional/02202e84.c:187](../project/src_functional/02202e84.c#L187) |
| `0x50` | 144 | 68 | 67 | — | [project/src_functional/02201f20.c:72](../project/src_functional/02201f20.c#L72) |
| `0xd4` | 140 | 61 | 61 | — | [project/src_functional/0220f6f6.c:21](../project/src_functional/0220f6f6.c#L21) |
| `0x70` | 139 | 58 | 58 | — | [project/src_functional/02203ed4.c:54](../project/src_functional/02203ed4.c#L54) |
| `0x43` | 136 | 65 | 65 | — | [project/src_functional/022094c4.c:36](../project/src_functional/022094c4.c#L36) |
| `0x98` | 135 | 44 | 44 | — | [project/src_functional/0220b3cc.c:29](../project/src_functional/0220b3cc.c#L29) |
| `0x78` | 134 | 49 | 49 | — | [project/src_functional/02203ed4.c:56](../project/src_functional/02203ed4.c#L56) |
| `0xc8` | 133 | 53 | 53 | — | [project/src_functional/0220b1c0.c:52](../project/src_functional/0220b1c0.c#L52) |
| `0x62` | 132 | 80 | 79 | — | [project/src_functional/02210b58.c:64](../project/src_functional/02210b58.c#L64) |
| `0xd8` | 132 | 59 | 59 | — | [project/src_functional/0221632e.c:44](../project/src_functional/0221632e.c#L44) |
| `0x74` | 128 | 62 | 62 | — | [project/src_functional/02203ed4.c:55](../project/src_functional/02203ed4.c#L55) |
| `0x4c` | 126 | 81 | 81 | — | [project/src_functional/02207360.c:74](../project/src_functional/02207360.c#L74) |
| `0x90` | 122 | 56 | 56 | — | [project/src_functional/0220b3cc.c:27](../project/src_functional/0220b3cc.c#L27) |
| `0x32` | 118 | 42 | 42 | — | [project/src_functional/02203ed4.c:48](../project/src_functional/02203ed4.c#L48) |
| `0x45` | 114 | 32 | 32 | — | [project/src_functional/0221e256.c:62](../project/src_functional/0221e256.c#L62) |
| `0x2c` | 113 | 56 | 56 | — | [project/src_functional/02205b20.c:63](../project/src_functional/02205b20.c#L63) |
| `0x56` | 112 | 64 | 64 | — | [project/src_functional/02211108.c:77](../project/src_functional/02211108.c#L77) |
| `0xe` | 110 | 49 | 49 | — | [project/src_functional/02201f20.c:45](../project/src_functional/02201f20.c#L45) |
| `0x8c` | 109 | 78 | 78 | — | [project/src_functional/0220b1c0.c:74](../project/src_functional/0220b1c0.c#L74) |

## Parse diagnostics

All source files parsed successfully.

## Baseline

Baseline generated: 2026-11-07T04:32:52.686880+00:00. Total noise **29,690 → 29,690 (+0)**; matched-function delta **+0**. Added functions: 0; removed functions: 0.

Zero-noise function share: 4.69% → 4.69%. Noisy-line share: 55.03% → 55.03%.

| Category | Delta (current − baseline) |
|---|---:|
| `raw_memory` | +0 |
| `offset_access` | +0 |
| `pointer_constant` | +0 |
| `hardware_address` | +0 |
| `address_function` | +0 |
| `numeric_call` | +0 |
| `unnamed_global` | +0 |
| `magic_offset` | +0 |
| `goto` | +0 |
| `label` | +0 |
| `register_variable` | +0 |
| `sign_extension` | +0 |

```sh
tools/decomp/venv/bin/python tools/decomp/noise_report.py
tools/decomp/venv/bin/python tools/decomp/noise_report.py --baseline save
tools/decomp/venv/bin/python tools/decomp/noise_report.py --baseline compare
```

Default outputs: notes/noise_report.md and work/noise_report.json. Baseline: work/noise_report_baseline.json; use --baseline-file for another snapshot. Save intentionally replaces that snapshot. Compare requires matching metric version/scope and complete parses, reports additions/removals separately, and matches functions by file/name independent of line-number movement. JSON records all per-function categories, event locations, candidates, input SHA-256 hashes, parse diagnostics, and comparison deltas.
