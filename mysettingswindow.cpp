/*
 * Copyright (C) 2013 BGmot <support@tm-k.com>
 */
#include <QtGui/QPalette>
#include <QtGui/QLabel>
#include <QtGui/QFrame>
#include <QtGui/QComboBox>
#include <QtGui/QScrollArea>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>
#include <QtCore/QSettings>
#include "qtermwidget.h"
#include "mymainwindow.h"
#include "mysettingswindow.h"
#include "uitheme.h"

extern CMyMainWindow *mainWindow;
extern QString gTerminalFontFamily;

namespace {

// [ title / hint ............ control ]
QWidget *settingsRow(QWidget *parent, const QString &title,
                     const QString &hint, QWidget *control)
{
	QWidget *w = new QWidget(parent);
	QHBoxLayout *h = new QHBoxLayout(w);
	h->setContentsMargins(0, 4, 0, 4);
	h->setSpacing(16);

	QVBoxLayout *text = new QVBoxLayout;
	text->setContentsMargins(0, 0, 0, 0);
	text->setSpacing(3);

	QLabel *t = new QLabel(title, w);
	t->setStyleSheet(QString("QLabel { color: %1; font-size: 25px; }")
	                     .arg(UiTheme::hex(UiTheme::Text)));
	text->addWidget(t);

	if (!hint.isEmpty()) {
		QLabel *d = new QLabel(hint, w);
		d->setWordWrap(true);
		d->setStyleSheet(UiTheme::subtitleStyle());
		text->addWidget(d);
	}

	h->addLayout(text, 1);
	if (control) {
		control->setParent(w);
		h->addWidget(control, 0, Qt::AlignVCenter);
	}
	return w;
}

} // namespace

CMySettingsWindow::CMySettingsWindow(QWidget *parent) :
	QWidget(parent)
{
	setWindowModality(Qt::ApplicationModal);
	setObjectName(tr("wdgSettingsWindow"));
	setAutoFillBackground(false);
}

