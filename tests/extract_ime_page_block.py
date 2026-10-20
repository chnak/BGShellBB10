#!/usr/bin/env python3
"""Extract imePageKeyHasNoPage() out of TerminalDisplay.cpp verbatim.

The BB10 build needs qcc / the NDK, which a normal dev box does not have.  To
still be able to test the page-key decision on a PC, the exact source lines are
lifted into an .inc file that tests/ime_page_hostcheck.cpp #includes - no
retyping, so the test can never drift away from the shipped code.

    usage: extract_ime_page_block.py <TerminalDisplay.cpp> <out-dir>

Writes <out-dir>/pagekey.inc
"""
import io
import os
import sys

NAME = 'static bool imePageKeyHasNoPage'


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__.strip())
    src_path, out_dir = sys.argv[1], sys.argv[2]
    src = io.open(src_path, encoding='utf-8', errors='replace').read()
    src = src.replace('\r\n', '\n')
    lines = src.split('\n')

    start = None
    for i, line in enumerate(lines):
        if line.startswith(NAME):
            start = i
            break
    if start is None:
        sys.exit('extract_ime_page_block: not found in %s: %r' % (src_path, NAME))

    depth = 0
    end = start
    for i in range(start, len(lines)):
        depth += lines[i].count('{') - lines[i].count('}')
        if depth == 0 and lines[i] == '}':
            end = i
            break
    else:
        sys.exit('extract_ime_page_block: unbalanced braces after line %d' % (start + 1))

    text = '\n'.join(lines[start:end + 1]) + '\n'
    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)
    io.open(os.path.join(out_dir, 'pagekey.inc'), 'w', encoding='utf-8').write(text)
    sys.stdout.write('extracted pagekey.inc: %d lines (cpp %d..%d)\n'
                     % (end - start + 1, start + 1, end + 1))


if __name__ == '__main__':
    main()
