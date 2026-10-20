#!/bin/sh
# Host-side compile + behaviour check for the pinyin page keys in
# TerminalDisplay.cpp.  A normal C++ compiler is enough; the BB10 cross
# toolchain (qcc / /opt/bbndk) is NOT required.
#
#   usage: tests/run-ime-page-check.sh
#
# It lifts imePageKeyHasNoPage() verbatim out of TerminalDisplay.cpp
# (extract_ime_page_block.py) and checks the rule that keeps "hello, world"
# intact: `,` `.` `-` `[` `]` `=` only belong to the engine while the strip
# really has a second page.
set -e

here=$(cd "$(dirname "$0")" && pwd)
src="$here/../TerminalDisplay.cpp"
tmp=${TMPDIR:-/tmp}/bgpage.$$
cxx=${CXX:-g++}

[ -f "$src" ] || { echo "missing: $src" >&2; exit 1; }
mkdir -p "$tmp"
trap 'rm -rf "$tmp"' EXIT INT TERM

python3 "$here/extract_ime_page_block.py" "$src" "$tmp"

echo "== compiling with $cxx =="
$cxx -std=c++11 -Wall -Wextra -I"$tmp" \
     -o "$tmp/hostcheck" "$here/ime_page_hostcheck.cpp"

echo "== replaying page-key decisions =="
"$tmp/hostcheck"
