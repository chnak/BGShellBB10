/*  Copyright (C) 2008 e_k (e_k@users.sourceforge.net)

    Ported to Blackberry Playbook by BGmot <support@tm-k.com>, 2012

    This application is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.
		
    This application is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.
				
    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/
						

#include <QtCore>
#include <QtGui>
#include <QtGui/QApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtGui/QFontDatabase>
#include <QtCore/QSettings>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>   // gettimeofday(): trackpad gesture lock timeout
#include <stdio.h>
#include <string.h>

// Filled in by the probe in main(): the ssh client we are about to use.
// 0.0 means "could not tell", which keeps the command builder on the
// conservative option set (no accept-new).
int gSshMajor = 0;
int gSshMinor = 0;
#include "qtermwidget.h"
#include "mysystemmenu.h"
#include "mymainwindow.h"
#include "uitheme.h"
#include <bps/virtualkeyboard.h>
#include "mydevicetype.h"
#include "cshell.h"
#include "csessiontabs.h"

// Headers for trackpad event handling
#include <screen/screen.h>
#include <bps/bps.h>
#include <bps/screen.h>
#include <bps/event.h>
#include <bps/navigator.h>

int masterFdG = -1; // will be used in parent process
int slaveFdG = -1;  // will be used in child process
int pidG_ = -1;     // child's PID
int nKBHeight = 0;  // current Virtual Keyboard height
int nMaxKBHeight = 0; // Maximum Keyboard Height in current mode (Landscape/Portrait)
QTermWidget *console; // our 'main' widget, let's make it global so it is available in Menu widget

// Multi-session state.  "console" above always points at the active tab, so
// every existing call site keeps working unchanged; gActiveFd is the pty the
// key handlers in TerminalDisplay.cpp write to.
int            gActiveFd   = -1;
CSessionTabs  *sessionTabs = 0;
int            gSessionSeq = 0;

// Session helpers defined below main()'s helpers block; the event filter near
// the top of this file needs the prototypes.
QTermWidget *bgshellCreateSession();
void bgshellCloseSession(int index);
void bgshellSelectSession(int index);
void bgshellApplyFont();
CMyMainWindow *mainWindow;
CMySystemMenu *SystemMenu; // System menu from 'swipe down' event
QFont font;
QString gTerminalFontFamily;  // family of the bundled CJK font ("" if none)
bool bOrientationJustChanged = false; // If we just changed screen orientation then do not handle hide/show keyboard event (skip one ScreenAvailableGeometry event)
bool bKBhidden = false; // True if VK is not shown
bool bCtrlFlag = false; // A flag that we use to interpret key - whether it should be treated as Ctrl+ or not
bool bShiftFlag = false; // SHIFT is active (was pressed), we need it for BB Passport that doesn't handle Shift properly
uDeviceType dtDevice = DONTCARE; // We need to differentiate between BB Passport and other devices

static QAbstractEventDispatcher::EventFilter mainEventFilter = 0; // To store old EventFilter (Application's)

void handleTrackpadEvent(bps_event_t *event) {
    // handle trackpad events from BB Classic / Q20 / Q10 / Q5
    int screen_val, button;
    int displacement[2];
    int sens;   //sensitivity of Trackpad

    sens=5;
    screen_event_t screen_event = screen_event_get_event(event);
    screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_TYPE, &screen_val);
    switch (screen_val) {
        case SCREEN_EVENT_JOYSTICK:
            if (0 != screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_DISPLACEMENT, displacement)) {
                ;
            }
            if (0 != screen_get_event_property_iv(screen_event, SCREEN_PROPERTY_BUTTONS, &button)) {
                ;
            }
            if(button) {
                // While the pinyin candidate strip is open a trackpad click
                // commits the highlighted candidate (see imepanel.cpp).
                if (console && console->imeTrackpadClick()) {
                    fprintf(stderr, "ime: trackpad click commits candidate\n");
                    break;
                }
                fprintf(stderr, "Trackpad pressed");
            }

            if (abs(displacement[0])>=sens || abs(displacement[1])>=sens){
                // Trackpad picks candidates while composing pinyin: left/right
                // steps the highlight, up/down turns the page. If no candidate
                // strip is open the movement stays an arrow key for the shell.
                //
                // The axis is decided ONCE per gesture from the travel
                // accumulated so far, not from each 5px sample on its own.  A
                // thumb never swipes a perfectly straight line, and judging
                // every frame separately was what made a sideways swipe flip
                // the page the moment one sample leaned up or down.  The lock
                // is dropped after a 180 ms pause, which on a trackpad means
                // the finger was lifted.
                static int  swipeX = 0, swipeY = 0;
                static int  swipeAxis = 0;   // 0 undecided, 1 across, 2 down
                static long swipeLast = 0;
                {
                    struct timeval tv;
                    gettimeofday(&tv, NULL);
                    const long nowMs = tv.tv_sec * 1000L + tv.tv_usec / 1000L;
                    if (nowMs - swipeLast > 180) {
                        swipeX = swipeY = 0;
                        swipeAxis = 0;
                    }
                    swipeLast = nowMs;
                }

                swipeX += displacement[0];
                swipeY += displacement[1];

                if (swipeAxis == 0 && (abs(swipeX) >= 12 || abs(swipeY) >= 12))
                    swipeAxis = (abs(swipeX) > abs(swipeY)) ? 1 : 2;

                int dx = 0, dy = 0;
                if (swipeAxis == 1 && abs(swipeX) >= sens) {
                    dx = (swipeX > 0) ? 1 : -1;
                    swipeX -= sens * dx;
                } else if (swipeAxis == 2 && abs(swipeY) >= 20) {
                    // Paging is the destructive one - it can throw away the
                    // candidate under your thumb - so it costs four times the
                    // travel of a highlight step.
                    dy = (swipeY > 0) ? 1 : -1;
                    swipeY -= 20 * dy;
                }

                if (dx != 0 || dy != 0) {
                    if (console && console->imeTrackpadStep(dx, dy))
                        break;
                }

                if(dx > 0){
                    char c[] = {0x1B,0x5B,'C'};
                    if(bCtrlFlag)
                        c[2] = 'c';
                    write(gActiveFd, c, 3);
                } else if(dx < 0){
                    char c[] = {0x1B,0x5B,'D'};
                    if(bCtrlFlag)
                        c[2] = 'd';
                    write(gActiveFd, c, 3);
                } else if(dy > 0){
                    char c[] = {0x1B,0x5B,'B'};
                    write(gActiveFd, c, 3);
                } else if(dy < 0){
                    char c[] = {0x1B,0x5B,'A'};
                    write(gActiveFd, c, 3);
                }

                fprintf(stderr, "The x:'%4d', y:'%4d'\n", displacement[0],displacement[1]);
	    }
            break;
        default:
            break;
    }

}


bool myEventFilter(void *message) {
	// here we should have all requested bps events (no Qt events here)
    bps_event_t *event = (bps_event_t*)message;
    handleTrackpadEvent(event); //Handle Trackpad events on Classic
    if (event) {
    	int domain = bps_event_get_domain(event);
    	if (domain == virtualkeyboard_get_domain()) {
    	    // We get here only if device has virtual keyboard (like Z10, Passport)
    		unsigned int ec = bps_event_get_code(event);
    		if (!bKBhidden && ec == VIRTUALKEYBOARD_EVENT_INFO){
    			int nNewKBHeight = virtualkeyboard_event_get_height(event);
    			virtualkeyboard_get_height(&nNewKBHeight);
    			if ( nNewKBHeight != nKBHeight)
    				nKBHeight = nNewKBHeight;
    		}
    		QRect r = QApplication::desktop()->screenGeometry(0);
    		if (ec == VIRTUALKEYBOARD_EVENT_VISIBLE){
				bKBhidden = false;
				nKBHeight = nMaxKBHeight;
				virtualkeyboard_change_options(VIRTUALKEYBOARD_LAYOUT_DEFAULT, VIRTUALKEYBOARD_ENTER_DEFAULT);
    		}else if (ec == VIRTUALKEYBOARD_EVENT_HIDDEN){
				bKBhidden = true;
				nKBHeight = 0;
    		}
    		if (sessionTabs)
			sessionTabs->setGeometry(0, 0, r.width()+1, r.height()-nKBHeight);
    		
			//font = QFont(QString("Courier New"), 6);
			font.setPixelSize(mainWindow->nFontSize);
		    font.setStyle(QFont::StyleNormal);
		    font.setWeight(QFont::Normal);
			bgshellApplyFont();
    	}
   	}
 	if (mainEventFilter)   // it is 0 until the filter is installed (and on a failed install)
		mainEventFilter(message); // Call replaced event filter so we deliever everything to Qt that runs in background
	return false;
}

// The pinyin IME needs a font that really has CJK glyphs - the terminal draws
// glyph by glyph and BB10's default families do not necessarily cover Chinese.
// The kit ships a subset of Maple Mono NF CN (ASCII + the 6763 GB2312
// characters, CJK advance exactly 2x the Latin advance, which is what the
// double width cell handling in Screen.cpp assumes). Register it and use it as
// the terminal font.
static QString installBundledFont()
{
    const QString appDir = QCoreApplication::applicationDirPath();

    QStringList candidates;
    candidates << QDir::currentPath() + QString("/assets/bgscjk.ttf")
               << QDir::currentPath() + QString("/app/native/assets/bgscjk.ttf")
               << appDir + QString("/assets/bgscjk.ttf")
               << appDir + QString("/app/native/assets/bgscjk.ttf")
               << appDir + QString("/../assets/bgscjk.ttf")
               << QDir::currentPath() + QString("/native/assets/bgscjk.ttf")
               << appDir + QString("/native/assets/bgscjk.ttf")
               << QString("/accounts/1000/appdata/com.q20.shell/app/native/assets/bgscjk.ttf");

    for (int i = 0; i < candidates.size(); ++i) {
        if (!QFile::exists(candidates.at(i)))
            continue;
        const int id = QFontDatabase::addApplicationFont(candidates.at(i));
        if (id < 0)
            continue;
        const QStringList families = QFontDatabase::applicationFontFamilies(id);
        if (!families.isEmpty()) {
            qDebug() << "bundled CJK font:" << families.first() << "from" << candidates.at(i);
            return families.first();
        }
    }
    qDebug() << "bundled CJK font not found - Chinese will not render";
    return QString();
}

// ---------------------------------------------------------------------------
// Sessions: one pty + one login shell per tab
// ---------------------------------------------------------------------------
QTermWidget *bgshellCreateSession()
{
    if (!sessionTabs)
        return 0;

    CShellSession *sh = new CShellSession(sessionTabs);
    if (!sh->start(100, 30)) {
        qWarning("cannot start a shell session");
        delete sh;
        return 0;
    }

    // startnow = 0: the Konsole Session must not fork its own shell, the pty
    // comes from CShellSession instead.
    QTermWidget *term = new QTermWidget(0, sessionTabs);
    term->attachShell(sh);
    term->setTerminalFont(font);
    term->setColorScheme(COLOR_SCHEME_BGSHELL);
    term->setScrollBarPosition(QTermWidget::ScrollBarRight);

    QSettings settings;
    const int nHistory = settings.value("HistorySize", 2000).toInt();
    if (nHistory > 0)
        term->setHistorySize(nHistory);

    ++gSessionSeq;
    const QString tabTitle = QString("sh %1").arg(gSessionSeq);
    term->setDefaultTitle(tabTitle);   // fallback when a program clears OSC 0
    sessionTabs->addPage(term, tabTitle);

    console   = term;
    gActiveFd = sh->fd();
    term->setFocus();

    QObject::connect(term, SIGNAL(finished()), mainWindow, SLOT(sessionFinished()));
    // OSC 0 / OSC 2 from the program running in this tab renames it.
    QObject::connect(term, SIGNAL(titleChanged(QString)),
                     mainWindow, SLOT(sessionTitleChanged(QString)));
    return term;
}

// ---------------------------------------------------------------------------
// Closing the last tab
// ---------------------------------------------------------------------------
// This used to call mainWindow->close() without a word.  A shell that had just
// exited (exit, Ctrl-D, a dropped ssh) made the whole window vanish, which is
// indistinguishable from a crash: the user is left staring at the home screen
// with no idea whether the app died or the session ended.  Both paths now say
// what happened and let the user pick.
static bool bgshellAskLastTab(const QString &title, const QString &text,
                              const QString &actionText)
{
    if (!mainWindow)
        return true;
    QMessageBox box(mainWindow);
    box.setWindowTitle(title);
    box.setText(text);
    box.setIcon(QMessageBox::Question);
    QPushButton *bDo  = box.addButton(actionText, QMessageBox::AcceptRole);
    QPushButton *bOut = box.addButton(QString::fromUtf8("退出 BGSHELL"),
                                      QMessageBox::RejectRole);
    box.setDefaultButton(bDo);
    box.exec();
    return box.clickedButton() == bDo;
}

void bgshellCloseSession(int index)
{
    if (!sessionTabs)
        return;
    if (sessionTabs->count() <= 1) {
        // removePage() refuses to empty the tab bar, so the last tab has to go
        // through the window.
        if (bgshellAskLastTab(QString::fromUtf8("关闭最后一个标签页"),
                              QString::fromUtf8("这是唯一的标签页，关闭它会退出 BGSHELL。"),
                              QString::fromUtf8("关闭并退出"))) {
            if (mainWindow)
                mainWindow->close();
        }
        return;
    }
    sessionTabs->removePage(index);   // emits currentChanged -> active fd updated
}

// Called when a tab's shell ends by itself (exit, Ctrl-D, ssh disconnect).
void bgshellSessionEnded(int index)
{
    if (!sessionTabs)
        return;
    if (sessionTabs->count() > 1) {
        sessionTabs->removePage(index);
        return;
    }

    if (bgshellAskLastTab(QString::fromUtf8("shell 已退出"),
                          QString::fromUtf8("这个标签页的 shell 已经结束")
                          + QString::fromUtf8("（exit、Ctrl-D 或连接断开）。"),
                          QString::fromUtf8("重新打开一个"))) {
        // Add first, then drop the dead one: removePage() will not remove the
        // only page, and this order keeps the tab bar non-empty throughout.
        bgshellCreateSession();
        sessionTabs->removePage(index);
    } else if (mainWindow) {
        mainWindow->close();
    }
}

void bgshellSelectSession(int index)
{
    if (sessionTabs)
        sessionTabs->setCurrentIndex(index);
}

void bgshellApplyFont()
{
    if (!sessionTabs || !mainWindow)
        return;
    font.setPixelSize(mainWindow->nFontSize);
    font.setStyle(QFont::StyleNormal);
    font.setWeight(QFont::Normal);
    sessionTabs->refreshMetrics();    // tab titles are measured with the UI font
    for (int i = 0; i < sessionTabs->count(); ++i) {
        QTermWidget *t = qobject_cast<QTermWidget *>(sessionTabs->pageAt(i));
        if (t)
            t->setTerminalFont(font);
    }
}

int main(int argc, char *argv[])
{
    // Let's search for programs first in our own 'app' folder
    QString oldPath = qgetenv("PATH");
    QString oldLDPath = qgetenv("LD_LIBRARY_PATH");
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL){
        fprintf(stderr, "Can't determine my current folder, terminating");
        exit(-1);
    }
    // ---- which ssh? -----------------------------------------------------
    // BB10 already ships OpenSSH in its read-only system partition; the app
    // also carries a bundle (ssh/scp/sftp/ssh-keygen + libcrypto/libz) under
    // app/native/bin for images that have none.  "data/sshclient" holds the
    // user's pick - "system", "bundled", or absent = auto (system when it
    // exists, bundle otherwise).  The bundle always stays on PATH, so a wrong
    // guess falls back to the old behaviour instead of leaving no ssh at all.
    QString sshChoice("auto");
    {
        QFile f(QString(cwd) + "/data/sshclient");
        if (f.open(QIODevice::ReadOnly)) {
            const QString s = QString::fromAscii(f.readAll()).trimmed().toLower();
            if (s == "system" || s == "bundled")
                sshChoice = s;
        }
    }
    const QString sysSsh = QFile::exists("/base/usr/bin/ssh") ? QString("/base/usr/bin/ssh")
                         : QFile::exists("/usr/bin/ssh")      ? QString("/usr/bin/ssh")
                                                              : QString();
    const bool useSystemSsh = (sshChoice == "system") ||
                              (sshChoice == "auto" && !sysSsh.isEmpty());

    const QString nativeDir = QString(cwd) + "/app/native";
    const QString bundleBin = nativeDir + "/bin";
    const QString bundleLib = nativeDir + "/lib";
    const QString newPath = useSystemSsh
        ? nativeDir + ":" + oldPath + ":" + bundleBin
        : bundleBin + ":" + nativeDir + ":" + oldPath;
    qputenv("PATH", newPath.toAscii().data());
    // Never let our libcrypto/libz shadow the ones a system ssh was linked
    // against; when the bundle is the active client the order is reversed.
    const QString newLd = useSystemSsh
        ? nativeDir + "/lib:/usr/lib/qt4/lib:" + oldLDPath + ":" + bundleLib
        : bundleLib + ":/usr/lib/qt4/lib:" + oldLDPath;
    qputenv("LD_LIBRARY_PATH", newLd.toAscii().data());

    // Version matters: -o StrictHostKeyChecking=accept-new needs OpenSSH 7.6;
    // older clients reject that value and fall back to an interactive yes/no
    // prompt, which on a phone keyboard is a dead end.
    {
        FILE *p = popen("ssh -V 2>&1", "r");
        if (p) {
            char vbuf[256] = { 0 };
            if (fgets(vbuf, sizeof(vbuf) - 1, p)) {
                const char *v = strstr(vbuf, "OpenSSH_");
                if (v)
                    sscanf(v + 8, "%d.%d", &gSshMajor, &gSshMinor);
            }
            pclose(p);
        }
        qDebug() << "ssh client:" << (useSystemSsh ? "system" : "bundled")
                 << sysSsh << "version" << gSshMajor << "." << gSshMinor;
    }

    // The bundled ssh suite ships as assets; some installers drop the exec bit
    // from files inside a .bar, so make sure the tools stay runnable.
    {
        static const char *tools[] = { "ssh", "scp", "sftp", "ssh-keygen" };
        for (int i = 0; i < (int)(sizeof(tools) / sizeof(tools[0])); ++i) {
            const QString tool = QString(cwd) + "/app/native/bin/" + tools[i];
            if (QFile::exists(tool))
                chmod(tool.toAscii().data(), 0755);
        }
    }
    
    // Sessions are no longer forked here: every tab owns a pty pair and a
    // login shell (see cshell.cpp).  masterFdG / slaveFdG / pidG_ stay
    // defined because the patched Konsole layer still references them, but
    // nothing starts a process at startup any more.

    // needed for correct QT initialization
    //qputenv("QT_QPA_FONTDIR", "/usr/fonts/font_repository/dejavu-ttf-2.17"); // needed for correct QT initialization

    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", "/usr/lib/qt4/plugins/platforms");

    //QCoreApplication::addLibraryPath("app/native/lib"); // blackberry plugin does not load without this line

    QApplication app(argc, argv);

    gTerminalFontFamily = installBundledFont();

    // One visual language for every piece of chrome: menu, settings, command
    // panel, dialogs (see uitheme.h). Applied after the bundled font is
    // registered so UI text and terminal text use the same family.
    UiTheme::apply(app);

    mainWindow = new CMyMainWindow();
    QRect r = QApplication::desktop()->screenGeometry(0);
    mainWindow->resize(r.width()+1, r.height()+1);
    if (r.width() == 1440)
        dtDevice = BB_PASSPORT;

    // One strip of browser-style tabs.  Every page is a QTermWidget with its
    // own pty pair and its own login shell (cshell.cpp), so an ssh session in
    // one tab no longer blocks the shell in another.
    sessionTabs = new CSessionTabs(mainWindow);
    sessionTabs->setGeometry(0, 0, r.width() + 1, r.height() - nKBHeight);

    // The settings card.  It is a sibling of the tab strip, anchored below the
    // gear at its right end (mysystemmenu.cpp), so it must be raised when it
    // opens.  A swipe down from the top bezel still brings it up as well.
    SystemMenu = new CMySystemMenu(mainWindow);
    SystemMenu->MenuInit();

    font = QFont(gTerminalFontFamily.isEmpty() ? QString("Courier New")
                                               : gTerminalFontFamily, 6);
    font.setPixelSize(mainWindow->nFontSize);
    font.setStyle(QFont::StyleNormal);
    font.setWeight(QFont::Normal);

    // We start the app with shown keyboard (it does not make any harm for devices with physical keyboard)
    virtualkeyboard_change_options(VIRTUALKEYBOARD_LAYOUT_DEFAULT, VIRTUALKEYBOARD_ENTER_DEFAULT);

    console = bgshellCreateSession();   // first tab

    QObject::connect(sessionTabs, SIGNAL(currentChanged(int)),
                     mainWindow, SLOT(sessionCurrentChanged(int)));
    QObject::connect(sessionTabs, SIGNAL(newSessionRequested()),
                     mainWindow, SLOT(sessionNew()));
    QObject::connect(sessionTabs, SIGNAL(closeRequested(int)),
                     mainWindow, SLOT(sessionClose(int)));
    QObject::connect(sessionTabs, SIGNAL(settingsRequested()),
                     mainWindow, SLOT(sessionMenu()));

    mainWindow->show();    

    // Install event filter
    mainEventFilter = QAbstractEventDispatcher::instance()->setEventFilter(myEventFilter);

    // Show virtual keyboard (does nothing for devices with physical keyboard)
    QEvent event(QEvent::RequestSoftwareInputPanel);
    QApplication::sendEvent(console, &event);
    virtualkeyboard_get_height(&nMaxKBHeight); // Init maximum VK height

    virtualkeyboard_request_events(0); // To catch show/hide VK events

    return app.exec();
}
