/*
 * Copyright (C) 2013 BGmot <support@tm-k.com>
 */
#ifndef MYSYSTEMMENU_H
#define MYSYSTEMMENU_H
#include <QtGui/QWidget>
#include <QtGui/QToolButton>
#include "mysettingswindow.h"

class CMySystemMenu : public QWidget
{
	Q_OBJECT

public:
    CMySystemMenu(QWidget *parent);
    int MenuInit();
    void SetGeometry();
    virtual ~CMySystemMenu(){};

    QToolButton *btnSettings;
    QToolButton *btnCmds;
    CMySettingsWindow *wdgSettingsWindow;

protected:
    // The menu is a floating card, not a grey strip: draw it by hand.
    void paintEvent(QPaintEvent *event);

private slots:
	void on_btnSettings_clicked();
	void on_btnCmds_clicked();
};

#endif
