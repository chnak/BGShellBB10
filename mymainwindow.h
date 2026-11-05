/*
 * Copyright (C) 2013 BGmot <support@tm-k.com>
 */
#ifndef MYMAINWINDOW_H
#define MYMAINWINDOW_H
#include <QtGui/QMainWindow>

class CMyQrScanner;

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
    // Open the QR scanner (same card, "扫二维码" entry).
    void sessionQrScan();
    // The scanner found a code: it is on the clipboard already.
    void qrScanned(const QString &text);
    // The scanner found a code and the user asked for it to be typed into
    // the shell.
    void qrInsert(const QString &text);

protected:
    // QEvent::PlatformPanel only arrives for the swipe-DOWN edge gesture, so
    // the card cannot be dismissed by swiping back up unless we watch for it.
    virtual bool eventFilter(QObject *watched, QEvent *event);

private:
    int  _menuDownY;     // screen Y where the current touch started
    bool _menuTracking;  // that touch began while the card was open
    bool _menuSwallow;   // ...it landed on the terminal: eat the whole gesture

    // The QR scanner overlay.  Created on first use and kept alive, because
    // the camera has to be released and re-taken on every open.
    CMyQrScanner *_qrScanner;
};

#endif
