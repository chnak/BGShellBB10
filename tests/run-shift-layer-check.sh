#!/bin/sh
# Host-side compile + behaviour check for the Shift / key layer in
# TerminalDisplay.cpp.  A normal C++ compiler is enough: the BB10 cross
# toolchain (qcc / /opt/bbndk) is NOT required, so this runs on any dev box.
#
#   usage: tests/run-shift-layer-check.sh
#
# It lifts the key-handling code verbatim out of TerminalDisplay.cpp
# (extract_shift_block.py), compiles it against minimal Qt stand-ins and
# replays the key sequences that must keep working on the phone:
# capitals with the left Shift, Ctrl via the right Shift, double taps.
set -e

here=$(cd "$(dirname "$0")" && pwd)
src="$here/../TerminalDisplay.cpp"
tmp=${TMPDIR:-/tmp}/bgshift.$$
cxx=${CXX:-g++}

[ -f "$src" ] || { echo "missing: $src" >&2; exit 1; }
mkdir -p "$tmp"
trap 'rm -rf "$tmp"' EXIT INT TERM

python3 "$here/extract_shift_block.py" "$src" "$tmp"

echo "== compiling with $cxx =="
$cxx -std=c++11 -Wall -Wextra -I"$here" -I"$tmp" \
     -o "$tmp/hostcheck" "$here/shift_layer_hostcheck.cpp"

echo "== replaying key sequences =="
"$tmp/hostcheck"
