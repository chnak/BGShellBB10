#include "cshell.h"

#include <QtCore>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>

CShellSession::CShellSession(QObject *parent)
	: QObject(parent), _fd(-1), _pid(-1), _notifier(0), _dead(false)
{
}

CShellSession::~CShellSession()
{
	hangup();
}

void CShellSession::closeFd()
{
	if (_notifier) {
		_notifier->setEnabled(false);
		delete _notifier;
		_notifier = 0;
	}
	if (_fd >= 0) {
		::close(_fd);
		_fd = -1;
	}
}

bool CShellSession::start(int cols, int rows, const QString &program, const QStringList &args)
{
	if (_fd >= 0)
		return true;

	int    master = -1;
	int    slave  = -1;
	QByteArray slaveName;

	// QNX style paired devices: /dev/ptypN <-> /dev/ttypN.  A ptyp that is
	// already owned by somebody else refuses the open, so the first success
	// is a free pair.
	for (int i = 0; i < 16 && master < 0; ++i) {
		const QByteArray mname = QString("/dev/ptyp%1").arg(i).toLatin1();
		const QByteArray sname = QString("/dev/ttyp%1").arg(i).toLatin1();
		const int m = ::open(mname.constData(), O_RDWR | O_NOCTTY);
		if (m < 0)
			continue;
		const int s = ::open(sname.constData(), O_RDWR | O_NOCTTY);
		if (s < 0) {
			::close(m);
			continue;
		}
		master = m;
		slave  = s;
		slaveName = sname;
	}

	// Fallback for build hosts / images that only have the unix98 ptmx.
	if (master < 0) {
#ifdef TIOCGPTN
		master = ::open("/dev/ptmx", O_RDWR | O_NOCTTY);
		if (master >= 0) {
			int n = 0;
			if (ioctl(master, TIOCGPTN, &n) != 0) {
				::close(master);
				master = -1;
			} else {
				int unlock = 0;
				ioctl(master, TIOCSPTLCK, &unlock);
				const QByteArray sname = QString("/dev/pts/%1").arg(n).toLatin1();
				slave = ::open(sname.constData(), O_RDWR | O_NOCTTY);
				if (slave < 0) {
					::close(master);
					master = -1;
				} else {
					slaveName = sname;
				}
			}
		}
#endif
	}

	if (master < 0 || slave < 0) {
		qWarning("CShellSession: no free pty pair");
		return false;
	}

	struct winsize ws;
	memset(&ws, 0, sizeof(ws));
	ws.ws_col = (cols > 1) ? cols : 80;
	ws.ws_row = (rows > 1) ? rows : 24;
	ioctl(master, TIOCSWINSZ, &ws);

	// Everything the child needs must be prepared before fork(): Qt's
	// allocator is not fork-safe once another thread holds its lock.
	QByteArray prog = program.isEmpty() ? QByteArray("/bin/sh") : QFile::encodeName(program);
	QList<QByteArray> argvStore;
	argvStore << prog;
	if (args.isEmpty()) {
		argvStore << QByteArray("-l");
	} else {
		for (int i = 0; i < args.size(); ++i)
			argvStore << QFile::encodeName(args.at(i));
	}
	QVector<char *> argv;
	for (int i = 0; i < argvStore.size(); ++i)
		argv << argvStore[i].data();
	argv << (char *)0;

	const QByteArray home = QFile::encodeName(QDir::homePath());

	_pid = fork();
	if (_pid == 0) {
		// child
		setsid();
#ifdef TIOCSCTTY
		ioctl(slave, TIOCSCTTY, 0);
#endif
		dup2(slave, STDIN_FILENO);
		dup2(slave, STDOUT_FILENO);
		dup2(slave, STDERR_FILENO);
		if (slave > STDERR_FILENO)
			::close(slave);
		::close(master);

		struct sigaction act;
		sigemptyset(&act.sa_mask);
		act.sa_handler = SIG_DFL;
		act.sa_flags = 0;
		for (int sig = 1; sig < NSIG; ++sig)
			sigaction(sig, &act, 0);

		::setenv("TERM", "xterm", 1);
		if (!home.isEmpty())
			::chdir(home.constData());

		execvp(argv[0], argv.data());
		_exit(127);
	}

	::close(slave);

	if (_pid < 0) {
		qWarning("CShellSession: fork failed (%s)", strerror(errno));
		::close(master);
		return false;
	}

	_fd = master;
	fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL) | O_NONBLOCK);
	_dead = false;

	_notifier = new QSocketNotifier(_fd, QSocketNotifier::Read, this);
	connect(_notifier, SIGNAL(activated(int)), this, SLOT(onReadable()));

	return true;
}

