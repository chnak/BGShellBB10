// Host-side check for the pinyin page keys (no BB10 toolchain needed).
//
// TerminalDisplay.cpp keeps `,` `.` `-` `[` `]` `=` for the engine only when a
// composition has a second candidate page; with one page they are ordinary
// punctuation, so an English sentence keeps its commas.  The decision lives in
// imePageKeyHasNoPage(), which extract_ime_page_block.py lifts out of the real
// source and this file #includes - the code under test is the shipped code.
//
//   tests/run-ime-page-check.sh
#include <cstdio>
#include <cstring>

#include "pagekey.inc"

namespace {

int failures = 0;

void expect(const char* what, int sym, bool composing, int pages, bool want)
{
    const bool got = imePageKeyHasNoPage(sym, composing, pages);
    if (got == want) {
        std::printf("PASS  %-42s -> %s\n", what, got ? "shell" : "engine");
    } else {
        std::printf("FAIL  %-42s -> %s (wanted %s)\n", what,
                    got ? "shell" : "engine", want ? "shell" : "engine");
        ++failures;
    }
}

} // namespace

int main()
{
    // one page: nothing to turn, so every page key becomes punctuation again
    expect("comma, one page",            ',', true, 1, true);
    expect("period, one page",           '.', true, 1, true);
    expect("hyphen, one page",           '-', true, 1, true);
    expect("open bracket, one page",     '[', true, 1, true);
    expect("close bracket, one page",    ']', true, 1, true);
    expect("equals, one page",           '=', true, 1, true);
    expect("comma, zero pages (defensive)", ',', true, 0, true);

    // a real second page: the engine keeps the key and turns the strip
    expect("comma, two pages",           ',', true, 2, false);
    expect("period, three pages",        '.', true, 3, false);
    expect("open bracket, nine pages",   '[', true, 9, false);
    expect("equals, two pages",          '=', true, 2, false);

    // no composition: the engine ignores these keys anyway, keep them out
    expect("comma, not composing",       ',', false, 1, false);

    // keys that are not page keys never take this path
    expect("letter a",                   'a', true, 1, false);
    expect("digit 1",                    '1', true, 1, false);
    expect("space",                      ' ', true, 1, false);
    expect("Esc",                       0x1b, true, 1, false);

    if (failures) {
        std::printf("%d FAILED\n", failures);
        return 1;
    }
    std::printf("all page-key cases OK\n");
    return 0;
}
