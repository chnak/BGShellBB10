#!/usr/bin/env python3
"""Extract the Shift / key-layer code out of TerminalDisplay.cpp verbatim.

The BB10 build needs the QNX/qcc toolchain, which a normal dev box does not
have.  To still be able to compile and exercise the key layer there, we lift
the exact source lines (no retyping, no copies drifting apart) into .inc files
that tests/shift_layer_hostcheck.cpp #includes.

    usage: extract_shift_block.py <TerminalDisplay.cpp> <out-dir>

Writes <out-dir>/block.inc     - the Shift block + double-tap cancel + Ctrl branch
       <out-dir>/helper.inc    - td_isRightShift()
       <out-dir>/release.inc   - the body of keyReleaseEvent()
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
            if lines[i].strip().startswith(prefix):
                return i
        sys.exit('extract_shift_block: not found in %s: %r' % (src_path, prefix))

    # 1. the shift block: from its comment header to the pinyin section
    a = find('// --- Shift: the left one stays a plain Shift')
    b = find('// --- pinyin IME', a)
    block = '\n'.join(lines[a:b]).rstrip() + '\n'

    # 2. td_isRightShift()
    c = find('static bool td_isRightShift')
    e = find('return false;', c)
    while lines[e] != '}':
        e += 1
    helper = '\n'.join(lines[c:e + 1]) + '\n'

    # 3. keyReleaseEvent() body
    f = find('void TerminalDisplay::keyReleaseEvent')
    g = f
    while lines[g] != '}':
        g += 1
    release = '\n'.join(lines[f + 1:g + 1]) + '\n'

    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)
    for name, text in (('block.inc', block), ('helper.inc', helper),
                       ('release.inc', release)):
        io.open(os.path.join(out_dir, name), 'w', encoding='utf-8').write(text)
    sys.stdout.write('extracted block %d lines, helper %d, release %d (cpp %d/%d/%d)\n'
                     % (b - a, e - c + 1, g - f, a + 1, c + 1, f + 2))


if __name__ == '__main__':
    main()
