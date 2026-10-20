// Compiles and runs the REAL shift-handling code out of ../TerminalDisplay.cpp
// (block.inc / helper.inc / release.inc are extracted verbatim by
// extract_shift_block.py) against minimal Qt stand-ins.
//
// Purpose: the BB10 cross toolchain (qcc, /opt/bbndk) is usually not installed
// on a dev box, so this is how the key layer stays checkable there - the
// extracted text is byte-identical to the widget source, nothing is retyped.
//
//   run: tests/run-shift-layer-check.sh

#include "qtstub.h"

qint64 QDateTime::s_now = 1000000;

// QNX keycodes (values only need to be distinct and stable here).
#define KEYCODE_LEFT_SHIFT  0xf0e1
#define KEYCODE_RIGHT_SHIFT 0xf0e2
#ifndef __QNXNTO__
#define __QNXNTO__          1     // exercise the real (keycode) branch
#endif

#include "helper.inc"             // the real td_isRightShift()

#define META_DOUBLE_TAP_MS 500

class TerminalDisplay : public QWidget
{
public:
    TerminalDisplay()
        : _metaArmed(false), _ctrlHeld(false), _lastShiftNative(0),
          _lastShiftScan(0), _lastShiftWasRight(false), _metaLastShiftMs(0),
          _metaLastRightMs(0), outLen(0), armCount(0), ctrlBytes(0), ctrlByte(0)
    { out[0] = 0; }

    // ---- the code under test -------------------------------------------
    void keyPressEvent(QKeyEvent *event)
    {
#include "block.inc"
        // Everything below is the untouched tail of the real keyPressEvent
        // (IME, SYM grid, BB10 shift workaround, emulation): out of scope.
        if (event->key() < 0x01000000) {
            char b[2] = { (char)event->key(), 0 };
            put(b);
        }
    }

    void keyReleaseEvent(QKeyEvent *event)
    {
#include "release.inc"
    }
    // --------------------------------------------------------------------

    void setMetaLayer(bool on)
    {
        _metaArmed = on;
        ++armCount;
    }

    void handleMetaKey(QKeyEvent *event)
    {
        put(event->key() == Qt::Key_H || event->key() == Qt::Key_J ||
            event->key() == Qt::Key_K || event->key() == Qt::Key_L ? "ARROW" : "ACTION");
    }

    void td_send_ctrl_key(QKeyEvent *event)
    {
        ctrlByte = (char)(event->key() & 0x1f);
        ++ctrlBytes;
        char b[16];
        std::snprintf(b, sizeof b, "^%c", (char)event->key());
        put(b);
    }

    void put(const char *s)
    {
        int n = (int)std::strlen(s);
        if (outLen + n < (int)sizeof out) {
            std::memcpy(out + outLen, s, n);
            outLen += n;
            out[outLen] = 0;
        }
    }

    bool _metaArmed, _ctrlHeld;
    int  _lastShiftNative, _lastShiftScan;
    bool _lastShiftWasRight;
    qint64 _metaLastShiftMs, _metaLastRightMs;
    char out[512];
    int  outLen, armCount, ctrlBytes;
    char ctrlByte;
};

// --------------------------------------------------------------------------
static int failures = 0;

static void feed(TerminalDisplay &t, qint64 tms, int key, int kc, bool release = false)
{
    QDateTime::s_now = tms;
    QKeyEvent ev(key, kc, kc);
    if (release)
        t.keyReleaseEvent(&ev);
    else
        t.keyPressEvent(&ev);
}

static void check(const char *title, const TerminalDisplay &t,
                  bool wantArmed, const char *wantOut, int wantCtrlBytes = 0)
{
    bool ok = (t._metaArmed == wantArmed) &&
              (std::strcmp(t.out, wantOut) == 0) &&
              (t.ctrlBytes == wantCtrlBytes);
    if (!ok)
        ++failures;
    std::printf("%-4s %-46s armed=%-5s out=%-9s ctrl=%d\n",
                ok ? "PASS" : "FAIL", title, t._metaArmed ? "true" : "false",
                t.out, t.ctrlBytes);
}

