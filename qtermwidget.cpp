/*  Copyright (C) 2008 e_k (e_k@users.sourceforge.net)

    Ported to Blackberry Playbook by BGmot <support@tm-k.com>, 2012

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
		
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
				
    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
						

#include "qtermwidget.h"

#include "Session.h"
#include "Emulation.h"
#include "TerminalDisplay.h"
#include "cshell.h"
#include "cmdpanel.h"

#include "mymainwindow.h"
extern CMyMainWindow *mainWindow;

using namespace Konsole;

void *createTermWidget(int startnow, void *parent)
{ 
    return (void*) new QTermWidget(startnow, (QWidget*)parent); 
}

struct TermWidgetImpl
{
    TermWidgetImpl(QWidget* parent = 0);

    TerminalDisplay *m_terminalDisplay;
    Session *m_session;
    QPushButton *btnCtrlC;
    
    Session* createSession();
    TerminalDisplay* createTerminalDisplay(Session *session, QWidget* parent);
};

TermWidgetImpl::TermWidgetImpl(QWidget* parent)
{
    this->m_session = createSession();
    this->m_terminalDisplay = createTerminalDisplay(this->m_session, parent);

}

Session *TermWidgetImpl::createSession()
{
    Session *session = new Session();
    session->setTitle(Session::NameRole, "QTermWidget");
    //+++session->setProgram("/bin/bash");
    session->setProgram("/bin/sh");
    QStringList args("");
    session->setArguments(args);
    session->setAutoClose(true);
		    
    session->setCodec(QTextCodec::codecForName("UTF-8"));
			
    session->setFlowControlEnabled(true);
    session->setHistoryType(HistoryTypeBuffer(1000));
    
    session->setDarkBackground(true);
	    
    session->setKeyBindings("");	    
    return session;
}

TerminalDisplay *TermWidgetImpl::createTerminalDisplay(Session *session, QWidget* parent)
{
//    TerminalDisplay* display = new TerminalDisplay(this);
    TerminalDisplay* display = new TerminalDisplay(parent);
    
    display->setBellMode(TerminalDisplay::NotifyBell);
    display->setTerminalSizeHint(true);
    display->setTripleClickMode(TerminalDisplay::SelectWholeLine);
    display->setTerminalSizeStartup(true);

    display->setRandomSeed(session->sessionId() * 31);
    
    return display;
}

QTermWidget::QTermWidget(int startnow, QWidget *parent)
:QWidget(parent)
{
    m_impl = new TermWidgetImpl(this);
    
    init();

    if (startnow && m_impl->m_session) {
    	m_impl->m_session->run();
    }
    
    this->setFocus( Qt::OtherFocusReason );
    m_impl->m_terminalDisplay->resize(this->size());
    
    this->setFocusProxy(m_impl->m_terminalDisplay);
}

void QTermWidget::startShellProgram()
{
    if ( m_impl->m_session->isRunning() )
	return;
	
    m_impl->m_session->run();
}

void QTermWidget::init()
{    
    m_impl->m_terminalDisplay->setSize(80, 40);
    
    QFont font = QApplication::font(); 
    font.setFamily("Courier New");
    //font.setPointSize(6);
    font.setPixelSize(mainWindow->nFontSize);
    font.setStyleHint(QFont::TypeWriter);
    setTerminalFont(font);
    setScrollBarPosition(NoScrollBar);    
        
    m_impl->m_session->addView(m_impl->m_terminalDisplay);
    
    connect(m_impl->m_session, SIGNAL(finished()), this, SLOT(sessionFinished()));
}


QTermWidget::~QTermWidget()
{
    emit destroyed();
}


void QTermWidget::setTerminalFont(QFont &font)
{
    if (!m_impl->m_terminalDisplay)
	return;
    m_impl->m_terminalDisplay->setVTFont(font);
}

void QTermWidget::setShellProgram(QString &progname)
{
    if (!m_impl->m_session)
	return;
    m_impl->m_session->setProgram(progname);	
}

void QTermWidget::setArgs(QStringList &args)
{
    if (!m_impl->m_session)
	return;
    m_impl->m_session->setArguments(args);	
}

void QTermWidget::setTextCodec(QTextCodec *codec)
{
    if (!m_impl->m_session)
	return;
    m_impl->m_session->setCodec(codec);	
}

void QTermWidget::setColorScheme(int scheme)
{
    switch(scheme) {
	case COLOR_SCHEME_WHITE_ON_BLACK:
		m_impl->m_terminalDisplay->setColorTable(whiteonblack_color_table);
		break;		
	case COLOR_SCHEME_GREEN_ON_BLACK:
		m_impl->m_terminalDisplay->setColorTable(greenonblack_color_table);
		break;
	case COLOR_SCHEME_BLACK_ON_LIGHT_YELLOW:
		m_impl->m_terminalDisplay->setColorTable(blackonlightyellow_color_table);
		break;
	default: //do nothing
	    break;
    };
}

void QTermWidget::setSize(int h, int v)
{
    if (!m_impl->m_terminalDisplay)
	return;
    m_impl->m_terminalDisplay->setSize(h, v);
}

void QTermWidget::setHistorySize(int lines)
{
    if (lines < 0)
        m_impl->m_session->setHistoryType(HistoryTypeFile());
    else
	m_impl->m_session->setHistoryType(HistoryTypeBuffer(lines));
}

void QTermWidget::setScrollBarPosition(ScrollBarPosition pos)
{
    if (!m_impl->m_terminalDisplay)
	return;
    m_impl->m_terminalDisplay->setScrollBarPosition((TerminalDisplay::ScrollBarPosition)pos);
}

void QTermWidget::sendText(QString &text)
{
    m_impl->m_session->sendText(text); 
}

void QTermWidget::resizeEvent(QResizeEvent*)
{
//+++qDebug("global window resizing...with %d %d", this->size().width(), this->size().height());
	m_impl->m_terminalDisplay->resize(this->size());
}

void QTermWidget::sessionFinished()
{
	//+++ We want to hide virtual keyboard before we exit
	QEvent event(QEvent::CloseSoftwareInputPanel);
	QApplication::sendEvent(m_impl->m_terminalDisplay, &event);
    emit finished();
}


bool QTermWidget::event( QEvent *e ){
    //+++fprintf(stderr,"QTermWidget::event type=%d\n",e->type());//+++

    // We want to deliver this signal to private m_impl
	if (e->type() == QEvent::RequestSoftwareInputPanel){
		QEvent event(QEvent::RequestSoftwareInputPanel);
		QApplication::sendEvent(m_impl->m_terminalDisplay, &event);
		return true;
	}
	return QWidget::event(e);
}


// ---------------------------------------------------------------------------
// Multi-session + IME / command panel plumbing
// ---------------------------------------------------------------------------

// Hand this widget its own pty.  Without it the Konsole Session would ask the
// patched kpty/k3process pair to fork a shell, and that pair can only ever
// serve one session - which is why every tab used to share the first shell.
void QTermWidget::attachShell(CShellSession *shell)
{
	if (_shell == shell)
		return;
	_shell = shell;
	if (!_shell)
		return;

	connect(_shell, SIGNAL(dataReceived(QByteArray)),
	        this, SLOT(onShellData(QByteArray)));
	connect(_shell, SIGNAL(finished()),
	        this, SLOT(sessionFinished()));

	// The emulation tells us the grid size; the pty has to follow.
	if (m_impl && m_impl->m_terminalDisplay) {
		connect(m_impl->m_terminalDisplay,
		        SIGNAL(changedContentSizeSignal(int,int)),
		        this, SLOT(onDisplayGridResized(int,int)),
		        Qt::DirectConnection);
	}

	// OSC 0 / OSC 2 rename the tab.
	Emulation *emulation = (m_impl && m_impl->m_session)
	                     ? m_impl->m_session->emulation() : 0;
	if (emulation) {
		connect(emulation, SIGNAL(titleChanged(int,QString)),
		        this, SLOT(onEmulationTitleChanged(int,QString)),
		        Qt::DirectConnection);
	}

	syncShellWindowSize();
}

// Bytes from the pty go into the emulation, which updates the screen and the
// character grid behind it.
void QTermWidget::onShellData(const QByteArray &data)
{
	if (!m_impl || !m_impl->m_session || data.isEmpty())
		return;
	Emulation *emulation = m_impl->m_session->emulation();
	if (!emulation)
		return;
	emulation->receiveData(data.constData(), data.size());
}

// The character grid changed size (widget, font or bottom reserve).  Konsole's
// own plumbing cannot push this to the pty: the Pty object it keeps is never
// started, so its setWindowSize() is a no-op.
void QTermWidget::onDisplayGridResized(int height, int width)
{
	Q_UNUSED(height);
	Q_UNUSED(width);
	syncShellWindowSize();
}

// Push the current grid size to the shell (TIOCSWINSZ), so a program that asks
// getsize sees the truth instead of the 100x30 the pty was created with.
void QTermWidget::syncShellWindowSize()
{
	if (!_shell || !_shell->isRunning() || !m_impl || !m_impl->m_terminalDisplay)
		return;

	const int cols = m_impl->m_terminalDisplay->columns();
	const int rows = m_impl->m_terminalDisplay->lines();
	if (cols > 1 && rows > 1)
		_shell->setWindowSize(cols, rows);
}

void QTermWidget::onEmulationTitleChanged(int what, const QString &newTitle)
{
	Q_UNUSED(what);
	const QString title = newTitle.trimmed().isEmpty() ? _defaultTitle
	                                                   : newTitle.trimmed();
	emit titleChanged(title);
}

// Trackpad driving of the IME candidate strip (see main.cpp).  True means the
// strip consumed the movement, so it does not also become an arrow key for the
// shell.
bool QTermWidget::imeTrackpadStep(int dx, int dy)
{
	if (!m_impl || !m_impl->m_terminalDisplay)
		return false;
	return m_impl->m_terminalDisplay->imeTrackpadStep(dx, dy);
}

bool QTermWidget::imeTrackpadClick()
{
	if (!m_impl || !m_impl->m_terminalDisplay)
		return false;
	return m_impl->m_terminalDisplay->imeTrackpadClick();
}

// Quick commands / saved SSH hosts overlay.
void QTermWidget::toggleCommandPanel()
{
	if (!_panel) {
		_panel = new CCommandPanel(this);
		connect(_panel, SIGNAL(commandChosen(QString,bool)),
		        this, SLOT(onCommandChosen(QString,bool)));
		connect(_panel, SIGNAL(sshChosen(QString,QString,int,QString,QString)),
		        this, SLOT(onSshChosen(QString,QString,int,QString,QString)));
	}

	if (_panel->isOpen())
		_panel->dismiss();
	else
		_panel->popup();
}

// "Insert" puts the line on the command line so it can still be edited;
// "Run" adds the Enter.
void QTermWidget::onCommandChosen(const QString &command, bool run)
{
	if (_panel)
		_panel->dismiss();

	QString text = command;
	if (run)
		text += QLatin1Char('\r');
	sendText(text);
	setFocus();
}

// Build the ssh command line and type it.  A saved key is passed through -i;
// a saved password is left to the interactive prompt, because typing it into
// the pty ourselves would put it on the screen and into the scrollback.
void QTermWidget::onSshChosen(const QString &user, const QString &host, int port,
                              const QString &identity, const QString &password)
{
	Q_UNUSED(password);

	if (_panel)
		_panel->dismiss();

	QString cmd = QString("ssh");
	if (port > 0 && port != 22)
		cmd += QString(" -p %1").arg(port);
	if (!identity.trimmed().isEmpty())
		cmd += QString(" -i %1").arg(identity.trimmed());
	if (!user.trimmed().isEmpty())
		cmd += QLatin1Char(' ') + user.trimmed() + QLatin1Char('@');
	cmd += host.trimmed();
	cmd += QLatin1Char('\r');

	sendText(cmd);
	setFocus();
}
