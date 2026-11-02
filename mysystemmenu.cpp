/*
 * Copyright (C) 2013 BGmot <support@tm-k.com>
 */
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainterPath>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>

#include "qtermwidget.h"
#include "mysystemmenu.h"
#include "mysettingswindow.h"
#include "uitheme.h"

CMySystemMenu::CMySystemMenu(QWidget *parent) :
QWidget(parent)
{
	wdgSettingsWindow = NULL;
	setVisible(false);
	setFocusPolicy(Qt::NoFocus);
	// We paint the card ourselves, so no palette fill underneath.
	setAutoFillBackground(false);
	setAttribute(Qt::WA_OpaquePaintEvent, false);
}

// Two entries, one card. The card effect is painted in paintEvent(); the
// buttons only carry text and a pressed highlight.
int CMySystemMenu::MenuInit(){
	btnSettings = new QToolButton(this);
	btnSettings->setObjectName(QString::fromUtf8("btnSettings"));
	btnSettings->setText(QString::fromUtf8("设置"));
	btnSettings->setFocusPolicy(Qt::NoFocus);

	btnCmds = new QToolButton(this);
	btnCmds->setObjectName(QString::fromUtf8("btnCmds"));
	btnCmds->setText(QString::fromUtf8("命令 / SSH"));
	btnCmds->setFocusPolicy(Qt::NoFocus);

	QToolButton *both[2] = { btnSettings, btnCmds };
	for (int i = 0; i < 2; ++i) {
		both[i]->setStyleSheet(QString(
		    "QToolButton { background-color: transparent; border: none;"
		    "  color: %1; font-size: 27px; padding: 0px 0px 0px 22px;"
		    "  text-align: left; }"
		    "QToolButton:pressed { background-color: rgba(0, 224, 90, 46);"
		    "  border-radius: 12px; }")
		    .arg(UiTheme::hex(UiTheme::Text)));
	}

    QMetaObject::connectSlotsByName(this);

    SetGeometry();

	return 0;
}

void CMySystemMenu::SetGeometry(){
	QRect r = QApplication::desktop()->screenGeometry(0);
	const int nScreenWidth = r.width();

	// The card hangs off the right end of the tab strip: the gear there opens
	// it (a swipe from the top bezel still works too, see mainWindow::event).
	// 54 = tab strip (46 px) + 8 px of air, so the tabs stay readable.
	const int nW    = qMin(340, nScreenWidth - 24);
	const int nRowH = 92;
	const int nPad  = 10;
	const int nGap  = 8;
	const int nH    = nPad * 2 + nRowH * 2 + nGap;

	this->setGeometry(nScreenWidth - nW - 12, 54, nW, nH);

	const int nBtnW = nW - 2 * nPad;
	btnSettings->setGeometry(QRect(nPad, nPad, nBtnW, nRowH));
	btnCmds    ->setGeometry(QRect(nPad, nPad + nRowH + nGap, nBtnW, nRowH));
}

void CMySystemMenu::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);

	// A floating card, so the terminal under it keeps showing through at the
	// rounded corners instead of a grey rectangle.
	const int radius = 18;
	QPainterPath path;
	path.addRoundedRect(QRectF(0.5, 0.5, width() - 1, height() - 1),
	                    radius, radius);
	p.setPen(QPen(UiTheme::Border, 1));
	p.setBrush(QColor(18, 24, 29, 248));
	p.drawPath(path);

	// Accent hairline along the top edge - the same accent the active tab
	// wears, so the card reads as part of the strip.
	p.setPen(QPen(UiTheme::Accent, 2));
	p.drawLine(radius, 1, width() - radius, 1);

	// Divider between the two entries.
	p.setPen(QPen(QColor(43, 53, 64, 200), 1));
	const int y = btnSettings->geometry().bottom() + 4;
	p.drawLine(16, y, width() - 16, y);
}

void CMySystemMenu::on_btnSettings_clicked(){
	if (!wdgSettingsWindow){
		wdgSettingsWindow = new CMySettingsWindow((QWidget*)this->parent());
		wdgSettingsWindow->Init();
		wdgSettingsWindow->show();
	}
	return;
}
void CMySystemMenu::on_btnCmds_clicked(){
	hide();
	extern QTermWidget *console;
	if (console)
		console->toggleCommandPanel();
}