int main()
{
    const int T0 = 1000000;
    TerminalDisplay t;

    // 1. typing capitals with the left shift: never arms, never Ctrl
    feed(t, T0,        Qt::Key_Shift, KEYCODE_LEFT_SHIFT);
    feed(t, T0 + 80,   Qt::Key_A,     KEYCODE_LEFT_SHIFT);
    feed(t, T0 + 400,  Qt::Key_Shift, KEYCODE_LEFT_SHIFT);
    feed(t, T0 + 480,  Qt::Key_B,     KEYCODE_LEFT_SHIFT);
    check("left shift,a,left shift,b (capitals)", t, false, "AB");

    // 2. double-tapping the LEFT shift must not arm the layer
    TerminalDisplay t2;
    feed(t2, T0,       Qt::Key_Shift, KEYCODE_LEFT_SHIFT);
    feed(t2, T0 + 150, Qt::Key_Shift, KEYCODE_LEFT_SHIFT);
    check("double-tap LEFT shift -> no layer", t2, false, "");

    // 3. double-tapping the RIGHT shift arms it
    TerminalDisplay t3;
    feed(t3, T0,       Qt::Key_Shift, KEYCODE_RIGHT_SHIFT);
    feed(t3, T0 + 150, Qt::Key_Shift, KEYCODE_RIGHT_SHIFT);
    feed(t3, T0 + 300, Qt::Key_K,     KEYCODE_RIGHT_SHIFT);
    check("double-tap RIGHT shift -> layer armed", t3, true, "ARROW");

    // 4. holding the right shift is Ctrl, and it is released with the key
    TerminalDisplay t4;
    feed(t4, T0,       Qt::Key_Shift, KEYCODE_RIGHT_SHIFT);
    feed(t4, T0 + 100, 'c',           KEYCODE_RIGHT_SHIFT);
    feed(t4, T0 + 200, Qt::Key_Shift, KEYCODE_RIGHT_SHIFT, true);
    feed(t4, T0 + 260, 'c',           KEYCODE_RIGHT_SHIFT);
    bool ok4 = (!t4._metaArmed && std::strcmp(t4.out, "^cc") == 0 &&
                t4.ctrlBytes == 1 && t4.ctrlByte == 0x03);
    std::printf("%-4s %-46s armed=%-5s out=%-9s ctrlByte=0x%02x\n",
                ok4 ? "PASS" : "FAIL",
                "hold RIGHT shift + c, release, + c", t4._metaArmed ? "true" : "false",
                t4.out, (unsigned char)t4.ctrlByte);
    if (!ok4)
        ++failures;

    // 5. no keycode reported at all: double tap of either shift still works
    TerminalDisplay t5;
    feed(t5, T0,       Qt::Key_Shift, 0);
    feed(t5, T0 + 150, Qt::Key_Shift, 0);
    check("no keycode: double-tap shift -> layer", t5, true, "");

    // 6. ... but a letter in between still cancels it
    TerminalDisplay t6;
    feed(t6, T0,       Qt::Key_Shift, 0);
    feed(t6, T0 + 80,  Qt::Key_A,     0);
    feed(t6, T0 + 300, Qt::Key_Shift, 0);
    feed(t6, T0 + 380, Qt::Key_B,     0);
    check("no keycode: shift,a,shift,b -> plain", t6, false, "AB");

    // 7. a slow second tap is not a double tap
    TerminalDisplay t7;
    feed(t7, T0,       Qt::Key_Shift, KEYCODE_RIGHT_SHIFT);
    feed(t7, T0 + 600, Qt::Key_Shift, KEYCODE_RIGHT_SHIFT);
    check("slow double-tap (600 ms) -> no layer", t7, false, "");

    // 8. right arrow: single tap falls through, double tap arms
    TerminalDisplay t8;
    feed(t8, T0,       Qt::Key_Right, 0x0100);
    feed(t8, T0 + 100, Qt::Key_H,     0x48);
    feed(t8, T0 + 200, Qt::Key_Right, 0x0100);
    check("right arrow, letter between -> no layer", t8, false, "H");

    TerminalDisplay t9;
    feed(t9, T0,       Qt::Key_Right, 0x0100);
    feed(t9, T0 + 120, Qt::Key_Right, 0x0100);
    check("double-tap RIGHT arrow -> layer", t9, true, "");

    std::printf("\n%s (%d failure%s)\n",
                failures ? "BUILD/RUN FAILED" : "ALL HOST CHECKS PASSED",
                failures, failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
