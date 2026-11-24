// Host-side check for the pinyin IME of the BGShellBB10 terminal.
//
// The Q30 keyboard has no number row of its own: the digits live on the Alt
// layer (Alt+W is 1, Alt+E is 2, Alt+R is 3, ... Alt+C is 9).  The IME used
// to throw every Alt chord away, so a candidate could only be picked by
// tapping the strip - and the chord fell through to the shell as a stray
// digit.  These checks replay the real key handling (imehelpers.inc /
// imekey.inc are lifted verbatim out of TerminalDisplay.cpp by
// extract_ime_key_block.py) against the real pinyin engine (cjk_ime.c), so
// what is verified here is what ships.
//
//   tests/run-ime-alt-digit-check.sh
#include <QByteArray>
#include <QDateTime>
#include <QEvent>
#include <QString>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "cjk_ime.h"

// The key event of the phone, not of a desktop.
//
// The BB10 keyboards latch Alt and Shift: pressing a modifier on its own
// arrives as a key event of its own (key == Qt::Key_Alt) that carries no
// character and - this is the part that matters - no modifier either; the
// key pressed after it carries the modifier.  TerminalDisplay.cpp says so
// about the Alt key ("key=1000023 and modifiers = 0"), and the guide repeats
// it ("按住 Alt 再按空格，或点一下 Alt 再按空格，都行").
//
// A desktop Qt5 would add the modifier of the key to the event, so the
// desktop QKeyEvent would hide exactly the path these checks are about: the
// lone Alt would look like a chord and take the "Alt is held" branch.  The
// event is therefore built here the way the phone sends it.
class QKeyEvent {
public:
    QKeyEvent(QEvent::Type type, int key, Qt::KeyboardModifiers modifiers,
              const QString &text = QString(), bool autorep = false)
        : _type(type), _key(key), _modifiers(modifiers), _text(text),
          _autorep(autorep), _accepted(false)
    {
    }

    int                    key()         const { return _key; }
    Qt::KeyboardModifiers  modifiers()   const { return _modifiers; }
    QString                text()        const { return _text; }
    bool                   isAutoRepeat() const { return _autorep; }
    void                   accept()            { _accepted = true; }
    bool                   isAccepted()  const { return _accepted; }

private:
    QEvent::Type           _type;
    int                    _key;
    Qt::KeyboardModifiers  _modifiers;
    QString                _text;
    bool                   _autorep;
    bool                   _accepted;
};

// The helpers of TerminalDisplay.cpp, verbatim.
#include "imehelpers.inc"

namespace {

// The candidate strip, as far as the key handling cares about it.
class CImePanel {
public:
    bool isOpen() const { return false; }
    void moveHighlight(int) {}
    bool hasHighlight() const { return false; }
    void commitHighlighted() {}
};

} // namespace

// The trace hook of TerminalDisplay.cpp.
static void imeTraceBytes(const char *, const char *, int) {}

// The widget, reduced to the members the IME path touches.
class TerminalDisplay {
public:
    bool         _imeOn;
    cjk_ime_t   *_ime;
    CImePanel   *_imePanel;
    QString      _imeRaw;
    qint64       _imeCommitMs;
    std::string  pty;          // everything that went to the shell

    TerminalDisplay()
        : _imeOn(false), _ime(0), _imePanel(0), _imeCommitMs(0)
    {
        _ime = (cjk_ime_t *)calloc(1, sizeof(cjk_ime_t));
        cjk_ime_init(_ime);
    }

    ~TerminalDisplay() { free(_ime); }

    void writeInputBytes(const char *b, int n) { pty.append(b, n); }
    void refreshImePanel() {}
    void update() {}

    // Same shape as the widget's sendImeSym(): feed the engine, write a
    // commit to the shell.
    void sendImeSym(int sym)
    {
        char out[192];
        size_t outLen = 0;
        const int r = cjk_ime_handle_key(_ime, sym, 0, 1, out, sizeof(out), &outLen);
        if (r == 2 && outLen > 0)
            writeInputBytes(out, (int)outLen);
    }

    void onImePageRequested(int delta) { sendImeSym(delta < 0 ? '[' : ']'); }