void CShellSession::onReadable()
{
	if (_fd < 0)
		return;

	char buf[8192];
	for (;;) {
		const ssize_t n = ::read(_fd, buf, sizeof(buf));
		if (n > 0) {
			emit dataReceived(QByteArray(buf, (int)n));
			continue;
		}
		if (n == 0) {
			// slave side gone
			reap();
			if (!_dead) {
				_dead = true;
				emit finished();
			}
			closeFd();
			return;
		}
		if (errno == EINTR)
			continue;
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		// EIO on Linux, EBADF on QNX once the child is gone
		reap();
		if (!_dead) {
			_dead = true;
			emit finished();
		}
		closeFd();
		return;
	}
}

void CShellSession::send(const char *data, int len)
{
	if (_fd < 0 || _dead || !data || len <= 0)
		return;

	int off = 0;
	while (off < len) {
		const ssize_t n = ::write(_fd, data + off, len - off);
		if (n > 0) {
			off += (int)n;
			continue;
		}
		if (n < 0 && errno == EINTR)
			continue;
		if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			// Master buffer full: wait for the shell to read instead of
			// dropping the rest.  A dropped tail is a half-typed command,
			// which is far worse than a 20 ms stall.  Give up only after
			// a quarter of a second of no room at all.
			static const int kBudgetMs = 250;
			static const int kSliceMs  = 20;
			int waited = 0;
			while (waited < kBudgetMs) {
				struct pollfd pfd;
				pfd.fd = _fd;
				pfd.events = POLLOUT;
				pfd.revents = 0;
				if (::poll(&pfd, 1, kSliceMs) > 0)
					break;
				waited += kSliceMs;
			}
			if (waited >= kBudgetMs)
				return;
			continue;
		}
		return;
	}
}

void CShellSession::send(const QByteArray &data)
{
	send(data.constData(), data.size());
}

void CShellSession::setWindowSize(int cols, int rows)
{
	if (_fd < 0 || cols < 2 || rows < 2)
		return;
	struct winsize ws;
	memset(&ws, 0, sizeof(ws));
	ws.ws_col = cols;
	ws.ws_row = rows;
	ioctl(_fd, TIOCSWINSZ, &ws);
}

void CShellSession::hangup()
{
	if (_pid > 0) {
		kill(_pid, SIGHUP);
		kill(_pid, SIGTERM);
		reap();
		// A shell that traps HUP/TERM would otherwise stay behind for good,
		// still holding the slave side of a pty nobody reads any more.
		if (_pid > 0) {
			kill(_pid, SIGKILL);
			reap();
		}
	}
	_dead = true;
	closeFd();
}

// Reap the child if it is already gone.  Nothing else in the app waits for
// our pids - k3processcontroller only knows the processes it started itself -
// so without this every tab whose shell exits (exit / Ctrl-D) or is closed
// leaks one zombie into the process table, and they accumulate for the life of
// the app.
void CShellSession::reap()
{
	if (_pid <= 0)
		return;

	int status = 0;
	const pid_t r = ::waitpid(_pid, &status, WNOHANG);
	if (r == _pid || (r < 0 && errno == ECHILD))
		_pid = -1;
}
