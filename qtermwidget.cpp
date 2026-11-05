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

#include <QtCore/QTimer>

#include "Session.h"
#include "Pty.h"        // setWriteFd(): per-tab input routing
#include "Emulation.h"
#include "TerminalDisplay.h"
#include "cshell.h"

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
:QWidget(parent), _shell(0)
{
    m_impl = new TermWidgetImpl(this);
    
    init();

    // A saved SSH host may carry a password: the display answers the prompt
    // from the pty output, so it needs to see everything the shell prints.
    connect(m_impl->m_session, SIGNAL(receivedData(QString)),
            m_impl->m_terminalDisplay, SLOT(onSessionOutput(QString)));

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

    // OSC 0 / OSC 2 title updates from the program running in this session.
    connect(m_impl->m_session->emulation(), SIGNAL(titleChanged(int,QString)),
            this, SLOT(onEmulationTitleChanged(int,QString)));

    // rows/columns changed (widget resize, font change, IME strip, key bar):
    // the pty has to be told, otherwise vim/tmux/claude draw for a grid that
    // is not the one on screen - boxes break and the status line floats.
    connect(m_impl->m_terminalDisplay, SIGNAL(changedContentSizeSignal(int,int)),
            this, SLOT(onDisplayGridResized(int,int)));
}


QTermWidget::~QTermWidget()
{
    if (_shell) {
        // Drop the pointers before the session goes away: the display and the
        // send queue may outlive us by a few microseconds (child widgets are
        // destroyed by ~QObject, after this body runs).
        if (m_impl) {
            if (m_impl->m_terminalDisplay)
                m_impl->m_terminalDisplay->setInputSession(0);
            if (m_impl->m_session && m_impl->m_session->shellProcess())
                m_impl->m_session->shellProcess()->setWriteFd(-1);
        }
        _shell->hangup();
        delete _shell;
        _shell = 0;
    }
    // No emit destroyed() here: QObject emits its own destroyed(QObject*) from
    // ~QObject, and an unqualified emit destroyed() only ever matched that
    // signal - which made every close look like "the widget is already gone".
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
	case COLOR_SCHEME_BGSHELL:
		m_impl->m_terminalDisplay->setColorTable(bgshell_color_table);
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

void QTermWidget::toggleCommandPanel()
{
    m_impl->m_terminalDisplay->toggleCommandPanel();
}

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

void QTermWidget::resizeEvent(QResizeEvent*)
{
//+++qDebug("global window resizing...with %d %d", this->size().width(), this->size().height());
	m_impl->m_terminalDisplay->resize(this->size());
	syncShellWindowSize();
	// The display may finish its row/column arithmetic after the layout settles
	// (child resize events are delivered bottom-up), so do one deferred pass
	// too: a full-screen TUI that gets the old size repaints itself all over.
	QTimer::singleShot(0, this, SLOT(syncShellWindowSize()));
}

// Any change of the character grid - resize, font change, IME strip, key bar -
// ends in TerminalDisplay::updateImageSize() emitting this.  That is the only
// place that knows the final rows/columns, and the pty must hear about it.
void QTermWidget::onDisplayGridResized(int, int)
{
	syncShellWindowSize();
}

// Attach a real pty/shell.  Data flows straight into the emulation; keys and
// pasted text leave through the shared active-fd hook in TerminalDisplay.
void QTermWidget::attachShell(CShellSession *shell)
{
    if (!shell || !m_impl)
        return;

    _shell = shell;

    // Input must leave through this tab's own pty, not through whichever tab
    // happens to be in front when the write happens.  Both the Konsole side
    // (Pty's send queue, fed by Emulation::sendData) and the direct writes in
    // TerminalDisplay (pastes, IME, SYM panel, meta layer) are pointed here.
    if (m_impl->m_session && m_impl->m_session->shellProcess())
        m_impl->m_session->shellProcess()->setWriteFd(shell->fd());
    if (m_impl->m_terminalDisplay)
        m_impl->m_terminalDisplay->setInputSession(shell);

    connect(shell, SIGNAL(dataReceived(QByteArray)),
            this, SLOT(onShellData(QByteArray)));
    connect(shell, SIGNAL(finished()), this, SLOT(sessionFinished()));

    // Tell the shell how wide we are before the first prompt is drawn.
    syncShellWindowSize();
}

void QTermWidget::onShellData(const QByteArray &data)
{
    if (!m_impl || data.isEmpty())
        return;

    if (m_impl->m_session && m_impl->m_session->emulation())
        m_impl->m_session->emulation()->receiveData(data.constData(), data.size());

    // The SSH password helper and the pinyin IME watch the raw stream.
    if (m_impl->m_terminalDisplay)
        m_impl->m_terminalDisplay->onSessionOutput(QString::fromLatin1(data));
}

void QTermWidget::syncShellWindowSize()
{
    if (!_shell || !m_impl || !m_impl->m_terminalDisplay)
        return;
    _shell->setWindowSize(m_impl->m_terminalDisplay->columns(),
                          m_impl->m_terminalDisplay->lines());
}

// OSC 0 / OSC 2: the program says what this tab should be called.
// vim sends the file name, ssh sends "user@host: ~", a configured bash
// sends "$PWD".  Whatever arrives has to survive a 46 px high strip, so
// control characters are dropped and the rest is collapsed and capped.
static QString tabTitleFromOsc(const QString &raw)
{
    QString t;
    t.reserve(raw.size());
    for (int i = 0; i < raw.size(); ++i) {
        const int u = raw.at(i).unicode();
        if (u < 0x20 || u == 0x7f)
            continue;
        t.append(raw.at(i));
    }
    t = t.simplified();
    if (t.size() > 72)
        t = t.left(72);
    return t;
}

void QTermWidget::onEmulationTitleChanged(int what, const QString &newTitle)
{
    // 0 = icon + title, 2 = title only, 1 = icon only -> not our business
    if (what == 1)
        return;

    QString t = tabTitleFromOsc(newTitle);
    if (t.isEmpty())
        t = _defaultTitle;      // the program cleared it again
    if (t.isEmpty())
        return;

    emit titleChanged(t);
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
