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
		    "  color: %1; font-size: 27px; padding: 0px; }"
		    "QToolButton:pressed { background-color: rgba(0, 224, 90, 46);"
		    "  border-radius: 16px; }")
		    .arg(UiTheme::hex(UiTheme::Text)));
	}

    QMetaObject::connectSlotsByName(this);

    SetGeometry();

	return 0;
}

void CMySystemMenu::SetGeometry(){
	QRect r = QApplication::desktop()->screenGeometry(0);
	int nScreenWidth = r.width();
	const int nHeight = 116;
	this->setGeometry(0, 0, nScreenWidth, nHeight);

	const int nPad = 16;
	const int nGap = 10;
	int nBtnWidth = (nScreenWidth - 2 * nPad - nGap) / 2;
	btnSettings->setGeometry(QRect(nPad, nPad, nBtnWidth, nHeight - nPad * 2));
	btnCmds    ->setGeometry(QRect(nPad + nBtnWidth + nGap, nPad,
	                               nScreenWidth - 2 * nPad - nGap - nBtnWidth,
	                               nHeight - nPad * 2));
}

void CMySystemMenu::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.setRenderHint(QPainter::Antialiasing, true);

	const int radius = 26;

	// Rounded card whose top edge is off-screen: only the bottom corners
	// show, which reads as a panel sliding down from the bezel.
	QPainterPath path;
	path.addRoundedRect(QRectF(0, -radius, width(), height() + radius),
	                    radius, radius);
	p.setPen(QPen(UiTheme::Border, 1));
	p.setBrush(QColor(18, 24, 29, 248));
	p.drawPath(path);

	// Accent hairline on the bottom edge + a divider between the two halves.
	p.setPen(QPen(UiTheme::Accent, 2));
	p.drawLine(radius, height() - 1, width() - radius, height() - 1);

	p.setPen(QPen(QColor(43, 53, 64, 200), 1));
	p.drawLine(width() / 2, 30, width() / 2, height() - 30);
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
