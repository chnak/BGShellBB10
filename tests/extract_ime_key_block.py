#!/usr/bin/env python3
"""Extract the pinyin IME key handling out of TerminalDisplay.cpp verbatim.

The BB10 build needs qcc / the NDK, which a normal dev box does not have.  To
still be able to run the IME on a PC, the exact source lines are lifted into
.inc files that tests/ime_alt_digit_hostcheck.cpp #includes - no retyping, so
the test can never drift away from the shipped code.

    usage: extract_ime_key_block.py <TerminalDisplay.cpp> <out-dir>

Writes <out-dir>/imehelpers.inc - imeSymForEvent(), the Alt layer helpers
         and imePageKeyHasNoPage()
       <out-dir>/imekey.inc      - the body of TerminalDisplay::handleImeKey()
"""
import io
import os
import sys


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__.strip())
    src_path, out_dir = sys.argv[1], sys.argv[2]
    src = io.open(src_path, encoding='utf-8', errors='replace').read()
    src = src.replace('\r\n', '\n')
    lines = src.split('\n')

    def find(prefix, start=0):
        for i in range(start, len(lines)):
            if lines[i].startswith(prefix):
                return i
        sys.exit('extract_ime_key_block: not found in %s: %r'
                 % (src_path, prefix))

    def block_end(first):
        """Line index of the closing brace of the function starting at first."""
        depth = 0
        for i in range(first, len(lines)):
            depth += lines[i].count('{') - lines[i].count('}')
            if depth == 0 and lines[i] == '}':
                return i
        sys.exit('extract_ime_key_block: unbalanced braces after line %d'
                 % (first + 1))

    # 1. the helpers: imeSymForEvent() .. imePageKeyHasNoPage()
    h_start = find('static int imeSymForEvent')
    p_start = find('static bool imePageKeyHasNoPage', h_start)
    h_end = block_end(p_start)
    helpers = '\n'.join(lines[h_start:h_end + 1]) + '\n'

    # the repeat guard the key handling uses, taken from the widget as well
    guard = find('#define IME_COMMIT_GUARD_MS')
    helpers = lines[guard] + '\n\n' + helpers

    # 2. the body of handleImeKey()
    k_start = find('bool TerminalDisplay::handleImeKey')
    brace = k_start
    while '{' not in lines[brace]:
        brace += 1
    k_end = block_end(brace)
    body = '\n'.join(lines[brace + 1:k_end]) + '\n'

    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)
    io.open(os.path.join(out_dir, 'imehelpers.inc'), 'w',
            encoding='utf-8').write(helpers)
    io.open(os.path.join(out_dir, 'imekey.inc'), 'w',
            encoding='utf-8').write(body)
    sys.stdout.write('extracted imehelpers.inc: %d lines (cpp %d..%d), '
                     'imekey.inc: %d lines (cpp %d..%d)\n'
                     % (h_end - h_start + 1, h_start + 1, h_end + 1,
                        k_end - brace - 1, brace + 2, k_end + 1))


if __name__ == '__main__':
    main()
