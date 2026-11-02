/*
    This file is part of Konsole, an X terminal.
    Copyright (C) 1997,1998 by Lars Doelle <lars.doelle@on-line.de>

    Rewritten for QT4 by e_k <e_k at users.sourceforge.net>, Copyright (C)2008

    Ported to Blackberry Playbook by BGmot <support@tm-k.com>, 2012

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301  USA.
*/

// Own
#include "Pty.h"

// System
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>

// Qt
#include <QtCore>

// KDE
//#include <KStandardDirs>
//#include <KLocale>
//#include <KDebug>
#include "kpty.h"

using namespace Konsole;

void Pty::donePty()
{
  emit done(exitStatus());
}

void Pty::setWindowSize(int lines, int cols)
{
  _windowColumns = cols;
  _windowLines = lines;

  if (pty()->masterFd() >= 0)
    pty()->setWinSize(lines, cols);
}
QSize Pty::windowSize() const
{
    return QSize(_windowColumns,_windowLines);
}

void Pty::setXonXoff(bool enable)
{
  _xonXoff = enable;

  if (pty()->masterFd() >= 0)
  {
    struct ::termios ttmode;
    pty()->tcGetAttr(&ttmode);
    if (!enable)
      ttmode.c_iflag &= ~(IXOFF | IXON);
    else
      ttmode.c_iflag |= (IXOFF | IXON);
    if (!pty()->tcSetAttr(&ttmode))
      qWarning("Unable to set terminal attributes.");
  }
}

void Pty::setUtf8Mode(bool enable)
{
#ifdef IUTF8 // XXX not a reasonable place to check it.
  _utf8 = enable;

  if (pty()->masterFd() >= 0)
  {
    struct ::termios ttmode;
    pty()->tcGetAttr(&ttmode);
    if (!enable)
      ttmode.c_iflag &= ~IUTF8;
    else
      ttmode.c_iflag |= IUTF8;
    if (!pty()->tcSetAttr(&ttmode))
      qWarning("Unable to set terminal attributes.");
  }
#endif
}

void Pty::setErase(char erase)
{
  _eraseChar = erase;
  
  if (pty()->masterFd() >= 0)
  {
    struct ::termios ttmode;

    pty()->tcGetAttr(&ttmode);

    ttmode.c_cc[VERASE] = erase;

    if (!pty()->tcSetAttr(&ttmode))
      qWarning("Unable to set terminal attributes.");
  }  
}

char Pty::erase() const
{
	if (pty()->masterFd() >= 0)
	{
		qDebug() << "Getting erase char";
		struct ::termios ttyAttributes;
		pty()->tcGetAttr(&ttyAttributes);
		return ttyAttributes.c_cc[VERASE];
	}

	return _eraseChar;
}

void Pty::addEnvironmentVariables(const QStringList& environment)
{
    QListIterator<QString> iter(environment);
    while (iter.hasNext())
    {
        QString pair = iter.next();

        // split on the first '=' character
        int pos = pair.indexOf('=');
        
        if ( pos >= 0 )
        {
            QString variable = pair.left(pos);
            QString value = pair.mid(pos+1);

            //kDebug() << "Setting environment pair" << variable <<
            //    " set to " << value;

            setEnvironment(variable,value);
        }
    }
}

int Pty::start(const QString& program, 
               const QStringList& programArguments, 
               const QStringList& environment, 
               ulong winid, 
               bool addToUtmp
//               const QString& dbusService, 
//               const QString& dbusSession)
		)
{
  clearArguments();

  setBinaryExecutable(program.toLatin1());

  addEnvironmentVariables(environment);

  QStringListIterator it( programArguments );
  while (it.hasNext())
    arguments.append( it.next().toUtf8() );

//  if ( !dbusService.isEmpty() )
//     setEnvironment("KONSOLE_DBUS_SERVICE",dbusService);
//  if ( !dbusSession.isEmpty() )
//     setEnvironment("KONSOLE_DBUS_SESSION", dbusSession);

  setEnvironment("WINDOWID", QString::number(winid));

  // unless the LANGUAGE environment variable has been set explicitly
  // set it to a null string
  // this fixes the problem where KCatalog sets the LANGUAGE environment
  // variable during the application's startup to something which
  // differs from LANG,LC_* etc. and causes programs run from
  // the terminal to display mesages in the wrong language
  //
  // this can happen if LANG contains a language which KDE
  // does not have a translation for
  //
  // BR:149300
  if (!environment.contains("LANGUAGE"))
      setEnvironment("LANGUAGE",QString());

  setUsePty(All, addToUtmp);

  pty()->open();
  
  struct ::termios ttmode;
  pty()->tcGetAttr(&ttmode);
  if (!_xonXoff)
    ttmode.c_iflag &= ~(IXOFF | IXON);
  else
    ttmode.c_iflag |= (IXOFF | IXON);
#ifdef IUTF8 // XXX not a reasonable place to check it.
  if (!_utf8)
    ttmode.c_iflag &= ~IUTF8;
  else
    ttmode.c_iflag |= IUTF8;
#endif

  if (_eraseChar != 0)
  	ttmode.c_cc[VERASE] = _eraseChar;
  
  if (!pty()->tcSetAttr(&ttmode))
    qWarning("Unable to set terminal attributes.");
  
  pty()->setWinSize(_windowLines, _windowColumns);

  if ( K3Process::start(NotifyOnExit, (Communication) (Stdin | Stdout)) == false )
     return -1;

  resume(); // Start...
  return 0;

}

