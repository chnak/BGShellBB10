#!/usr/bin/env python3
"""Extract the Alt layer helpers out of TerminalDisplay.cpp verbatim.

The BB10 build needs qcc / the NDK, which a normal dev box does not have.  To
still be able to test the Alt layer on a PC, the exact source lines are
lifted into an .inc file that tests/alt_layer_hostcheck.cpp #includes - no
retyping, so the test can never drift away from the shipped code.

    usage: extract_alt_layer_block.py <TerminalDisplay.cpp> <out-dir>

Writes <out-dir>/altlayer.inc with altLayerChar(), altLayerDigit() and
imeAltLayerDigit().
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
        sys.exit('extract_alt_layer_block: not found in %s: %r'
                 % (src_path, prefix))

    start = find('static char altLayerChar')
    last = find('static char imeAltLayerDigit', start)

    depth = 0
    end = last
    for i in range(last, len(lines)):
        depth += lines[i].count('{') - lines[i].count('}')
        if depth == 0 and lines[i] == '}':
            end = i
            break
    else:
        sys.exit('extract_alt_layer_block: unbalanced braces after line %d'
                 % (last + 1))

    text = '\n'.join(lines[start:end + 1]) + '\n'
    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)
    io.open(os.path.join(out_dir, 'altlayer.inc'), 'w',
            encoding='utf-8').write(text)
    sys.stdout.write('extracted altlayer.inc: %d lines (cpp %d..%d)\n'
                     % (end - start + 1, start + 1, end + 1))


if __name__ == '__main__':
    main()
