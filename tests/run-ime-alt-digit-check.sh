#!/bin/sh
# Host-side compile + behaviour check for the pinyin IME of the terminal.
# A normal C++ compiler and Qt5 are enough; the BB10 cross toolchain
# (qcc / /opt/bbndk) is NOT required, so this runs on any dev box.
#
#   usage: tests/run-ime-alt-digit-check.sh
#
# It lifts the IME key handling verbatim out of TerminalDisplay.cpp
# (extract_ime_key_block.py), links it against the real pinyin engine
# (cjk_ime.c) and replays the chords of the Q30 keyboard: the digits live on
# the Alt layer (Alt+W is 1, Alt+E is 2, Alt+R is 3, ...), and while a
# composition runs they have to pick a candidate instead of falling through
# to the shell as a stray digit.
set -e

here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/.." && pwd)
tmp=${TMPDIR:-/tmp}/bgimedigit.$$
cxx=${CXX:-g++}
cc=${CC:-gcc}

[ -f "$root/TerminalDisplay.cpp" ] || { echo "missing: $root/TerminalDisplay.cpp" >&2; exit 1; }
mkdir -p "$tmp"
trap 'rm -rf "$tmp"' EXIT INT TERM

python3 "$here/extract_ime_key_block.py" "$root/TerminalDisplay.cpp" "$tmp"

qt_cflags=$(pkg-config --cflags Qt5Gui Qt5Core 2>/dev/null || true)
qt_libs=$(pkg-config --libs Qt5Gui Qt5Core 2>/dev/null || echo "-lQt5Gui -lQt5Core")

echo "== compiling the pinyin engine =="
$cc -O1 -c "$root/cjk_ime.c" -o "$tmp/cjk_ime.o"

echo "== compiling with $cxx =="
$cxx -std=c++11 -Wall -Wextra -fPIC $qt_cflags -I"$root" -I"$tmp" \
     -o "$tmp/hostcheck" "$here/ime_alt_digit_hostcheck.cpp" "$tmp/cjk_ime.o" \
     $qt_libs

echo "== replaying IME key sequences =="
"$tmp/hostcheck"
