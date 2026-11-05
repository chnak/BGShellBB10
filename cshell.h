/*
 * CShellSession - one real pty + one child shell per terminal tab.
 *
 * The original app opened /dev/ptyp1 once in main() and forked a single
 * /bin/sh, with every part of the Konsole layer (kpty.cpp, k3process.cpp)
 * patched to reuse that one fd/pid.  That made multi-tab impossible.  This
 * class allocates a free pty pair, forks a login shell for it and pumps the
 * master fd into plain callbacks, so N tabs each own a shell.
 *
 * QNX has no forkpty()/posix_openpt() guarantee, so the pair is found by
 * scanning /dev/ptyp0../dev/ttyp0.. the way the original code hard-coded p1.
 */
#ifndef CSHELL_H
#define CSHELL_H

#include <QObject>
#include <QByteArray>
#include <QStringList>

class QSocketNotifier;

class CShellSession : public QObject
{
	Q_OBJECT

public:
	explicit CShellSession(QObject *parent = 0);
	~CShellSession();

	/** Allocate a pty pair and fork @a program (default /bin/sh -l). */
	bool start(int cols, int rows,
	           const QString &program = QString(),
	           const QStringList &args = QStringList());

	int  fd() const { return _fd; }
	int  pid() const { return _pid; }
	bool isRunning() const { return _fd >= 0 && !_dead; }

	/** Raw bytes -> shell (keys, pasted text, command lines). */
	void send(const char *data, int len);
	void send(const QByteArray &data);

	/** TIOCSWINSZ on our own master fd. */
	void setWindowSize(int cols, int rows);

	/** SIGHUP the shell and stop watching the fd (tab close). */
	void hangup();

signals:
	void dataReceived(const QByteArray &data);
	void finished();

private slots:
	void onReadable();

private:
	void closeFd();
	/** waitpid(WNOHANG): collect the child once it is gone (no zombies). */
	void reap();

	int               _fd;
	int               _pid;
	QSocketNotifier  *_notifier;
	bool              _dead;
};

#endif // CSHELL_H
