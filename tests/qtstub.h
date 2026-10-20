// Minimal Qt4 stand-ins, just enough to compile the real TerminalDisplay.cpp
// key-handling block on the host (no BB10 NDK / Qt headers available here).
#ifndef QTSTUB_H
#define QTSTUB_H

#include <cstdio>
#include <cstring>
#include <cstdlib>

typedef long long qint64;

namespace Qt {
enum Modifier {
    NoModifier      = 0x00000000,
    ShiftModifier   = 0x02000000,
    ControlModifier = 0x04000000,
    AltModifier     = 0x08000000
};
enum Key {
    Key_Escape = 0x01000000,
    Key_Right  = 0x01000014,
    Key_Shift  = 0x01000020,
    Key_A = 0x41, Key_B = 0x42, Key_C = 0x43, Key_H = 0x48,
    Key_J = 0x4a, Key_K = 0x4b, Key_L = 0x4c
};
}

class QKeyEvent
{
public:
    QKeyEvent(int key, int nativeVk = 0, int nativeScan = 0, bool autorep = false)
        : _key(key), _nv(nativeVk), _ns(nativeScan), _rep(autorep),
          _acc(false), _mods(Qt::NoModifier) {}
    int key() const { return _key; }
    int nativeVirtualKey() const { return _nv; }
    int nativeScanCode() const { return _ns; }
    bool isAutoRepeat() const { return _rep; }
    int modifiers() const { return _mods; }
    void setModifiers(int m) { _mods = m; }
    void accept() { _acc = true; }
    bool accepted() const { return _acc; }
    void ignore() { _acc = false; }
private:
    int _key, _nv, _ns;
    bool _rep, _acc;
    int _mods;
};

class QDateTime
{
public:
    static qint64 currentMSecsSinceEpoch() { return s_now; }
    static qint64 s_now;
};

class QWidget
{
public:
    virtual ~QWidget() {}
    virtual void keyReleaseEvent(QKeyEvent *event) { (void)event; }
};

#define Q_UNUSED(x) (void)(x)

#endif // QTSTUB_H
