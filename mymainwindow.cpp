/*
 * Copyright (C) 2013 BGmot <support@tm-k.com>
 */
#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QSettings>
#include <QtCore/QFile>
#include <QtGui/QMessageBox>
#include <QtGui/QPalette>
#include <QtGui/QTouchEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QColor>
#include <QtCore/QDir>
#include <bps/netstatus.h>
#include <qplatformnativeinterface_qpa.h>
#include <qapplication.h>

#include "mymainwindow.h"
#include "mysystemmenu.h"
#include "mysettingswindow.h"
#include "qtermwidget.h"
#include "csessiontabs.h"
#include "cshell.h"

extern CMySystemMenu *SystemMenu;
extern CMyMainWindow *mainWindow;
extern QTermWidget *console;
extern CSessionTabs *sessionTabs;
extern int gActiveFd;
extern QFont font;
QTermWidget *bgshellCreateSession();
void bgshellCloseSession(int index);
// A tab whose shell ended by itself needs a different prompt from one the user
// closed on purpose.
void bgshellSessionEnded(int index);
#define SSH_CONFIG_FNAME "data/.ssh/config"
#define SSH_CONFIG_FNAME_TMP "data/.ssh/config.tmp"
#define SSH_PROXY_AUTH_FNAME "data/.ssh/myauth"

CMyMainWindow::CMyMainWindow() :
	QMainWindow(),
	_menuDownY(0),
	_menuTracking(false),
	_menuSwallow(false)
{
	// Behind the terminal (e.g. while the keyboard resizes it) never show
	// the default grey: paint the same backdrop the terminal uses.
	QPalette pal = palette();
	pal.setColor(QPalette::Window, QColor(0x0b, 0x0e, 0x11));
	setPalette(pal);
	setAutoFillBackground(true);

	QSettings settings;
	int nConfigFontSize = settings.value("FontSize").toInt();
	if (nConfigFontSize != 0)
		nFontSize = nConfigFontSize;
	else
		nFontSize = 22;
	// Watch touches ourselves: the framework only reports the swipe-DOWN
	// edge gesture (PlatformPanel), never the swipe back up.
	qApp->installEventFilter(this);
	ReadAndApplyProxySettings();
}

bool CMyMainWindow::event(QEvent *e){
    //qDebug()<<"CMyMainWindow::event() type="<<e->type();
	if (e->type() ==  QEvent::PlatformPanel){
		// Swipe-down detected
		if (SystemMenu->wdgSettingsWindow){
			// Swipe when Settings window is active
			// Save Proxy Settings
			QSettings settings;
			if (SystemMenu->wdgSettingsWindow->cbxProxyOn->isChecked())
				settings.setValue("HttpProxyOn", 1);
			else
				settings.setValue("HttpProxyOn", 0);
			ReadAndApplyProxySettings();

			SystemMenu->wdgSettingsWindow->close();
			delete SystemMenu->wdgSettingsWindow;
			SystemMenu->wdgSettingsWindow = NULL;
		}
		if (SystemMenu->isVisible()){
			SystemMenu->hide();
		}else{
			SystemMenu->raise();     // the tab strip is its sibling: stay on top
			SystemMenu->show();
		}
	}
	return QMainWindow::event(e);
}

