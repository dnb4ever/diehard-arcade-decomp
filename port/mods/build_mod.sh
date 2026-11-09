#!/usr/bin/env bash
# Usage: build_mod.sh SOURCE_FOLDER OUTPUT_FOLDER TARGET_FINGERPRINT_FILE
# A shell with cc/clang/gcc (MSYS2 MinGW on Windows); no runtime imports required.
set -euo pipefail
src=$1
out=$2
target_file=$3
sdk=$(cd "$(dirname "$0")/sdk" && pwd)
mkdir -p "$out"
case "$(uname -s)" in
  Darwin) library=mod.dylib; flags=(-dynamiclib -fPIC) ;;
  MINGW*|MSYS*|CYGWIN*) library=mod.dll; flags=(-shared) ;;
  *) library=mod.so; flags=(-shared -fPIC) ;;
esac
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -fvisibility=hidden "${flags[@]}" -I"$sdk" "$src/mod.c" -o "$out/$library"
# Fingerprint input is machine-generated hex, not shell code.
target=$(tr -d '\r\n' < "$target_file")
[[ "$target" =~ ^[0-9a-f]{64}$ ]] || { echo 'Expected exact 64-digit target build fingerprint' >&2; exit 2; }
sed -e "s/@TARGET@/$target/g" -e "s/@LIBRARY@/$library/g" "$src/manifest.in" > "$out/manifest.ini"
printf 'Built %s\n' "$out/$library"
