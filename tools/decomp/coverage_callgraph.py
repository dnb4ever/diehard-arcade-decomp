#!/usr/bin/env python3
"""True UNIQUE coverage: canonicalize every referenced func address to sh2 form
(cart-offset +0x02000000; runtime/HWRAM -0x03E04000) before comparing to defined.
Also report the non-canonical call-sites (the naming-normalization cleanup targets)."""
import pathlib, re, collections
SRC = pathlib.Path("project/src_functional")
pat = re.compile(r"func_([0-9a-fA-F]{8})\b")

def canon(a):
    if 0x00200000 <= a <= 0x0029ffff:      # cart-offset form -> sh2
        return a + 0x02000000
    if 0x06000000 <= a <= 0x06100000:      # runtime/HWRAM form -> sh2
        return a - 0x03E04000
    return a

defined = set()
for p in SRC.glob("*.c"):
    try:
        defined.add(int(p.stem, 16))
    except ValueError:
        pass

referenced_raw = set()
noncanon = collections.defaultdict(list)   # canon_addr -> [(file, raw_form)]
for p in SRC.glob("*.c"):
    for m in set(pat.findall(p.read_text(errors="replace"))):
        a = int(m, 16)
        referenced_raw.add(a)
        c = canon(a)
        if c != a:
            noncanon[c].append((p.name, m))

referenced = {canon(a) for a in referenced_raw}
universe = defined | referenced
undone = sorted(referenced - defined)

print("defined (files):                        %d" % len(defined))
print("distinct referenced (canonicalized):    %d" % len(referenced))
print("universe:                               %d" % len(universe))
print("TRUE unique referenced-but-undefined:   %d" % len(undone))
print("TRUE CALL-GRAPH COVERAGE:               %.1f%%" % (100.0 * len(defined) / len(universe)))
print()
print("non-canonical call-site references (naming cleanup targets): %d distinct" % len(noncanon))
for c in sorted(noncanon):
    tag = "ALREADY DEFINED" if c in defined else "undone"
    forms = ", ".join("%s uses func_%s" % (f, r) for f, r in noncanon[c][:3])
    print("  canon %08x [%s]: %s" % (c, tag, forms))
print()
print("--- the %d TRUE unique undone (sorted) ---" % len(undone))
print(" ".join("%08x" % a for a in undone))


# These diagnostics deliberately do not alter the historical metrics above.
# Ignore comments and strings for definitions, but scan all tokens for warnings
# so malformed references in documentation are visible too.
def func_definitions(source):
    code = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                  lambda m: '\n' * m[0].count('\n'), source, flags=re.S)
    tokens = re.findall(r'[A-Za-z_][A-Za-z_0-9]*|[^\s]', code)
    depth = parens = 0
    names = []
    for i, token in enumerate(tokens):
        if depth == 0 and parens == 0 and token.startswith('func_') and tokens[i+1:i+2] == ['(']:
            level, j = 1, i+2
            while j < len(tokens) and level:
                level += (tokens[j] == '(') - (tokens[j] == ')')
                j += 1
            if level == 0 and tokens[j:j+1] == ['{']:
                names.append(token)
        if token == '{': depth += 1
        elif token == '}': depth -= 1
        elif token == '(': parens += 1
        elif token == ')': parens -= 1
    return names


malformed = []
multiple = []
for p in sorted(SRC.glob('*.c')):
    source = p.read_text(errors='replace')
    for match in re.finditer(r'\bfunc_[A-Za-z_0-9]*\b', source):
        if not re.fullmatch(r'func_[0-9a-fA-F]{8}', match[0]):
            malformed.append((p.name, source.count('\n', 0, match.start())+1, match[0]))
    names = func_definitions(source)
    if len(names) > 1:
        multiple.append((p.name, names))

print()
print('WARNING: func_ tokens without exactly 8 hex digits: %d occurrences' % len(malformed))
for filename, line, token in malformed:
    print('  %s:%d: %s' % (filename, line, token))
print()
print('files defining more than one func_: %d' % len(multiple))
for filename, names in multiple:
    print('  %s: %s' % (filename, ', '.join(names)))