// The framework only reports "the user swiped down" (QEvent::PlatformPanel).
// A card that opens but cannot be swiped away feels broken, so while it is up:
//   swipe up   -> close it
//   tap below  -> close it and swallow the tap, so nothing gets typed
bool CMyMainWindow::eventFilter(QObject *watched, QEvent *event)
{
	// Cheap guard: the filter is installed application-wide, so ignore every
	// event that cannot move or dismiss the card.
	switch (event->type()){
	case QEvent::TouchBegin:
	case QEvent::TouchUpdate:
	case QEvent::TouchEnd:
	case QEvent::MouseButtonPress:
	case QEvent::MouseMove:
	case QEvent::MouseButtonRelease:
		break;
	default:
		return QMainWindow::eventFilter(watched, event);
	}

	// Finish eating a gesture that already dismissed the card; otherwise the
	// terminal would be left holding a half-finished press.
	if (_menuSwallow){
		switch (event->type()){
		case QEvent::TouchEnd:
			case QEvent::MouseButtonRelease:
			_menuSwallow = false;
			break;
		default:
			break;
		}
		return true;
	}

	if (SystemMenu && SystemMenu->isVisible()){
		// The card no longer spans the screen, so "outside" means outside the
		// card rectangle - otherwise a tap to its left would fall through to
		// the terminal and be typed.
		const QRect cardRect(SystemMenu->mapToGlobal(QPoint(0, 0)),
		                     SystemMenu->size());

		switch (event->type()){
		case QEvent::TouchBegin: {
			QTouchEvent *pTouch = static_cast<QTouchEvent *>(event);
			if (!pTouch->touchPoints().isEmpty()){
				const QPointF sp = pTouch->touchPoints().at(0).screenPos();
				_menuDownY = (int)sp.y();
				_menuTracking = true;
				if (!cardRect.contains(QPoint((int)sp.x(), (int)sp.y()))){
					// Finger landed outside the card.
					SystemMenu->hide();
					if (console)
						console->setFocus();
					_menuTracking = false;
					_menuSwallow = true;
					return true;
				}
			}
			break;
		}
		case QEvent::TouchUpdate: {
			QTouchEvent *pTouch = static_cast<QTouchEvent *>(event);
			if (_menuTracking && !pTouch->touchPoints().isEmpty()){
				const int nY = (int)pTouch->touchPoints().at(0).screenPos().y();
				if (_menuDownY - nY > 60){        // finger travelled upwards
					SystemMenu->hide();
					if (console)
						console->setFocus();
					_menuTracking = false;
				}
			}
			break;
		}
		case QEvent::TouchEnd:
				_menuTracking = false;
			break;
		case QEvent::MouseButtonPress: {
			// Same for a mouse (host build / simulator) -- and a fallback for
			// platforms that turn touches into mouse events.
			QMouseEvent *pMouse = static_cast<QMouseEvent *>(event);
			if (!cardRect.contains(pMouse->globalPos())){
				SystemMenu->hide();
				if (console)
					console->setFocus();
				_menuSwallow = true;
				return true;
			}
			_menuDownY = (int)pMouse->globalY();
			_menuTracking = true;
			break;
		}
		case QEvent::MouseMove: {
			QMouseEvent *pMouse = static_cast<QMouseEvent *>(event);
			if (_menuTracking && (_menuDownY - (int)pMouse->globalY()) > 60){
				SystemMenu->hide();
				if (console)
					console->setFocus();
				_menuTracking = false;
			}
			break;
		}
		case QEvent::MouseButtonRelease:
			_menuTracking = false;
			break;
		default:
			break;
		}
	}

	return QMainWindow::eventFilter(watched, event);
}

