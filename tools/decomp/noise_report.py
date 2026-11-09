#!/usr/bin/env python3
"""Readability noise for functional SH-2 C, v1.00 (independent implementation).

Run from anywhere with tools/decomp/venv/bin/python. No compiler, ROM, or network
is needed. Macro calls remain unexpanded: naming an address/field is an actual
readability improvement. pycparser handles C syntax, not C type checking.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import sys

try:
    from pycparser import c_ast, c_parser
except ImportError:
    sys.exit("pycparser required: run with tools/decomp/venv/bin/python")

VERSION = "1.00"
SCHEMA = 1
ROOT = Path(__file__).resolve().parents[2]
CATEGORIES = {
    "raw_memory": "Numeric absolute memory accesses (MEM*, dereferences, local accessor aliases)",
    "offset_access": "Numeric offset accessor calls (M*, MP, FS*/FU*, FPTR)",
    "pointer_constant": "Nonzero numeric integer-to-pointer casts / PTR* constructors",
    "hardware_address": "Raw Saturn / SH-2 hardware address literals",
    "address_function": "Uses of func_/FUN_ followed by 8 hex digits (not definitions/prototypes)",
    "numeric_call": "Calls through numeric pointer expressions / numeric callback slots",
    "unnamed_global": "Uses of g_ followed by 8 hex digits (not declarations)",
    "magic_offset": "Numeric byte-pointer arithmetic and offset-named struct members",
    "goto": "goto statements",
    "label": "C labels (excluding switch case/default)",
    "register_variable": "Declarations/parameters named r0..r15, R0..R15, sr/gbr/vbr/pr/mach/macl",
    "sign_extension": "Signed widening of narrow casts, shift-pair / xor-subtract sign extension, sext helpers",
}
FUNC_NAME = re.compile(r"(?:func_|FUN_)([0-9a-fA-F]{8})\Z")
GLOBAL_NAME = re.compile(r"g_([0-9a-fA-F]{8})\Z")
REGISTER = re.compile(r"(?:[rR](?:[0-9]|1[0-5])|sr|gbr|vbr|pr|mach|macl)\Z", re.I)
FIELD = re.compile(r"(?:unk_?|field_|pad_?)([0-9a-fA-F]+)\Z")
OFFSET_MACRO = re.compile(r"(?:MU?(?:8|16|32|64)|MP|F[SU](?:8|16|32|64)|FPTR)\Z")
MEM_MACRO = re.compile(r"MEM[US]?(?:8|16|32|64)\Z")
PTR_MACRO = re.compile(r"PTR(?:8|16|32|64)?\Z")
LITERALS = re.compile(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
TOKENS = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|[A-Za-z_]\w*|[^\s]')
DIRECTIVE = re.compile(r"^[ \t]*#(?:[^\n]*\\\n)*[^\n]*", re.M)
# The shared headers contain only these typedefs and access macros. Do not
# expand them: that would turn named accesses back into numeric CPU operations.
PRELUDE = " ".join(
    f"typedef {ctype} {name};" for name, ctype in {
        "s8": "signed char", "u8": "unsigned char", "s16": "short",
        "u16": "unsigned short", "s32": "int", "u32": "unsigned int",
        "s64": "long long", "u64": "unsigned long long", "f32": "float",
        "f64": "double", "int8_t": "signed char", "uint8_t": "unsigned char",
        "int16_t": "short", "uint16_t": "unsigned short", "int32_t": "int",
        "uint32_t": "unsigned int", "int64_t": "long long",
        "uint64_t": "unsigned long long", "uintptr_t": "unsigned long",
        "intptr_t": "long", "size_t": "unsigned long", "ptrdiff_t": "long",
        "va_list": "void *",
    }.items()
)


def blank(text):
    return "".join("\n" if ch == "\n" else " " for ch in text)


def prepare(source):
    """Keep coordinates, drop comments/directives, mask macro type arguments.

    This is deliberately not a preprocessor. Conditional directives are rejected
    rather than counting both branches or silently choosing the wrong one.
    Object-like macros and wrapper macros are named abstractions, so their
    implementation bodies are outside the function metric.
    """
    code = LITERALS.sub(lambda m: blank(m[0]) if m[0].startswith(("/*", "//")) else m[0], source)
    aliases = {}
    for directive in DIRECTIVE.finditer(code):
        match = re.match(r"#[ \t]*define[ \t]+(\w+)\(([^)\n]*)\)[ \t]*(.*)", directive[0].lstrip().replace("\\\n", " "), re.S)
        if not match:
            continue
        name, params, body = match.groups()
        params = [p.strip() for p in params.split(",")]
        # Local absolute accessors conventionally use uintptr_t. Requiring the
        # address parameter keeps unrelated function macros out of this family.
        if params and re.search(r"\(\s*uintptr_t\s*\)\s*\(?\s*" + re.escape(params[0]) + r"\b", body):
            aliases[name] = "raw_memory" if re.search(r"\(\s*\*", body) else "pointer_constant"
    if re.search(r"^[ \t]*#[ \t]*(?:if|ifdef|ifndef|elif|else|endif)\b", code, re.M):
        raise ValueError("conditional preprocessing requires explicit support")
    code = DIRECTIVE.sub(lambda m: blank(m[0]), code)
    tokens = list(TOKENS.finditer(code))
    chars = list(code)

    def mask_type(start, end):
        replacement = list(blank(code[start:end]))
        for j, ch in enumerate(code[start:end]):
            if not ch.isspace():
                replacement[j] = "0"
                break
        chars[start:end] = replacement

    for i, tok in enumerate(tokens):
        # PTR(type, address), CALL(address, typedef): retain invocation and raw
        # address, replace only the type argument with an expression placeholder.
        which = 0 if tok[0] == "PTR" else 1 if tok[0] in ("CALL", "va_arg") else None
        if which is None or i + 1 >= len(tokens) or tokens[i + 1][0] != "(":
            continue
        depth, arg, start = 0, 0, tokens[i + 1].end()
        for t in tokens[i + 2:]:
            if t[0] in ("(", "["): depth += 1
            if t[0] in (")", "]"):
                if depth == 0:
                    if arg == which:
                        mask_type(start, t.start())
                    break
                depth -= 1
            if t[0] == "," and depth == 0:
                if arg == which:
                    mask_type(start, t.start())
                start, arg = t.end(), arg + 1
    return "".join(chars), aliases


def walk(node):
    if node is not None:
        yield node
        for _, child in node.children():
            yield from walk(child)


def integer(node):
    if not isinstance(node, c_ast.Constant) or node.type in ("char", "float", "double", "string"):
        return None
    value = re.sub(r"[uUlL]+$", "", node.value)
    try:
        return int(value, 16 if value.lower().startswith("0x") else 8 if len(value) > 1 and value.startswith("0") else 10)
    except ValueError:
        return None


def constant(node):
    """Evaluate only integer syntax, never resolve a named constant to a number."""
    if isinstance(node, c_ast.Cast):
        return constant(node.expr)
    if isinstance(node, c_ast.UnaryOp) and node.op in ("+", "-", "~"):
        n = constant(node.expr)
        return None if n is None else {"+": lambda: n, "-": lambda: -n, "~": lambda: ~n}[node.op]()
    if isinstance(node, c_ast.BinaryOp):
        a, b = constant(node.left), constant(node.right)
        if a is None or b is None:
            return None
        ops = {"+": lambda: a + b, "-": lambda: a - b, "*": lambda: a * b,
               "<<": lambda: a << b, ">>": lambda: a >> b, "|": lambda: a | b,
               "&": lambda: a & b, "^": lambda: a ^ b}
        if node.op not in ops or (node.op in ("<<", ">>") and not 0 <= b <= 63):
            return None
        return ops[node.op]()
    return integer(node)


def numeric_parts(node):
    """A literal-only expression, or large address bases within an expression."""
    if constant(node) is not None:
        return [n for n in walk(node) if integer(n) is not None]
    if isinstance(node, (c_ast.Cast, c_ast.BinaryOp, c_ast.UnaryOp)):
        return [n for n in walk(node) if integer(n) is not None and integer(n) >= 0x100000]
    return []


def hardware(value):
    # P0/P1 aliases of Saturn peripherals, SMPC, SH-2 on-chip registers.
    # Top all-ones values are frequent sentinels, not register addresses.
    physical = value & 0x1FFFFFFF
    return (0x05000000 <= physical <= 0x05FFFFFF or
            0x00100000 <= physical <= 0x0010007F or
            0xFFFFFE00 <= value < 0xFFFFFFF0)


def canon(value):
    if 0x00200000 <= value <= 0x0029FFFF:
        return value + 0x02000000
    if 0x06000000 <= value <= 0x06100000:
        return value - 0x03E04000
    return value


def load_symbols(path):
    symbols = defaultdict(list)
    if path.exists():
        for line in path.read_text().splitlines():
            m = re.match(r"\s*0x([\da-fA-F]+)\s+([A-Za-z_]\w*)", line)
            if m and not FUNC_NAME.fullmatch(m[2]) and not GLOBAL_NAME.fullmatch(m[2]) and m[2] not in symbols[int(m[1], 16)]:
                symbols[int(m[1], 16)].append(m[2])
    return dict(symbols)


def resolve_type(node, typedefs, seen=()):
    if isinstance(node, (c_ast.TypeDecl, c_ast.Typename, c_ast.Decl, c_ast.Typedef)):
        return resolve_type(node.type, typedefs, seen)
    if isinstance(node, c_ast.IdentifierType) and len(node.names) == 1:
        name = node.names[0]
        if name in typedefs and name not in seen:
            return resolve_type(typedefs[name], typedefs, seen + (name,))
    return node


def signed_width(node, typedefs):
    node = resolve_type(node, typedefs)
    if not isinstance(node, c_ast.IdentifierType):
        return None
    names = node.names
    if "unsigned" in names or "float" in names or "double" in names or "void" in names:
        return None
    if "char" in names:
        return 8 if "signed" in names else None
    return 16 if "short" in names else 64 if names.count("long") == 2 else 32


class FunctionNoise(c_ast.NodeVisitor):
    def __init__(self, typedefs, aliases, variables=None):
        self.typedefs = dict(typedefs)
        self.aliases = aliases
        self.events = []
        self.candidates = {}
        self.variables = dict(variables or {})

    def visit_Compound(self, node):
        variables, typedefs = self.variables.copy(), self.typedefs.copy()
        self.generic_visit(node)
        self.variables, self.typedefs = variables, typedefs

    def visit_For(self, node):
        variables, typedefs = self.variables.copy(), self.typedefs.copy()
        self.generic_visit(node)
        self.variables, self.typedefs = variables, typedefs

    def visit_Typedef(self, node):
        self.typedefs[node.name] = node.type

    def emit(self, category, node, detail):
        self.events.append({"category": category, "line": node.coord.line,
                            "column": node.coord.column, "detail": detail})

    def candidate(self, kind, value, node):
        key = (kind, value, node.coord.line, node.coord.column)
        self.candidates[key] = {"kind": kind, "value": value, "line": node.coord.line,
                                "column": node.coord.column}

    def addresses(self, node):
        for part in numeric_parts(node):
            value = integer(part)
            if value >= 0x100000 or constant(node) == value:
                self.candidate("address", value, part)

    def pointer(self, typ):
        return isinstance(resolve_type(typ, self.typedefs), c_ast.PtrDecl)

    def byte_pointer(self, node):
        typ = node.to_type if isinstance(node, c_ast.Cast) else self.variables.get(node.name) if isinstance(node, c_ast.ID) else None
        typ = resolve_type(typ, self.typedefs)
        if not isinstance(typ, c_ast.PtrDecl):
            return False
        base = resolve_type(typ.type, self.typedefs)
        return isinstance(base, c_ast.IdentifierType) and ("char" in base.names or base.names == ["void"])

    def visit_Decl(self, node):
        if isinstance(node.type, c_ast.FuncDecl):
            return  # Local prototypes are declarations, too.
        if node.name:
            self.variables[node.name] = node.type
            if REGISTER.fullmatch(node.name) and not isinstance(node.type, c_ast.FuncDecl):
                self.emit("register_variable", node, node.name)
        # Declaration names/types are not ID uses; only expressions count.
        if isinstance(node.type, c_ast.ArrayDecl) and node.type.dim:
            self.visit(node.type.dim)
        if node.init:
            self.visit(node.init)
        if node.bitsize:
            self.visit(node.bitsize)

    def visit_ID(self, node):
        for pattern, category in ((FUNC_NAME, "address_function"), (GLOBAL_NAME, "unnamed_global")):
            m = pattern.fullmatch(node.name)
            if m:
                self.emit(category, node, node.name)
                self.candidate("address", int(m[1], 16), node)

    def visit_Constant(self, node):
        value = integer(node)
        if value is not None and hardware(value):
            self.emit("hardware_address", node, f"0x{value:08x}")
            self.candidate("address", value, node)

    def visit_StructRef(self, node):
        self.visit(node.name)
        m = FIELD.fullmatch(node.field.name)
        if m:
            self.emit("magic_offset", node.field, node.field.name)
            self.candidate("offset", int(m[1], 16), node.field)

    def visit_Cast(self, node):
        if self.pointer(node.to_type) and numeric_parts(node.expr) and constant(node.expr) != 0:
            self.emit("pointer_constant", node, "integer-to-pointer cast")
            self.addresses(node.expr)
        elif self.pointer(node.to_type):
            expr = node.expr
            while isinstance(expr, c_ast.Cast):
                expr = expr.expr
            if isinstance(expr, c_ast.BinaryOp) and expr.op in ("+", "-"):
                base = expr.left
                base_type = base.to_type if isinstance(base, c_ast.Cast) else self.variables.get(base.name) if isinstance(base, c_ast.ID) else None
                # Casting integer address arithmetic to a pointer exposes a
                # byte offset. Typed pointer + 1 remains ordinary C indexing.
                if base_type is not None and not self.pointer(base_type):
                    self.offset_arithmetic(expr, force=True)
        outer = signed_width(node.to_type, self.typedefs)
        inner = signed_width(node.expr.to_type, self.typedefs) if isinstance(node.expr, c_ast.Cast) else None
        if outer and inner in (8, 16) and outer > inner:
            self.emit("sign_extension", node, "signed widening of narrow cast")
        self.visit(node.expr)

    def visit_UnaryOp(self, node):
        if node.op == "*" and numeric_parts(node.expr):
            self.emit("raw_memory", node, "numeric pointer dereference")
            self.addresses(node.expr)
        self.visit(node.expr)

    def visit_FuncCall(self, node):
        name = node.name.name if isinstance(node.name, c_ast.ID) else None
        args = node.args.exprs if isinstance(node.args, c_ast.ExprList) else []
        if name and OFFSET_MACRO.fullmatch(name) and len(args) >= 2:
            parts = [n for n in walk(args[1]) if integer(n) is not None]
            if parts:
                self.emit("offset_access", node, name)
                for part in parts:
                    self.candidate("offset", integer(part), part)
            if numeric_parts(args[0]):
                self.emit("raw_memory", node, name + " numeric base")
                self.addresses(args[0])
        elif name and (MEM_MACRO.fullmatch(name) or PTR_MACRO.fullmatch(name) or name in self.aliases):
            index = 1 if name == "PTR" else 0
            if len(args) > index and numeric_parts(args[index]) and (not PTR_MACRO.fullmatch(name) or constant(args[index]) != 0):
                category = self.aliases.get(name, "pointer_constant" if PTR_MACRO.fullmatch(name) else "raw_memory")
                self.emit(category, node, name)
                self.addresses(args[index])
            # MEM8(task + 15) still exposes an object layout.
            if args and isinstance(args[0], c_ast.BinaryOp) and args[0].op in ("+", "-") and not numeric_parts(args[0]):
                self.offset_arithmetic(args[0], force=True)
        if not name and self.numeric_callee(node.name):
            self.emit("numeric_call", node, "numeric function pointer / callback slot")
        if name and re.fullmatch(r"(?:SEXT|sext|sign_extend)_?(?:8|16)", name):
            self.emit("sign_extension", node, name)
        self.generic_visit(node)

    def numeric_callee(self, node):
        if isinstance(node, c_ast.FuncCall):
            name = node.name.name if isinstance(node.name, c_ast.ID) else ""
            args = node.args.exprs if node.args else []
            index = 1 if name == "PTR" else 0
            return bool((name in self.aliases or MEM_MACRO.fullmatch(name) or PTR_MACRO.fullmatch(name)) and len(args) > index and numeric_parts(args[index]))
        return bool(numeric_parts(node))

    def offset_arithmetic(self, node, force=False):
        # A byte/void pointer plus a raw constant exposes byte layout; ordinary
        # typed array indexing and integer arithmetic do not.
        if node.op not in ("+", "-"):
            return
        side = node.right if force or self.byte_pointer(node.left) else node.left if node.op == "+" and self.byte_pointer(node.right) else None
        if side is not None and constant(side) is not None and not numeric_parts(node):
            # Large bases are addresses rather than object offsets.
            if abs(constant(side)) < 0x100000:
                self.emit("magic_offset", node, "numeric byte-pointer displacement")
                self.candidate("offset", constant(side), side)

    def visit_BinaryOp(self, node):
        self.offset_arithmetic(node)
        left = node.left
        while isinstance(left, c_ast.Cast):
            left = left.expr
        if (node.op == ">>" and isinstance(left, c_ast.BinaryOp) and left.op == "<<"
                and constant(node.right) in (16, 24) and constant(left.right) == constant(node.right)):
            self.emit("sign_extension", node, "shift pair (signedness heuristic)")
        if (node.op == "-" and isinstance(left, c_ast.BinaryOp) and left.op == "^"
                and constant(node.right) in (0x80, 0x8000) and constant(left.right) == constant(node.right)):
            self.emit("sign_extension", node, "xor/subtract sign extension")
        self.generic_visit(node)

    def visit_Goto(self, node):
        self.emit("goto", node, node.name)

    def visit_Label(self, node):
        self.emit("label", node, node.name)
        self.visit(node.stmt)


def analyze_source(source, filename="sample.c"):
    code, aliases = prepare(source)
    tree = c_parser.CParser().parse(PRELUDE + "\n#line 1\n" + code, filename=filename)
    typedefs = {n.name: n.type for n in tree.ext if isinstance(n, c_ast.Typedef)}
    variables = {n.name: n.type for n in tree.ext if isinstance(n, c_ast.Decl)}
    lines = code.splitlines()
    starts = [0]
    for line in code.splitlines(keepends=True):
        starts.append(starts[-1] + len(line))
    # Find physical signature starts, including split return types/qualifiers.
    signature_starts, depth, parens, segment = {}, 0, 0, None
    for token in TOKENS.finditer(code):
        if segment is None:
            segment = token.start()
        if token[0] == "(": parens += 1
        elif token[0] == ")": parens -= 1
        elif token[0] == "{":
            if depth == 0 and parens == 0:
                signature_starts[token.start()] = segment
            depth += 1
        elif token[0] == "}":
            depth -= 1
            if depth == 0 and parens == 0:
                segment = None
        elif token[0] == ";" and depth == 0 and parens == 0:
            segment = None
    functions = []
    for fn in tree.ext:
        if not isinstance(fn, c_ast.FuncDef):
            continue
        visitor = FunctionNoise(typedefs, aliases, variables)
        if fn.decl.type.args:
            visitor.visit(fn.decl.type.args)
        visitor.visit(fn.body)
        begin = starts[fn.body.coord.line - 1] + fn.body.coord.column - 1
        first_line = code.count("\n", 0, signature_starts.get(begin, begin)) + 1
        depth, end = 0, None
        for token in TOKENS.finditer(code, begin):
            if token[0] == "{": depth += 1
            elif token[0] == "}":
                depth -= 1
                if depth == 0:
                    end = code.count("\n", 0, token.end()) + 1
                    break
        if end is None:
            raise ValueError(f"missing function end: {fn.decl.name}")
        # One operation/category/location counts once (e.g. byte-pointer
        # displacement seen both in a MEM argument and in the general visitor).
        events = list({(e["category"], e["line"], e["column"]): e for e in visitor.events}.values())
        counts = Counter(e["category"] for e in events)
        functions.append({
            "id": f"{filename}:{fn.decl.name}:{fn.decl.coord.line}",
            "file": filename, "name": fn.decl.name,
            "start_line": first_line, "end_line": end,
            "code_lines": sum(bool(line.strip()) for line in lines[first_line - 1:end]),
            "code_line_numbers": [i for i in range(first_line, end + 1) if lines[i - 1].strip()],
            "noise_lines": len({e["line"] for e in events}),
            "total": sum(counts.values()),
            "categories": {c: counts[c] for c in CATEGORIES},
            "events": sorted(events, key=lambda e: (e["line"], e["column"], e["category"])),
            "candidates": list(visitor.candidates.values()),
        })
    return functions


def percent(n, d):
    return round(100 * n / d, 2) if d else 0.0


def repeated_candidates(functions, symbols):
    groups = {}
    for fn in functions:
        for item in fn["candidates"]:
            key = (item["kind"], item["value"])
            group = groups.setdefault(key, {"kind": key[0], "value": key[1], "occurrences": 0, "functions": set(), "files": set(), "examples": []})
            group["occurrences"] += 1
            group["functions"].add(fn["id"])
            group["files"].add(fn["file"])
            if len(group["examples"]) < 3:
                group["examples"].append({"file": fn["file"], "function": fn["name"], "line": item["line"]})
    results = {"address": [], "offset": []}
    for (kind, value), group in groups.items():
        group["functions"] = len(group["functions"])
        group["files"] = len(group["files"])
        group["hex"] = f"0x{value:08x}" if kind == "address" else (f"-0x{-value:x}" if value < 0 else f"0x{value:x}")
        group["symbols"] = list(dict.fromkeys(symbols.get(value, []) + (symbols.get(canon(value), []) if kind == "address" else []))) if kind == "address" else []
        results[kind].append(group)
    for values in results.values():
        values.sort(key=lambda g: (-g["occurrences"], -g["files"], g["value"]))
    return results


def build_report(src, symbols_path, root=ROOT):
    functions, failures, inputs, empty = [], [], [], []
    paths = sorted(src.glob("*.c"))
    for path in paths:
        filename = path.relative_to(root).as_posix() if path.is_relative_to(root) else path.as_posix()
        data = path.read_bytes()
        inputs.append({"file": filename, "sha256": hashlib.sha256(data).hexdigest()})
        try:
            found = analyze_source(data.decode("utf-8", errors="replace"), filename)
            functions.extend(found)
            if not found:
                empty.append(filename)
        except (c_parser.ParseError, ValueError) as exc:
            failures.append({"file": filename, "error": str(exc)})
    totals = {c: sum(f["categories"][c] for f in functions) for c in CATEGORIES}
    lines = len({(f["file"], line) for f in functions for line in f["code_line_numbers"]})
    noisy_lines = len({(f["file"], e["line"]) for f in functions for e in f["events"]})
    clean = sum(f["total"] == 0 for f in functions)
    symbols = load_symbols(symbols_path)
    return {
        "schema_version": SCHEMA, "tool_version": VERSION,
        "generated_at": datetime.now(timezone.utc).isoformat(),
        "metric": "source_readability_noise", "category_descriptions": CATEGORIES,
        "scope": {"glob": "*.c", "source": str(src.relative_to(root) if src.is_relative_to(root) else src),
                  "macro_policy": "unexpanded; named macro bodies excluded", "function_policy": "definitions including helpers; declarations excluded"},
        "files": {"scanned": len(paths), "parsed": len(paths) - len(failures), "without_functions": empty, "failures": failures},
        "summary": {"functions": len(functions), "total_noise": sum(totals.values()),
                    "zero_noise_functions": clean, "zero_noise_percent": percent(clean, len(functions)),
                    "code_lines": lines, "noise_lines": noisy_lines, "noise_line_percent": percent(noisy_lines, lines)},
        "categories": totals, "functions": functions,
        "repeated": repeated_candidates(functions, symbols), "inputs": inputs,
        "symbols_sha256": hashlib.sha256(symbols_path.read_bytes()).hexdigest() if symbols_path.exists() else None,
    }


def compare_baseline(report, baseline):
    if baseline.get("schema_version") != SCHEMA or baseline.get("tool_version") != VERSION:
        raise ValueError("baseline schema/tool version differs; save a fresh baseline")
    if baseline.get("scope") != report["scope"]:
        raise ValueError("baseline scope differs from this report")
    if baseline["files"]["failures"] or report["files"]["failures"]:
        raise ValueError("baseline comparison requires complete parses in both reports")
    # Line numbers move during cleanup; function identity is file + name. Reject
    # duplicate definitions instead of silently pairing unrelated functions.
    def index(functions):
        result = {}
        for fn in functions:
            key = (fn["file"], fn["name"])
            if key in result:
                raise ValueError(f"ambiguous baseline function identity: {key}")
            result[key] = fn
        return result
    old, new = index(baseline["functions"]), index(report["functions"])
    common = old.keys() & new.keys()
    changes = [{"file": key[0], "name": key[1], "before": old[key]["total"], "after": new[key]["total"],
                "delta": new[key]["total"] - old[key]["total"]} for key in sorted(common) if new[key]["total"] != old[key]["total"]]
    changes.sort(key=lambda f: (f["delta"], f["file"], f["name"]))
    return {"baseline_generated_at": baseline["generated_at"],
            "before": baseline["summary"], "after": report["summary"],
            "delta": {key: report["summary"][key] - value for key, value in baseline["summary"].items()},
            "categories": {c: report["categories"][c] - baseline["categories"][c] for c in CATEGORIES},
            "matched_function_noise_delta": sum(new[k]["total"] - old[k]["total"] for k in common),
            "added_functions": [{"file": k[0], "name": k[1], "noise": new[k]["total"]} for k in sorted(new.keys() - old.keys())],
            "removed_functions": [{"file": k[0], "name": k[1], "noise": old[k]["total"]} for k in sorted(old.keys() - new.keys())],
            "changed_functions": changes}


def markdown(report):
    s = report["summary"]
    out = [f"# Decomp readability noise report v{VERSION}", "", f"Generated: {report['generated_at']}", "",
           f"Source: `{report['scope']['source']}/*.c`. Parsed **{report['files']['parsed']}/{report['files']['scanned']} files**, **{s['functions']:,} function definitions** (including helpers).", "",
           f"Total noise: **{s['total_noise']:,} category occurrences**. Zero-noise functions: **{s['zero_noise_functions']:,}/{s['functions']:,} ({s['zero_noise_percent']:.2f}%)**. Lines with noise: **{s['noise_lines']:,}/{s['code_lines']:,} ({s['noise_line_percent']:.2f}%)**.", "",
           "## Counting rules and limits", "",
           "This measures how much source still exposes CPU addresses, object layout, and SH-2 idioms; it is independent of coverage and correctness. Lower is more readable under these rules. A zero score does not prove that a function is well named or understood.", "",
           "pycparser parses each translation unit with the shared integer typedef vocabulary, retaining macro invocations. Comments, strings, includes, directives, header bodies, prototypes, definition names, and file-scope declarations/initializers do not contribute noise. Function parameter declarations and local initializers do. Named constants, semantic members, and named wrapper macro bodies are clean; replacing an address with a named symbol reduces the metric. Local uintptr_t accessor aliases are recognized. PTR/CALL/va_arg type arguments are masked for parsing with source coordinates preserved. Conditional preprocessing is rejected explicitly.", "",
           "Categories overlap intentionally: `MEM16(0x25f80000)` has raw-memory and hardware-address noise; `*(u32 *)0x06000000` has raw-memory and pointer-constant noise. Each category/operation/location counts once. Address-named function definitions are excluded; uses as calls or callbacks count. Register variables count once per declaration, including parameters, rather than on every read/write. Gotos and labels count separately; case/default are clean.", "",
           "The line denominator is nonblank, noncomment physical lines within parsed function definitions (signature through closing brace). The numerator is distinct lines with at least one event, so overlapping categories never multiply line counts. Totals deduplicate physical lines even when two functions share a line. Parse failures are excluded from metrics and listed below; any failure makes the command exit nonzero.", "",
           "Offset accessors include decimal and zero offsets, computed offsets containing literals, MP and FS/FU/FPTR. Other magic offsets are numeric byte/void-pointer displacements, integer-address displacements explicitly cast to pointers, and unk/field_/pad offset members (hex suffix). Ordinary array indices, typed pointer strides, and general arithmetic constants are excluded. Sign-extension detection is conservative: nested signed 8/16-to-wider casts, equal 16/24-bit left/right shifts (a signedness heuristic), xor/subtract 0x80/0x8000, and sext/sign_extend helpers; plain narrowing casts are excluded. It does not perform dataflow or infer pointer types through arbitrary expressions. Absolute-address detection uses literal-only expressions or literals at least 0x100000 inside address expressions; bare integer address assignments without pointer/access context are outside this metric.", "",
           "Hardware literals include Saturn physical/P1 peripheral windows 0x05xxxxxx/0x25xxxxxx, SMPC 0x00100000..0x0010007f and its aliases, and SH-2 on-chip 0xfffffe00..0xffffffef. All-ones sentinels are excluded. Function-local named macro implementations are deliberately excluded; audit their definitions separately if measuring hidden implementation complexity.", "",
           "## Categories", "", "| Category | Occurrences | Share of noise | Rule |", "|---|---:|---:|---|"]
    for category, description in CATEGORIES.items():
        n = report["categories"][category]
        out.append(f"| `{category}` | {n:,} | {percent(n, s['total_noise']):.2f}% | {description} |")
    out += ["", "## Top 50 noisiest functions", "", "Ranked by category occurrences, then source path/name. Counts are unweighted; larger functions naturally rank higher.", "",
            "| Rank | Function | Source | Noise | Noisy/code lines | Largest categories |", "|---:|---|---|---:|---:|---|"]
    for rank, fn in enumerate(sorted(report["functions"], key=lambda f: (-f["total"], f["file"], f["name"]))[:50], 1):
        categories = ", ".join(f"{k}: {v}" for k, v in sorted(fn["categories"].items(), key=lambda kv: (-kv[1], kv[0]))[:3] if v)
        out.append(f"| {rank} | `{fn['name']}` | [{fn['file']}:{fn['start_line']}](../{fn['file']}#L{fn['start_line']}) | {fn['total']:,} | {fn['noise_lines']}/{fn['code_lines']} | {categories} |")
    out += ["", "## Repeated naming candidates", "",
            "Occurrences count source sites once per value and kind, not overlapping category events. Addresses include numeric memory/pointer/call sites, hardware literals, and address-named global/function uses. Zero/null pointer casts are clean. Repetition tables show the top 50 of each kind; JSON includes every candidate and example sites. Semantic symbol suggestions come from notes/symbols.txt, never automatic noise exemptions; address-only placeholder names are omitted. Multiple aliases can conflict: confirm them against their evidence. Runtime/cart-offset function aliases use coverage_callgraph.py's canonical mapping for symbol lookup only; address rows remain separate. Offset values are pooled across unrelated object layouts: validate the base type before naming a struct member; counts are opportunities, not guaranteed reductions.", ""]
    for kind in ("address", "offset"):
        out += [f"### Raw {kind} candidates", "", "| Value | Occurrences | Functions | Files | Existing symbol suggestions | Example |", "|---|---:|---:|---:|---|---|"]
        for item in report["repeated"][kind][:50]:
            example = item["examples"][0]
            out.append(f"| `{item['hex']}` | {item['occurrences']:,} | {item['functions']} | {item['files']} | {', '.join(item['symbols']) or '—'} | [{example['file']}:{example['line']}](../{example['file']}#L{example['line']}) |")
        out.append("")
    out += ["## Parse diagnostics", ""]
    if report["files"]["failures"]:
        out += [f"- `{f['file']}`: {f['error']}" for f in report["files"]["failures"]]
    else:
        out.append("All source files parsed successfully.")
    if report["files"]["without_functions"]:
        out += ["", "Files without function definitions: " + ", ".join(f"`{f}`" for f in report["files"]["without_functions"])]
    out += ["", "## Baseline", ""]
    if "baseline" in report:
        b = report["baseline"]
        out += [f"Baseline generated: {b['baseline_generated_at']}. Total noise **{b['before']['total_noise']:,} → {s['total_noise']:,} ({b['delta']['total_noise']:+,})**; matched-function delta **{b['matched_function_noise_delta']:+,}**. Added functions: {len(b['added_functions'])}; removed functions: {len(b['removed_functions'])}.", "",
                f"Zero-noise function share: {b['before']['zero_noise_percent']:.2f}% → {s['zero_noise_percent']:.2f}%. Noisy-line share: {b['before']['noise_line_percent']:.2f}% → {s['noise_line_percent']:.2f}%.", "",
                "| Category | Delta (current − baseline) |", "|---|---:|"]
        out += [f"| `{c}` | {b['categories'][c]:+,} |" for c in CATEGORIES]
    else:
        out.append("No comparison requested.")
    out += ["", "```sh", "tools/decomp/venv/bin/python tools/decomp/noise_report.py",
            "tools/decomp/venv/bin/python tools/decomp/noise_report.py --baseline save",
            "tools/decomp/venv/bin/python tools/decomp/noise_report.py --baseline compare", "```", "",
            "Default outputs: notes/noise_report.md and work/noise_report.json. Baseline: work/noise_report_baseline.json; use --baseline-file for another snapshot. Save intentionally replaces that snapshot. Compare requires matching metric version/scope and complete parses, reports additions/removals separately, and matches functions by file/name independent of line-number movement. JSON records all per-function categories, event locations, candidates, input SHA-256 hashes, parse diagnostics, and comparison deltas.", ""]
    return "\n".join(out)


def write_json(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2) + "\n")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", action="version", version=VERSION)
    parser.add_argument("--src", type=Path, default=ROOT / "project/src_functional")
    parser.add_argument("--symbols", type=Path, default=ROOT / "notes/symbols.txt")
    parser.add_argument("--markdown", type=Path, default=ROOT / "notes/noise_report.md")
    parser.add_argument("--json", type=Path, default=ROOT / "work/noise_report.json")
    parser.add_argument("--baseline", choices=("save", "compare"))
    parser.add_argument("--baseline-file", type=Path, default=ROOT / "work/noise_report_baseline.json")
    args = parser.parse_args(argv)
    if not args.src.is_dir() or not any(args.src.glob("*.c")):
        parser.error(f"no .c inputs in {args.src}")
    outputs = [args.markdown.resolve(), args.json.resolve()]
    if len(set(outputs)) != 2 or args.baseline_file.resolve() in outputs:
        parser.error("Markdown, JSON, and baseline paths must be distinct")
    report = build_report(args.src.resolve(), args.symbols.resolve())
    if args.baseline == "compare":
        try:
            report["baseline"] = compare_baseline(report, json.loads(args.baseline_file.read_text()))
        except (OSError, ValueError, KeyError, TypeError) as exc:
            parser.error(f"cannot compare baseline: {exc}")
    if args.baseline == "save":
        if report["files"]["failures"]:
            print("Cannot save an incomplete baseline; writing diagnostics only.", file=sys.stderr)
        else:
            write_json(args.baseline_file, report)
    args.markdown.parent.mkdir(parents=True, exist_ok=True)
    args.markdown.write_text(markdown(report))
    write_json(args.json, report)
    s = report["summary"]
    print(f"Noise v{VERSION}: {s['total_noise']:,} events / {s['functions']:,} functions; "
          f"{s['zero_noise_percent']:.2f}% clean; {s['noise_line_percent']:.2f}% noisy lines; "
          f"{len(report['files']['failures'])} parse failures")
    return 1 if report["files"]["failures"] else 0


if __name__ == "__main__":
    sys.exit(main())