void Pty::setWriteable(bool writeable)
{
  struct stat sbuf;
  stat(pty()->ttyName(), &sbuf);
  if (writeable)
    chmod(pty()->ttyName(), sbuf.st_mode | S_IWGRP);
  else
    chmod(pty()->ttyName(), sbuf.st_mode & ~(S_IWGRP|S_IWOTH));
}

Pty::Pty()
    : _writeFd(-1),
      _bufferFull(false),
      _windowColumns(0),
      _windowLines(0),
      _eraseChar(0),
      _xonXoff(true),
      _utf8(true)
{
  connect(this, SIGNAL(receivedStdout(K3Process *, char *, int )),
	  this, SLOT(dataReceived(K3Process *,char *, int)));
  connect(this, SIGNAL(processExited(K3Process *)),
          this, SLOT(donePty()));
  connect(this, SIGNAL(wroteStdin(K3Process *)),
          this, SLOT(writeReady()));
  _pty = new KPty;

  setUsePty(All, false); // utmp will be overridden later
}

Pty::~Pty()
{
    delete _pty;
}

// The pty of every tab is owned by CShellSession (cshell.cpp); the Konsole
// process object is never started, so its stdin fd stays -1.  writeStdin()
// therefore failed on every single key press and the queue was never drained:
// typing into a tab did nothing at all, and the tab looked dead.  The queue now
// leaves through the pty of the tab that has the focus (gActiveFd) and is
// drained synchronously - on this device a pty takes a few bytes at a time, so
// there is nothing to be won from the old async dance.
extern int gActiveFd;

void Pty::writeReady()
{
  if (!_pendingSendJobs.isEmpty())
    _pendingSendJobs.erase(_pendingSendJobs.begin());
  _bufferFull = false;
  doSendJobs();
}

void Pty::doSendJobs() {
  if(_pendingSendJobs.isEmpty())
  {
     emit bufferEmpty(); 
     return;
  }

  // Write the whole job, waiting out EAGAIN instead of dropping the tail.
  // The master fd is O_NONBLOCK, so pasting a screenful of text (or a fast
  // typist in front of a slow reader) used to be cut off in the middle: the
  // shell got half a command and the rest vanished without a word.  A short
  // poll() is enough - the reader drains the buffer within a frame or two.
  // This tab's own pty when one was attached; the global active-tab hook
  // only remains for the legacy single-session paths.
  const int outFd = (_writeFd >= 0) ? _writeFd : gActiveFd;

  while (!_pendingSendJobs.isEmpty()) {
    SendJob& job = _pendingSendJobs.first();

    bool ok = false;
    if (outFd >= 0) {
      const char *p = job.data();
      int left = job.length();
      ok = true;
      int stalled = 0;              // ms waited for room in the pty buffer
      while (left > 0) {
        const ssize_t n = ::write(outFd, p, left);
        if (n > 0) {
          p += n;
          left -= (int)n;
          stalled = 0;
          continue;
        }
        if (n < 0 && errno == EINTR)
          continue;
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
          if (stalled >= 250) {   // no room for a quarter second: give up
            ok = false;
            break;
          }
          struct pollfd pfd;
          pfd.fd = outFd;
          pfd.events = POLLOUT;
          pfd.revents = 0;
          ::poll(&pfd, 1, 20);
          stalled += 20;
          continue;
        }
        ok = false;
        break;
      }
    } else {
      ok = writeStdin(job.data(), job.length());
    }

    // Never keep a failed job at the head of the queue: one dead fd would
    // otherwise swallow every key that follows it, for good.
    _pendingSendJobs.erase(_pendingSendJobs.begin());

    if (!ok)
      qWarning("Pty::doSendJobs - input dropped, no live pty for the active tab.");
  }

  emit bufferEmpty();
}

void Pty::appendSendJob(const char* s, int len)
{
  _pendingSendJobs.append(SendJob(s,len));
}

void Pty::sendData(const char* s, int len)
{
  appendSendJob(s,len);
  if (!_bufferFull)
     doSendJobs();
}

void Pty::dataReceived(K3Process *,char *buf, int len)
{
  emit receivedData(buf,len);
}

void Pty::lockPty(bool lock)
{
  if (lock)
    suspend();
  else
    resume();
}

int Pty::foregroundProcessGroup() const
{
    int pid = tcgetpgrp(pty()->masterFd());

    if ( pid != -1 )
    {
        return pid;
    } 

    return 0;
}

//#include "moc_Pty.cpp"