int CMySettingsWindow::Init(){
	QRect r = QApplication::desktop()->screenGeometry(0);
	int nWidth = r.width();
	int nHeight = r.height();
	this->setGeometry(0, 0, nWidth, nHeight);

	QSettings settings;

	QVBoxLayout *root = new QVBoxLayout(this);
	root->setContentsMargins(22, 18, 22, 20);
	root->setSpacing(16);

	root->addWidget(UiTheme::newHeader(this, QString::fromUtf8("设置"),
	                                  QString::fromUtf8("下滑菜单唤出 · 改动立即生效")));

	QScrollArea *scroll = new QScrollArea(this);
	scroll->setFrameShape(QFrame::NoFrame);
	scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	scroll->setWidgetResizable(true);
	scroll->viewport()->setAutoFillBackground(false);
	root->addWidget(scroll, 1);

	QWidget *content = new QWidget(scroll);
	content->setAutoFillBackground(false);
	QVBoxLayout *col = new QVBoxLayout(content);
	col->setContentsMargins(0, 0, 6, 0);
	col->setSpacing(16);
	scroll->setWidget(content);

	// ---- 连接 -------------------------------------------------------
	QVBoxLayout *netBody = 0;
	QFrame *netCard = UiTheme::newCard(content, QString::fromUtf8("连接"), QString(), netBody);

	cbxProxyOn = new QCheckBox(QString::fromUtf8("使用 HTTP 代理"), netCard);
	cbxProxyOn->setChecked(settings.value("HttpProxyOn").toInt() != 0);
	netBody->addWidget(settingsRow(netCard, QString::fromUtf8("HTTP 代理"),
	                               QString::fromUtf8("装在 Work 分区或有严格防火墙时用；"
	                                                 "需系统设置里已填好代理"),
	                               cbxProxyOn));
	netBody->addWidget(settingsRow(netCard, QString::fromUtf8("保活"),
	                               QString::fromUtf8("30 秒一个心跳，熄屏不再掉线（固定）"),
	                               0));
	col->addWidget(netCard);

	// ---- 外观 -------------------------------------------------------
	QVBoxLayout *lookBody = 0;
	QFrame *lookCard = UiTheme::newCard(content, QString::fromUtf8("外观"), QString(), lookBody);

	lblFontSample = new QLabel(QString::fromUtf8("Aa 中文"), lookCard);
	lblFontSample->setObjectName(tr("lblFontSample"));
	{
		QFont fontSample = QFont(QString("Courier New"), 6);
		fontSample.setFamily(gTerminalFontFamily.isEmpty() ? QString("Courier New")
		                                                   : gTerminalFontFamily);
		fontSample.setPixelSize(mainWindow->nFontSize);
		lblFontSample->setFont(fontSample);
		lblFontSample->setStyleSheet(QString("QLabel { color: %1; }")
		                                 .arg(UiTheme::hex(UiTheme::Accent)));
	}

	QComboBox *cbFontSize = new QComboBox(lookCard);
	cbFontSize->setObjectName(tr("cbFontSize"));
	cbFontSize->setMaxVisibleItems(9);
	for(int i=18; i < 52; i += 4){
	  cbFontSize->addItem(QString::number(i));
	  if (i == mainWindow->nFontSize)
	    cbFontSize->setCurrentIndex(cbFontSize->count()-1); // Current Font size
	}

	QWidget *fontControl = new QWidget(lookCard);
	QHBoxLayout *fontRow = new QHBoxLayout(fontControl);
	fontRow->setContentsMargins(0, 0, 0, 0);
	fontRow->setSpacing(12);
	fontRow->addWidget(lblFontSample);
	fontRow->addWidget(cbFontSize);

	lookBody->addWidget(settingsRow(lookCard, QString::fromUtf8("终端字体大小"),
	                                QString::fromUtf8("也可两指捏合缩放"),
	                                fontControl));

	QComboBox *cbHistory = new QComboBox(lookCard);
	cbHistory->setObjectName(tr("cbHistory"));
	cbHistory->setMaxVisibleItems(9);
	const int histVals[] = { 500, 1000, 2000, 5000, 10000 };
	const int nHistCur = settings.value("HistorySize", 2000).toInt();
	for (int i = 0; i < 5; ++i) {
	  cbHistory->addItem(QString::number(histVals[i]));
	  if (histVals[i] == nHistCur)
	    cbHistory->setCurrentIndex(cbHistory->count() - 1);
	}
	lookBody->addWidget(settingsRow(lookCard, QString::fromUtf8("回滚缓冲行数"),
	                                QString::fromUtf8("终端保留的历史行数"),
	                                cbHistory));

	col->addWidget(lookCard);

	QLabel *foot = new QLabel(QString::fromUtf8("快捷键与手势说明：CTRL 层按 ? 查看"), content);
	foot->setStyleSheet(UiTheme::subtitleStyle());
	foot->setAlignment(Qt::AlignHCenter);
	col->addWidget(foot);

	col->addStretch(1);

	btnClose = new QToolButton(this);
	btnClose->setObjectName(QString::fromUtf8("btnClose"));
	btnClose->setText(QString::fromUtf8("关闭"));
	btnClose->setMinimumHeight(76);
	btnClose->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	btnClose->setFocusPolicy(Qt::NoFocus);
	// objectName stays "btnClose" (connectSlotsByName needs it), so the
	// primary look is applied locally instead of via the sheet's #primary.
	btnClose->setStyleSheet(QString(
	    "QToolButton { background-color: %1; color: %2; border: none;"
	    "  border-radius: 16px; font-size: 26px; font-weight: bold; }"
	    "QToolButton:pressed { background-color: #00c24e; }")
	    .arg(UiTheme::hex(UiTheme::Accent), UiTheme::hex(UiTheme::AccentInk)));
	root->addWidget(btnClose);

	QMetaObject::connectSlotsByName(this);

	return 0;
}

void CMySettingsWindow::paintEvent(QPaintEvent *)
{
	QPainter p(this);
	p.fillRect(rect(), UiTheme::Bg);
}

extern QTermWidget *console;
void CMySettingsWindow::on_cbFontSize_currentIndexChanged(const QString & text){
    // User selected something from drop-down list
    int nSize = text.toInt();
	mainWindow->nFontSize = nSize;

	// Save new settings
	QSettings settings;
	settings.setValue("FontSize", nSize);

	// Create new Font
	QFont newfont = QFont(QString("Courier New"), 6);
	newfont.setFamily(gTerminalFontFamily.isEmpty() ? QString("Courier New")
	                                                : gTerminalFontFamily);
	newfont.setPixelSize(nSize);

    // Apply new Font/size to Sample Text
    lblFontSample->setFont(newfont);

    // Apply new Font/size to console
    if (console)
        console->setTerminalFont(newfont);
}

void CMySettingsWindow::on_cbHistory_currentIndexChanged(const QString &text){
    int nLines = text.toInt();
    if (nLines <= 0)
        return;

    QSettings settings;
    settings.setValue("HistorySize", nLines);

    // Takes effect on the running session too: only lines that fall out of
    // the new window are lost, growing the buffer keeps everything.
    if (console)
        console->setHistorySize(nLines);
}

void CMySettingsWindow::on_btnClose_clicked(){
    QEvent* event= new QEvent(QEvent::PlatformPanel);
    QApplication::sendEvent(this->parent(), event);
}
