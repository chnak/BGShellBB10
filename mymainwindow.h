/*
 * Copyright (C) 2013 BGmot <support@tm-k.com>
 */
#ifndef MYMAINWINDOW_H
#define MYMAINWINDOW_H
#include <QtGui/QMainWindow>

class CMyMainWindow : public QMainWindow
{
	Q_OBJECT

public:
	int nFontSize;

    CMyMainWindow();
    virtual bool event(QEvent *e);
    void ReadAndApplyProxySettings();
public slots:
    // Tab strip notifications (see csessiontabs.h).  The active tab decides
    // which pty the keyboard, the SYM panel and the trackpad talk to.
    void sessionCurrentChanged(int index);
    void sessionNew();
    void sessionClose(int index);
    void sessionFinished();
    // OSC 0 / OSC 2 pushed a new title for the tab that owns the sender.
    void sessionTitleChanged(const QString &title);
    // Show / dismiss the settings card (gear at the right of the tab strip).
    void sessionMenu();

protected:
    // QEvent::PlatformPanel only arrives for the swipe-DOWN edge gesture, so
    // the card cannot be dismissed by swiping back up unless we watch for it.
    virtual bool eventFilter(QObject *watched, QEvent *event);

private:
    int  _menuDownY;     // screen Y where the current touch started
    bool _menuTracking;  // that touch began while the card was open
    bool _menuSwallow;   // ...it landed on the terminal: eat the whole gesture
};

#endif
