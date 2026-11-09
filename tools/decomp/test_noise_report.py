#!/usr/bin/env python3
"""Regression tests: python -m unittest discover -s tools/decomp -p test_noise_report.py."""
import contextlib
import copy
import io
import json
from pathlib import Path
import tempfile
import unittest

import noise_report as noise


def one(source):
    functions = noise.analyze_source(source)
    if len(functions) != 1:
        raise AssertionError(f"expected one function, found {len(functions)}")
    return functions[0]


class SyntaxTests(unittest.TestCase):
    def test_comments_strings_prototypes_and_definition_names_are_clean(self):
        fn = one(r'''
            extern int func_06020000(int r14);
            extern int g_060987bc;
            void func_02210000(void) {
                /* MEM32(0x060987bc); goto label; r4; */
                const char *text = "func_06020000 g_060987bc 0x25f80000";
                char brace = '}'; // label: goto label;
            }
        ''')
        self.assertEqual(fn["total"], 0)
        self.assertEqual(fn["end_line"], 8)

    def test_memory_and_hardware_overlap_candidates_do_not(self):
        fn = one("void f(void) { MEMU16(0x25F80000u) = MEM32(0x060987bc); }")
        self.assertEqual(fn["categories"]["raw_memory"], 2)
        self.assertEqual(fn["categories"]["hardware_address"], 1)
        self.assertEqual(fn["noise_lines"], 1)
        self.assertEqual(len(fn["candidates"]), 2)

    def test_nested_offset_macros_and_named_offsets(self):
        fn = one("void f(void *obj) { M32(MP(obj, 0x8), 0x34) = M32(obj, SLOT_OFFSET); FPTR(obj, 0); FU16(obj, 12); }")
        self.assertEqual(fn["categories"]["offset_access"], 4)
        self.assertEqual(fn["categories"]["magic_offset"], 0)
        self.assertEqual({c["value"] for c in fn["candidates"]}, {0, 8, 12, 0x34})

    def test_computed_offsets(self):
        fn = one("void f(void *obj, int i) { M32(obj, i * 4 + 0x10); M32(obj, i * STRIDE + FIELD); }")
        self.assertEqual(fn["categories"]["offset_access"], 1)
        self.assertEqual({c["value"] for c in fn["candidates"]}, {4, 16})

    def test_numeric_object_base_and_offset(self):
        fn = one("void f(void) { M32(0x060987bc, 0x10); }")
        self.assertEqual(fn["categories"]["raw_memory"], 1)
        self.assertEqual(fn["categories"]["offset_access"], 1)
        self.assertEqual({(c["kind"], c["value"]) for c in fn["candidates"]}, {("address", 0x060987bc), ("offset", 16)})

    def test_multiline_type_macro_coordinates(self):
        fn = one("void f(void) {\n PTR(\n s32 *,\n 0x060987bc);\n}")
        event = fn["events"][0]
        self.assertEqual(event["category"], "pointer_constant")
        self.assertEqual(event["line"], 2)
        self.assertEqual(fn["candidates"][0]["line"], 4)
        self.assertEqual(fn["end_line"], 5)

    def test_integer_casts_pointer_typedefs_null_and_named_constants(self):
        fn = one('''
            typedef void (*callback)(void);
            void f(int value) {
                (callback)(uintptr_t)(0x06020000 + 4);
                (void *)(0x060987bc + value * 4);
                (void *)0; (void *)(1 - 1); (void *)NULL;
                (void *)KNOWN_BASE; (int)0x060987bc;
            }
        ''')
        self.assertEqual(fn["categories"]["pointer_constant"], 2)
        self.assertEqual(fn["categories"]["raw_memory"], 0)
        self.assertEqual(len(fn["candidates"]), 2)

    def test_pointer_dereference_and_numeric_call(self):
        fn = one('''
            typedef void (*callback)(void);
            void f(void) {
                *(volatile u16 *)(uintptr_t)0x25f80000 = 1;
                ((callback)(uintptr_t)0x06020000)();
                (*(callback *)(uintptr_t)0x06000610)();
                named_callback();
            }
        ''')
        self.assertEqual(fn["categories"]["pointer_constant"], 3)
        self.assertEqual(fn["categories"]["numeric_call"], 2)
        self.assertEqual(fn["categories"]["raw_memory"], 2)

    def test_local_accessors_and_callback_type_argument(self):
        fn = one('''
            #define GP(a) (*(void **)(uintptr_t)(a))
            #define AP(a) ((void *)(uintptr_t)(a))
            #define CALL(a, T) (*(T *)(uintptr_t)(a))
            typedef void (*callback)(void);
            void f(void) {
                GP(0x060987bc); AP(0x060987bc);
                CALL(0x06000610, callback)();
            }
        ''')
        self.assertEqual(fn["categories"]["raw_memory"], 2)
        self.assertEqual(fn["categories"]["pointer_constant"], 1)
        self.assertEqual(fn["categories"]["numeric_call"], 1)

    def test_named_wrappers_and_multiline_directives_are_clean(self):
        fn = one('''
            #define TVMD (*(volatile u16 *)0x25f80000)
            #define READ_PANEL(buf) \\
                ((void (*)(u32))(uintptr_t)MEM32(0x06000660))((u32)(buf))
            #define EMIT(obj) do { \\
                M32(obj, 0x10) += 1; \\
            } while (0)
            void f(void *obj) { TVMD = 1; EMIT(obj); READ_PANEL(obj); }
        ''')
        self.assertEqual(fn["total"], 0)

    def test_va_arg_type_arguments(self):
        fn = one("void f(int n, ...) { va_list ap; va_start(ap,n); va_arg(ap, void *); va_arg(ap,s32); va_end(ap); }")
        self.assertEqual(fn["total"], 0)

    def test_address_function_uses_and_global_uses(self):
        fn = one('''
            extern int g_060987bc;
            extern void func_06020000(void);
            void func_02210000(void) {
                func_06020000(); register_callback(func_06020000);
                g_060987bc = g_060987bc + 1;
            }
        ''')
        self.assertEqual(fn["categories"]["address_function"], 2)
        self.assertEqual(fn["categories"]["unnamed_global"], 2)

    def test_labels_not_cases_or_members(self):
        fn = one('''
            struct S { int g_060987bc; int func_06020000; };
            void f(struct S *obj, int x) {
                switch (x) { case 0: goto done; default: break; }
                done: obj->g_060987bc = obj->func_06020000;
            }
        ''')
        self.assertEqual(fn["categories"]["goto"], 1)
        self.assertEqual(fn["categories"]["label"], 1)
        self.assertEqual(fn["total"], 2)

    def test_register_declarations_count_once(self):
        fn = one("int f(int r4) { int r14 = r4, r16 = 0; r14 += r4; return r14; }")
        self.assertEqual(fn["categories"]["register_variable"], 2)

    def test_local_prototype_register_names_are_clean(self):
        fn = one("void f(void) { extern int helper(int r14); }")
        self.assertEqual(fn["total"], 0)

    def test_integer_object_address_offsets(self):
        fn = one("void f(u32 obj, int *array) { *(int *)(uintptr_t)(obj + 0x10) = 1; (int *)(array + 1); }")
        self.assertEqual(fn["categories"]["magic_offset"], 1)
        self.assertEqual(fn["categories"]["pointer_constant"], 0)

    def test_pointer_and_typedef_scopes(self):
        fn = one('''
            void f(u8 *obj, int n) {
                typedef int value_type;
                { int *obj; typedef void *value_type; (value_type)1234; obj + 1; }
                (value_type)1234; obj + 0x10;
                for (int *obj = 0; n; n--) { obj + 1; }
                obj + 0x10;
            }
        ''')
        self.assertEqual(fn["categories"]["magic_offset"], 2)
        self.assertEqual(fn["categories"]["pointer_constant"], 1)

    def test_split_signature_lines(self):
        fn = one("static\nint\nf(int r4) {\n return r4;\n}")
        self.assertEqual(fn["start_line"], 1)
        self.assertEqual(fn["code_lines"], 5)

    def test_magic_byte_offsets_and_placeholder_members(self):
        fn = one('''
            struct S { int unk34; int field_10; int named; };
            void f(u8 *obj, int *array, struct S *actor) {
                *(u32 *)(obj + 0x10) = 1;
                MEM8(obj + 15); MEM8(obj + NAMED_OFFSET);
                array[3] = *(array + 1);
                actor->unk34 = actor->field_10 + actor->named;
            }
        ''')
        self.assertEqual(fn["categories"]["magic_offset"], 4)
        self.assertEqual(fn["categories"]["raw_memory"], 0)

    def test_sign_extension_idioms_not_fixed_point_shifts(self):
        fn = one('''
            int f(int x) {
                (s32)(s8)x; (s32)(s16)x;
                ((s32)(x << 24)) >> 24;
                (x ^ 0x80) - 0x80; SEXT16(x);
                (s16)x; x >> 16; (x * 4) >> 16;
                return x;
            }
        ''')
        self.assertEqual(fn["categories"]["sign_extension"], 5)

    def test_hardware_windows_and_sentinels(self):
        for value in (0x25f80000, 0x05f80000, 0x20100001, 0x00100077, 0xffffff00, 0xfffffe92):
            self.assertTrue(noise.hardware(value), hex(value))
        for value in (0x060987bc, 0x02210000, 0xffffffff, 0xfffffffe, 0x1234):
            self.assertFalse(noise.hardware(value), hex(value))

    def test_helper_function_boundaries(self):
        functions = noise.analyze_source('''
            static int helper(void) { return 1; }
            void f(void) { MEM32(0x060987bc); }
            extern int unused(int r4);
        ''')
        self.assertEqual([f["name"] for f in functions], ["helper", "f"])
        self.assertEqual([f["total"] for f in functions], [0, 1])

    def test_unsupported_conditionals_are_explicit(self):
        with self.assertRaisesRegex(ValueError, "conditional"):
            noise.analyze_source("#if 0\nvoid f(void) {}\n#endif\n")


class ReportTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="noise_report_test_")
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.src = self.root / "src"
        self.src.mkdir()
        self.symbols = self.root / "symbols.txt"
        self.symbols.write_text("0x0221c000 resident_helper # HIGH\n0x060987bc frame_counter # HIGH\n")

    def report(self, text):
        (self.src / "a.c").write_text(text)
        return noise.build_report(self.src, self.symbols, root=self.root)

    def test_aggregate_percentages_and_repeated_sites(self):
        r = self.report("void clean(void) {}\nvoid noisy(void) { MEM32(0x060987bc); MEM16(0x060987bc); }\n")
        self.assertEqual(r["summary"]["zero_noise_percent"], 50)
        self.assertEqual(r["summary"]["noise_line_percent"], 50)
        self.assertEqual(r["summary"]["total_noise"], 2)
        candidate = r["repeated"]["address"][0]
        self.assertEqual(candidate["occurrences"], 2)
        self.assertEqual(candidate["functions"], 1)
        self.assertEqual(candidate["files"], 1)
        self.assertEqual(candidate["symbols"], ["frame_counter"])

    def test_physical_lines_shared_by_two_definitions(self):
        r = self.report("void f(void) {} void g(void) { MEM32(0x060987bc); }\n")
        self.assertEqual(r["summary"]["code_lines"], 1)
        self.assertEqual(r["summary"]["noise_lines"], 1)
        self.assertEqual(r["summary"]["noise_line_percent"], 100)

    def test_symbol_lookup_uses_coverage_canonicalization(self):
        r = self.report("void f(void) { func_06020000(); func_0021c000(); }")
        for item in r["repeated"]["address"]:
            self.assertEqual(item["symbols"], ["resident_helper"])
        self.assertEqual(len(r["repeated"]["address"]), 2)

    def test_invalid_input_is_reported_not_silently_clean(self):
        r = self.report("void f( { ")
        self.assertEqual(len(r["files"]["failures"]), 1)
        self.assertEqual(r["files"]["parsed"], 0)
        self.assertEqual(r["summary"]["functions"], 0)
        self.assertIn("Parse diagnostics", noise.markdown(r))

    def test_baseline_tracks_cleanup_line_movement_additions_removals(self):
        before = self.report("void f(void) { MEM32(0x060987bc); }\nvoid removed(void) { goto end; end:; }\n")
        after = self.report("\n\nvoid f(void) { MEM32(FRAME_COUNTER); }\nvoid added(void) {}\n")
        delta = noise.compare_baseline(after, before)
        self.assertEqual(delta["matched_function_noise_delta"], -1)
        self.assertEqual(delta["delta"]["total_noise"], -3)
        self.assertEqual(len(delta["added_functions"]), 1)
        self.assertEqual(len(delta["removed_functions"]), 1)
        self.assertEqual(delta["changed_functions"][0]["name"], "f")
        after["baseline"] = delta
        self.assertIn("matched-function delta **-1**", noise.markdown(after))

    def test_baseline_rejects_incompatible_or_incomplete_metrics(self):
        r = self.report("void f(void) {}")
        for key, replacement in (("tool_version", "2.00"), ("schema_version", 999), ("scope", {})):
            old = copy.deepcopy(r)
            old[key] = replacement
            with self.assertRaises(ValueError):
                noise.compare_baseline(r, old)
        old = copy.deepcopy(r)
        old["files"]["failures"] = [{"file": "bad.c", "error": "bad"}]
        with self.assertRaisesRegex(ValueError, "complete parses"):
            noise.compare_baseline(r, old)

    def test_cli_save_compare_json_and_error_paths(self):
        self.report("void f(void) { MEM32(0x060987bc); }")
        md, js, baseline = [self.root / p for p in ("out.md", "out.json", "baseline.json")]
        argv = ["--src", str(self.src), "--symbols", str(self.symbols), "--markdown", str(md), "--json", str(js), "--baseline-file", str(baseline)]
        with contextlib.redirect_stdout(io.StringIO()):
            self.assertEqual(noise.main(argv + ["--baseline", "save"]), 0)
            self.assertTrue(baseline.exists())
            (self.src / "a.c").write_text("void f(void) { MEM32(FRAME_COUNTER); }")
            self.assertEqual(noise.main(argv + ["--baseline", "compare"]), 0)
        r = json.loads(js.read_text())
        self.assertEqual(r["baseline"]["delta"]["total_noise"], -1)
        self.assertIn("Top 50", md.read_text())
        self.assertTrue(r["inputs"][0]["sha256"])
        self.assertNotIn("baseline", json.loads(baseline.read_text()))
        (self.src / "a.c").write_text("void f( {")
        original_baseline = baseline.read_bytes()
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(noise.main(argv + ["--baseline", "save"]), 1)
        self.assertEqual(baseline.read_bytes(), original_baseline)
        self.assertEqual(len(json.loads(js.read_text())["files"]["failures"]), 1)

    def test_cli_missing_baseline_does_not_overwrite_reports(self):
        self.report("void f(void) {}")
        md, js = self.root / "out.md", self.root / "out.json"
        md.write_text("keep")
        argv = ["--src", str(self.src), "--markdown", str(md), "--json", str(js), "--baseline", "compare", "--baseline-file", str(self.root / "absent.json")]
        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as exc:
            noise.main(argv)
        self.assertEqual(exc.exception.code, 2)
        self.assertEqual(md.read_text(), "keep")
        self.assertFalse(js.exists())


if __name__ == "__main__":
    unittest.main()
