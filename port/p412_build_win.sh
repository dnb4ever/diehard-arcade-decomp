#!/usr/bin/env bash
# P4.13: no-root Linux -> Windows x64/UCRT, using current shared sources read-only.
set -euo pipefail
cd "$(dirname "$0")/.."
ROOT=$PWD
WIN=$ROOT/work/win
export P412_REPO_ROOT=$ROOT
# Downloads are pinned and verified against GitHub's published release digests.
python3 - <<'PY'
import hashlib, json, os, tarfile, urllib.request
from pathlib import Path
root = Path(os.environ['P412_REPO_ROOT']) / 'work/win/opt'
root.mkdir(parents=True, exist_ok=True)
assets = [
 ('https://github.com/mstorsjo/llvm-mingw/releases/download/20261104/',
  'llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64.tar.xz',
  '919380403de87dca3c581c2053230cd298af5e891286628ba0613b7fed370926',
  'llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64'),
 ('https://github.com/libsdl-org/SDL/releases/download/release-3.4.20/',
  'SDL3-devel-3.4.20-mingw.tar.gz',
  'd2d3bf3fb97a90b22e908a306d7362631ef1bab229d988fe6669595dc29cd9ed',
  'SDL3-3.4.20'),
]
for base, name, digest, directory in assets:
    archive = root / name
    if not archive.exists():
        pending = archive.with_suffix(archive.suffix + '.part')
        urllib.request.urlretrieve(base + name, pending)
        pending.replace(archive)
    actual = hashlib.sha256(archive.read_bytes()).hexdigest()
    if actual != digest:
        raise SystemExit(f'SHA256 mismatch: {archive}: {actual}, expected {digest}')
    print(f'Verified {name}: {actual}', flush=True)
    if not (root / directory).exists():
        with tarfile.open(archive) as tar:
            tar.extractall(root, filter='data')
(root / 'build-downloads.json').write_text(json.dumps([
    dict(url=b+n, name=n, sha256=h) for b,n,h,_ in assets], indent=2)+'\n')
PY
# P4.13: refresh the isolated sources when runtime/harness/portability inputs change.
# Historical P4.12 baseline and its snapshot.json remain intact for audit.
python3 port/p413_stage_win.py
PATCH=$ROOT/port/runtime-patches/p412-windows-portability.patch
if patch --batch --force --dry-run -R -p1 -d "$WIN/saturn-recomp" < "$PATCH" >/dev/null 2>&1; then
    echo 'P4.12 patch already applied to the snapshot.'
else
    patch --batch --forward -p1 -d "$WIN/saturn-recomp" < "$PATCH"
fi
# Run the idempotent continuation pass only on our copied generated C++.
python3 "$WIN/saturn-recomp/port/p410_continuations.py" "$WIN/recomp"
python3 "$WIN/saturn-recomp/port/p415_idle_loops.py" "$WIN/recomp"
TOOLCHAIN=$WIN/opt/llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64
SDL=$WIN/opt/SDL3-3.4.20/x86_64-w64-mingw32
cmake -S "$WIN/recomp" -B "$WIN/build-windows" -G 'Unix Makefiles' \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$ROOT/port/p412_mingw_toolchain.cmake" \
    -DLLVM_MINGW_ROOT="$TOOLCHAIN" -DSATURN_RUNTIME="$WIN/saturn-recomp/runtime" \
    -DSATURN_EXTRA="$WIN/saturn-recomp/port/stv.cmake" \
    -DSDL3_DIR="$SDL/lib/cmake/SDL3" -DSATURN_NO_SDL=OFF -DSATURN_REQUIRE_SDL=ON
cmake --build "$WIN/build-windows" --target saturn_stv -j "${JOBS:-6}"
python3 port/p412_package_win.py --toolchain "$TOOLCHAIN" --sdl "$SDL" --zip
du -sh "$WIN"