void CMyMainWindow::ReadAndApplyProxySettings(){
	// Read current proxy settings
    netstatus_proxy_details_t details;
    memset(&details, 0, sizeof(details));
    if (netstatus_get_proxy_details(&details) == BPS_FAILURE) {
    	qDebug() << QString("CMyMainWindow::ReadAndApplyProxySettings(): netstatus_get_proxy_details() failed");
        return;
    }
    QString host = details.http_proxy_host;
    QString port = QString::number(details.http_proxy_port);
    QString user = details.http_proxy_login_user;
    QString password = details.http_proxy_login_password;
    netstatus_free_proxy_details(&details);

    QDir SshDir(QString("data"));
    QDir SshDir1(QString("data/.ssh"));
    if (!SshDir1.exists())
    	if (!SshDir.mkdir(".ssh"))
    		qDebug() << "CMyMainWindow::ReadAndApplyProxySettings(): Can't create data/.ssh";

	QSettings settings;
	int nProxyOn = settings.value("HttpProxyOn").toInt();
	if (nProxyOn && !host.isEmpty()){
		// Proxy should be turned on

	    QString proxycommand;
		if (user.isEmpty()){
			proxycommand = "\tProxyCommand corkscrew " + host + " " + port + " %h %p\n";
			QFile fileSshProxyAuth(SSH_PROXY_AUTH_FNAME);
			if (fileSshProxyAuth.exists())
				fileSshProxyAuth.remove();
		}else{
		    // Create auth file
			QFile fileSshProxyAuth(SSH_PROXY_AUTH_FNAME);
		    if (fileSshProxyAuth.open(QIODevice::WriteOnly)){
		    	QString line = user + ":" + password + "\n";
		    	fileSshProxyAuth.write(line.toAscii());
		    	fileSshProxyAuth.close();
		    	fileSshProxyAuth.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
		    	proxycommand = "\tProxyCommand corkscrew " + host + " " + port + " %h %p ~/.ssh/myauth\n";
		    }else{
				qDebug() << QString("CMyMainWindow::ReadAndApplyProxySettings(): can't open ") + QString(SSH_PROXY_AUTH_FNAME) + QString(" for writing");
				proxycommand = "\tProxyCommand corkscrew " + host + " " + port + " %h %p\n";
		    }
		}
		QFile fileSshConfig(SSH_CONFIG_FNAME);
		if (fileSshConfig.exists()){
			// file does exist
			if (!fileSshConfig.open(QIODevice::ReadOnly)){
				qDebug() << QString("CMyMainWindow::ReadAndApplyProxySettings(): can't open ") + QString(SSH_CONFIG_FNAME) + QString(" for reading");
				return;
			}
			QFile fileSshConfigTmp(SSH_CONFIG_FNAME_TMP);
			if (!fileSshConfigTmp.open(QIODevice::WriteOnly)){
				qDebug() << QString("CMyMainWindow::ReadAndApplyProxySettings(): can't open ") + QString(SSH_CONFIG_FNAME_TMP) + QString(" for writing");
				fileSshConfig.close();
				return;
			}
			bool bHostAllFound = false;
			bool bProxyCommandFound = false;
			QTextStream in(&fileSshConfig);
		    while (!in.atEnd()) {
		    	QString line = in.readLine();
		    	if (bHostAllFound){
		    		// We already saw line "Host *", searching for "ProxyCommand"
		    		if (!bProxyCommandFound){
		    			if (line.contains(QString("ProxyCommand"), Qt::CaseInsensitive)){
		    				bProxyCommandFound = true;
		    				fileSshConfigTmp.write(proxycommand.toAscii());
		    			}else{
		    				if (line.contains(QString("Host"), Qt::CaseInsensitive)){
		    					// Next "Host" block we need to write our proxy before
			    				fileSshConfigTmp.write(proxycommand.toAscii());
			    				bProxyCommandFound = true;
		    				}
		    				line += '\n';
		    				fileSshConfigTmp.write(line.toAscii());
		    			}
		    		}else{
		    			line += '\n';
		    			fileSshConfigTmp.write(line.toAscii());
		    		}
		    	}else{
		    		// "Host *" is not found yet
			    	if (line.contains(QString("Host *"), Qt::CaseInsensitive))
			    		bHostAllFound = true;
		    		line += '\n';
		    		fileSshConfigTmp.write(line.toAscii());
		    	}
		    }
	    	if (!bHostAllFound){
				fileSshConfigTmp.write("Host *\n");
				fileSshConfigTmp.write(proxycommand.toAscii());
	    	}else{
	    		if (!bProxyCommandFound)
					fileSshConfigTmp.write(proxycommand.toAscii());
	    	}
		    fileSshConfig.close();
		    fileSshConfig.remove();
		    fileSshConfigTmp.close();
		    fileSshConfigTmp.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
		    fileSshConfigTmp.rename(SSH_CONFIG_FNAME);

		    return;
		}else{
			// file does not exist, let's create it
			if (!fileSshConfig.open(QIODevice::WriteOnly)){
				qDebug() << QString("CMyMainWindow::ReadAndApplyProxySettings(): can't open ") + QString(SSH_CONFIG_FNAME) + QString(" for writing");
				return;
			}
			fileSshConfig.write("Host *\n");
			fileSshConfig.write(proxycommand.toAscii());
			fileSshConfig.close();
			fileSshConfig.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
		}
	}else{
		// Proxy should be turned off
		QFile fileSshProxyAuth(SSH_PROXY_AUTH_FNAME);
		if (fileSshProxyAuth.exists())
			fileSshProxyAuth.remove();

		QFile fileSshConfig(SSH_CONFIG_FNAME);
		if (fileSshConfig.exists()){
			// file does exist, let's make sure that there is no proxy for Host *
			if (!fileSshConfig.open(QIODevice::ReadOnly)){
				qDebug() << QString("CMyMainWindow::ReadAndApplyProxySettings(): can't open ") + QString(SSH_CONFIG_FNAME) + QString(" for reading");
				return;
			}
			QFile fileSshConfigTmp(SSH_CONFIG_FNAME_TMP);
			if (!fileSshConfigTmp.open(QIODevice::WriteOnly)){
				qDebug() << QString("CMyMainWindow::ReadAndApplyProxySettings(): can't open ") + QString(SSH_CONFIG_FNAME_TMP) + QString(" for writing");
				fileSshConfig.close();
				return;
			}
			QTextStream in(&fileSshConfig);
		    while (!in.atEnd()) {
		    	QString line = in.readLine();
		    	if (!line.contains(QString("ProxyCommand"), Qt::CaseInsensitive)){
		    		// Write everything except ProxyCommand into new file
		    		line += '\n';
		    		fileSshConfigTmp.write(line.toAscii());
		    	}
		    }
		    fileSshConfig.close();
		    fileSshConfig.remove();
		    fileSshConfigTmp.close();
		    fileSshConfigTmp.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
		    fileSshConfigTmp.rename(SSH_CONFIG_FNAME);

		    return;
		}else{
			// file does not exist, so nothing to do
			return;

		}
	}
}

