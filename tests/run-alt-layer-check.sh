#!/bin/sh
# Host-side compile + behaviour check for the Alt layer of the BB10 phone
# keyboards.  A normal C++ compiler is enough; the BB10 cross toolchain
# (qcc / /opt/bbndk) is NOT required, so this runs on any dev box.
#
#   usage: tests/run-alt-layer-check.sh
#
# It lifts the Alt layer helpers verbatim out of TerminalDisplay.cpp
# (extract_alt_layer_block.py) and checks the table the pinyin IME depends
# on: the number row of these keyboards lives on the letter keys (Alt+W is 1,
# Alt+E is 2, ... Alt+C is 9), and those chords are how a candidate is picked
# without touching the screen.
set -e

here=$(cd "$(dirname "$0")" && pwd)
src="$here/../TerminalDisplay.cpp"
tmp=${TMPDIR:-/tmp}/bgalt.$$
cxx=${CXX:-g++}

[ -f "$src" ] || { echo "missing: $src" >&2; exit 1; }
mkdir -p "$tmp"
trap 'rm -rf "$tmp"' EXIT INT TERM

python3 "$here/extract_alt_layer_block.py" "$src" "$tmp"

echo "== compiling with $cxx =="
$cxx -std=c++11 -Wall -Wextra -I"$tmp" \
     -o "$tmp/hostcheck" "$here/alt_layer_hostcheck.cpp"

echo "== replaying Alt-layer decisions =="
"$tmp/hostcheck"
