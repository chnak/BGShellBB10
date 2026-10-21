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
};

#endif