// ---------------------------------------------------------------------------
// Multi-session plumbing
//
// "console" is kept as the one global pointer every other file already uses;
// switching tabs simply repoints it (and the pty the key handlers write to).
// ---------------------------------------------------------------------------
void CMyMainWindow::sessionCurrentChanged(int index)
{
    Q_UNUSED(index);
    if (!sessionTabs)
        return;

    QTermWidget *term = qobject_cast<QTermWidget *>(sessionTabs->currentPage());
    if (!term)
        return;

    console   = term;
    gActiveFd = term->shell() ? term->shell()->fd() : -1;

    // The font setting may have changed while another tab was visible.
    term->setTerminalFont(::font);
    term->setFocus();
}

void CMyMainWindow::sessionNew()
{
    bgshellCreateSession();
}

void CMyMainWindow::sessionClose(int index)
{
    bgshellCloseSession(index);
}

void CMyMainWindow::sessionMenu()
{
    if (!SystemMenu)
        return;
    if (SystemMenu->isVisible()) {
        SystemMenu->hide();
    } else {
        SystemMenu->raise();     // the tab strip is its sibling: stay on top
        SystemMenu->show();
    }
}

void CMyMainWindow::sessionTitleChanged(const QString &title)
{
    QTermWidget *term = qobject_cast<QTermWidget *>(sender());
    if (!term || !sessionTabs || title.isEmpty())
        return;

    for (int i = 0; i < sessionTabs->count(); ++i) {
        if (sessionTabs->pageAt(i) == term) {
            sessionTabs->setPageTitle(i, title);
            return;
        }
    }
}

void CMyMainWindow::sessionFinished()
{
    QTermWidget *term = qobject_cast<QTermWidget *>(sender());
    if (!term || !sessionTabs)
        return;

    for (int i = 0; i < sessionTabs->count(); ++i) {
        if (sessionTabs->pageAt(i) == term) {
            bgshellSessionEnded(i);
            return;
        }
    }
}
