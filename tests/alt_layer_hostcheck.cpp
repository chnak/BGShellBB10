// Host-side check for the Alt layer of the BB10 phone keyboards.
//
// On these keyboards the number row lives on the letter keys: Alt+W is 1,
// Alt+E is 2, Alt+R is 3, ... Alt+C is 9.  The pinyin IME needs those digits
// to pick a candidate, so the table in TerminalDisplay.cpp (altLayerChar(),
// altLayerDigit(), imeAltLayerDigit()) is lifted out verbatim by
// extract_alt_layer_block.py and #included here - the code under test is the
// shipped code, no BB10 toolchain needed.
//
//   tests/run-alt-layer-check.sh
#include <cstdio>

// The key codes the extracted code needs.  Same values as Qt's (Key_A is
// 0x41, Key_Dollar is 0x24, the digits are their ASCII codes).
namespace Qt {
enum Key {
    Key_0      = 0x30,
    Key_Dollar = 0x24,
    Key_A = 0x41, Key_B = 0x42, Key_C = 0x43, Key_D = 0x44, Key_E = 0x45,
    Key_F = 0x46, Key_G = 0x47, Key_H = 0x48, Key_I = 0x49, Key_J = 0x4a,
    Key_K = 0x4b, Key_L = 0x4c, Key_M = 0x4d, Key_N = 0x4e, Key_O = 0x4f,
    Key_P = 0x50, Key_Q = 0x51, Key_R = 0x52, Key_S = 0x53, Key_T = 0x54,
    Key_U = 0x55, Key_V = 0x56, Key_W = 0x57, Key_X = 0x58, Key_Y = 0x59,
    Key_Z = 0x5a
};
}

#include "altlayer.inc"

namespace {

int failures = 0;

void expectChar(const char *what, char got, char want)
{
    if (got == want) {
        std::printf("PASS  %-46s -> '%c'\n", what, got ? got : '?');
    } else {
        std::printf("FAIL  %-46s -> '%c' (wanted '%c')\n", what,
                    got ? got : '?', want ? want : '?');
        ++failures;
    }
}

void expectDigit(const char *what, char got, char want)
{
    if (got == want) {
        std::printf("PASS  %-46s -> '%c'\n", what, got ? got : '?');
    } else {
        std::printf("FAIL  %-46s -> '%c' (wanted '%c')\n", what,
                    got ? got : '?', want ? want : '?');
        ++failures;
    }
}

} // namespace

int main()
{
    // The number row of the Alt layer: Alt+W .. Alt+C are 1 .. 9.
    expectDigit("Alt+W is 1", altLayerDigit(Qt::Key_W), '1');
    expectDigit("Alt+E is 2", altLayerDigit(Qt::Key_E), '2');
    expectDigit("Alt+R is 3", altLayerDigit(Qt::Key_R), '3');
    expectDigit("Alt+S is 4", altLayerDigit(Qt::Key_S), '4');
    expectDigit("Alt+D is 5", altLayerDigit(Qt::Key_D), '5');
    expectDigit("Alt+F is 6", altLayerDigit(Qt::Key_F), '6');
    expectDigit("Alt+Z is 7", altLayerDigit(Qt::Key_Z), '7');
    expectDigit("Alt+X is 8", altLayerDigit(Qt::Key_X), '8');
    expectDigit("Alt+C is 9", altLayerDigit(Qt::Key_C), '9');

    // Everything else on the layer is punctuation, never a candidate key.
    expectDigit("Alt+Q carries no digit", altLayerDigit(Qt::Key_Q), 0);
    expectDigit("Alt+B carries no digit", altLayerDigit(Qt::Key_B), 0);
    expectDigit("Alt+0 carries no digit", altLayerDigit(0x30), 0);
    expectDigit("a plain letter carries no digit",
                altLayerDigit(Qt::Key_A), 0);

    // The layer still delivers its punctuation to the shell.
    expectChar("Alt+Q is #", altLayerChar(Qt::Key_Q), '#');
    expectChar("Alt+V is ?", altLayerChar(Qt::Key_V), '?');
    expectChar("Alt+B is !", altLayerChar(Qt::Key_B), '!');
    expectChar("Alt+N is ,", altLayerChar(Qt::Key_N), ',');
    expectChar("Alt+0 is 0", altLayerChar(0x30), '0');

    // The IME asks the same table, with the key text as the first guess:
    // some layouts hand the digit over as the text, some only as the key.
    expectDigit("Alt+W with the text 1", imeAltLayerDigit(Qt::Key_W, '1'), '1');
    expectDigit("Alt+W without a text", imeAltLayerDigit(Qt::Key_W, 0), '1');
    expectDigit("Alt+E with the text 2", imeAltLayerDigit(Qt::Key_E, '2'), '2');
    expectDigit("Alt+B with the text !", imeAltLayerDigit(Qt::Key_B, '!'), 0);
    expectDigit("Alt+Q with the text #", imeAltLayerDigit(Qt::Key_Q, '#'), 0);

    if (failures) {
        std::printf("%d FAILED\n", failures);
        return 1;
    }
    std::printf("all Alt-layer cases OK\n");
    return 0;
}