    bool handleImeKey(QKeyEvent *event)
    {
#include "imekey.inc"
    }
};

namespace {

int failures = 0;

void ok(const char *what, bool cond, const std::string &detail = std::string())
{
    if (cond) {
        std::printf("PASS  %s\n", what);
    } else {
        std::printf("FAIL  %s  %s\n", what, detail.c_str());
        ++failures;
    }
}

// A key press, the way the device delivers it.
void press(TerminalDisplay &t, int key, Qt::KeyboardModifiers mods,
           const char *text)
{
    QKeyEvent ev(QEvent::KeyPress, key, mods, QString::fromLatin1(text));
    t.handleImeKey(&ev);
}

// A letter key: Qt::Key_A .. Qt::Key_Z with the letter as the key text.
void type(TerminalDisplay &t, const char *letters)
{
    for (const char *p = letters; *p; ++p) {
        const char letter[2] = { *p, 0 };
        press(t, toupper(*p), Qt::NoModifier, letter);
    }
}

std::string candidate(TerminalDisplay &t, int index)
{
    const char *c = cjk_ime_candidate(t._ime, index);
    return c ? std::string(c) : std::string();
}

} // namespace

int main()
{
    // Keep the word frequency file out of the way: the checks below are
    // about the key handling, not about what the user picked before.
    cjk_ime_set_freq_path("/tmp/bgime-alt-digit-freq");

    // ---- 1. Alt+W (the "1" of the Alt layer) takes candidate 1 ----------
    {
        TerminalDisplay t;
        t._imeOn = true;
        cjk_ime_set_enabled(t._ime, 1);
        type(t, "nihao");

        const std::string c1 = candidate(t, 0);
        ok("nihao has a candidate", !c1.empty(), c1);

        press(t, Qt::Key_W, Qt::AltModifier, "1");
        ok("Alt+W writes candidate 1 to the shell", t.pty == c1,
           "got '" + t.pty + "' wanted '" + c1 + "'");
        ok("Alt+W leaves no stray digit in the shell",
           t.pty.find('1') == std::string::npos, t.pty);
        ok("Alt+W ends the composition",
           cjk_ime_preedit(t._ime)[0] == '\0');
    }

    // ---- 2. Alt+E takes candidate 2 -----------------------------------
    {
        TerminalDisplay t;
        t._imeOn = true;
        cjk_ime_set_enabled(t._ime, 1);
        type(t, "shi");

        ok("shi has a second candidate", cjk_ime_candidate_count(t._ime) > 1);
        const std::string c2 = candidate(t, 1);
        press(t, Qt::Key_E, Qt::AltModifier, "2");
        ok("Alt+E writes candidate 2 to the shell", t.pty == c2,
           "got '" + t.pty + "' wanted '" + c2 + "'");
    }

    // ---- 3. the same chord without the key text (layout gives only the key)
    {
        TerminalDisplay t;
        t._imeOn = true;
        cjk_ime_set_enabled(t._ime, 1);
        type(t, "shi");

        const std::string c1 = candidate(t, 0);
        press(t, Qt::Key_W, Qt::AltModifier, "");
        ok("Alt+W without a key text still takes candidate 1", t.pty == c1,
           "got '" + t.pty + "' wanted '" + c1 + "'");
    }

    // ---- 4. Alt+R (the "3" of the Alt layer) takes candidate 3 ----------
    {
        TerminalDisplay t;
        t._imeOn = true;
        cjk_ime_set_enabled(t._ime, 1);
        type(t, "shi");

        ok("shi has a third candidate", cjk_ime_candidate_count(t._ime) > 2);
        const std::string c3 = candidate(t, 2);
        press(t, Qt::Key_R, Qt::AltModifier, "3");
        ok("Alt+R writes candidate 3 to the shell", t.pty == c3,
           "got '" + t.pty + "' wanted '" + c3 + "'");
    }

    // ---- 5. a chord that is not a digit keeps its own meaning ---------
    {
        TerminalDisplay t;
        t._imeOn = true;
        cjk_ime_set_enabled(t._ime, 1);
        type(t, "shi");

        press(t, Qt::Key_Q, Qt::AltModifier, "#");
        ok("Alt+Q does not touch the composition",
           cjk_ime_preedit(t._ime)[0] != '\0');
        ok("Alt+Q writes nothing to the shell", t.pty.empty(), t.pty);
    }

    // ---- 6. no composition: the digit belongs to the shell ------------
    {
        TerminalDisplay t;
        t._imeOn = true;
        cjk_ime_set_enabled(t._ime, 1);

        press(t, Qt::Key_W, Qt::AltModifier, "1");
        ok("Alt+W without a composition writes nothing to the IME",
           cjk_ime_preedit(t._ime)[0] == '\0');
        ok("Alt+W without a composition leaves the shell alone",
           t.pty.empty(), t.pty);
    }

    // ---- 7. Ctrl chords are still Ctrl --------------------------------
    {
        TerminalDisplay t;
        t._imeOn = true;
        cjk_ime_set_enabled(t._ime, 1);
        type(t, "shi");

        press(t, Qt::Key_W, Qt::ControlModifier, "\x17");
        ok("Ctrl+W does not pick a candidate",
           cjk_ime_preedit(t._ime)[0] != '\0');
        ok("Ctrl+W writes nothing to the shell", t.pty.empty(), t.pty);
    }

    // ---- 8. plain letters still compose -------------------------------
    {
        TerminalDisplay t;
        t._imeOn = true;
        cjk_ime_set_enabled(t._ime, 1);
        type(t, "nihao");
        ok("plain letters still build the pinyin buffer",
           std::string(cjk_ime_preedit(t._ime)) == "nihao",
           cjk_ime_preedit(t._ime));
        ok("plain letters write nothing to the shell", t.pty.empty(), t.pty);
    }

    // ---- 9. the Alt key on its own must not end the composition ------
    // The BB10 keyboards latch Alt: tapping it alone arrives as a key
    // event of its own (key == Qt::Key_Alt, no modifier set, no text),
    // and the key pressed after it carries the modifier.  That first event
    // carries no character, so it must not be read as "a key the engine
    // cannot use" - which flushed the letters typed so far to the shell as
    // plain English and dropped the candidates, so the chord that was
    // meant to pick one found nothing left to pick.
    {
        TerminalDisplay t;
        t._imeOn = true;
        cjk_ime_set_enabled(t._ime, 1);
        type(t, "shi");

        press(t, Qt::Key_Alt, Qt::NoModifier, "");
        ok("the Alt key alone keeps the composition",
           cjk_ime_preedit(t._ime)[0] != '\0');
        ok("the Alt key alone writes no English to the shell",
           t.pty.empty(), t.pty);

        // ... and the key that follows (with the modifier set) picks a
        // candidate as usual.
        const std::string c1 = candidate(t, 0);
        press(t, Qt::Key_W, Qt::AltModifier, "1");
        ok("Alt+W after the latched Alt takes candidate 1", t.pty == c1,
           "got '" + t.pty + "' wanted '" + c1 + "'");
        ok("Alt+W after the latched Alt leaves no stray English",
           t.pty.find('s') == std::string::npos &&
           t.pty.find('h') == std::string::npos &&
           t.pty.find('i') == std::string::npos, t.pty);
    }

    // ---- 10. the same for the other modifiers -------------------------
    {
        TerminalDisplay t;
        t._imeOn = true;
        cjk_ime_set_enabled(t._ime, 1);
        type(t, "shi");

        press(t, Qt::Key_Control, Qt::NoModifier, "");
        ok("the Control key alone keeps the composition",
           cjk_ime_preedit(t._ime)[0] != '\0');
        ok("the Control key alone writes no English to the shell",
           t.pty.empty(), t.pty);

        press(t, Qt::Key_Shift, Qt::NoModifier, "");
        ok("the Shift key alone keeps the composition",
           cjk_ime_preedit(t._ime)[0] != '\0');
        ok("the Shift key alone writes no English to the shell",
           t.pty.empty(), t.pty);
    }

    if (failures) {
        std::printf("%d FAILED\n", failures);
        return 1;
    }
    std::printf("all IME Alt-digit cases OK\n");
    return 0;
}
