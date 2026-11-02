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
						    

#ifndef _Q_TERM_WIDGET
#define _Q_TERM_WIDGET

#include <QtGui>

struct TermWidgetImpl;
class CShellSession;

enum COLOR_SCHEME {     COLOR_SCHEME_WHITE_ON_BLACK	= 1,
		        COLOR_SCHEME_GREEN_ON_BLACK,
		        COLOR_SCHEME_BLACK_ON_LIGHT_YELLOW,
		        COLOR_SCHEME_BGSHELL };   // "Midnight", the app default

class QTermWidget : public QWidget
{
    Q_OBJECT
public:
    
    enum ScrollBarPosition
    {
        /** Do not show the scroll bar. */
        NoScrollBar=0,
        /** Show the scroll bar on the left side of the display. */
        ScrollBarLeft=1,
        /** Show the scroll bar on the right side of the display. */
        ScrollBarRight=2
    };


    //Creation of widget
    QTermWidget(int startnow = 1, //start shell programm immediatelly
		QWidget *parent = 0);
    ~QTermWidget();

    //start shell program if it was not started in constructor
    void startShellProgram();
    
    //look-n-feel, if you don`t like defaults

    //	Terminal font
    // Default is application font with family Monospace, size 10
    void setTerminalFont(QFont &font); 
    
    //	Shell program, default is /bin/bash
    void setShellProgram(QString &progname);
    
    // Shell program args, default is none
    void setArgs(QStringList &args);
    
    //Text codec, default is UTF-8
    void setTextCodec(QTextCodec *codec);

    //Color scheme, default is white on black
    void setColorScheme(int scheme);
    
    //set size
    void setSize(int h, int v);
    
    // History size for scrolling 
    void setHistorySize(int lines); //infinite if lines < 0

    // Presence of scrollbar
    void setScrollBarPosition(ScrollBarPosition);
    
    // Send some text to terminal
    void sendText(QString &text);

    // Trackpad of a Q20 / Q10 / Q5 / Classic (see main.cpp): true when the IME
    // candidate strip consumed the movement / the click.
    bool imeTrackpadStep(int dx, int dy);
    bool imeTrackpadClick();

    // Quick commands / saved SSH hosts (system menu -> "Cmds").
    void toggleCommandPanel();

    // Multi-session support: hand this widget its own pty/shell.  Without it
    // the widget would ask Konsole's Session to fork a shell through the
    // patched kpty/k3process pair, which can only ever serve one session.
    void attachShell(CShellSession *shell);
    CShellSession *shell() const { return _shell; }

    // Push the current character grid to the pty (TIOCSWINSZ).
    void syncShellWindowSize();

    // Tab title.  Programs announce it with OSC 0 / OSC 2
    // ("\033]0;user@host: ~/src\007"); vim, tmux, ssh and bash all do.
    // The default is used when the program clears the title again.
    void setDefaultTitle(const QString &title) { _defaultTitle = title; }
            
signals:
    void finished();
    void titleChanged(const QString &title);
        
protected: 
    virtual void resizeEvent(QResizeEvent *);
    virtual bool event( QEvent * ); //+++
    
protected slots:
    void sessionFinished();
    void onShellData(const QByteArray &data);
    // The character grid changed size because the widget, the font or the
    // bottom reserve changed - push it to the pty (TIOCSWINSZ).  Konsole's own
    // plumbing cannot do this for us: the Pty object it keeps is never
    // started, so its setWindowSize() is a no-op.
    void onDisplayGridResized(int height, int width);
    void onEmulationTitleChanged(int what, const QString &newTitle);
    
private:
    void init();    
    TermWidgetImpl *m_impl;
    CShellSession   *_shell;
    QString          _defaultTitle;
};


//Maybe useful, maybe not

#ifdef __cplusplus
extern "C"
#endif
void *createTermWidget(int startnow, void *parent); 

#endif

