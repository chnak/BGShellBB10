/*
 * Crash diagnostics that outlive the app.
 *
 * When the app dies - a segfault in the terminal code, or the system
 * killing it because it used too much memory - the only evidence left is
 * whatever was written to disk before it died.  Everything here goes into
 * the *persistent* data directory (bgshellDataPath()), so the log is still
 * there after the app is gone: the shared folder is on the device storage
 * and can be read over USB or from a shell on the phone.
 *
 * The log is plain text, one line per event.  It ends up in the persistent
 * directory - on a phone without an SD card that is
 * /accounts/1000/shared/bgshell/bgshell-diag.log:
 *
 *   2026-11-23 10:12:33.401 app start pid=4711
 *   2026-11-23 10:12:35.002 resize 24x80 (rows x cols)
 *   2026-11-23 10:12:41.700 shell pid=4712 exited with code 139
 *   2026-11-23 10:12:41.701 *** SIGNAL 11 code=1 addr=0x00000000 pid=4711 ***
 */
#ifndef BG_DIAG_H
#define BG_DIAG_H

#include <QtCore/QObject>

/** Open the log, install the crash handlers and record the app start. */
void bgshellDiagInit();

/** Start the heartbeat (needs an event loop, so call it after QApplication). */
void bgshellDiagHeartbeat();

/** Append one line to the log. */
void bgshellDiag(const char *fmt, ...);

/**
 * The heartbeat needs a slot to tick on.  It lives in the header because
 * that is where qmake looks for classes that need moc.
 */
class DiagTicker : public QObject
{
    Q_OBJECT
public:
    DiagTicker() {}
public slots:
    void tick();
};

#endif
