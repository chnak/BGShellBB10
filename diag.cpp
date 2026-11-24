/*
 * See diag.h.
 */
#include "diag.h"
#include "bgshelldirs.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QObject>
#include <QtCore/QTimer>

#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>

// The log lives in the persistent data directory, so it survives the death of
// the app (and even a reinstall of it).
static QByteArray gDiagPath;
static int         gDiagFd = -1;
static int         gDiagBytes = 0;

// --- the lowest level: one write() -----------------------------------------
// The crash handler runs on the stack of a dying process.  It may only use
// write() and snprintf() - anything that allocates could deadlock on a
// malloc lock that is already held.
static void diagRaw(const char *buf, int len)
{
    if (gDiagFd < 0 || len <= 0)
        return;
    const ssize_t n = ::write(gDiagFd, buf, (size_t)len);
    (void)n;
    gDiagBytes += (n > 0) ? (int)n : 0;
}

static void diagSignalHandler(int sig, siginfo_t *info, void * /*ctx*/)
{
    char buf[256];
    const int n = snprintf(buf, sizeof buf,
                           "\n*** SIGNAL %d code=%d addr=%p pid=%d ***\n",
                           sig,
                           info ? info->si_code : 0,
                           info ? info->si_addr : (void *)0,
                           (int)getpid());
    if (n > 0)
        diagRaw(buf, n);

    // Hand the signal back to the default handler, so the platform still
    // records the crash the way it expects to see it.
    struct sigaction dfl;
    sigemptyset(&dfl.sa_mask);
    dfl.sa_handler = SIG_DFL;
    dfl.sa_flags = 0;
    sigaction(sig, &dfl, (struct sigaction *)0);
    raise(sig);
}

void bgshellDiagInit()
{
    if (gDiagFd >= 0)
        return;

    const QString path = bgshellDataPath(QString::fromLatin1("data/bgshell-diag.log"));
    gDiagPath = QFile::encodeName(path);

    // The file is appended to on every start - keep only the last one.
    const QFileInfo fi(path);
    if (fi.exists() && fi.size() > 512 * 1024) {
        QFile::remove(path + QString::fromLatin1(".1"));
        QFile::rename(path, path + QString::fromLatin1(".1"));
    }

    gDiagFd = ::open(gDiagPath.constData(), O_WRONLY | O_CREAT | O_APPEND, 0644);

    struct sigaction act;
    sigemptyset(&act.sa_mask);
    act.sa_handler = SIG_DFL;
    act.sa_sigaction = diagSignalHandler;
    act.sa_flags = SA_SIGINFO;
    const int sigs[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT, SIGSYS };
    for (unsigned i = 0; i < sizeof(sigs) / sizeof(sigs[0]); ++i)
        sigaction(sigs[i], &act, (struct sigaction *)0);

    bgshellDiag("app start pid=%d", (int)getpid());
}

void bgshellDiag(const char *fmt, ...)
{
    if (gDiagFd < 0)
        return;

    char body[800];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(body, sizeof(body) - 1, fmt, ap);
    va_end(ap);
    if (n < 0)
        return;
    if (n > (int)sizeof(body) - 2)
        n = (int)sizeof(body) - 2;

    char line[1024];
    const QByteArray ts =
        QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz").toLatin1();
    const int len = snprintf(line, sizeof(line), "%s %s\n", ts.constData(), body);
    if (len > 0)
        diagRaw(line, len);

    // The heartbeat alone writes ~160 KB per hour - keep the file small by
    // keeping the last megabyte only.
    if (gDiagBytes > 1024 * 1024) {
        ::close(gDiagFd);
        gDiagFd = -1;
        const QString path = QString::fromLocal8Bit(gDiagPath);
        QFile::remove(path + QString::fromLatin1(".1"));
        QFile::rename(path, path + QString::fromLatin1(".1"));
        gDiagFd = ::open(gDiagPath.constData(), O_WRONLY | O_CREAT | O_APPEND, 0644);
        gDiagBytes = 0;
    }
}

// --- the heartbeat ----------------------------------------------------------
// Every two seconds: how much memory the app is using and how much it is
// allowed to use.  If the app is killed (SIGKILL, no handler runs) the last
// line before the gap is the story.
static void diagHeartbeat()
{
    struct rusage ru;
    memset(&ru, 0, sizeof(ru));
    getrusage(RUSAGE_SELF, &ru);

    struct rlimit rl;
    memset(&rl, 0, sizeof(rl));
    getrlimit(RLIMIT_AS, &rl);

#ifdef __QNXNTO__
    // QNX keeps the memory a process may still use in RLIMIT_FREEMEM - that
    // is the number that matters when the system kills the app for using too
    // much memory.
    struct rlimit rf;
    memset(&rf, 0, sizeof(rf));
    getrlimit(RLIMIT_FREEMEM, &rf);
    bgshellDiag("hb maxrss=%ld as=%llu/%llu freemem=%llu/%llu",
                (long)ru.ru_maxrss,
                (unsigned long long)rl.rlim_cur,
                (unsigned long long)rl.rlim_max,
                (unsigned long long)rf.rlim_cur,
                (unsigned long long)rf.rlim_max);
#else
    bgshellDiag("hb maxrss=%ld as=%llu/%llu",
                (long)ru.ru_maxrss,
                (unsigned long long)rl.rlim_cur,
                (unsigned long long)rl.rlim_max);
#endif
}

void DiagTicker::tick()
{
    diagHeartbeat();
}

void bgshellDiagHeartbeat()
{
    static DiagTicker *ticker = 0;
    if (ticker)
        return;

    ticker = new DiagTicker();
    QTimer *timer = new QTimer(ticker);
    QObject::connect(timer, SIGNAL(timeout()), ticker, SLOT(tick()));
    timer->start(2000);

    diagHeartbeat();
}
